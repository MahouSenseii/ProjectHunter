// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Character/HUD/HunterHUDBaseWidget.h"
#include "GameplayEffectTypes.h"
#include "AttributeSet.h"
#include "AbilitySystem/Library/MMCEnumLibrary.h"
#include "Engine/TimerHandle.h"
#include "Styling/SlateTypes.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "HunterHUDResourceWidget.generated.h"

// EHunterResourceType is defined in AbilitySystem/Library/MMCEnumLibrary.h

class UProgressBar;
class USizeBox;
class UAbilitySystemComponent;

/**
 * HUD widget that tracks a three-part resource: Current, Max, and Reserved.
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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource")
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
	// Bar feel — interpolation speeds for the smoothed display values
	// ─────────────────────────────────────────────────────────────────────────

	/** How fast the fill bar catches up to the real value. Higher = snappier. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Feel", meta = (ClampMin = "0.0"))
	float FillInterpSpeed = 12.f;

	/** Seconds the damage-lag trail waits after a loss before it starts catching up. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Feel", meta = (ClampMin = "0.0"))
	float DamageLagDelay = 0.4f;

	/** How fast the damage-lag trail catches up once the delay elapses. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Feel", meta = (ClampMin = "0.0"))
	float DamageLagInterpSpeed = 6.f;

	// Bar appearance - applied to the bound ProgressBars when available.

	/** Fill direction for Current and DamageLag. Reserved always uses the opposite direction. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Bar")
	TEnumAsByte<EProgressBarFillType::Type> BarFillType = EProgressBarFillType::LeftToRight;

	/** Fill color used by the Current bar. DamageLag and Reserved derive darker translucent colors from this. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Bar")
	FLinearColor CurrentFillColor = FLinearColor::White;

	/** Enables applying ProgressBarImageStyle during PreConstruct. Leave off to keep each BP progress bar's own style. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Bar")
	bool bApplyProgressBarImageStyle = false;

	/** Optional image style source. SetImage uses only BackgroundImage and FillImage from this struct. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Bar", meta = (EditCondition = "bApplyProgressBarImageStyle"))
	FProgressBarStyle ProgressBarImageStyle;

	/** Optional SizeBox width override. Values <= 0 clear the override. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Bar", meta = (ClampMin = "0.0"))
	float BarWidthOverride = 0.f;

	/** Optional SizeBox height override. Values <= 0 clear the override. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Bar", meta = (ClampMin = "0.0"))
	float BarHeightOverride = 0.f;

	/** Forces a visible sample in the UMG designer so the bar can be tuned outside PIE. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Designer")
	bool bShowDesignerPreview = true;

	/** Designer-only Current preview percent. Runtime values still come from the bound character. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Designer", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bShowDesignerPreview"))
	float DesignerPreviewFillPercent = 0.65f;

	/** Designer-only DamageLag preview percent. Keep this above Current to see the trailing segment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Designer", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bShowDesignerPreview"))
	float DesignerPreviewDamageLagPercent = 0.85f;

	/** Designer-only Reserved preview percent. Runtime values still come from the bound character. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Designer", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bShowDesignerPreview"))
	float DesignerPreviewReservedPercent = 0.20f;

	// ─────────────────────────────────────────────────────────────────────────
	// Managed visibility (optional) — C++ shows/hides the bar by policy.
	// Static styling can be driven by the BP defaults or the setters below.
	// ─────────────────────────────────────────────────────────────────────────

	/** When true, C++ drives visibility (player Stamina hides when full; enemy bars show briefly on change). Turn off to control visibility entirely in BP. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD|Resource|Visibility")
	bool bManageVisibility = true;

	/** When true, non-player bars stay hidden until Current changes, then auto-hide after NonPlayerAutoHideDelay. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Visibility")
	bool bAutoHideNonPlayerBar = true;

	/** Player Stamina bar hides at/above this fill fraction (it only matters while being spent). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Visibility", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StaminaHideWhenFullThreshold = 0.85f;

	/** Non-player bar stays visible this many seconds after a value change when bAutoHideNonPlayerBar is true. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD|Resource|Visibility", meta = (ClampMin = "0.0"))
	float NonPlayerAutoHideDelay = 3.0f;

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

	// ─────────────────────────────────────────────────────────────────────────
	// Smoothed display values — bound ProgressBars use these directly; BP can also
	// read them when custom visuals need to drive the percent manually.
	// ─────────────────────────────────────────────────────────────────────────

	/** Interpolated fill (0–1). Bind the main bar's Percent to this. */
	UFUNCTION(BlueprintPure, Category = "HUD|Resource")
	float GetDisplayFillPercent() const { return DisplayFillPercent; }

	/** Trailing "damage lag" fill (0–1): snaps up on gain, lags behind on loss. */
	UFUNCTION(BlueprintPure, Category = "HUD|Resource")
	float GetDamageLagPercent() const { return DamageLagPercent; }

	/** Interpolated reserved fraction (0–1). Bind the reserved bar's Percent to this. */
	UFUNCTION(BlueprintPure, Category = "HUD|Resource")
	float GetDisplayReservedPercent() const { return DisplayReservedPercent; }

	/** Updates and returns the damage-lag percent using FInterpTo(Current, GetFillPercent, DeltaTime, InterpSpeed). */
	UFUNCTION(BlueprintCallable, Category = "HUD|Resource|Bar")
	float DamageLag();

	/** Sets fill direction on Current/DamageLag and the reversed direction on Reserved. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Resource|Bar")
	void SetBarFillType(TEnumAsByte<EProgressBarFillType::Type> InBarFillType);

	/** Sets the Current bar color and derives darker translucent colors for DamageLag and Reserved. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Resource|Bar")
	void SetColor(FLinearColor InCurrentFillColor);

	/** Applies BackgroundImage and FillImage from the supplied style to every bound progress bar. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Resource|Bar")
	void SetImage(const FProgressBarStyle& InProgressBarStyle);

	/** Applies SizeBox width/height overrides. Values <= 0 clear the matching override. */
	UFUNCTION(BlueprintCallable, Category = "HUD|Resource|Bar")
	void SetSize(float InWidthOverride, float InHeightOverride);

