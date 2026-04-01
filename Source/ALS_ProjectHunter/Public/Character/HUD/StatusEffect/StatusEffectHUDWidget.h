// Character/HUD/StatusEffect/StatusEffectHUDWidget.h
// Horizontal strip of UStatusEffectIconWidgets that reflects active GEs.
//
// ARCHITECTURE:
//   This widget extends UHunterHUDBaseWidget so AHunterHUD can manage its
//   lifetime with the same BindWidgetsToCharacter / HandlePawnChanged flow
//   as the existing stat widgets.
//
// HOW IT WORKS:
//   1. NativeInitializeForCharacter() subscribes to ASC's GE added/removed
//      delegates.
//   2. When a tagged GE is added, a UStatusEffectIconWidget is created and
//      added to the UHorizontalBox (or whatever container BP provides via
//      the IconContainerSlot binding).
//   3. When the GE is removed (or the icon's BP_OnEffectExpired fires),
//      the icon is removed from the container.
//
// BLUEPRINT SETUP:
//   - Create a WBP_StatusEffectHUD Blueprint child.
//   - Add a UHorizontalBox named "IconContainer" to your layout.
//   - Set IconWidgetClass to your WBP_StatusEffectIcon Blueprint child.
//   - Override BP_GetIconForEffect to return the right texture per tag.
//   - Set IconContainerSlotName to "IconContainer".
//
// TAG FILTERING:
//   Only GEs that grant at least one tag in StatusEffectTagFilter are shown.
//   Leave the filter empty to show ALL active GEs (not recommended).
//   Typical filter: "StatusEffect.Buff.*", "StatusEffect.Debuff.*"
#pragma once

#include "CoreMinimal.h"
#include "Character/HUD/HunterHUDBaseWidget.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "StatusEffectHUDWidget.generated.h"

class UStatusEffectIconWidget;
class UAbilitySystemComponent;
class UHorizontalBox;
class UTexture2D;

DECLARE_LOG_CATEGORY_EXTERN(LogStatusEffectHUD, Log, All);

// ─────────────────────────────────────────────────────────────────────────────
UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UStatusEffectHUDWidget : public UHunterHUDBaseWidget
{
	GENERATED_BODY()

public:
	// ── Configuration ─────────────────────────────────────────────────────────

	/**
	 * The widget class to instantiate for each active status effect.
	 * Must be a Blueprint child of UStatusEffectIconWidget.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "StatusEffect")
	TSubclassOf<UStatusEffectIconWidget> IconWidgetClass;

	/**
	 * GEs must grant at least one tag from this container to appear in the HUD.
	 * Use Gameplay Tag Query or leave empty to show all GEs.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "StatusEffect")
	FGameplayTagContainer StatusEffectTagFilter;

	/**
	 * Maximum number of icons displayed simultaneously.
	 * Older effects are hidden when the cap is exceeded (they are still tracked).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "StatusEffect",
		meta = (ClampMin = 1, ClampMax = 32))
	int32 MaxVisibleIcons = 12;

	/**
	 * Name of the UHorizontalBox (or UWrapBox) widget in the Blueprint layout.
	 * Icons will be added to this container via AddChildToHorizontalBox.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "StatusEffect|Layout")
	FName IconContainerSlotName = TEXT("IconContainer");

	// ── Public API ────────────────────────────────────────────────────────────

	/** Force a full refresh — removes all icons and re-adds from current GEs. */
	UFUNCTION(BlueprintCallable, Category = "StatusEffect")
	void RefreshAllIcons();

	/** Returns how many icon widgets are currently visible. */
	UFUNCTION(BlueprintPure, Category = "StatusEffect")
	int32 GetActiveIconCount() const { return ActiveIcons.Num(); }

protected:
	// ── UHunterHUDBaseWidget overrides ────────────────────────────────────────

	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) override;
	virtual void NativeReleaseCharacter() override;

	// ── Blueprint hooks ───────────────────────────────────────────────────────

	/**
	 * Implement in Blueprint to map a Gameplay Tag to an icon texture.
	 * Return nullptr to use the default icon (or hide the effect).
	 * @param GrantedTags  All tags granted by the effect.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "StatusEffect")
	UTexture2D* BP_GetIconForEffect(const FGameplayTagContainer& GrantedTags) const;

	/**
	 * Implement in Blueprint to decide if a GE is a buff (true) or debuff (false).
	 * Affects the tint applied to the icon border.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "StatusEffect")
	bool BP_IsEffectBuff(const FGameplayTagContainer& GrantedTags) const;

	/**
	 * Called when an icon is added to the strip.
	 * Use to play a "new effect" animation or sound.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "StatusEffect")
	void BP_OnIconAdded(UStatusEffectIconWidget* Icon);

	/**
	 * Called when an icon is removed from the strip.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "StatusEffect")
	void BP_OnIconRemoved(FActiveGameplayEffectHandle Handle);

private:
	// ── GE event handlers ─────────────────────────────────────────────────────

	void OnGameplayEffectAdded(UAbilitySystemComponent* ASC,
		const FGameplayEffectSpec& Spec,
		FActiveGameplayEffectHandle Handle);

	void OnGameplayEffectRemoved(const FActiveGameplayEffect& Effect);

	// ── Icon helpers ──────────────────────────────────────────────────────────

	void AddIconForEffect(UAbilitySystemComponent* ASC,
		FActiveGameplayEffectHandle Handle,
		const FGameplayEffectSpec& Spec);

	void RemoveIconForHandle(FActiveGameplayEffectHandle Handle);

	bool PassesTagFilter(const FGameplayTagContainer& GrantedTags) const;

	UHorizontalBox* GetIconContainer() const;

	// ── State ─────────────────────────────────────────────────────────────────

	/** Handle → icon widget mapping for fast removal lookup */
	UPROPERTY()
	TMap<FActiveGameplayEffectHandle, TObjectPtr<UStatusEffectIconWidget>> ActiveIcons;

	/** Delegate handles so we can unbind cleanly */
	FDelegateHandle OnEffectAddedHandle;
	FDelegateHandle OnEffectRemovedHandle;
};
