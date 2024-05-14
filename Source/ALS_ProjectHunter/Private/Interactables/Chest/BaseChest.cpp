// Fill out your copyright notice in the Description page of Project Settings.


#include "Interactables/Chest/BaseChest.h"

#include "AbilitySystem/PHAttributeSet.h"
#include "Character/Player/PHPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/InteractableManager.h"
#include "Components/SpawnableLootManager.h"

ABaseChest::ABaseChest()
{
	// Load the Skeletal Mesh
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshAsset(TEXT("/Game/Loot_Anim_Set/Props/TreasureChest/TreasureChest_SkelMesh.TreasureChest_SkelMesh"));
	if (MeshAsset.Succeeded())
	{
		SkeletalMesh->SetSkeletalMesh(MeshAsset.Object);
	}
	StaticMesh = nullptr;
	// Check if the SkeletalMeshAsset is valid
	if (SkeletalMesh && SkeletalMeshAsset)
	{
		SkeletalMesh->SetSkeletalMesh(SkeletalMeshAsset);
	}

	// Check if the ItemSpawnBox component has not been created
	if (!ItemSpawnBox)
	{
		// Create the ItemSpawnBox component and name it
		ItemSpawnBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ItemSpawnBox"));
		// Attach the ItemSpawnBox to the RootComponent
		ItemSpawnBox->SetupAttachment(RootComponent);

		// Define the relative location, rotation, and scale for the ItemSpawnBox
		const FVector Location(0.0f, -305.0f, 60.0f);
		const FRotator Rotation(0.0f, 0.0f, 0.0f);
		const FVector Scale(3.0f, 2.0f, 0.5f);
		const FTransform NewTransform(Rotation, Location, Scale);

		// Set the relative transform of the ItemSpawnBox
		ItemSpawnBox->SetRelativeTransform(NewTransform);
	}

	SpawnableLootManager = CreateDefaultSubobject<USpawnableLootManager>("LootManager");
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

			if (const APHPlayerCharacter* AlsCharacter = Cast<APHPlayerCharacter>(CurrentInteractor))
			{
				SpawnableLootManager->GetSpawnItem(Cast<UPHAttributeSet>(AlsCharacter->GetAttributeSet()));
				InteractableManager->RemoveInteraction();
			}
		}
	}
}

void ABaseChest::GetAnimation(UAnimationAsset* NewAnimToPlay) const
{
	SkeletalMesh->PlayAnimation(NewAnimToPlay, false);
}

