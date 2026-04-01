// Character/Component/MovesetManager.cpp
#include "Character/Component/MovesetManager.h"
#include "Character/Component/EquipmentManager.h"
#include "Item/ItemInstance.h"
#include "Item/Moveset/MovesetData.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogMovesetManager, Log, All);

UMovesetManager::UMovesetManager()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UMovesetManager::BeginPlay()
{
	Super::BeginPlay();
	CacheComponents();

	// Wire up to EquipmentManager so we react to weapon swaps
	if (EquipmentManager)
	{
		EquipmentManager->OnEquipmentChanged.AddDynamic(this, &UMovesetManager::HandleEquipmentChanged);
	}
}

void UMovesetManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UMovesetManager, SocketedMovesetsArray);
}

// ─────────────────────────────────────────────────────────────────────────────
// Socketing
// ─────────────────────────────────────────────────────────────────────────────

bool UMovesetManager::SocketMoveset(EEquipmentSlot WeaponSlot,
	const FMovesetInstance& Moveset, FMovesetInstance& OutPreviousMoveset)
{
	if (!GetOwner()->HasAuthority())
	{
		Server_SocketMoveset(WeaponSlot, Moveset);
		return true; // Optimistic — server will validate
	}

	if (!EquipmentManager || !Moveset.IsValid())
	{
		UE_LOG(LogMovesetManager, Warning, TEXT("SocketMoveset: invalid moveset or no EquipmentManager"));
		return false;
	}

	// Compatibility check
	UItemInstance* WeaponItem = EquipmentManager->GetEquippedItem(WeaponSlot);
	if (!WeaponItem)
	{
		UE_LOG(LogMovesetManager, Warning,
			TEXT("SocketMoveset: no weapon equipped in slot %d"), static_cast<int32>(WeaponSlot));
		return false;
	}

	UMovesetData* Data = Moveset.MovesetData.LoadSynchronous();
	if (Data && !Data->IsCompatibleWithWeapon(WeaponItem->GetItemSubType()))
	{
		UE_LOG(LogMovesetManager, Warning,
			TEXT("SocketMoveset: %s is not compatible with weapon %s"),
			*Data->MovesetName.ToString(), *WeaponItem->GetDisplayName().ToString());
		return false;
	}

	// Return any previously socketed moveset
	FMovesetInstance* Existing = SocketedMovesetsMap.Find(WeaponSlot);
	if (Existing)
	{
		RevokeMovesetAbilities(WeaponSlot);
		OutPreviousMoveset = *Existing;
	}

	// Socket the new moveset
	FMovesetInstance NewMoveset = Moveset;
	SetMovesetEntry(WeaponSlot, NewMoveset);
	GrantMovesetAbilities(WeaponSlot, *SocketedMovesetsMap.Find(WeaponSlot));

	OnMovesetSocketed.Broadcast(WeaponSlot, Moveset);
	UE_LOG(LogMovesetManager, Log, TEXT("SocketMoveset: socketed in slot %d"), static_cast<int32>(WeaponSlot));
	return true;
}

bool UMovesetManager::UnsocketMoveset(EEquipmentSlot WeaponSlot, FMovesetInstance& OutMoveset)
{
	if (!GetOwner()->HasAuthority())
	{
		Server_UnsocketMoveset(WeaponSlot);
		return true;
	}

	FMovesetInstance* Existing = SocketedMovesetsMap.Find(WeaponSlot);
	if (!Existing)
	{
		return false;
	}

	RevokeMovesetAbilities(WeaponSlot);
	OutMoveset = *Existing;
	RemoveMovesetEntry(WeaponSlot);

	OnMovesetUnsocketed.Broadcast(WeaponSlot);
	return true;
}

bool UMovesetManager::IsCompatibleWithEquippedWeapon(EEquipmentSlot WeaponSlot,
	const UMovesetData* Data) const
{
	if (!Data || !EquipmentManager)
	{
		return false;
	}

	UItemInstance* WeaponItem = EquipmentManager->GetEquippedItem(WeaponSlot);
	if (!WeaponItem)
	{
		return false;
	}

	return Data->IsCompatibleWithWeapon(WeaponItem->GetItemSubType());
}

