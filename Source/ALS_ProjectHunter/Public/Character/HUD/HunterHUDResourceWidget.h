// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Character/HUD/HunterHUDBaseWidget.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "AbilitySystem/Library/MMCEnumLibrary.h"
#include "HunterHUDResourceWidget.generated.h"

// EHunterResourceType is defined in AbilitySystem/Library/MMCEnumLibrary.h

/**
 * HUD widget that tracks a three-part resource: Current, Max, and Reserved.
 *
 * Visual model:
 *
 *   ┌─────────────────────────────┬──────────┬──────────┐
 *   │  Current (active bar)       │ Reserved │  Empty   │
 *   └─────────────────────────────┴──────────┴──────────┘
 *   <─────────────────── Max ──────────────────────────>
 *
 *   FillPercent     = Current  / Max   → drive the filled portion of the bar
 *   ReservedPercent = Reserved / Max   → drive the locked/greyed portion
 *
 * Setup (choose one):
 *   A) Set ResourceType in Blueprint defaults. The correct attribute triplet
 *      is resolved automatically in NativeInitializeForCharacter.
 *   B) Leave ResourceType at its default and set CurrentAttribute, MaxAttribute,
 *      and ReservedAttribute directly for a fully custom mapping.
 *
 * Blueprint implementation:
 *   Implement OnResourceUpdated to drive your progress bar.
 *   Implement OnCurrentValueChanged / OnMaxValueChanged / OnReservedValueChanged
 *   for fine-grained reactions (e.g. flash on damage, clamp animation on overheal).
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UHunterHUDResourceWidget : public UHunterHUDBaseWidget
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────────────────
	// Resource type — set this in BP defaults, everything else is automatic
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Which resource pool to track.
	 * Automatically wires CurrentAttribute / MaxAttribute / ReservedAttribute
	 * on initialization. Override the individual attributes below for custom mappings.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Resource")
	EHunterResourceType ResourceType = EHunterResourceType::Health;

	// ─────────────────────────────────────────────────────────────────────────
	// Attribute overrides — leave invalid to use the ResourceType preset
	// ─────────────────────────────────────────────────────────────────────────

	/** The live resource pool. Auto-set from ResourceType if left invalid. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Resource|Override")
	FGameplayAttribute CurrentAttribute;

	/**
	 * The effective maximum — the ceiling of the bar.
	 * Uses MaxEffective* variants so the reserved portion is already factored in.
	 * Auto-set from ResourceType if left invalid.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Resource|Override")
	FGameplayAttribute MaxAttribute;

	/**
	 * The reserved (locked) portion — shown as a greyed segment at the right of the bar.
	 * Auto-set from ResourceType if left invalid.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Resource|Override")
	FGameplayAttribute ReservedAttribute;

	// ─────────────────────────────────────────────────────────────────────────
	// Accessors — read current cached values from Blueprint
	// ─────────────────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "HUD|Resource")
	float GetCurrentValue() const { return CachedCurrent; }

	UFUNCTION(BlueprintPure, Category = "HUD|Resource")
	float GetMaxValue() const { return CachedMax; }

	UFUNCTION(BlueprintPure, Category = "HUD|Resource")
	float GetReservedValue() const { return CachedReserved; }

	/** Current / Max.  Drives the filled portion of the bar (0–1). */
	UFUNCTION(BlueprintPure, Category = "HUD|Resource")
	float GetFillPercent() const;

	/** Reserved / Max.  Drives the locked portion of the bar (0–1). */
	UFUNCTION(BlueprintPure, Category = "HUD|Resource")
	float GetReservedPercent() const;

protected:
	// ─────────────────────────────────────────────────────────────────────────
	// HunterHUDBaseWidget overrides
	// ─────────────────────────────────────────────────────────────────────────

	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

	// ─────────────────────────────────────────────────────────────────────────
	// Blueprint events — implement the bar visuals in BP
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Fired whenever any of the three tracked values changes.
	 * This is the primary hook for driving a progress bar widget.
	 *
	 * @param Current          Raw current value
	 * @param Max              Raw max value
	 * @param Reserved         Raw reserved value
	 * @param FillPercent      Current / Max  (0–1)
	 * @param ReservedPercent  Reserved / Max (0–1)
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Resource")
	void OnResourceUpdated(float Current, float Max, float Reserved,
	                       float FillPercent, float ReservedPercent);

	/** Fired only when Current changes — useful for damage/heal flash effects. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Resource")
	void OnCurrentValueChanged(float NewValue, float Delta);

	/** Fired only when Max changes — useful for scaling the bar container. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Resource")
	void OnMaxValueChanged(float NewValue);

	/** Fired only when Reserved changes — useful for animating the locked segment. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD|Resource")
	void OnReservedValueChanged(float NewValue);

private:
	// ─────────────────────────────────────────────────────────────────────────
	// Internal helpers
	// ─────────────────────────────────────────────────────────────────────────

	/** Resolves attributes from ResourceType if the individual fields are not set. */
	void ResolveAttributesFromResourceType();

	void HandleCurrentChanged(const FOnAttributeChangeData& Data);
	void HandleMaxChanged(const FOnAttributeChangeData& Data);
	void HandleReservedChanged(const FOnAttributeChangeData& Data);

	/** Broadcasts OnResourceUpdated with current cached values. */
	void BroadcastResourceState();

	// ─────────────────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────────────────

	float CachedCurrent  = 0.f;
	float CachedMax      = 1.f;  // Start at 1 to avoid div-by-zero before init
	float CachedReserved = 0.f;

	FDelegateHandle CurrentHandle;
	FDelegateHandle MaxHandle;
	FDelegateHandle ReservedHandle;
};
