// Character/HUD/StatusEffect/StatusEffectIconWidget.h
// Single status-effect icon displayed in the strip:  [Icon] [Duration text]
//
// USAGE IN UMG:
//   Create a Blueprint child of this class.  In the Blueprint event graph:
//   • BP_OnIconDataSet(IconTexture, RemainingTime, bIsBuff) — update Image widget,
//     time text, and tint the border accordingly.
//   • BP_OnTimeUpdate(RemainingTime, Progress) — animate the radial/sweep bar.
//   • BP_OnEffectExpired() — play a fade-out animation before RemoveFromParent.
//
// The C++ layer polls GAS each tick to update remaining time.  The Blueprint
// layer is responsible for ALL visuals; no assumptions about widget hierarchy.
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "StatusEffectIconWidget.generated.h"

class UTexture2D;

DECLARE_LOG_CATEGORY_EXTERN(LogStatusEffectIcon, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UStatusEffectIconWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ── Setup — called once by UStatusEffectHUDWidget after creation ──────────

	/**
	 * Bind this icon to a specific active GE handle.
	 * @param InASC            The ASC that owns the effect.
	 * @param InHandle         The active effect handle to display.
	 * @param InIcon           Texture to show in the icon slot.
	 * @param InEffectName     Localised name for tooltip / accessibility.
	 * @param InIsBuff         True = buff (green tint), False = debuff (red tint).
	 */
	UFUNCTION(BlueprintCallable, Category = "StatusEffect")
	void BindToEffect(UAbilitySystemComponent* InASC,
		FActiveGameplayEffectHandle InHandle,
		UTexture2D* InIcon,
		const FText& InEffectName,
		bool InIsBuff);

	/** Detach from the GE handle (called before RemoveFromParent). */
	UFUNCTION(BlueprintCallable, Category = "StatusEffect")
	void UnbindEffect();

	// ── Accessors ─────────────────────────────────────────────────────────────

	UFUNCTION(BlueprintPure, Category = "StatusEffect")
	FActiveGameplayEffectHandle GetHandle() const { return EffectHandle; }

	UFUNCTION(BlueprintPure, Category = "StatusEffect")
	float GetRemainingTime() const { return CachedRemainingTime; }

	/** 0.0 = expired, 1.0 = full duration remaining */
	UFUNCTION(BlueprintPure, Category = "StatusEffect")
	float GetNormalisedProgress() const;

	UFUNCTION(BlueprintPure, Category = "StatusEffect")
	bool IsBound() const { return EffectHandle.IsValid(); }

protected:
	// ── NativeTick — polls GAS for remaining time each frame ─────────────────
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ── Blueprint hooks — implement all visuals in BP ─────────────────────────

	/**
	 * Fired once immediately after BindToEffect — set up the initial icon state.
	 * @param Icon            The texture to display.
	 * @param RemainingTime   Total duration in seconds (or -1 for permanent).
	 * @param bBuff           True if this is a beneficial effect.
	 * @param EffectName      Localised display name.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "StatusEffect")
	void BP_OnIconDataSet(UTexture2D* Icon, float RemainingTime,
		bool bBuff, const FText& EffectName);

	/**
	 * Fired each tick while the effect is active.
	 * @param RemainingTime   Seconds remaining.
	 * @param Progress        0.0 (expired) → 1.0 (full duration).
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "StatusEffect")
	void BP_OnTimeUpdate(float RemainingTime, float Progress);

	/**
	 * Fired when the effect handle becomes invalid (effect expired or was removed).
	 * Play a fade-out animation here, then call RemoveFromParent when done.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "StatusEffect")
	void BP_OnEffectExpired();

private:
	// ── Internal state ────────────────────────────────────────────────────────

	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	FActiveGameplayEffectHandle EffectHandle;

	UPROPERTY(BlueprintReadOnly, Category = "StatusEffect",
		meta = (AllowPrivateAccess = "true"))
	float CachedRemainingTime = 0.0f;

	float TotalDuration = 0.0f;   // Cached at bind time for progress calc

	bool bExpiredBroadcast = false;
};
