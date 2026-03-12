// World/Actor/ISMContainerActor.cpp

#include "Tower/Actors/ISMContainerActor.h"
#include "Components/SceneComponent.h"

AISMContainerActor::AISMContainerActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	
	// Create root component
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	SetRootComponent(RootSceneComponent);
}