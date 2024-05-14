// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractionProcessInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractionProcessInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ALS_PROJECTHUNTER_API IInteractionProcessInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void StartInteractionWithObject(UInteractableManager* Interactable);
	virtual void StartInteractionWithObject_Implementation(UInteractableManager* Interactable) {};

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void EndInteractionWithObject(UInteractableManager* Interactable);
	virtual void EndInteractionWithObject_Implementation(UInteractableManager* Interactable) {};

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void RemoveInteractionFromObject(UInteractableManager* Interactable);
	virtual void RemoveInteractionFromObject_Implementation(UInteractableManager* Interactable) {};

	UFUNCTION(BlueprintNativeEvent,  BlueprintCallable, Category = "Interaction")
	void InitializeInteractionWithObject(UInteractableManager* Interactable);
	virtual void InitializeInteractionWithObject_Implementation(UInteractableManager* Interactable) {};
	
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	AActor* GetCurrentInteractableObject();
	virtual  AActor* GetCurrentInteractableObject_Implementation() {return nullptr;};
	
};
