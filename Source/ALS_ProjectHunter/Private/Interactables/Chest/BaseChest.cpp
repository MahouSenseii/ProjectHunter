// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Chest/BaseChest.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Character/PHBaseCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/InteractableManager.h"
#include "Components/SpawnableLootManager.h"

ABaseChest::ABaseChest()
{
	// Override parent logic to ensure SkeletalMesh is used instead of StaticMesh
	StaticMesh = nullptr;
	

	// Load and assign skeletal mesh asset
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/Loot_Anim_Set/Props/TreasureChest/TreasureChest_SkelMesh.TreasureChest_SkelMesh"));
	if (MeshAsset.Succeeded())
	{
		SkeletalMeshAsset = MeshAsset.Object;
		if (SkeletalMesh)
		{
			SkeletalMesh->SetSkeletalMesh(SkeletalMeshAsset);
		}
	}

	// Setup ItemSpawnBox if not already created
	ItemSpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ItemSpawnBox"));
	ItemSpawnBox->SetupAttachment(RootComponent);
	const FVector Location(0.0f, -305.0f, 60.0f);
	const FRotator Rotation(0.0f, 0.0f, 0.0f);
	const FVector Scale(3.0f, 2.0f, 0.5f);
	ItemSpawnBox->SetRelativeTransform(FTransform(Rotation, Location, Scale));

	// Create Loot Manager
	SpawnableLootManager = CreateDefaultSubobject<USpawnableLootManager>(TEXT("LootManager"));
}


void ABaseChest::BeginPlay()
{

	Super::BeginPlay();

	SpawnableLootManager->SetSpawnBox(ItemSpawnBox);

}

void ABaseChest::BPIClientEndInteraction_Implementation(AActor* Interactor)
{
	if(IsValid(Interactor))
	{
		CurrentInteractor = Interactor;
		this->SetOwner(nullptr);
	}
}

void ABaseChest::BPIInteraction_Implementation(AActor* Interactor, bool WasHeld)
{
	if(IsValid(Interactor))
	{
		CurrentInteractor = Interactor;
		SetOwner(Interactor);
		GetAnimation(AnimToPlay);
		if (SpawnableLootManager->SpawnableItems)
		{

			if (const APHBaseCharacter* AlsCharacter = Cast<APHBaseCharacter>(CurrentInteractor))
			{
				SpawnableLootManager->GetSpawnItem(Cast<UPHAttributeSet>(AlsCharacter->GetAttributeSet()));

				if (InteractableManager)
				{
					InteractableManager->RemoveInteraction();
				}
			}
		}
	}
}

void ABaseChest::GetAnimation(UAnimationAsset* NewAnimToPlay) const
{
	if (SkeletalMesh && NewAnimToPlay)
	{
		SkeletalMesh->PlayAnimation(NewAnimToPlay, false);
	}
}

