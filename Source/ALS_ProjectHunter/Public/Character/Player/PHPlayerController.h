// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Character/ALSPlayerController.h"
#include "Components/InteractionManager.h"
#include "Interfaces/InteractionProcessInterface.h"
#include "PHPlayerController.generated.h"

class UWidgetManager;
class APHBaseCharacter;
class UPHInputConfig;


struct FDoOnceState
{
	bool bHasBeenInitialized = false;
	bool bIsClosed = false;
};

/**
 * @class APHPlayerController
 * PHPlayerController provides functionality for handling player interactions and inputs within the game.
 */
UCLASS()
class ALS_PROJECTHUNTER_API APHPlayerController : public AALSPlayerController, public IInteractionProcessInterface
{
	GENERATED_BODY()

public:

	APHPlayerController(const FObjectInitializer& ObjectInitializer);
	virtual void BeginPlay() override;
	virtual AActor* GetCurrentInteractableObject_Implementation() override;
	virtual void InitializeInteractionWithObject_Implementation(UInteractableManager* Interactable) override;
	UFUNCTION(Client, Reliable) void  ClientInitializeInteractionWithObject(UInteractableManager* Interactable);
	UFUNCTION() void InitializeInteraction(UInteractableManager* Interactable);

	virtual void StartInteractionWithObject_Implementation(UInteractableManager* Interactable,  bool WasHeld) override;
	UFUNCTION(Server, Reliable) void ServerStartInteractionWithObject(UInteractableManager* Interactable,  bool WasHeld);
	UFUNCTION(Client, Reliable) void ClientStartInteractionWithObject(UInteractableManager* Interactable,  bool WasHeld);

	virtual void EndInteractionWithObject_Implementation(UInteractableManager* Interactable) override;
	UFUNCTION(Server, Reliable) void ServerEndInteractionWithObject(UInteractableManager* Interactable);
	UFUNCTION(Client, Reliable) void ClientEndInteractionWithObject(UInteractableManager* Interactable);

	virtual void RemoveInteractionFromObject_Implementation(UInteractableManager* Interactable) override;
	UFUNCTION(Server, Reliable) void ServerRemoveInteractionFromObject(UInteractableManager* Interactable);
	UFUNCTION(Client, Reliable) void ClientRemoveInteractionFromObject(UInteractableManager* Interactable);
	
	UFUNCTION(BlueprintCallable, Category = "Manager") UWidgetManager* GetWidgetManager() const { return WidgetManager;}
	
protected:
	
	UFUNCTION() void EndInteraction(const UInteractableManager* Interactable);
	UFUNCTION()void StartInteraction(UInteractableManager* Interactable, bool  WasHeld);
	UFUNCTION()static void RemoveInteraction(UInteractableManager* Interactable);
	UFUNCTION(BlueprintCallable) void SetCurrentInteractable(UInteractableManager* InInteractable);
	UFUNCTION(BlueprintCallable) void RemoveCurrentInteractable(UInteractableManager* RemovedInteractable);


	// Inputs 
	UFUNCTION() void Menu(const FInputActionValue& Value) const;
	
	UFUNCTION()
	void Interact(const FInputActionValue& Value);

	//helper
	void RunInteractionCheck();


	UFUNCTION(BlueprintCallable)
	EInteractType CheckInputType(float Elapsed, float InRequiredHeldTime, bool bReset = false);


	UFUNCTION(BlueprintCallable)
	float GetElapsedSeconds(const UInputAction* Action) const;

	UFUNCTION(BlueprintCallable)
	const UInputAction* GetInputActionByName(const FString& InString) const;
	
private:

public:

	UPROPERTY(BlueprintReadOnly,EditInstanceOnly) TObjectPtr<UInteractionManager> InteractionManager;
	
protected:
	
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UInteractableManager> CurrentInteractable;
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Manager") TObjectPtr<UWidgetManager> WidgetManager;
	UPROPERTY(BlueprintReadOnly) float RequiredHeldTime = 0.25f;
	UPROPERTY(BlueprintReadOnly) float CachedHoldTime = 0.0f;

	static bool DoOnce(FDoOnceState& State, bool bReset, bool bStartClosed);
	// Timer handle for delay
	FTimerHandle InteractionDelayHandle;
	bool bHasCheckedInputType = false;
private:
	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UPHInputConfig> InputConfig;
	FDoOnceState MyDoOnce;

	UPROPERTY()
	UInteractableManager* SavedInteractable = nullptr;
	static bool bHasRun;

};
