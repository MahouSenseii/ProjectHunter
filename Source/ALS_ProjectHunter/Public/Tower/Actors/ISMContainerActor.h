// Fill out your copyright notice in the Description page of Project Settings.

// World/Actor/ISMContainerActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ISMContainerActor.generated.h"

/**
 * Simple container actor for holding ISM components
 * Used by GroundItemSubsystem
 */
UCLASS(NotBlueprintable, NotPlaceable)
class ALS_PROJECTHUNTER_API AISMContainerActor : public AActor
{
	GENERATED_BODY()

public:
	AISMContainerActor();

protected:
	/** Root component (required for actor to exist) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USceneComponent* RootSceneComponent;
};
