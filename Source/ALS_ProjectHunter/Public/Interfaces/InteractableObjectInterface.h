// Copyright@2024 Quentin Davis 

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractableObjectInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractableObjectInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class ALS_PROJECTHUNTER_API IInteractableObjectInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	
	// Declare the function as BlueprintNativeEvent
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void BPIInitialize();
	virtual void BPIInitialize_Implementation() {};

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void BPIInteraction(AActor* Interactor, bool WasHeld);
	virtual void BPIInteraction_Implementation(AActor* Interactor, bool WasHeld) {};


	// Handle the end of an interaction with the provided actor.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void BPIEndInteraction(AActor* Interactor);
	virtual void  BPIEndInteraction_Implementation(AActor* Interactor) {};

	// Handle the removal of an interaction.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void BPIRemoveInteraction();
	virtual void BPIRemoveInteraction_Implementation() {};

	// Called on the client to start an interaction with the provided actor.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void BPIClientStartInteraction(AActor* Interactor, const bool bIsHeld);
	virtual void BPIClientStartInteraction_Implementation(AActor* Interactor, const bool bIsHeld) {};

	// Called on the client to end an interaction with the provided actor.
	UFUNCTION(BlueprintNativeEvent,  BlueprintCallable, Category = "Interaction")
	void BPIClientEndInteraction(AActor* Interactor);
	virtual void BPIClientEndInteraction_Implementation(AActor* Interactor) {};

	// Called on the client before starting an interaction with the provided actor.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void BPIClientPreInteraction(AActor* Interactor);
	virtual void BPIClientPreInteraction_Implementation(AActor* Interactor) {};

	// Determine if the object implementing this interface can be interacted with.
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanBeInteractedWith();
	virtual bool CanBeInteractedWith_Implementation() { return false; }

};
