// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Character/ALSPlayerController.h"
#include "Components/InteractionManager.h"
#include "Interfaces/InteractionProcessInterface.h"
#include "PHPlayerController.generated.h"

class UPHInputConfig;
/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API APHPlayerController : public AALSPlayerController, public IInteractionProcessInterface
{
	GENERATED_BODY()

public:

	APHPlayerController(const FObjectInitializer& ObjectInitializer);
	
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


	
protected:
	
	UFUNCTION() void EndInteraction(const UInteractableManager* Interactable);
	UFUNCTION()void StartInteraction(UInteractableManager* Interactable, bool  WasHeld);
	UFUNCTION()static void RemoveInteraction(UInteractableManager* Interactable);
	UFUNCTION(BlueprintCallable) void SetCurrentInteractable(UInteractableManager* InInteractable);
	UFUNCTION(BlueprintCallable) void RemoveCurrentInteractable(UInteractableManager* RemovedInteractable);


	
private:

public:

	UPROPERTY(BlueprintReadOnly,EditInstanceOnly) TObjectPtr<UInteractionManager> InteractionManager;
	
protected:
	
	UPROPERTY(BlueprintReadOnly) TObjectPtr<UInteractableManager> CurrentInteractable;

	
private:


	
	UPROPERTY(EditDefaultsOnly, Category = "Input")
	TObjectPtr<UPHInputConfig> InputConfig;
};
