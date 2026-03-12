// Interactable/Interface/Interactable.h
#pragma once

#include "CoreMinimal.h"
#include "Interactable/Library/InteractionEnumLibrary.h"
#include "Interactable/Library/InteractionStructLibrary.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for creating interactable objects in the game.
 * Provides functionality for basic interactions, hold interactions,
 * mashing interactions, and tooltip support.
 */
class ALS_PROJECTHUNTER_API IInteractable
{
	GENERATED_BODY()

public:
	// ═══════════════════════════════════════════════
	// BASIC INTERACTION (Tap/Single Press)
	// ═══════════════════════════════════════════════

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnInteract(AActor* Interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* Interactor) const;

	// ═══════════════════════════════════════════════
	// INTERACTION TYPE (Which input method to use)
	// ═══════════════════════════════════════════════

	/**
	 * Get the interaction type
	 * @return Which type of interaction this supports
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	EInteractionType GetInteractionType() const;

	// ═══════════════════════════════════════════════
	// FOCUS EVENTS (For highlighting/widget display)
	// ═══════════════════════════════════════════════

	/**
	 * Called when player starts looking at this interactable
	 * @param Interactor - The actor now focusing on this
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnBeginFocus(AActor* Interactor);

	/**
	 * Called when player stops looking at this interactable
	 * @param Interactor - The actor no longer focusing on this
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void OnEndFocus(AActor* Interactor);

	// ═══════════════════════════════════════════════
	// HIGHLIGHT SUPPORT
	// ═══════════════════════════════════════════════

	/**
	 * Default highlight style for this interactable.
	 * Interactor / InteractionManager may override color/width/threshold
	 * using player settings before applying to the post-process system.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Highlight")
	FInteractableHighlightStyle GetHighlightStyle() const;

	// ═══════════════════════════════════════════════
	// HOLD INTERACTION
	// ═══════════════════════════════════════════════

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Hold")
	float GetTapHoldThreshold() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Hold")
	float GetHoldDuration() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Hold")
	void OnHoldInteractionStart(AActor* Interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Hold")
	void OnHoldInteractionUpdate(AActor* Interactor, float Progress);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Hold")
	void OnHoldInteractionComplete(AActor* Interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Hold")
	void OnHoldInteractionCancelled(AActor* Interactor);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Hold")
	FText GetHoldInteractionText() const;

	// ═══════════════════════════════════════════════
	// MASHING INTERACTION
	// ═══════════════════════════════════════════════

	/** How many presses required to complete */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Mash")
	int32 GetRequiredMashCount() const;

	/** How fast progress decays (per second) when not mashing */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Mash")
	float GetMashDecayRate() const;

	/** Called when mashing starts (first press) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Mash")
	void OnMashInteractionStart(AActor* Interactor);

	/** Called each time player presses during mashing */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Mash")
	void OnMashInteractionUpdate(AActor* Interactor, int32 CurrentCount, int32 RequiredCount, float Progress);

	/** Called when mashing completes successfully */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Mash")
	void OnMashInteractionComplete(AActor* Interactor);

	/** Called when mashing fails (progress decayed to 0) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Mash")
	void OnMashInteractionFailed(AActor* Interactor);

	/** Get mashing interaction text */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Mash")
	FText GetMashInteractionText() const;

	// ═══════════════════════════════════════════════
	// TOOLTIP SUPPORT
	// ═══════════════════════════════════════════════

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Tooltip")
	bool HasTooltip() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Tooltip")
	UObject* GetTooltipData() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Tooltip")
	FVector GetTooltipWorldLocation() const;

	// ═══════════════════════════════════════════════
	// INTERACTION SUPPORT
	// ═══════════════════════════════════════════════

	/**
	 * Get interaction action input asset
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	UInputAction* GetInputAction() const;

	/**
	 * Get interaction display text
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionText() const;

	/**
	 * Get widget offset (world space)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FVector GetWidgetOffset() const;
};