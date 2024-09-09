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
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API AItemPickup : public ABaseInteractable
{
	GENERATED_BODY()


public:

	AItemPickup();


protected:

	virtual void BeginPlay() override;

public:
	
	// Rotating movement component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	URotatingMovementComponent* RotatingMovementComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Particles")
	UParticleSystemComponent* ParticleSystemComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ItemInfomation")
	FItemInformation ItemInfo;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ItemInfomation")
	mutable UBaseItem* ObjItem;

	UFUNCTION(BlueprintCallable)
	virtual UBaseItem* GenerateItem() const;

	virtual bool InteractionHandle(AActor* Actor, bool WasHeld) const override;

	UFUNCTION(BlueprintCallable)
	virtual void HandleHeldInteraction(APHBaseCharacter* Character) const;

	UFUNCTION(BlueprintCallable)
	virtual void HandleSimpleInteraction(APHBaseCharacter* Character) const;

	UFUNCTION(BlueprintCallable)
	void SetWidgetRarity();
	
};
