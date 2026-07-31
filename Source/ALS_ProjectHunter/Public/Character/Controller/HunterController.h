
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

#include "InputActionValue.h"
#include "Character/ALSPlayerController.h"
#include "Character/Library/Structs/HunterControllerStructs.h"
#include "Interactable/Library/Enums/InteractionEnums.h"
#include "HunterController.generated.h"

class UInteractionManager;
class UInputMappingContext;
class UHunterCheatComponent;

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

	UFUNCTION()
	void Interact(const FInputActionValue& Value);

	UFUNCTION()
	void Interact_Started(const FInputActionValue& Value);

	UFUNCTION()
	void Interact_Completed(const FInputActionValue& Value);

	UFUNCTION()
	void Interact_Canceled(const FInputActionValue& Value);

	/**
	 * Enhanced Input handler for an Axis1D action. Bind its Started event so
	 * positive values select next and negative values select previous.
	 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void CycleGroundItem(const FInputActionValue& Value);

	/** Blueprint-friendly alternative for separate Next and Previous actions. */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void CycleGroundItemDirection(int32 Direction);

	/**
	 * Menu input handler - toggles the tabbed menu on AHunterHUD.
	 * BlueprintCallable so the BP controller's EnhancedInputAction event can
	 * route here with a single node (pass the action's bool value).
	 */
	UFUNCTION(BlueprintCallable, Category = "Input")
	void Menu(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable)
	const UInputAction* GetInputActionByName(const FString& InString) const;

	UFUNCTION(BlueprintCallable)
	float GetElapsedSeconds(const UInputAction* Action) const;

	UFUNCTION(BlueprintCallable)
	bool DoOnce(FDoOnceState& State, bool bReset, bool bStartClosed);


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Project Hunter|Cheats", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UHunterCheatComponent> CheatComponent;

protected:

	/** Cached interaction manager */
	UPROPERTY()
	TObjectPtr<UInteractionManager> InteractionManager = nullptr;

	/** Cache component references on possess */
	void CacheComponents();
};