protected:
	// ─────────────────────────────────────────────────────────────────────────
	// HunterHUDBaseWidget overrides
	// ─────────────────────────────────────────────────────────────────────────

	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;
	virtual void NativePreConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Bound widgets - name these exactly in the widget Blueprint designer.

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Resource|Widgets", meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_DamageLag;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Resource|Widgets", meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_Current;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Resource|Widgets", meta = (BindWidget))
	TObjectPtr<UProgressBar> Bar_Reserved;

	UPROPERTY(BlueprintReadOnly, Category = "HUD|Resource|Widgets", meta = (BindWidget))
	TObjectPtr<USizeBox> BarSize;

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

	bool TryBindToAbilitySystem(APHBaseCharacter* Character);
	void UnbindAttributeDelegates();
	void ScheduleBindingRetry();
	void HandleBindingRetry();

	/** Broadcasts OnResourceUpdated with current cached values. */
	void BroadcastResourceState();

	void ApplyConfiguredBarAppearance();
	void ApplyProgressBarImages(const FProgressBarStyle& InProgressBarStyle);
	void ApplyDesignerPreview();
	void ApplyBarPercents() const;
	float UpdateDamageLagPercent(float InDeltaTime, float TargetFill);

	/** Applies the visibility policy; called on init and on each Current-value change. */
	void UpdateManagedVisibility(float ChangeDelta);

	/** Timer callback: hides a non-player bar after NonPlayerAutoHideDelay. */
	void HideBar();

	// ─────────────────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────────────────

	float CachedCurrent  = 0.f;
	float CachedMax      = 1.f;  // Start at 1 to avoid div-by-zero before init
	float CachedReserved = 0.f;

	// Smoothed values driven in NativeTick; bars bind their Percent to the getters.
	float DisplayFillPercent      = 0.f;
	float DamageLagPercent        = 0.f;
	float DisplayReservedPercent  = 0.f;
	float DamageLagDelayRemaining = 0.f;
	bool  bDisplayInitialized     = false;

	// Managed-visibility state.
	bool         bIsPlayerOwned = false;
	UPROPERTY()
	TWeakObjectPtr<UAbilitySystemComponent> BoundASC;

	FTimerHandle NonPlayerHideTimerHandle;
	FTimerHandle BindingRetryTimerHandle;

	FDelegateHandle CurrentHandle;
	FDelegateHandle MaxHandle;
	FDelegateHandle ReservedHandle;
};
