#include "Progression/Components/PHPassiveTreeComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "Progression/Components/CharacterProgressionManager.h"
#include "Progression/Data/PHPassiveTreeDataAsset.h"
#include "Progression/Settings/PHPassiveTreeSettings.h"
#include "Tags/PHGameplayTags.h"
#include "Tower/Library/FunctionLibraries/RunSeedFunctionLibrary.h"

DEFINE_LOG_CATEGORY_STATIC(LogPHPassiveTree, Log, All);

UPHPassiveTreeComponent::UPHPassiveTreeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPHPassiveTreeComponent::BeginPlay()
{
	Super::BeginPlay();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		RefreshPassiveEffects();
		if (bAutoRollRandomStartOnBeginPlay)
		{
			// No-ops when an origin already exists, so a restored character keeps the one it woke up
			// on rather than being handed a second.
			RollRandomStart(RandomStartSeed);
		}
	}
}

void UPHPassiveTreeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		ClearPassiveEffects();
	}
	Super::EndPlay(EndPlayReason);
}

void UPHPassiveTreeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UPHPassiveTreeComponent, AllocatedNodeIDs, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(UPHPassiveTreeComponent, RandomStartNodeID, COND_OwnerOnly);
}

UPHPassiveTreeDataAsset* UPHPassiveTreeComponent::GetTreeData() const
{
	if (CachedTree)
	{
		return CachedTree;
	}

	UPHPassiveTreeDataAsset* Resolved = nullptr;
	if (!TreeDataOverride.IsNull())
	{
		Resolved = TreeDataOverride.LoadSynchronous();
	}
	else if (const UPHPassiveTreeSettings* Settings = GetDefault<UPHPassiveTreeSettings>())
	{
		Resolved = Settings->DefaultTree.LoadSynchronous();
	}

	// The tree is fixed for the lifetime of the component, so the resolve is memoised. CachedTree is
	// a UPROPERTY, which is also what keeps the loaded asset alive against GC.
	const_cast<UPHPassiveTreeComponent*>(this)->CachedTree = Resolved;
	return Resolved;
}

bool UPHPassiveTreeComponent::IsNodeAllocated(const FName NodeID) const
{
	return AllocatedLookup.Contains(NodeID);
}

void UPHPassiveTreeComponent::RebuildAllocatedLookup()
{
	AllocatedLookup.Reset();
	AllocatedLookup.Reserve(AllocatedNodeIDs.Num());
	AllocatedLookup.Append(AllocatedNodeIDs);
}

bool UPHPassiveTreeComponent::CanAllocateNode(const FName NodeID, FText& OutReason) const
{
	OutReason = FText::GetEmpty();
	const UPHPassiveTreeDataAsset* Tree = GetTreeData();
	if (!Tree)
	{
		OutReason = NSLOCTEXT("PHPassiveTree", "NoTree", "No passive tree is configured.");
		return false;
	}

	const FPHPassiveNodeDefinition* Node = Tree->FindNode(NodeID);
	if (!Node)
	{
		OutReason = NSLOCTEXT("PHPassiveTree", "UnknownNode", "That passive node no longer exists.");
		return false;
	}
	if (IsNodeAllocated(NodeID))
	{
		OutReason = NSLOCTEXT("PHPassiveTree", "AlreadyAllocated", "This passive is already active.");
		return false;
	}
	if (Node->PointCost <= 0 || Node->Modifiers.IsEmpty())
	{
		OutReason = NSLOCTEXT("PHPassiveTree", "InvalidNode", "This node has invalid authored data.");
		return false;
	}

	const bool bConnected = Tree->AreConnectionsSatisfied(*Node,
		[this](const FName ConnectedID) { return IsNodeAllocated(ConnectedID); },
		AllowsUnconnectedStarts(*Tree));
	if (!bConnected)
	{
		OutReason = NSLOCTEXT("PHPassiveTree", "Locked", "Allocate the connected path first.");
		return false;
	}

	for (const FPHPassiveModifier& Modifier : Node->Modifiers)
	{
		if (!Modifier.AttributeTag.IsValid() ||
			!FPHGameplayTags::GetAttributeFromTag(Modifier.AttributeTag).IsValid() ||
			!FMath::IsFinite(Modifier.Magnitude))
		{
			OutReason = NSLOCTEXT("PHPassiveTree", "InvalidModifier", "This node contains an invalid attribute modifier.");
			return false;
		}
	}

	const UCharacterProgressionManager* Progression = GetProgression();
	if (!Progression)
	{
		OutReason = NSLOCTEXT("PHPassiveTree", "NoProgression", "The character has no progression owner.");
		return false;
	}
	if (Progression->UnspentPassivePoints < Node->PointCost)
	{
		OutReason = FText::Format(
			NSLOCTEXT("PHPassiveTree", "NotEnoughPoints", "Requires {0} passive point(s)."),
			FText::AsNumber(Node->PointCost));
		return false;
	}

	return true;
}

