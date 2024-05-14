// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interactables/BaseInteractable.h"
#include "BaseChest.generated.h"

class USpawnableLootManager;
class UBoxComponent;
/**
 * 
 */

UCLASS()
class ALS_PROJECTHUNTER_API ABaseChest : public ABaseInteractable
{
	GENERATED_BODY()


public:

	ABaseChest();

	virtual void BeginPlay() override;

	virtual void BPIClientEndInteraction_Implementation(AActor* Interactor) override;

	virtual void BPIInteraction_Implementation(AActor* Interactor, bool WasHeld) override;

	void GetAnimation(UAnimationAsset* NewAnimToPlay) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh")
	USkeletalMesh* SkeletalMeshAsset;


	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UAnimationAsset* AnimToPlay;

protected:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category ="Manager")
	TObjectPtr<USpawnableLootManager> SpawnableLootManager;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "ItemsSpawnBox")
	TObjectPtr<UBoxComponent> ItemSpawnBox;

	UPROPERTY()
	TObjectPtr<AActor> CurrentInteractor;
};
