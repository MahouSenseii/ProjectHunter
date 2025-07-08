// Copyright@2024 Quentin Davis 

#include "Character/PHBaseCharacter.h"

#include "PHGameplayTags.h"
#include "AbilitySystem/PHAbilitySystemComponent.h"
#include "AbilitySystem/PHAttributeSet.h"
#include "Character/ALSPlayerController.h"
#include "Character/Player/PHPlayerController.h"
#include "Character/Player/State/PHPlayerState.h"
#include "Components/EquipmentManager.h"
#include "Components/InventoryManager.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/CombatManager.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interactables/Pickups/ConsumablePickup.h"
#include "Interactables/Pickups/EquipmentPickup.h"
#include "UI/ToolTip/ConsumableToolTip.h"
#include "UI/HUD/PHHUD.h"

class APHPlayerState;

/* =========================== */
/* === Constructor & Setup === */
/* =========================== */



APHBaseCharacter::APHBaseCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	EquipmentManager = CreateDefaultSubobject<UEquipmentManager>(TEXT("EquipmentManager"));
	InventoryManager = CreateDefaultSubobject<UInventoryManager>(TEXT("InventoryManager"));
	CombatManager = CreateDefaultSubobject<UCombatManager>(TEXT("Combat Manager"));
	StatsManager = CreateDefaultSubobject<UStatsManager>(TEXT("Stats Manager"));

	// GAS Setup
	if (!bIsPlayer)
	{
		AbilitySystemComponent = CreateDefaultSubobject<UPHAbilitySystemComponent>("Ability System Component");
		AbilitySystemComponent->SetIsReplicated(true);
		AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
		AttributeSet = CreateDefaultSubobject<UPHAttributeSet>("Attribute Set");
	}
	
	// Mini-map
	MiniMapIndicator = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Mini-Map Indicator"));
}

void APHBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}


void APHBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	InitAbilityActorInfo();
	
	CurrentController = Cast<APHPlayerController>(NewController);
	if (CurrentController)
	{
		CurrentController->InteractionManager->OnRemoveCurrentInteractable.AddDynamic(this, &APHBaseCharacter::CloseToolTip);
		CurrentController->InteractionManager->OnNewInteractableAssigned.AddDynamic(this, &APHBaseCharacter::OpenToolTip);
	}
}

UAbilitySystemComponent* APHBaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APHBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

void APHBaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}


void APHBaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const bool bHasMovingTag = ASC->HasMatchingGameplayTag(FPHGameplayTags::Get().Condition_WhileMoving);

		if (bIsMoving && !bHasMovingTag)
			ASC->AddLooseGameplayTag(FPHGameplayTags::Get().Condition_WhileMoving);
		else if (!bIsMoving && bHasMovingTag)
			ASC->RemoveLooseGameplayTag(FPHGameplayTags::Get().Condition_WhileMoving);

		const bool bHasStationaryTag = ASC->HasMatchingGameplayTag(FPHGameplayTags::Get().Condition_WhileStationary);

		if (!bIsMoving && !bHasStationaryTag)
			ASC->AddLooseGameplayTag(FPHGameplayTags::Get().Condition_WhileStationary);
		else if (bIsMoving && bHasStationaryTag)
			ASC->RemoveLooseGameplayTag(FPHGameplayTags::Get().Condition_WhileStationary);
	}	
}

bool APHBaseCharacter::CanSprint() const
{
	const bool bReturnValue =  Super::CanSprint();
	
	if(bIsInRecovery)
	{
		return false;
	}
	return bReturnValue;
}

void APHBaseCharacter::OnGaitChanged(EALSGait PreviousGait)
{
	Super::OnGaitChanged(PreviousGait);
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return;

	const FGameplayTag& StaminaDegenTag = FPHGameplayTags::Get().Attributes_Secondary_Vital_StaminaDegen;

	if (Gait == EALSGait::Sprinting)
	{
		ASC->AddLooseGameplayTag(StaminaDegenTag);
	}
	else
	{
		ASC->RemoveLooseGameplayTag(StaminaDegenTag);
	}
}


/* =========================== */
/* === Ability System (GAS) === */
/* =========================== */

