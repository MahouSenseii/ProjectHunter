// Character/Component/MovesetManager.h
// Manages weapon movesets — PoE1-style skill sockets.
// Lives on APHBaseCharacter.  Listens to EquipmentManager for weapon changes,
// grants/revokes GAS abilities when a moveset is socketed/unsocketed, and
// provides the corruption and XP-leveling APIs.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/Library/ItemEnums.h"
#include "Item/Moveset/MovesetStructs.h"
#include "MovesetManager.generated.h"

class UAbilitySystemComponent;
class UEquipmentManager;
class UItemInstance;
class UMovesetData;

DECLARE_LOG_CATEGORY_EXTERN(LogMovesetManager, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
// Delegates
// ─────────────────────────────────────────────────────────────────────────────
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMovesetSocketed,
	EEquipmentSlot, WeaponSlot, const FMovesetInstance&, Moveset);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMovesetUnsocketed,
	EEquipmentSlot, WeaponSlot);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnMovesetLevelUp,
	EEquipmentSlot, WeaponSlot, int32, OldLevel, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMovesetCorrupted,
	EEquipmentSlot, WeaponSlot, EMovesetCorruptionResult, Result);

// ─────────────────────────────────────────────────────────────────────────────
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UMovesetManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UMovesetManager();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ── Socketing API ─────────────────────────────────────────────────────────

	/**
	 * Socket a moveset into the weapon at @param WeaponSlot.
	 * - Validates weapon is equipped and moveset is compatible.
	 * - If a moveset is already socketed it is returned in OutPreviousMoveset.
	 * - Abilities are granted immediately to the ASC.
	 * Server-authoritative — clients call this and it routes via Server RPC.
	 */
	UFUNCTION(BlueprintCallable, Category = "Moveset")
	bool SocketMoveset(EEquipmentSlot WeaponSlot, const FMovesetInstance& Moveset,
		FMovesetInstance& OutPreviousMoveset);

	/**
	 * Remove the moveset from @param WeaponSlot.
	 * Abilities are revoked from the ASC.
	 * Returned in OutMoveset so the caller can give it back to inventory.
	 */
	UFUNCTION(BlueprintCallable, Category = "Moveset")
	bool UnsocketMoveset(EEquipmentSlot WeaponSlot, FMovesetInstance& OutMoveset);

	/**
	 * Check whether @param Data is compatible with the weapon in @param WeaponSlot.
	 * Returns false if no weapon is equipped or types don't match.
	 */
	UFUNCTION(BlueprintPure, Category = "Moveset")
	bool IsCompatibleWithEquippedWeapon(EEquipmentSlot WeaponSlot,
		const UMovesetData* Data) const;

	/**
	 * Get read-only pointer to the active moveset in @param WeaponSlot.
	 * Returns nullptr if no moveset is socketed.
	 * C++ only — UHT cannot expose raw USTRUCT pointers via UFUNCTION.
	 * Blueprint callers: use HasMovesetInSlot / GetMovesetLevelForSlot instead.
	 */
	const FMovesetInstance* GetMovesetForSlot(EEquipmentSlot WeaponSlot) const;

	/** Blueprint-safe: returns true if a moveset is currently socketed in this slot. */
	UFUNCTION(BlueprintPure, Category = "Moveset")
	bool HasMovesetInSlot(EEquipmentSlot WeaponSlot) const;

	/** Blueprint-safe: returns the current level of the moveset in this slot, or 0 if empty. */
	UFUNCTION(BlueprintPure, Category = "Moveset")
	int32 GetMovesetLevelForSlot(EEquipmentSlot WeaponSlot) const;

	// ── Corruption ────────────────────────────────────────────────────────────

	/**
	 * Attempt to corrupt the moveset in @param WeaponSlot.
	 * Outcome is seeded-random; result stored on FMovesetInstance.bIsCorrupted.
	 * - Enhanced: +1 level, CorruptionBonusAbility granted if set.
	 * - Bricked:  moveset destroyed, slot cleared.
	 * - Transmuted: replaced with random compatible moveset at same level.
	 * - NoChange: moveset is still locked (corruption attempt marks it corrupted).
	 * Returns false if slot is empty or moveset is already corrupted.
	 * Server-only.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Moveset")
	bool CorruptMoveset(EEquipmentSlot WeaponSlot);

	// ── XP & Leveling ─────────────────────────────────────────────────────────

	/**
	 * Award @param XPAmount XP to the moveset in @param WeaponSlot.
	 * If the moveset levels up OnMovesetLevelUp is broadcast.
	 * Typically called by CharacterProgressionManager on kill.
	 * Server-only.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Moveset")
	void AwardMovesetXP(EEquipmentSlot WeaponSlot, int64 XPAmount);

	/**
	 * Award XP to ALL currently socketed movesets (main hand + off hand).
	 * Convenience wrapper for kill XP distribution.
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Moveset")
	void AwardMovesetXPToAll(int64 XPAmount);

	// ── Events ────────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintAssignable, Category = "Moveset|Events")
	FOnMovesetSocketed OnMovesetSocketed;

	UPROPERTY(BlueprintAssignable, Category = "Moveset|Events")
	FOnMovesetUnsocketed OnMovesetUnsocketed;

	UPROPERTY(BlueprintAssignable, Category = "Moveset|Events")
	FOnMovesetLevelUp OnMovesetLevelUp;

	UPROPERTY(BlueprintAssignable, Category = "Moveset|Events")
	FOnMovesetCorrupted OnMovesetCorrupted;

	/** Listener entrypoint wired by UCharacterSystemCoordinatorComponent. */
	UFUNCTION()
	void HandleEquipmentChanged(EEquipmentSlot Slot, UItemInstance* NewItem,
		UItemInstance* OldItem);

	// ── Replicated state ──────────────────────────────────────────────────────

	/**
	 * Array version of SocketedMovesetsMap (TMaps can't replicate).
	 * Do not modify directly — use SocketMoveset/UnsocketMoveset.
	 */
	UPROPERTY(ReplicatedUsing = OnRep_SocketedMovesets, BlueprintReadOnly,
		Category = "Moveset")
	TArray<FEquipmentSlotMovesetEntry> SocketedMovesetsArray;