bool UPHPassiveTreeComponent::RequestAllocateNode(const FName NodeID)
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	if (Owner->HasAuthority())
	{
		return AllocateNode(NodeID);
	}

	FText Reason;
	if (!CanAllocateNode(NodeID, Reason))
	{
		Reject(NodeID, Reason);
		return false;
	}

	ServerAllocateNode(NodeID);
	return true;
}

bool UPHPassiveTreeComponent::AllocateNode(const FName NodeID)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		Reject(NodeID, NSLOCTEXT("PHPassiveTree", "NoAuthority", "Only the owning authority can allocate passives."));
		return false;
	}

	FText Reason;
	if (!CanAllocateNode(NodeID, Reason))
	{
		Reject(NodeID, Reason);
		return false;
	}

	UPHPassiveTreeDataAsset* Tree = GetTreeData();
	const FPHPassiveNodeDefinition* Node = Tree ? Tree->FindNode(NodeID) : nullptr;
	if (!Node)
	{
		Reject(NodeID, NSLOCTEXT("PHPassiveTree", "LostNode", "The passive definition could not be loaded."));
		return false;
	}

	FActiveGameplayEffectHandle EffectHandle;
	if (!ApplyNodeEffect(*Node, EffectHandle))
	{
		Reject(NodeID, NSLOCTEXT("PHPassiveTree", "EffectFailed", "The passive effect could not be applied."));
		return false;
	}

	UCharacterProgressionManager* Progression = GetProgression();
	if (!Progression || !Progression->SpendPassivePoints(Node->PointCost))
	{
		if (UAbilitySystemComponent* ASC = GetAbilitySystem())
		{
			ASC->RemoveActiveGameplayEffect(EffectHandle);
		}
		RuntimeEffects.Remove(NodeID);
		Reject(NodeID, NSLOCTEXT("PHPassiveTree", "SpendFailed", "Passive points changed before allocation completed."));
		return false;
	}

	AllocatedNodeIDs.Add(NodeID);
	AllocatedLookup.Add(NodeID);
	ActiveEffectHandles.Add(NodeID, EffectHandle);
	OnPassiveTreeChanged.Broadcast();

	UE_LOG(LogPHPassiveTree, Log, TEXT("Allocated passive '%s' on %s for %d point(s)."),
		*NodeID.ToString(), *GetNameSafe(Owner), Node->PointCost);
	return true;
}

void UPHPassiveTreeComponent::RestoreAllocatedNodes(const TArray<FName>& SavedNodeIDs)
{
	RestorePassiveState(SavedNodeIDs, NAME_None);
}