void APHBaseCharacter::InitAbilityActorInfo()
{
	if (!bIsPlayer)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		Cast<UPHAbilitySystemComponent>(AbilitySystemComponent)->AbilityActorInfoSet();
	}
	else
	{
		APHPlayerState* LocalPlayerState = GetPlayerState<APHPlayerState>();
		check(LocalPlayerState);

		
		AbilitySystemComponent = LocalPlayerState->GetAbilitySystemComponent();
		AbilitySystemComponent->InitAbilityActorInfo(LocalPlayerState, this);
		Cast<UPHAbilitySystemComponent>(AbilitySystemComponent)->AbilityActorInfoSet();
		UPHAbilitySystemComponent* ASC =  Cast<UPHAbilitySystemComponent>(AbilitySystemComponent);
		FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
		Context.AddSourceObject(this);
		StatsManager->ASC = ASC;
		
		StatsManager->InitializeDefaultAttributes();
		
		AttributeSet = LocalPlayerState->GetAttributeSet();

		if (AALSPlayerController* PHPlayerController = Cast<AALSPlayerController>(GetController()))
		{
			if (APHHUD* LocalPHHUD = Cast<APHHUD>(PHPlayerController->GetHUD()))
			{
				LocalPHHUD->InitOverlay(PHPlayerController, GetPlayerState<APHPlayerState>(), AbilitySystemComponent, AttributeSet);
			}
		}
	}
}

/* =========================== */
/* === UI & MiniMap === */
/* =========================== */

int32 APHBaseCharacter::GetPlayerLevel()
{
	if(bIsPlayer)
	{
		const APHPlayerState* LocalPlayerState = GetPlayerState<APHPlayerState>();
		check(LocalPlayerState)
		return  LocalPlayerState->GetPlayerLevel();
	}

	return Level;
}

int32 APHBaseCharacter::GetCurrentXP()
{
		const APHPlayerState* LocalPlayerState = GetPlayerState<APHPlayerState>();
		check(LocalPlayerState)
		return  LocalPlayerState->GetXP();
}

int32 APHBaseCharacter::GetXPFNeededForNextLevel()
{
	const APHPlayerState* LocalPlayerState = GetPlayerState<APHPlayerState>();
	check(LocalPlayerState)
	return  LocalPlayerState->GetXPForNextLevel();
}

void APHBaseCharacter::OpenToolTip(UInteractableManager* InteractableManager)
{
	if (!InteractableManager) return;
	check(EquippableToolTipClass);
	
	AItemPickup* PickupItem = Cast<AItemPickup>(InteractableManager->GetOwner());
	if (!PickupItem) return;

	if (CurrentToolTip)
	{
		CurrentToolTip->RemoveFromParent();
		CurrentToolTip = nullptr;
	}

	// Use the Tooltip Widget as the Outer to keep the item alive
	UUserWidget* ToolTipWidget = nullptr;

	if (Cast<AEquipmentPickup>(PickupItem) && EquippableToolTipClass)
	{
		ToolTipWidget = CreateWidget<UEquippableToolTip>(GetWorld(), EquippableToolTipClass);
		
	}
	else if (Cast<AConsumablePickup>(PickupItem) && ConsumableToolTipClass)
	{
		ToolTipWidget = CreateWidget<UConsumableToolTip>(GetWorld(), ConsumableToolTipClass);
	}

	if (!ToolTipWidget) return;

	// Create the item instance using the widget as the Outer to avoid GC issues
	UBaseItem* ItemObject = PickupItem->CreateItemObject(ToolTipWidget);
	if (!ItemObject) return;

	ItemObject->SetItemInfo(PickupItem->ItemInfo);
	ItemObject->SetRotated(PickupItem->ItemInfo.ItemInfo.Rotated);

	// Cast and assign
	if (UPHBaseToolTip* ToolTip = Cast<UPHBaseToolTip>(ToolTipWidget))
	{
		if(UEquippableToolTip* EquipToolTip = Cast<UEquippableToolTip>(ToolTip))
		{
			EquipToolTip->SetItemInfo(ItemObject->GetItemInfo());
			EquipToolTip->OwnerCharacter = this;
			EquipToolTip->AddToViewport();
			CurrentToolTip = EquipToolTip;
		}
		else
		{
			ToolTip->SetItemInfo(ItemObject->GetItemInfo());
			ToolTip->AddToViewport();
			CurrentToolTip = ToolTip;
		}

	}
}

void APHBaseCharacter::CloseToolTip(UInteractableManager* InteractableManager)
{
	if (IsValid(CurrentToolTip))
	{
		CurrentToolTip->RemoveFromParent();
	}
}

void APHBaseCharacter::SetupMiniMapCamera()
{
	MiniMapSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("MiniMapSpringArm"));
	MiniMapSpringArm->SetupAttachment(RootComponent);
	MiniMapCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MiniMapCapture"));
	MiniMapCapture->SetupAttachment(MiniMapSpringArm);

	MiniMapSpringArm->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	MiniMapSpringArm->TargetArmLength = 600.f;
	MiniMapSpringArm->bDoCollisionTest = false;
	MiniMapCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	MiniMapCapture->OrthoWidth = 1500.f;
}
