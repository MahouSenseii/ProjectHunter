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
#include "Components/CombatManager.h"
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
/* === Equipment Handles   === */
/* =========================== */

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


void APHBaseCharacter::ApplyEffectToSelf(TSubclassOf<UGameplayEffect> GameplayEffectClass, float InLevel) const
{
	check(IsValid(GetAbilitySystemComponent()));
	check(GameplayEffectClass);
	
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(GameplayEffectClass, InLevel, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), GetAbilitySystemComponent());
}

FActiveGameplayEffectHandle APHBaseCharacter::ApplyEffectToSelfWithReturn(TSubclassOf<UGameplayEffect> InEffect, float InLevel)
{
	if (!InEffect || !AbilitySystemComponent) return FActiveGameplayEffectHandle();

	const FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(InEffect, InLevel, Context);

	if (SpecHandle.IsValid())
	{
		return AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}

	return FActiveGameplayEffectHandle();
}


void APHBaseCharacter::InitializeDefaultAttributes() const
{
	if (!IsValid(AbilitySystemComponent)) return;

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	ApplyEffectToSelf(DefaultPrimaryAttributes,1.0);
	// === PRIMARY ATTRIBUTES ===
	FGameplayEffectSpecHandle PrimarySpec = AbilitySystemComponent->MakeOutgoingSpec(DefaultPrimaryAttributes, 1.0f, Context);
	if (PrimarySpec.IsValid())
	{
		const auto& PHTags = FPHGameplayTags::Get();

		PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Strength, PrimaryInitAttributes.Strength);
		PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Intelligence, PrimaryInitAttributes.Intelligence);
		PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Dexterity, PrimaryInitAttributes.Dexterity);
		PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Endurance, PrimaryInitAttributes.Endurance);
		PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Affliction, PrimaryInitAttributes.Affliction);
		PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Luck, PrimaryInitAttributes.Luck);
		PrimarySpec.Data->SetSetByCallerMagnitude(PHTags.Attributes_Primary_Covenant, PrimaryInitAttributes.Covenant);

		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*PrimarySpec.Data);
	}

	ApplyEffectToSelf(DefaultVitalAttributes,1.0);
	FGameplayEffectSpecHandle VitalSpec = AbilitySystemComponent->MakeOutgoingSpec(DefaultVitalAttributes, 1.0f, Context);
	if ( VitalSpec.IsValid())
	{
		const auto& PhTags = FPHGameplayTags::Get();

		VitalSpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Vital_Health, VitalInitAttributes.Health);
		VitalSpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Vital_Mana, VitalInitAttributes.Mana);
		VitalSpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Vital_Stamina, VitalInitAttributes.Stamina);
		
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*VitalSpec.Data);
		
	}
	ApplyEffectToSelf(DefaultSecondaryCurrentAttributes,1.0);
	ApplyEffectToSelf(DefaultSecondaryMaxAttributes,1.0);
	// === SECONDARY ATTRIBUTES ===
	FGameplayEffectSpecHandle SecondarySpec = AbilitySystemComponent->MakeOutgoingSpec(DefaultSecondaryCurrentAttributes, 1.0f, Context);
	if ( SecondarySpec.IsValid())
	{
		const auto& PhTags = FPHGameplayTags::Get();

		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_HealthRegenRate, SecondaryInitAttributes.HealthRegenRate);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_ManaRegenRate, SecondaryInitAttributes.ManaRegenRate);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_StaminaRegenRate, SecondaryInitAttributes.StaminaRegenRate);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_ArcaneShieldRegenRate, SecondaryInitAttributes.ArcaneShieldRegenRate);

		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_HealthRegenAmount, SecondaryInitAttributes.HealthRegenAmount);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_ManaRegenAmount, SecondaryInitAttributes.ManaRegenAmount);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_StaminaRegenAmount, SecondaryInitAttributes.StaminaRegenAmount);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_ArcaneShieldRegenAmount, SecondaryInitAttributes.ArcaneShieldRegenAmount);

		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_HealthFlatReservedAmount, SecondaryInitAttributes.FlatReservedHealth);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_ManaFlatReservedAmount, SecondaryInitAttributes.FlatReservedMana);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_StaminaFlatReservedAmount, SecondaryInitAttributes.FlatReservedStamina);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_ArcaneShieldFlatReservedAmount, SecondaryInitAttributes.FlatReservedArcaneShield);

		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_HealthPercentageReserved, SecondaryInitAttributes.PercentageReservedHealth);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_ManaPercentageReserved, SecondaryInitAttributes.PercentageReservedMana);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_StaminaPercentageReserved, SecondaryInitAttributes.PercentageReservedStamina);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Vital_ArcaneShieldPercentageReserved, SecondaryInitAttributes.PercentageReservedArcaneShield);

		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Resistances_FireResistanceFlat, SecondaryInitAttributes.FireResistanceFlat);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Resistances_IceResistanceFlat, SecondaryInitAttributes.IceResistanceFlat);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Resistances_LightningResistanceFlat, SecondaryInitAttributes.LightningResistanceFlat);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Resistances_LightResistanceFlat, SecondaryInitAttributes.LightResistanceFlat);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Resistances_CorruptionResistanceFlat, SecondaryInitAttributes.CorruptionResistanceFlat);

		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Resistances_FireResistancePercentage, SecondaryInitAttributes.FireResistancePercent);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Resistances_IceResistancePercentage, SecondaryInitAttributes.IceResistancePercent);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Resistances_LightningResistancePercentage, SecondaryInitAttributes.LightningResistancePercent);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Resistances_LightResistancePercentage, SecondaryInitAttributes.LightResistancePercent);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Resistances_CorruptionResistancePercentage, SecondaryInitAttributes.CorruptionResistancePercent);

		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Money_Gems, SecondaryInitAttributes.Gems);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Misc_LifeLeech, SecondaryInitAttributes.LifeLeech);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Misc_ManaLeech, SecondaryInitAttributes.ManaLeech);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Misc_MovementSpeed, SecondaryInitAttributes.MovementSpeed);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Misc_CritChance, SecondaryInitAttributes.CritChance);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Misc_CritMultiplier, SecondaryInitAttributes.CritMultiplier);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Misc_Poise, SecondaryInitAttributes.Poise);
		SecondarySpec.Data->SetSetByCallerMagnitude(PhTags.Attributes_Secondary_Misc_StunRecovery, SecondaryInitAttributes.StunRecovery);

		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SecondarySpec.Data);
	}


	
}

/* =========================== */
/* === Poise System === */
/* =========================== */

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

void APHBaseCharacter::OpenToolTip(const UInteractableManager* InteractableManager)
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
