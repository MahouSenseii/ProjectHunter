// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HunterHUDBaseWidget.generated.h"

class APHBaseCharacter;

/**
 * Abstract base for all player-stat HUD widgets.
 *
 * Lifecycle:
 *   AHunterHUD creates the widget and calls InitializeForCharacter() once the
 *   local pawn is ready.  On pawn change (respawn / repossession) the HUD calls
 *   ReleaseCharacter() followed by InitializeForCharacter() with the new pawn so
 *   all delegate handles are cleanly swapped over.
 *
 * Subclassing:
 *   Override NativeInitializeForCharacter() to bind to GAS attribute delegates or
 *   component delegates.  Override NativeReleaseCharacter() to remove those handles.
 *   Use the BlueprintImplementableEvent hooks to drive the actual UMG visuals in BP.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UHunterHUDBaseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ─────────────────────────────────────────────────────────────────────────
	// Public API — called by AHunterHUD
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Bind this widget to a character.  Safe to call multiple times — will
	 * automatically release any previously bound character first.
	 */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void InitializeForCharacter(APHBaseCharacter* Character);

	/** Unbind from the current character and clear all delegate handles. */
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ReleaseCharacter();

	/** Returns true if currently bound to a live character. */
	UFUNCTION(BlueprintPure, Category = "HUD")
	bool IsBoundToCharacter() const { return BoundCharacter.IsValid(); }

	/** Returns the currently bound character (may be null). */
	UFUNCTION(BlueprintPure, Category = "HUD")
	APHBaseCharacter* GetBoundCharacter() const { return BoundCharacter.Get(); }

protected:
	// ─────────────────────────────────────────────────────────────────────────
	// Overridable hooks for subclasses
	// ─────────────────────────────────────────────────────────────────────────

	/**
	 * Called after BoundCharacter is set — bind your GAS / component delegates here.
	 * Character is guaranteed non-null and valid at this point.
	 */
	virtual void NativeInitializeForCharacter(APHBaseCharacter* Character) {}

	/**
	 * Called before BoundCharacter is cleared — remove your delegate handles here.
	 * BoundCharacter is still valid inside this call.
	 */
	virtual void NativeReleaseCharacter() {}

	/** Cleans up delegates when the widget itself is destroyed. */
	virtual void NativeDestruct() override;

	// ─────────────────────────────────────────────────────────────────────────
	// Blueprint events — implement visuals in BP
	// ─────────────────────────────────────────────────────────────────────────

	/** Fired after a character is bound and the initial stat snapshot has been sent. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnCharacterBound(APHBaseCharacter* Character);

	/** Fired just before the character binding is released. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnCharacterReleased();

	// ─────────────────────────────────────────────────────────────────────────
	// State
	// ─────────────────────────────────────────────────────────────────────────

	/** Weak reference so we don't extend the character's lifetime. */
	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	TWeakObjectPtr<APHBaseCharacter> BoundCharacter;
};