void UPHPassiveTreeComponent::RestorePassiveState(
	const TArray<FName>& SavedNodeIDs,
	const FName SavedRandomStartNodeID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	AllocatedNodeIDs.Reset();
	AllocatedLookup.Reset();
	RandomStartNodeID = NAME_None;
	const UPHPassiveTreeDataAsset* Tree = GetTreeData();
	if (!Tree)
	{
		ClearPassiveEffects();
		OnPassiveTreeChanged.Broadcast();
		return;
	}

	// The origin is seeded first and unconditionally: it was granted, not bought, so it does not have
	// to satisfy connections the way the rest of the saved set does.
	if (!SavedRandomStartNodeID.IsNone())
	{
		if (Tree->FindNode(SavedRandomStartNodeID))
		{
			RandomStartNodeID = SavedRandomStartNodeID;
			AllocatedNodeIDs.Add(SavedRandomStartNodeID);
			AllocatedLookup.Add(SavedRandomStartNodeID);
		}
		else
		{
			UE_LOG(LogPHPassiveTree, Warning,
				TEXT("Saved random start '%s' is absent from tree '%s'; this character will re-roll."),
				*SavedRandomStartNodeID.ToString(), *Tree->TreeID.ToString());
		}
	}

	TSet<FName> Pending;
	Pending.Append(SavedNodeIDs);
	Pending.Remove(RandomStartNodeID);
	bool bAddedAny = true;
	while (bAddedAny && !Pending.IsEmpty())
	{
		bAddedAny = false;
		for (auto It = Pending.CreateIterator(); It; ++It)
		{
			const FPHPassiveNodeDefinition* Node = Tree->FindNode(*It);
			if (!Node)
			{
				UE_LOG(LogPHPassiveTree, Warning, TEXT("Ignoring saved passive '%s'; it is absent from tree '%s'."),
					*It->ToString(), *Tree->TreeID.ToString());
				It.RemoveCurrent();
				continue;
			}

			const bool bRequirementsMet = Tree->AreConnectionsSatisfied(*Node,
				[this](const FName ConnectedID) { return AllocatedLookup.Contains(ConnectedID); },
				AllowsUnconnectedStarts(*Tree));
			if (bRequirementsMet)
			{
				AllocatedNodeIDs.Add(Node->NodeID);
				AllocatedLookup.Add(Node->NodeID);
				It.RemoveCurrent();
				bAddedAny = true;
			}
		}
	}

	for (const FName DisconnectedID : Pending)
	{
		UE_LOG(LogPHPassiveTree, Warning, TEXT("Ignoring disconnected saved passive '%s'."), *DisconnectedID.ToString());
	}

	RefreshPassiveEffects();
	OnPassiveTreeChanged.Broadcast();
}

void UPHPassiveTreeComponent::RefreshPassiveEffects()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	ClearPassiveEffects();
	const UPHPassiveTreeDataAsset* Tree = GetTreeData();
	if (!Tree)
	{
		return;
	}

	for (const FName NodeID : AllocatedNodeIDs)
	{
		const FPHPassiveNodeDefinition* Node = Tree->FindNode(NodeID);
		FActiveGameplayEffectHandle Handle;
		if (Node && ApplyNodeEffect(*Node, Handle))
		{
			ActiveEffectHandles.Add(NodeID, Handle);
		}
	}
}

void UPHPassiveTreeComponent::ServerAllocateNode_Implementation(const FName NodeID)
{
	AllocateNode(NodeID);
}

void UPHPassiveTreeComponent::OnRep_AllocatedNodeIDs()
{
	RebuildAllocatedLookup();
	OnPassiveTreeChanged.Broadcast();
}

void UPHPassiveTreeComponent::OnRep_RandomStartNodeID()
{
	OnPassiveTreeChanged.Broadcast();
}

bool UPHPassiveTreeComponent::IsRandomStartNode(const FName NodeID) const
{
	return !NodeID.IsNone() && NodeID == RandomStartNodeID;
}

bool UPHPassiveTreeComponent::AllowsUnconnectedStarts(const UPHPassiveTreeDataAsset& Tree) const
{
	return RandomStartNodeID.IsNone() || !Tree.RandomStart.bReplacesAuthoredStarts;
}

FName UPHPassiveTreeComponent::RollRandomStart(const int32 ParentSeed)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority())
	{
		return NAME_None;
	}
	if (!RandomStartNodeID.IsNone())
	{
		return RandomStartNodeID;
	}

	const UPHPassiveTreeDataAsset* Tree = GetTreeData();
	if (!Tree || !Tree->RandomStart.bEnabled)
	{
		return NAME_None;
	}

	// Routed through DeriveSeed so a character seed produces the same origin every time, and so the
	// passive roll cannot correlate with any other draw made from the same parent seed.
	const int32 Seed = URunSeedFunctionLibrary::DeriveSeed(
		ParentSeed != 0 ? ParentSeed : FMath::Rand(), TEXT("PassiveStart"), 0);
	const FName Picked = Tree->PickRandomStart(Seed);
	if (Picked.IsNone())
	{
		UE_LOG(LogPHPassiveTree, Warning,
			TEXT("Random starts are enabled on tree '%s' but nothing is eligible."), *Tree->TreeID.ToString());
		return NAME_None;
	}

	return SetRandomStart(Picked) ? Picked : NAME_None;
}