protected:
	// ── Replication ───────────────────────────────────────────────────────────

	UFUNCTION()
	void OnRep_SocketedMovesets();

	// ── Internal helpers ──────────────────────────────────────────────────────

	/** Grant all abilities for the moveset in Slot to the ASC. */
	void GrantMovesetAbilities(EEquipmentSlot Slot, FMovesetInstance& Moveset);

	/** Revoke all abilities for the moveset in Slot from the ASC. */
	void RevokeMovesetAbilities(EEquipmentSlot Slot);

	/** Rebuild the fast-lookup TMap from SocketedMovesetsArray. */
	void RebuildMovesetsMap();

	/** Add/update an entry in both the array and map. */
	void SetMovesetEntry(EEquipmentSlot Slot, const FMovesetInstance& Moveset);

	/** Remove an entry from both the array and map. */
	void RemoveMovesetEntry(EEquipmentSlot Slot);

	void CacheComponents();

	void ApplyLevelUp(EEquipmentSlot Slot, FMovesetInstance& Moveset, int32 NewLevel);

	// ── Cached component refs ─────────────────────────────────────────────────

	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UEquipmentManager> EquipmentManager;

	/** Fast lookup — NOT replicated; rebuilt from array on OnRep. */
	UPROPERTY(Transient)
	TMap<EEquipmentSlot, FMovesetInstance> SocketedMovesetsMap;

	// ── Server RPCs ───────────────────────────────────────────────────────────

	UFUNCTION(Server, Reliable)
	void Server_SocketMoveset(EEquipmentSlot WeaponSlot, FMovesetInstance Moveset);

	UFUNCTION(Server, Reliable)
	void Server_UnsocketMoveset(EEquipmentSlot WeaponSlot);

	UFUNCTION(Server, Reliable)
	void Server_CorruptMoveset(EEquipmentSlot WeaponSlot);
};
