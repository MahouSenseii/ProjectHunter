// Fill out your copyright notice in the Description page of Project Settings.


#include "Item/EquippedObject.h"
#include "Components/TimelineComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Library/PHItemFunctionLibrary.h"
#include "Library/PHItemStructLibrary.h"

// Sets default values
AEquippedObject::AEquippedObject()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	// Create and set the default scene component as the root component
	DefaultScene = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultScene"));
	RootComponent = DefaultScene;

	// Create other components
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	DamageSpline = CreateDefaultSubobject<USplineComponent>(TEXT("DamageSpline"));

	// Attach other components to DefaultScene
	StaticMesh->SetupAttachment(DefaultScene);
	SkeletalMesh->SetupAttachment(DefaultScene);
	DamageSpline->SetupAttachment(DefaultScene);

	// Set properties for the StaticMesh
	StaticMesh->SetStaticMesh(ItemInfo.ItemInfo.StaticMesh);
	if (StaticMesh)
	{
		StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if(!ItemStats.bAffixesGenerated)
	{
		ItemStats = UPHItemFunctionLibrary::GenerateStats(StatsDataTable);
		ItemInfo = UPHItemFunctionLibrary::GenerateItemName(ItemStats,ItemInfo);
	}
		

}

void AEquippedObject::Destroyed()
{
	// Clean up components
	if (StaticMesh)
	{
		StaticMesh->DestroyComponent();
	}
	if (SkeletalMesh)
	{
		SkeletalMesh->DestroyComponent();
	}
	if (DamageSpline)
	{
		DamageSpline->DestroyComponent();
	}
	
	Super::Destroyed();
}

void AEquippedObject::SetItemInfo(FItemInformation Info)
{
	ItemInfo = Info;
}

void AEquippedObject::SetItemInfoRotated(bool inBool)
{ ItemInfo.ItemInfo.Rotated = inBool; }

// Called when the game starts or when spawned
void AEquippedObject::BeginPlay()
{
	Super::BeginPlay();

	if (DefaultScene == NULL)
	{
		DefaultScene = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultScene"));
		DefaultScene = RootComponent;
	}
	
}

UCombatManager* AEquippedObject::ConvertActorToCombatManager(AActor* InActor)
{
	if (InActor != nullptr)
	{
		if (UCombatManager* CombatManager =  InActor->FindComponentByClass<UCombatManager>())
		{
			return CombatManager;
		}
	}
	return nullptr;
}


void AEquippedObject::SetMesh(UStaticMesh* Mesh) const
{
	if(StaticMesh)
	{
		StaticMesh->SetStaticMesh(Mesh);
	}
}