bool UPHPassiveTreeComponent::SetRandomStart(const FName NodeID)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || NodeID.IsNone())
	{
		return false;
	}

	const UPHPassiveTreeDataAsset* Tree = GetTreeData();
	const FPHPassiveNodeDefinition* Node = Tree ? Tree->FindNode(NodeID) : nullptr;
	if (!Node)
	{
		UE_LOG(LogPHPassiveTree, Warning, TEXT("Cannot start on unknown passive '%s'."), *NodeID.ToString());
		return false;
	}
	if (!RandomStartNodeID.IsNone())
	{
		UE_LOG(LogPHPassiveTree, Warning,
			TEXT("Passive origin is already '%s'; ignoring a second roll of '%s'."),
			*RandomStartNodeID.ToString(), *NodeID.ToString());
		return false;
	}

	if (!IsNodeAllocated(NodeID) && !GrantNodeWithoutCost(*Node))
	{
		return false;
	}

	RandomStartNodeID = NodeID;
	OnPassiveTreeChanged.Broadcast();
	UE_LOG(LogPHPassiveTree, Log, TEXT("%s wakes up on passive '%s'."), *GetNameSafe(Owner), *NodeID.ToString());
	return true;
}

bool UPHPassiveTreeComponent::GrantNodeWithoutCost(const FPHPassiveNodeDefinition& Node)
{
	FActiveGameplayEffectHandle EffectHandle;
	if (!ApplyNodeEffect(Node, EffectHandle))
	{
		UE_LOG(LogPHPassiveTree, Warning,
			TEXT("Could not apply the granted passive '%s'."), *Node.NodeID.ToString());
		return false;
	}

	AllocatedNodeIDs.Add(Node.NodeID);
	AllocatedLookup.Add(Node.NodeID);
	ActiveEffectHandles.Add(Node.NodeID, EffectHandle);
	return true;
}

UCharacterProgressionManager* UPHPassiveTreeComponent::GetProgression() const
{
	if (CachedProgression.IsValid())
	{
		return CachedProgression.Get();
	}

	UCharacterProgressionManager* Found =
		GetOwner() ? GetOwner()->FindComponentByClass<UCharacterProgressionManager>() : nullptr;
	CachedProgression = Found;
	return Found;
}

UAbilitySystemComponent* UPHPassiveTreeComponent::GetAbilitySystem() const
{
	const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(GetOwner());
	return AbilityOwner ? AbilityOwner->GetAbilitySystemComponent() :
		(GetOwner() ? GetOwner()->FindComponentByClass<UAbilitySystemComponent>() : nullptr);
}

bool UPHPassiveTreeComponent::ApplyNodeEffect(
	const FPHPassiveNodeDefinition& Node,
	FActiveGameplayEffectHandle& OutHandle)
{
	UAbilitySystemComponent* ASC = GetAbilitySystem();
	if (!ASC)
	{
		return false;
	}

	UGameplayEffect* Effect = NewObject<UGameplayEffect>(this);
	Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;
	for (const FPHPassiveModifier& Modifier : Node.Modifiers)
	{
		const FGameplayAttribute Attribute = FPHGameplayTags::GetAttributeFromTag(Modifier.AttributeTag);
		if (!Attribute.IsValid() || !FMath::IsFinite(Modifier.Magnitude))
		{
			return false;
		}

		FGameplayModifierInfo& Info = Effect->Modifiers.AddDefaulted_GetRef();
		Info.Attribute = Attribute;
		Info.ModifierOp = Modifier.Operation;
		Info.ModifierMagnitude = FScalableFloat(Modifier.Magnitude);
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);
	OutHandle = ASC->ApplyGameplayEffectToSelf(Effect, 1.0f, Context);
	if (!OutHandle.IsValid())
	{
		return false;
	}

	RuntimeEffects.Add(Node.NodeID, Effect);
	return true;
}

void UPHPassiveTreeComponent::ClearPassiveEffects()
{
	if (UAbilitySystemComponent* ASC = GetAbilitySystem())
	{
		for (const TPair<FName, FActiveGameplayEffectHandle>& Pair : ActiveEffectHandles)
		{
			ASC->RemoveActiveGameplayEffect(Pair.Value);
		}
	}

	ActiveEffectHandles.Reset();
	RuntimeEffects.Reset();
}

void UPHPassiveTreeComponent::Reject(const FName NodeID, const FText& Reason)
{
	UE_LOG(LogPHPassiveTree, Verbose, TEXT("Passive '%s' rejected on %s: %s"),
		*NodeID.ToString(), *GetNameSafe(GetOwner()), *Reason.ToString());
	OnPassiveAllocationRejected.Broadcast(NodeID, Reason);
}
