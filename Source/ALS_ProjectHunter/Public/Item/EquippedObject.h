// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Library/PHItemStructLibrary.h"
#include "EquippedObject.generated.h"

class AALSBaseCharacter;
struct FItemInformation;
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

	UFUNCTION(BlueprintCallable, BlueprintSetter)
	void SetItemInfo(FItemInformation Info);
		
	void SetOwningCharacter(AALSBaseCharacter* InOwner) { OwningCharacter = InOwner; }
	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

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

protected:

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category = "Stats")
	FItemInformation ItemInfo;

	UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
	TObjectPtr<AALSBaseCharacter> OwningCharacter;

	UPROPERTY(BlueprintReadWrite,EditAnywhere, Category = "Components")
	TObjectPtr<UStaticMeshComponent> StaticMesh;

	UPROPERTY(BlueprintReadWrite,  EditAnywhere, Category = "Components")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

};
