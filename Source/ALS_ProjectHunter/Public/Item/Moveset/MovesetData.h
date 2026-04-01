// Item/Moveset/MovesetData.h
// Data asset that defines one weapon moveset (set of abilities + leveling curve).
// Create one asset per moveset in the editor and reference it via FMovesetInstance.
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Item/Library/ItemEnums.h"
#include "MovesetData.generated.h"

class UGameplayAbility;
class UCurveFloat;

UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UMovesetData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	// ── Identity ─────────────────────────────────────────────────────────────

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	FText MovesetName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = true))
	FText Description;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Identity")
	TObjectPtr<UTexture2D> Icon;

	// ── Abilities ─────────────────────────────────────────────────────────────

	/**
	 * Primary attack ability.  Granted when this moveset is socketed and the
	 * weapon is equipped.  Blueprint subclass drives the visual and hitbox.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TSubclassOf<UGameplayAbility> PrimaryAbilityClass;

	/**
	 * Additional combo or follow-up abilities granted alongside the primary.
	 * E.g. a three-hit combo would list each follow-up class here.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> ComboAbilityClasses;

	// ── Weapon Compatibility ──────────────────────────────────────────────────

	/**
	 * Weapon sub-types this moveset can be socketed into.
	 * Leave empty to allow any weapon type.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Compatibility")
	TArray<EItemSubType> CompatibleWeaponTypes;

	// ── Leveling ─────────────────────────────────────────────────────────────

	/** Hard cap on level (default 20, mirrors PoE1). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leveling",
		meta = (ClampMin = 1, ClampMax = 20))
	int32 MaxLevel = 20;

	/**
	 * XP required to reach each level.
	 * X axis = target level (2–MaxLevel), Y axis = cumulative XP required.
	 * If null a default exponential curve is used.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Leveling")
	TObjectPtr<UCurveFloat> XPCurve;

	// ── Corruption ────────────────────────────────────────────────────────────

	/** Whether this moveset can be submitted to a corruption attempt. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corruption")
	bool bCanBeCorrupted = true;

	/**
	 * Outcome weights (must conceptually sum to 100 for clear designer intent,
	 * but the code normalises them so any positive integers work).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corruption",
		meta = (ClampMin = 0))
	int32 WeightNoChange = 50;

	/** +1 level beyond cap, plus optionally grants CorruptionBonusAbilityClass */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corruption",
		meta = (ClampMin = 0))
	int32 WeightEnhanced = 25;

	/** Moveset is permanently destroyed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corruption",
		meta = (ClampMin = 0))
	int32 WeightBricked = 10;

	/** Replaced with a random moveset of the same level from the transmute pool */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corruption",
		meta = (ClampMin = 0))
	int32 WeightTransmuted = 15;

	/**
	 * Extra ability granted only on an Enhanced corruption outcome.
	 * Optional — leave null if Enhanced only applies the +1 level bonus.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Corruption")
	TSubclassOf<UGameplayAbility> CorruptionBonusAbilityClass;

	// ── Helpers ───────────────────────────────────────────────────────────────

	/**
	 * XP required to reach @param TargetLevel (must be >= 2).
	 * Uses XPCurve if set, otherwise falls back to: 500 * 2^(level-2).
	 */
	UFUNCTION(BlueprintPure, Category = "Moveset")
	int64 GetXPRequiredForLevel(int32 TargetLevel) const;

	/** Returns true if this moveset can be socketed into the given weapon subtype. */
	UFUNCTION(BlueprintPure, Category = "Moveset")
	bool IsCompatibleWithWeapon(EItemSubType WeaponSubType) const;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("MovesetData"), GetFName());
	}
};
