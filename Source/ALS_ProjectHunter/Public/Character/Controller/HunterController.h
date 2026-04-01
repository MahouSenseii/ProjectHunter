// Character/Controller/HunterController.h
// SIMPLIFIED - NO TICK DEPENDENCY!
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "InputActionValue.h"
#include "Character/ALSPlayerController.h"
#include "Interactable/Library/InteractionEnumLibrary.h"
#include "HunterController.generated.h"

// Forward declarations
class UInteractionManager;
class UInputMappingContext;

/**
 * DoOnce State - Helper for DoOnce pattern
 * Ensures code executes only once until explicitly reset.
 * Lives here because it is exclusively used by HunterController::DoOnce().
 */
USTRUCT(BlueprintType)
struct FDoOnceState
{
	GENERATED_BODY()

	bool bHasBeenInitialized = false;
	bool bIsClosed = false;

	FDoOnceState() = default;
};

/**
 * Hunter Player Controller 
 */
UCLASS()
class ALS_PROJECTHUNTER_API AHunterController : public AALSPlayerController
{
	GENERATED_BODY()

public:
	AHunterController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void OnPossess(APawn* NewPawn) override;
	

	// ═══════════════════════════════════════════════
	// INPUT HANDLERS (Route to Components)
	// ═══════════════════════════════════════════════

	/**
	 * Interact input handler
	 * Routes to InteractionManager - that's it! :D 
	 */
	UFUNCTION()
	void Interact(const FInputActionValue& Value);
	
	/**
	 * Menu input handler
	 */
	void Menu(const FInputActionValue& Value) const;
	
	/* Relays input action*/
	UFUNCTION(BlueprintCallable)
	const UInputAction* GetInputActionByName(const FString& InString) const;
	
	UFUNCTION(BlueprintCallable)
	float GetElapsedSeconds(const UInputAction* Action) const;
	
	UFUNCTION(BlueprintCallable)
	bool DoOnce(FDoOnceState& State, bool bReset, bool bStartClosed);

protected:
	// ═══════════════════════════════════════════════
	// CACHED REFERENCES
	// ═══════════════════════════════════════════════

	/** Cached interaction manager */
	UPROPERTY()
	TObjectPtr<UInteractionManager> InteractionManager = nullptr;
	
	// ═══════════════════════════════════════════════
	// INTERNAL METHODS
	// ═══════════════════════════════════════════════

	/** Cache component references on possess */
	void CacheComponents();
};
