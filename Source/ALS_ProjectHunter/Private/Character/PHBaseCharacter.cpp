// Copyright@2024 Quentin Davis 

#include "Character/PHBaseCharacter.h"
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
#include "Net/UnrealNetwork.h"
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
	DOREPLIFETIME(APHBaseCharacter, CurrentPoiseDamage);
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

	if (AbilitySystemComponent)
	{
		if (const UPHAttributeSet* AttributeSetPtr = AbilitySystemComponent->GetSet<UPHAttributeSet>())
		{
			// Setup regeneration timers
			SetRegenerationTimer(HealthRegenTimer, &APHBaseCharacter::HealthRegeneration, AttributeSetPtr->GetHealthRegenRate());
			SetRegenerationTimer(ManaRegenTimer, &APHBaseCharacter::ManaRegeneration, AttributeSetPtr->GetManaRegenRate());
			SetRegenerationTimer(StaminaRegenTimer, &APHBaseCharacter::StaminaRegeneration, AttributeSetPtr->GetStaminaRegenRate());
		}
	}
}

void APHBaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	StaminaDegen(DeltaSeconds);
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

/* =========================== */
/* === Equipment Handles   === */
/* =========================== */

float APHBaseCharacter::GetStatBase(const FGameplayAttribute& Attr) const
{
	if (!Attr.IsValid()) return 0.0f;

	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		return ASC->GetNumericAttribute(Attr);
	}

	return 0.0f;
}

void APHBaseCharacter::ApplyFlatStatModifier( const FGameplayAttribute& Attr, const float InValue) const
{
	if (!Attr.IsValid()) return;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const float CurrentValue = ASC->GetNumericAttribute(Attr);
		const float NewValue = CurrentValue + InValue;

		
		// May add later if needed if going - causes issues or to clamp max  
		// const float ClampedValue = FMath::Clamp(NewValue, 0.f, MaxAllowed);

		ASC->SetNumericAttributeBase(Attr, NewValue);
	}
}

void APHBaseCharacter::RemoveFlatStatModifier(const FGameplayAttribute& Attribute, float Delta)
{
	if (!Attribute.IsValid()) return;

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		const float CurrentValue = ASC->GetNumericAttribute(Attribute);
		const float NewValue = CurrentValue - Delta;

		// Optional clamp to prevent negatives if needed
		ASC->SetNumericAttributeBase(Attribute, NewValue);
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
		InitializeDefaultAttributes();
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

void APHBaseCharacter::SetRegenerationTimer(FTimerHandle& TimerHandle, void(APHBaseCharacter::* RegenFunction)() const, float RegenRate)
{
	auto& TimerManager = GetWorldTimerManager();

	// Check and clear the timer if it's already set.
	if (TimerManager.IsTimerActive(TimerHandle))
	{
		TimerManager.ClearTimer(TimerHandle);
	}

	// Set the new timer with the provided regeneration rate and function.
	TimerManager.SetTimer(TimerHandle, this, RegenFunction, RegenRate, true);
}

void APHBaseCharacter::HealthRegenRateChange()
{
	if (const UPHAttributeSet* AttributeSetPtr = AbilitySystemComponent->GetSet<UPHAttributeSet>())
	{
		SetRegenerationTimer(HealthRegenTimer, &APHBaseCharacter::HealthRegeneration, AttributeSetPtr->GetHealthRegenRate());
	}
}

void APHBaseCharacter::ManaRegenRateChange()
{
	if (const UPHAttributeSet* AttributeSetPtr = AbilitySystemComponent->GetSet<UPHAttributeSet>())
	{
		SetRegenerationTimer(ManaRegenTimer, &APHBaseCharacter::ManaRegeneration, AttributeSetPtr->GetManaRegenRate());
	}
}

void APHBaseCharacter::StaminaRegenRateChange()
{
	if (const UPHAttributeSet* AttributeSetPtr = AbilitySystemComponent->GetSet<UPHAttributeSet>())
	{
		SetRegenerationTimer(StaminaRegenTimer, &APHBaseCharacter::StaminaRegeneration, AttributeSetPtr->GetStaminaRegenRate());
	}
}

void APHBaseCharacter::HealthRegeneration() const
{
	check(HealthRegenEffect)
	ApplyEffectToSelf(HealthRegenEffect,1);
}

void APHBaseCharacter::ManaRegeneration() const
{
	check(ManaRegenEffect)
	ApplyEffectToSelf(ManaRegenEffect,1);
}

void APHBaseCharacter::StaminaRegeneration() const
{
	check(StaminaRegenEffect)
	ApplyEffectToSelf(StaminaRegenEffect,1);
	
}

void APHBaseCharacter::StaminaDegen(const float DeltaTime)
{
	check(StaminaDegenEffect);
	if(const UPHAttributeSet* PhAttributeSet = Cast<UPHAttributeSet>(AttributeSet); !bIsInRecovery && PhAttributeSet->GetStamina() <= 0)
	{
		bIsInRecovery = true;
	}

	
	if(GetGait() == EALSGait::Sprinting)
	{
		ApplyEffectToSelf(StaminaDegenEffect, 1);
	}

	
	if(bIsInRecovery)
	{
		TimeSinceLastRecovery += DeltaTime;
		if(TimeSinceLastRecovery >= 3.0f)
		{
			bIsInRecovery = false;
			TimeSinceLastRecovery = 0.0f;
		}
	}
}

void APHBaseCharacter::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float InLevel) const
{
	check(IsValid(GetAbilitySystemComponent()));
	check(GameplayEffectClass);
	
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(GameplayEffectClass, InLevel, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), GetAbilitySystemComponent());
}

