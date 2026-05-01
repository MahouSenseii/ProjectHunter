// Character/Component/EquipmentPresentationComponent.cpp
// PH-1.4 — Equipment Presentation Component (complete)
//
// Migrated from UEquipmentManager:
//   UpdateEquippedWeapon  → HandleEquipmentChanged + AttachItemVisual/DetachItemVisual
//   SpawnWeaponActor      → SpawnWeaponActor
//   SpawnWeaponMesh       → SpawnWeaponMesh
//   CleanupWeapon         → DetachItemVisual
//   GetSocketContextForSlot → GetSocketContextForSlot (static)
//   ConvertAttachmentRules  → ConvertAttachmentRules (static)

#include "Equipment/Components/EquipmentPresentationComponent.h"

#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Item/ItemInstance.h"
#include "Item/Library/ItemStructs.h"
#include "Equipment/Actors/EquippedItemRuntimeActor.h"

DEFINE_LOG_CATEGORY(LogEquipmentPresentation);

// ═══════════════════════════════════════════════════════════════════════
// LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════

UEquipmentPresentationComponent::UEquipmentPresentationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// Presentation is purely cosmetic; no replication needed.
	SetIsReplicatedByDefault(false);
}

void UEquipmentPresentationComponent::BeginPlay()
{
	Super::BeginPlay();

	// Cache the character mesh once. AttachItemVisual re-resolves if null.
	if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		CharacterMesh = Character->GetMesh();
	}
}

void UEquipmentPresentationComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Destroy all spawned actors.
	for (auto& Pair : SpawnedActors)
	{
		if (AEquippedItemRuntimeActor* Actor = Pair.Value)
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Reset();

	// Destroy all spawned mesh components.
	for (auto& Pair : SpawnedMeshComponents)
	{
		if (USceneComponent* Component = Pair.Value)
		{
			Component->DestroyComponent();
		}
	}
	SpawnedMeshComponents.Reset();

	Super::EndPlay(EndPlayReason);
}

// ═══════════════════════════════════════════════════════════════════════
// LISTENER ENTRYPOINT
// ═══════════════════════════════════════════════════════════════════════

void UEquipmentPresentationComponent::HandleEquipmentChanged(EEquipmentSlot Slot, UItemInstance* NewItem)
{
	if (Slot == EEquipmentSlot::ES_None)
	{
		return;
	}

	// Always tear down the previous visual first.
	DetachItemVisual(Slot);

	if (NewItem != nullptr)
	{
		AttachItemVisual(Slot, NewItem);
	}

	// Notify downstream visual systems (FX, anim, moveset cosmetics).
	OnWeaponUpdated.Broadcast(Slot, NewItem);
}

// ═══════════════════════════════════════════════════════════════════════
// ACCESSOR
// ═══════════════════════════════════════════════════════════════════════

AEquippedItemRuntimeActor* UEquipmentPresentationComponent::GetActiveRuntimeItemActor(EEquipmentSlot Slot) const
{
	if (const TObjectPtr<AEquippedItemRuntimeActor>* Found = SpawnedActors.Find(Slot))
	{
		return Found->Get();
	}
	return nullptr;
}

// ═══════════════════════════════════════════════════════════════════════
// VISUAL LIFECYCLE
// ═══════════════════════════════════════════════════════════════════════

