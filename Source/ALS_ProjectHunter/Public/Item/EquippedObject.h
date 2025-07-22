// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/CombatManager.h"
#include "GameFramework/Actor.h"
#include "Library/PHItemStructLibrary.h"
#include "EquippedObject.generated.h"

class AALSBaseCharacter;
class USceneComponent;
class USplineComponent;
class UTimelineComponent;

UCLASS()
class ALS_PROJECTHUNTER_API AEquippedObject : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEquippedObject();
	virtual void Destroyed() override;

	UFUNCTION(BlueprintCallable, BlueprintSetter)
	void SetItemInfo(FItemInformation Info);
	void SetOwningCharacter(AALSBaseCharacter* InOwner) { OwningCharacter = InOwner; }

	UFUNCTION(BlueprintCallable)
	FItemInformation GetItemInfo() { return  ItemInfo;}

	UFUNCTION(BlueprintCallable)
	void SetItemInfoRotated(bool InBool);


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Converter")
	static UCombatManager* ConvertActorToCombatManager(AActor* InActor);

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	FTransform Transform;


	UPROPERTY(BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> DefaultScene;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collison")
	TObjectPtr<USplineComponent> DamageSpline;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collison")
	TObjectPtr<UTimelineComponent> DamageTraceTl;

	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collison")
	TArray<FVector> PrevTracePoints;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Collison")
	TArray<AActor*> CurrentActorHit;
	
	// Function to set the mesh
	void SetMesh(UStaticMesh* Mesh) const;
	
protected:

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Stats")
	FItemInformation ItemInfo;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<AALSBaseCharacter> OwningCharacter;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	UPROPERTY(BlueprintReadWrite,  EditAnywhere, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	// Set in BP will be all stats that the item can generate
	UPROPERTY(EditAnywhere, Category = "ItemInfo|Stats")
	UDataTable* StatsDataTable;
	
	UPROPERTY(EditAnywhere, Category = "ItemInfo|Stats")
	FPHItemStats ItemStats;
};
