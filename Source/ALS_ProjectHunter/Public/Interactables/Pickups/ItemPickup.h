// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/PHBaseCharacter.h"
#include "Interactables/BaseInteractable.h"
#include "Item/BaseItem.h"
#include "Library/PHItemStructLibrary.h"
#include "ItemPickup.generated.h"

class URotatingMovementComponent;
/**
 * AItemPickup is a class representing an item that can be picked up by characters in the game.
 * It inherits from ABaseInteractable and provides functionalities for handling interactions with characters.
 */
UCLASS()
class ALS_PROJECTHUNTER_API AItemPickup : public ABaseInteractable
{
	GENERATED_BODY()


public:

	AItemPickup();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	UItemDefinitionAsset* ItemDefinition;
	
	UFUNCTION(BlueprintCallable)
	virtual UBaseItem* CreateItemObject(UObject* Outer);
protected:

	virtual void BeginPlay() override;

public:
	
	// Rotating movement component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	URotatingMovementComponent* RotatingMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Particles")
	UParticleSystemComponent* ParticleSystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ItemInfo")
	mutable UBaseItem* ObjItem;

	virtual void BPIInteraction_Implementation(AActor* Interactor, bool WasHeld) override;
	
	UFUNCTION(BlueprintCallable)
	virtual UBaseItem* GenerateItem() ;

	virtual bool HandleInteraction(AActor* Actor, bool WasHeld, UItemDefinitionAsset*& ItemInfo,
		FConsumableItemData ConsumableItemData) const;

	UFUNCTION(BlueprintCallable)
	virtual void HandleHeldInteraction(APHBaseCharacter* Character) const;

	UFUNCTION(BlueprintCallable)
	virtual void HandleSimpleInteraction(APHBaseCharacter* Character) const;

	UFUNCTION(BlueprintCallable)
	void SetWidgetRarity();
	
};