void APHBaseCharacter::InitializeDefaultAttributes() const
{
	ApplyEffectToSelf(DefaultPrimaryAttributes, 1.f);
	ApplyEffectToSelf(DefaultSecondaryMaxAttributes,1.f);
	ApplyEffectToSelf(DefaultSecondaryCurrentAttributes,1.f);
	ApplyEffectToSelf(DefaultVitalAttributes, 1.f);
}

void APHBaseCharacter::OnRep_PoiseDamage()
{
}

/* =========================== */
/* === Poise System === */
/* =========================== */

float APHBaseCharacter::GetPoisePercentage() const
{
	const UPHAttributeSet* Attributes = Cast<UPHAttributeSet>(GetAttributeSet());
	if (!Attributes) return 100.0f; // Assume full poise if attributes not found

	const float PoiseThreshold = Attributes->GetPoise();
	const float CurrentPoise = GetCurrentPoiseDamage();

	return (PoiseThreshold > 0.0f) ? FMath::Clamp((1.0f - (CurrentPoise / PoiseThreshold)) * 100.0f, 0.0f, 100.0f) : 0.0f;
}

UAnimMontage* APHBaseCharacter::GetStaggerAnimation()
{
	return  StaggerMontage;
}

void APHBaseCharacter::ApplyPoiseDamage(float PoiseDamage)
{
	if (!HasAuthority()) return; // Server-side only

	CurrentPoiseDamage = FMath::Max(0.0f, CurrentPoiseDamage + PoiseDamage);
	TimeSinceLastPoiseHit = 0.0f; // Reset timer

	ForceNetUpdate(); // Ensure replication
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
	ItemObject->SetRotated(PickupItem->ItemInfo.Rotated);

	// Cast and assign
	if (UPHBaseToolTip* ToolTip = Cast<UPHBaseToolTip>(ToolTipWidget))
	{
		if(UEquippableToolTip* EquipToolTip = Cast<UEquippableToolTip>(ToolTip))
		{
			EquipToolTip->SetItemInfo(ItemObject->GetItemInfo());
			EquipToolTip->ItemData = Cast<AEquipmentPickup>(PickupItem)->EquipmentData;
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

float APHBaseCharacter::GetCurrentPoiseDamage() const
{
	return CurrentPoiseDamage;
}

float APHBaseCharacter::GetTimeSinceLastPoiseHit() const
{
	return TimeSinceLastPoiseHit;
}

void APHBaseCharacter::SetTimeSinceLastPoiseHit(float NewTime)
{
	TimeSinceLastPoiseHit = NewTime;
}