const FMovesetInstance* UMovesetManager::GetMovesetForSlot(EEquipmentSlot WeaponSlot) const
{
	return SocketedMovesetsMap.Find(WeaponSlot);
}

bool UMovesetManager::HasMovesetInSlot(EEquipmentSlot WeaponSlot) const
{
	return SocketedMovesetsMap.Contains(WeaponSlot);
}

int32 UMovesetManager::GetMovesetLevelForSlot(EEquipmentSlot WeaponSlot) const
{
	const FMovesetInstance* Found = SocketedMovesetsMap.Find(WeaponSlot);
	return Found ? Found->CurrentLevel : 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Corruption
// ─────────────────────────────────────────────────────────────────────────────

bool UMovesetManager::CorruptMoveset(EEquipmentSlot WeaponSlot)
{
	FMovesetInstance* Moveset = SocketedMovesetsMap.Find(WeaponSlot);
	if (!Moveset || !Moveset->IsValid() || Moveset->bIsCorrupted)
	{
		return false;
	}

	UMovesetData* Data = Moveset->MovesetData.LoadSynchronous();
	if (!Data || !Data->bCanBeCorrupted)
	{
		return false;
	}

	// Weighted random selection
	const int32 TotalWeight = Data->WeightNoChange + Data->WeightEnhanced
		+ Data->WeightBricked + Data->WeightTransmuted;

	if (TotalWeight <= 0)
	{
		return false;
	}

	const int32 Roll = FMath::RandRange(0, TotalWeight - 1);
	EMovesetCorruptionResult Result = EMovesetCorruptionResult::MCR_NoChange;

	int32 Cumulative = Data->WeightNoChange;
	if (Roll < Cumulative)
	{
		Result = EMovesetCorruptionResult::MCR_NoChange;
	}
	else
	{
		Cumulative += Data->WeightEnhanced;
		if (Roll < Cumulative)
		{
			Result = EMovesetCorruptionResult::MCR_Enhanced;
		}
		else
		{
			Cumulative += Data->WeightBricked;
			if (Roll < Cumulative)
			{
				Result = EMovesetCorruptionResult::MCR_Bricked;
			}
			else
			{
				Result = EMovesetCorruptionResult::MCR_Transmuted;
			}
		}
	}

	Moveset->bIsCorrupted = true;
	Moveset->CorruptionResult = Result;

	switch (Result)
	{
	case EMovesetCorruptionResult::MCR_Enhanced:
		// +1 level beyond normal cap
		Moveset->CurrentLevel = FMath::Min(Moveset->CurrentLevel + 1, Data->MaxLevel + 1);
		// Grant bonus ability if defined
		if (Data->CorruptionBonusAbilityClass && AbilitySystemComponent)
		{
			FGameplayAbilitySpec Spec(Data->CorruptionBonusAbilityClass, Moveset->CurrentLevel);
			FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);
			Moveset->GrantedAbilityHandles.Add(Handle);
		}
		UE_LOG(LogMovesetManager, Log, TEXT("CorruptMoveset: Enhanced — bonus level granted"));
		break;

	case EMovesetCorruptionResult::MCR_Bricked:
		// Destroy the moveset
		RevokeMovesetAbilities(WeaponSlot);
		RemoveMovesetEntry(WeaponSlot);
		UE_LOG(LogMovesetManager, Log, TEXT("CorruptMoveset: Bricked — moveset destroyed"));
		OnMovesetCorrupted.Broadcast(WeaponSlot, Result);
		return true;

	case EMovesetCorruptionResult::MCR_Transmuted:
		// Transmutation would need a pool of compatible movesets — designers can
		// implement the replacement logic in Blueprint by listening to OnMovesetCorrupted
		UE_LOG(LogMovesetManager, Log, TEXT("CorruptMoveset: Transmuted — Blueprint handles replacement"));
		break;

	case EMovesetCorruptionResult::MCR_NoChange:
		UE_LOG(LogMovesetManager, Log, TEXT("CorruptMoveset: No change — moveset locked but unchanged"));
		break;

	default:
		break;
	}

	// Update the entry in the array so it replicates
	SetMovesetEntry(WeaponSlot, *Moveset);
	OnMovesetCorrupted.Broadcast(WeaponSlot, Result);
	return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// XP & Leveling
// ─────────────────────────────────────────────────────────────────────────────

void UMovesetManager::AwardMovesetXP(EEquipmentSlot WeaponSlot, int64 XPAmount)
{
	FMovesetInstance* Moveset = SocketedMovesetsMap.Find(WeaponSlot);
	if (!Moveset || !Moveset->CanLevelUp(0) || !Moveset->IsValid())
	{
		return;
	}

	UMovesetData* Data = Moveset->MovesetData.LoadSynchronous();
	if (!Data)
	{
		return;
	}

	Moveset->CurrentXP += XPAmount;

	// Check for level-ups (may level multiple times from a large XP award)
	while (Moveset->CanLevelUp(Data->MaxLevel))
	{
		const int64 Required = Data->GetXPRequiredForLevel(Moveset->CurrentLevel + 1);
		if (Moveset->CurrentXP < Required)
		{
			break;
		}

		const int32 OldLevel = Moveset->CurrentLevel;
		Moveset->CurrentLevel++;
		UE_LOG(LogMovesetManager, Log,
			TEXT("AwardMovesetXP: Moveset leveled up to %d in slot %d"),
			Moveset->CurrentLevel, static_cast<int32>(WeaponSlot));

		ApplyLevelUp(WeaponSlot, *Moveset, Moveset->CurrentLevel);
		OnMovesetLevelUp.Broadcast(WeaponSlot, OldLevel, Moveset->CurrentLevel);
	}

	SetMovesetEntry(WeaponSlot, *Moveset);
}

void UMovesetManager::AwardMovesetXPToAll(int64 XPAmount)
{
	for (auto& Entry : SocketedMovesetsMap)
	{
		AwardMovesetXP(Entry.Key, XPAmount);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Ability grant / revoke
// ─────────────────────────────────────────────────────────────────────────────

void UMovesetManager::GrantMovesetAbilities(EEquipmentSlot Slot, FMovesetInstance& Moveset)
{
	if (!AbilitySystemComponent || !Moveset.IsValid())
	{
		return;
	}

	UMovesetData* Data = Moveset.MovesetData.LoadSynchronous();
	if (!Data)
	{
		return;
	}

	// Grant primary ability
	if (Data->PrimaryAbilityClass)
	{
		FGameplayAbilitySpec Spec(Data->PrimaryAbilityClass, Moveset.CurrentLevel);
		Moveset.GrantedAbilityHandles.Add(AbilitySystemComponent->GiveAbility(Spec));
	}

	// Grant combo abilities
	for (TSubclassOf<UGameplayAbility> ComboClass : Data->ComboAbilityClasses)
	{
		if (ComboClass)
		{
			FGameplayAbilitySpec Spec(ComboClass, Moveset.CurrentLevel);
			Moveset.GrantedAbilityHandles.Add(AbilitySystemComponent->GiveAbility(Spec));
		}
	}

	// Grant corruption bonus ability if applicable
	if (Moveset.bIsCorrupted
		&& Moveset.CorruptionResult == EMovesetCorruptionResult::MCR_Enhanced
		&& Data->CorruptionBonusAbilityClass)
	{
		FGameplayAbilitySpec Spec(Data->CorruptionBonusAbilityClass, Moveset.CurrentLevel);
		Moveset.GrantedAbilityHandles.Add(AbilitySystemComponent->GiveAbility(Spec));
	}
}

void UMovesetManager::RevokeMovesetAbilities(EEquipmentSlot Slot)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	FMovesetInstance* Moveset = SocketedMovesetsMap.Find(Slot);
	if (!Moveset)
	{
		return;
	}

	for (const FGameplayAbilitySpecHandle& Handle : Moveset->GrantedAbilityHandles)
	{
		AbilitySystemComponent->ClearAbility(Handle);
	}
	Moveset->GrantedAbilityHandles.Empty();
}

void UMovesetManager::ApplyLevelUp(EEquipmentSlot Slot, FMovesetInstance& Moveset, int32 NewLevel)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Revoke then re-grant at the new level so ability specs pick up the new level value
	RevokeMovesetAbilities(Slot);
	GrantMovesetAbilities(Slot, Moveset);
}

// ─────────────────────────────────────────────────────────────────────────────
// Equipment change callback
// ─────────────────────────────────────────────────────────────────────────────

void UMovesetManager::HandleEquipmentChanged(EEquipmentSlot Slot,
	UItemInstance* NewItem, UItemInstance* OldItem)
{
	// Only care about weapon slots
	if (Slot != EEquipmentSlot::ES_MainHand
		&& Slot != EEquipmentSlot::ES_OffHand
		&& Slot != EEquipmentSlot::ES_TwoHand)
	{
		return;
	}

	if (!NewItem)
	{
		// Weapon removed — revoke moveset abilities but keep the moveset data
		// so it can be re-socketed when a new weapon is equipped.
		RevokeMovesetAbilities(Slot);
	}
	else
	{
		// New weapon equipped — re-grant any socketed moveset abilities
		FMovesetInstance* Moveset = SocketedMovesetsMap.Find(Slot);
		if (Moveset && Moveset->IsValid())
		{
			GrantMovesetAbilities(Slot, *Moveset);
		}
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Map / array helpers
// ─────────────────────────────────────────────────────────────────────────────

void UMovesetManager::SetMovesetEntry(EEquipmentSlot Slot, const FMovesetInstance& Moveset)
{
	// Update map
	SocketedMovesetsMap.FindOrAdd(Slot) = Moveset;

	// Update array (for replication)
	for (FEquipmentSlotMovesetEntry& Entry : SocketedMovesetsArray)
	{
		if (Entry.Slot == Slot)
		{
			Entry.Moveset = Moveset;
			return;
		}
	}
	SocketedMovesetsArray.Add(FEquipmentSlotMovesetEntry(Slot, Moveset));
}

void UMovesetManager::RemoveMovesetEntry(EEquipmentSlot Slot)
{
	SocketedMovesetsMap.Remove(Slot);
	SocketedMovesetsArray.RemoveAll([Slot](const FEquipmentSlotMovesetEntry& E)
	{
		return E.Slot == Slot;
	});
}

void UMovesetManager::RebuildMovesetsMap()
{
	SocketedMovesetsMap.Empty();
	for (const FEquipmentSlotMovesetEntry& Entry : SocketedMovesetsArray)
	{
		SocketedMovesetsMap.Add(Entry.Slot, Entry.Moveset);
	}
}

void UMovesetManager::OnRep_SocketedMovesets()
{
	RebuildMovesetsMap();
}

// ─────────────────────────────────────────────────────────────────────────────
// Server RPCs
// ─────────────────────────────────────────────────────────────────────────────

void UMovesetManager::Server_SocketMoveset_Implementation(EEquipmentSlot WeaponSlot,
	FMovesetInstance Moveset)
{
	FMovesetInstance Prev;
	SocketMoveset(WeaponSlot, Moveset, Prev);
}

void UMovesetManager::Server_UnsocketMoveset_Implementation(EEquipmentSlot WeaponSlot)
{
	FMovesetInstance Out;
	UnsocketMoveset(WeaponSlot, Out);
}

void UMovesetManager::Server_CorruptMoveset_Implementation(EEquipmentSlot WeaponSlot)
{
	CorruptMoveset(WeaponSlot);
}

// ─────────────────────────────────────────────────────────────────────────────
// Initialisation
// ─────────────────────────────────────────────────────────────────────────────

void UMovesetManager::CacheComponents()
{
	if (AActor* Owner = GetOwner())
	{
		AbilitySystemComponent = Owner->FindComponentByClass<UAbilitySystemComponent>();
		EquipmentManager       = Owner->FindComponentByClass<UEquipmentManager>();
	}
}