void UEquipmentPresentationComponent::AttachItemVisual(EEquipmentSlot Slot, UItemInstance* Item)
{
	if (!Item)
	{
		return;
	}

	// Lazily resolve mesh if BeginPlay ran before possession was complete.
	if (!CharacterMesh)
	{
		if (const ACharacter* Character = Cast<ACharacter>(GetOwner()))
		{
			CharacterMesh = Character->GetMesh();
		}
	}

	if (!CharacterMesh)
	{
		PH_LOG_WARNING(LogEquipmentPresentation, "AttachItemVisual failed: Owner=%s had no CharacterMesh. Visual skipped.", *GetNameSafe(GetOwner()));
		return;
	}

	const FItemBase* BaseData = Item->GetBaseData();
	if (!BaseData)
	{
		PH_LOG_WARNING(LogEquipmentPresentation, "AttachItemVisual failed: Item=%s had no base data.", *GetNameSafe(Item));
		return;
	}

	const FName SocketName = ResolveSocketForSlot(Slot, BaseData);
	if (SocketName == NAME_None || !CharacterMesh->DoesSocketExist(SocketName))
	{
		PH_LOG_WARNING(LogEquipmentPresentation, "AttachItemVisual failed: Socket=%s does not exist on Owner=%s. Visual skipped.", *SocketName.ToString(), *GetNameSafe(GetOwner()));
		return;
	}

	const TSubclassOf<AActor> RuntimeActorClass = BaseData->GetRuntimeActorClass();
	const bool bHasCompatibleRuntimeActorClass =
		RuntimeActorClass && RuntimeActorClass->IsChildOf(AEquippedItemRuntimeActor::StaticClass());
	const bool bMustUseRuntimeActor = BaseData->IsWeapon();

	if (bMustUseRuntimeActor || (BaseData->UsesRuntimeActor() && bHasCompatibleRuntimeActorClass))
	{
		SpawnWeaponActor(Slot, Item, BaseData, SocketName);
	}
	else
	{
		if (BaseData->UsesRuntimeActor())
		{
			PH_LOG_WARNING(LogEquipmentPresentation, "AttachItemVisual falling back to mesh for Item=%s because RuntimeActorClass was null.", *GetNameSafe(Item));
		}
		SpawnWeaponMesh(Slot, Item, BaseData, SocketName);
	}
}

void UEquipmentPresentationComponent::DetachItemVisual(EEquipmentSlot Slot)
{
	// Destroy a runtime actor if present.
	if (TObjectPtr<AEquippedItemRuntimeActor>* FoundActor = SpawnedActors.Find(Slot))
	{
		if (AEquippedItemRuntimeActor* Actor = FoundActor->Get())
		{
			if (IsValid(Actor))
			{
				Actor->Destroy();
			}
		}
		SpawnedActors.Remove(Slot);
	}

	// Destroy mesh component if present.
	if (TObjectPtr<USceneComponent>* FoundMesh = SpawnedMeshComponents.Find(Slot))
	{
		if (USceneComponent* Mesh = FoundMesh->Get())
		{
			if (IsValid(Mesh))
			{
				Mesh->DestroyComponent();
			}
		}
		SpawnedMeshComponents.Remove(Slot);
	}
}

// ═══════════════════════════════════════════════════════════════════════
// SPAWN HELPERS
// ═══════════════════════════════════════════════════════════════════════

