#include "Equipment/Actors/EquippedItemRuntimeActor.h"

#include "Character/PHBaseCharacter.h"
#include "Combat/Components/CombatManager.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Item/ItemInstance.h"
#include "UObject/UnrealType.h"

DEFINE_LOG_CATEGORY(LogEquippedItemRuntimeActor);

AEquippedItemRuntimeActor::AEquippedItemRuntimeActor()
{
	PrimaryActorTick.bCanEverTick = false;

	// Replicate so clients see the actor for visuals and client-predicted traces.
	// Movement replication stays off because socket attachment drives placement.
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMeshComponent"));
	StaticMeshComponent->SetupAttachment(SceneRoot);
	StaticMeshComponent->SetVisibility(false);
	StaticMeshComponent->SetHiddenInGame(true);
	StaticMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMeshComponent->bEditableWhenInherited = true;

	SkeletalMeshComponent = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMeshComponent"));
	SkeletalMeshComponent->SetupAttachment(SceneRoot);
	SkeletalMeshComponent->SetVisibility(false);
	SkeletalMeshComponent->SetHiddenInGame(true);
	SkeletalMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMeshComponent->bEditableWhenInherited = true;

	DamageTrace = CreateDefaultSubobject<USplineComponent>(TEXT("DamageTrace"));
	DamageTrace->SetupAttachment(SceneRoot);
	DamageTrace->bEditableWhenInherited = true;
	DamageTrace->SetClosedLoop(false);
	DamageTrace->SetDrawDebug(true);

	if (DamageTrace->GetNumberOfSplinePoints() < 2)
	{
		DamageTrace->ClearSplinePoints(false);
		DamageTrace->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
		DamageTrace->AddSplinePoint(FVector(50.0f, 0.0f, 0.0f), ESplineCoordinateSpace::Local, true);
	}
}

void AEquippedItemRuntimeActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyConfiguredPreviewMeshes();
}

void AEquippedItemRuntimeActor::PostLoad()
{
	Super::PostLoad();
	ApplyConfiguredPreviewMeshes();
}

#if WITH_EDITOR
void AEquippedItemRuntimeActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyConfiguredPreviewMeshes();
}
#endif

void AEquippedItemRuntimeActor::InitializeFromItem(UItemInstance* Item, APawn* OwnerPawn, EEquipmentSlot Slot)
{
	ItemInstance = Item;
	OwnerActor = Cast<APHBaseCharacter>(OwnerPawn);
	if (!OwnerActor)
	{
		OwnerActor = Cast<APHBaseCharacter>(GetOwner());
	}

	if (!OwnerActor)
	{
		UE_LOG(LogEquippedItemRuntimeActor, Warning,
			TEXT("InitializeFromItem: Owning pawn is null for '%s'. Combat source resolution will fall back to GetOwner()."),
			*GetName());
	}

	if (ItemInstance)
	{
		if (USkeletalMesh* ItemSkeletalMesh = ItemInstance->GetEquippedMesh())
		{
			SkeletalMesh = ItemSkeletalMesh;
			StaticMesh = nullptr;
		}
		else if (UStaticMesh* ItemStaticMesh = ItemInstance->GetGroundMesh())
		{
			StaticMesh = ItemStaticMesh;
			SkeletalMesh = nullptr;
		}
	}

	ApplyConfiguredPreviewMeshes();
}

UCombatManager* AEquippedItemRuntimeActor::GetCombatManager() const
{
	if (!OwnerActor)
	{
		return nullptr;
	}

	return OwnerActor->CombatManager;
}

void AEquippedItemRuntimeActor::SetStaticMesh(UStaticMesh* NewMesh)
{
	StaticMesh = NewMesh;
	if (NewMesh != nullptr)
	{
		SkeletalMesh = nullptr;
	}

	ApplyConfiguredPreviewMeshes();
}

void AEquippedItemRuntimeActor::SetSkeletalMesh(USkeletalMesh* NewMesh)
{
	SkeletalMesh = NewMesh;
	if (NewMesh != nullptr)
	{
		StaticMesh = nullptr;
	}

	ApplyConfiguredPreviewMeshes();
}

void AEquippedItemRuntimeActor::RefreshVisualPreview()
{
	ApplyConfiguredPreviewMeshes();
}

void AEquippedItemRuntimeActor::ApplyConfiguredPreviewMeshes()
{
	if (!StaticMeshComponent || !SkeletalMeshComponent)
	{
		return;
	}

	const bool bUseSkeletalMesh = SkeletalMesh != nullptr;
	const bool bUseStaticMesh = !bUseSkeletalMesh && StaticMesh != nullptr;

	SkeletalMeshComponent->SetSkeletalMesh(bUseSkeletalMesh ? SkeletalMesh.Get() : nullptr);
	SkeletalMeshComponent->SetVisibility(bUseSkeletalMesh, true);
	SkeletalMeshComponent->SetHiddenInGame(!bUseSkeletalMesh);

	StaticMeshComponent->SetStaticMesh(bUseStaticMesh ? StaticMesh.Get() : nullptr);
	StaticMeshComponent->SetVisibility(bUseStaticMesh, true);
	StaticMeshComponent->SetHiddenInGame(!bUseStaticMesh);
}
