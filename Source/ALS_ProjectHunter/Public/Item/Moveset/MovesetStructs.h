// Item/Moveset/MovesetStructs.h
// Weapon Moveset System — PoE1-style skill sockets on weapons.
// Movesets are equippable skill packages that grant abilities when
// socketed into a compatible weapon.  They level independently,
// accumulate XP from kills, and can be corrupted (fixed state,
// but potentially with a bonus corruption outcome).
#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "Item/Library/ItemEnums.h"
#include "MovesetStructs.generated.h"

// ─────────────────────────────────────────────────────────────────────────────
// Corruption outcome — applied once on Corrupt action, permanent
// ─────────────────────────────────────────────────────────────────────────────
UENUM(BlueprintType)
enum class EMovesetCorruptionResult : uint8
{
	MCR_None        UMETA(DisplayName = "None"),
	/** +1 bonus level beyond normal cap; optionally grants an extra ability */
	MCR_Enhanced    UMETA(DisplayName = "Enhanced"),
	/** Moveset is destroyed — slot cleared, item becomes corrupted */
	MCR_Bricked     UMETA(DisplayName = "Bricked"),
	/** Replaced with a randomly selected moveset of equal level */
	MCR_Transmuted  UMETA(DisplayName = "Transmuted"),
	/** Nothing happens — the most common outcome */
	MCR_NoChange    UMETA(DisplayName = "No Change"),
};

// ─────────────────────────────────────────────────────────────────────────────
// Runtime moveset instance (lives on the weapon's ItemInstance)
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FMovesetInstance
{
	GENERATED_BODY()

	/** Soft reference — loaded on demand to avoid cooking all movesets upfront */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moveset")
	TSoftObjectPtr<class UMovesetData> MovesetData;

	/** Current level (1 – MaxLevel on UMovesetData).  Locked when corrupted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moveset", meta = (ClampMin = 1, ClampMax = 20))
	int32 CurrentLevel = 1;

	/** XP accumulated toward the next level.  Locked when corrupted. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moveset")
	int64 CurrentXP = 0;

	/**
	 * Quality 0-20 (like PoE gem quality).
	 * Adds a flat % bonus to all ability magnitudes.
	 * 20% quality = +20% effect.  Locked when corrupted.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moveset", meta = (ClampMin = 0, ClampMax = 20))
	int32 Quality = 0;

	/**
	 * Corruption state.  Once true: level, quality and data are frozen.
	 * The moveset can still be socketed and used — corruption only prevents
	 * further modification.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moveset")
	bool bIsCorrupted = false;

	/** Result of the corruption attempt — only meaningful when bIsCorrupted = true */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moveset")
	EMovesetCorruptionResult CorruptionResult = EMovesetCorruptionResult::MCR_None;

	// ── Runtime-only (not serialised) ─────────────────────────────────────────

	/** Handles to the abilities currently granted to the owning character's ASC.
	 *  Populated by MovesetManager when the weapon is equipped.
	 *  Transient — NOT saved.  Re-granted on equip. */
	UPROPERTY(Transient)
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;

	// ── Helpers ───────────────────────────────────────────────────────────────

	FORCEINLINE bool IsValid() const { return !MovesetData.IsNull(); }
	FORCEINLINE bool CanBeModified() const { return !bIsCorrupted && IsValid(); }
	FORCEINLINE bool CanLevelUp(int32 MaxLevel) const
	{
		return !bIsCorrupted && IsValid() && CurrentLevel < MaxLevel;
	}

	/** Quality multiplier to pass to abilities.  1.0 = 0%, 1.2 = 20%. */
	FORCEINLINE float GetQualityMultiplier() const
	{
		return 1.0f + (static_cast<float>(Quality) / 100.0f);
	}
};

// ─────────────────────────────────────────────────────────────────────────────
// Replication helper — one entry per weapon slot that has a moveset socketed
// ─────────────────────────────────────────────────────────────────────────────
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FEquipmentSlotMovesetEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	EEquipmentSlot Slot = EEquipmentSlot::ES_None;

	UPROPERTY(BlueprintReadOnly)
	FMovesetInstance Moveset;

	FEquipmentSlotMovesetEntry() = default;
	FEquipmentSlotMovesetEntry(EEquipmentSlot InSlot, const FMovesetInstance& InMoveset)
		: Slot(InSlot), Moveset(InMoveset)
	{}
};