void UEquipmentPresentationComponent::SpawnWeaponActor(EEquipmentSlot Slot, UItemInstance* Item,
                                                       const FItemBase* BaseData, FName SocketName)
{
	TSubclassOf<AActor> RuntimeActorClass = BaseData->GetRuntimeActorClass();
	if (BaseData->IsWeapon() &&
		(!RuntimeActorClass || !RuntimeActorClass->IsChildOf(AEquippedItemRuntimeActor::StaticClass())))
	{
		PH_LOG_WARNING(LogEquipmentPresentation, "SpawnWeaponActor defaulted RuntimeActorClass for Weapon=%s to AEquippedItemRuntimeActor.", *GetNameSafe(Item));
		RuntimeActorClass = AEquippedItemRuntimeActor::StaticClass();
	}

	if (!RuntimeActorClass || !GetOwner())
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	SpawnParams.Instigator = OwnerPawn;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AEquippedItemRuntimeActor* WeaponActor = GetWorld()->SpawnActor<AEquippedItemRuntimeActor>(
		RuntimeActorClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (!WeaponActor)
	{
		PH_LOG_ERROR(LogEquipmentPresentation, "SpawnWeaponActor failed: Could not spawn RuntimeActorClass=%s.", *GetNameSafe(RuntimeActorClass));
		return;
	}

	// Attach to the character mesh socket.
	const FAttachmentTransformRules AttachRules = ConvertAttachmentRules(BaseData->AttachmentRules);
	WeaponActor->AttachToComponent(CharacterMesh, AttachRules, SocketName);

	// Initialize the runtime actor with item data.
	WeaponActor->InitializeFromItem(Item, OwnerPawn, Slot);

	// Store for later retrieval and cleanup.
	SpawnedActors.Add(Slot, WeaponActor);

	UE_LOG(LogEquipmentPresentation, Log,
		TEXT("UEquipmentPresentationComponent: Spawned runtime actor '%s' on socket '%s'."),
		*GetNameSafe(WeaponActor), *SocketName.ToString());
}

void UEquipmentPresentationComponent::SpawnWeaponMesh(EEquipmentSlot Slot, UItemInstance* Item,
                                                      const FItemBase* BaseData, FName SocketName)
{
	if (!GetOwner())
	{
		return;
	}

	const FAttachmentTransformRules AttachRules = ConvertAttachmentRules(BaseData->AttachmentRules);
	USceneComponent* NewComponent = nullptr;

	if (BaseData->SkeletalMesh)
	{
		USkeletalMeshComponent* SkeletalComp = NewObject<USkeletalMeshComponent>(
			GetOwner(), USkeletalMeshComponent::StaticClass());

		if (SkeletalComp)
		{
			SkeletalComp->SetSkeletalMesh(BaseData->SkeletalMesh);
			SkeletalComp->RegisterComponent();
			SkeletalComp->AttachToComponent(CharacterMesh, AttachRules, SocketName);
			NewComponent = SkeletalComp;

			UE_LOG(LogEquipmentPresentation, Verbose,
				TEXT("UEquipmentPresentationComponent: Attached SkeletalMesh '%s' to socket '%s'."),
				*BaseData->SkeletalMesh->GetName(), *SocketName.ToString());
		}
	}
	else if (BaseData->StaticMesh)
	{
		UStaticMeshComponent* StaticComp = NewObject<UStaticMeshComponent>(
			GetOwner(), UStaticMeshComponent::StaticClass());

		if (StaticComp)
		{
			StaticComp->SetStaticMesh(BaseData->StaticMesh);
			StaticComp->RegisterComponent();
			StaticComp->AttachToComponent(CharacterMesh, AttachRules, SocketName);
			NewComponent = StaticComp;

			UE_LOG(LogEquipmentPresentation, Verbose,
				TEXT("UEquipmentPresentationComponent: Attached StaticMesh '%s' to socket '%s'."),
				*BaseData->StaticMesh->GetName(), *SocketName.ToString());
		}
	}
	else
	{
		PH_LOG_WARNING(LogEquipmentPresentation, "SpawnWeaponMesh failed: Item=%s had no mesh asset.", *GetNameSafe(Item));
	}

	if (NewComponent)
	{
		SpawnedMeshComponents.Add(Slot, NewComponent);
	}
}

// ═══════════════════════════════════════════════════════════════════════
// PURE HELPERS
// ═══════════════════════════════════════════════════════════════════════

FName UEquipmentPresentationComponent::GetSocketContextForSlot(EEquipmentSlot Slot)
{
	switch (Slot)
	{
	case EEquipmentSlot::ES_MainHand: return FName("MainHand");
	case EEquipmentSlot::ES_OffHand:  return FName("OffHand");
	case EEquipmentSlot::ES_TwoHand:  return FName("TwoHand");
	default:                          return NAME_None;
	}
}

FName UEquipmentPresentationComponent::ResolveSocketForSlot(EEquipmentSlot Slot, const FItemBase* BaseData)
{
	if (!BaseData)
	{
		return NAME_None;
	}

	const FName SocketContext = GetSocketContextForSlot(Slot);

	if (SocketContext != NAME_None)
	{
		const FName ContextSocket = BaseData->GetSocketForContext(SocketContext);
		if (ContextSocket != NAME_None)
		{
			return ContextSocket;
		}
	}

	// Fall back to the item's default attachment socket.
	return BaseData->AttachmentSocket;
}

FAttachmentTransformRules UEquipmentPresentationComponent::ConvertAttachmentRules(const FItemAttachmentRules& ItemRules)
{
	auto ConvertRule = [](EPHAttachmentRule Rule) -> EAttachmentRule
	{
		switch (Rule)
		{
		case EPHAttachmentRule::AR_KeepRelative: return EAttachmentRule::KeepRelative;
		case EPHAttachmentRule::AR_KeepWorld:    return EAttachmentRule::KeepWorld;
		case EPHAttachmentRule::AR_SnapToTarget: return EAttachmentRule::SnapToTarget;
		default:                                 return EAttachmentRule::KeepRelative;
		}
	};

	return FAttachmentTransformRules(
		ConvertRule(ItemRules.LocationRule),
		ConvertRule(ItemRules.RotationRule),
		ConvertRule(ItemRules.ScaleRule),
		ItemRules.bWeldSimulatedBodies
	);
}
