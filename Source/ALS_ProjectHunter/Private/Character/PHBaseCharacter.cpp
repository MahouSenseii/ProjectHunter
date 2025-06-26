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
#include "Library/PHCharacterStructLibrary.h"
#include "Net/UnrealNetwork.h"
#include "UI/ToolTip/ConsumableToolTip.h"
#include "UI/HUD/PHHUD.h"

class APHPlayerState;

/* =========================== */
/* === Constructor & Setup === */
/* =========================== */


namespace
{
	constexpr float MinAllowedEffectPeriod = 0.1f;
}

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


void APHBaseCharacter::ApplyPeriodicEffectToSelf(const FInitialGameplayEffectInfo& EffectInfo)
{
	if (!IsValid(AbilitySystemComponent) || !EffectInfo.EffectClass)
	{
		return;
	}

	// 1. Clear any existing effect for this SetByCallerTag
	if (EffectInfo.SetByCallerTag.IsValid())
	{
		if (const FActiveGameplayEffectHandle* FoundHandle = ActivePeriodicEffects.Find(EffectInfo.SetByCallerTag))
		{
			if (FoundHandle->IsValid())
			{
				AbilitySystemComponent->RemoveActiveGameplayEffect(*FoundHandle);
			}
		}
	}

	// 2. Build context and spec
	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
		EffectInfo.EffectClass, EffectInfo.Level, Context);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 3. Pull current values from attributes or fallback to defaults
	const float RawRate = EffectInfo.RateAttribute.IsValid()
		? AbilitySystemComponent->GetNumericAttribute(EffectInfo.RateAttribute)
		: EffectInfo.DefaultRate;

	const float ClampedRate = FMath::Max(RawRate, MinAllowedEffectPeriod);

	const float RawAmount = EffectInfo.AmountAttribute.IsValid()
		? AbilitySystemComponent->GetNumericAttribute(EffectInfo.AmountAttribute)
		: EffectInfo.DefaultAmount;

	// 4. Set SetByCallerMagnitude (used in MMC) and Period (for ticking)
	SpecHandle.Data->SetSetByCallerMagnitude(EffectInfo.SetByCallerTag, RawAmount);
	SpecHandle.Data->Period = ClampedRate;

	// 5. Apply the effect to self and track the handle
	FActiveGameplayEffectHandle Handle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(
		*SpecHandle.Data.Get(), AbilitySystemComponent);

	if (EffectInfo.SetByCallerTag.IsValid() && Handle.IsValid())
	{
		ActivePeriodicEffects.Add(EffectInfo.SetByCallerTag, Handle);
	}

	// 6. Optional: debug log
#if WITH_EDITOR
	UE_LOG(LogTemp, Log, TEXT("[GAS] Applied %s with Amount %.2f, Period %.2f"),
		*EffectInfo.EffectClass->GetName(), RawAmount, ClampedRate);
#endif
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

	for (const FInitialGameplayEffectInfo& EffectInfo : StartupEffects)
	{
		// Bind rate/amount change listeners
		if (EffectInfo.RateAttribute.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(EffectInfo.RateAttribute)
				.AddUObject(this, &APHBaseCharacter::OnAnyVitalPeriodicStatChanged);
		}

		if (EffectInfo.AmountAttribute.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(EffectInfo.AmountAttribute)
				.AddUObject(this, &APHBaseCharacter::OnAnyVitalPeriodicStatChanged);
		}

		// Bind trigger tag change listener (if any)
		if (EffectInfo.TriggerTag.IsValid())
		{
			AbilitySystemComponent->RegisterGameplayTagEvent(EffectInfo.TriggerTag, EGameplayTagEventType::NewOrRemoved)
				.AddUObject(this, &APHBaseCharacter::OnRegenTagChanged);
		}

		// Apply the effect initially
		ApplyPeriodicEffectToSelf(EffectInfo);
	}
}

void APHBaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	ClearAllPeriodicEffects();
}


void APHBaseCharacter::ClearAllPeriodicEffects()
{
	
	TArray<FActiveGameplayEffectHandle> Handles;
	ActivePeriodicEffects.GenerateValueArray(Handles);

	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		if (Handle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
		}
	}

	ActivePeriodicEffects.Empty();
}

bool APHBaseCharacter::HasValidPeriodicEffect(FGameplayTag EffectTag) const
{
	TArray<FActiveGameplayEffectHandle> Handles;
	ActivePeriodicEffects.MultiFind(EffectTag, Handles);

	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		if (Handle.IsValid() && AbilitySystemComponent->GetActiveGameplayEffect(Handle))
		{
			return true;
		}
	}

	return false;
}

void APHBaseCharacter::PurgeInvalidPeriodicHandles()
{
	TArray<FGameplayTag> PHTags;
	ActivePeriodicEffects.GetKeys( PHTags);

	for (const FGameplayTag& Tag :  PHTags)
	{
		TArray<FActiveGameplayEffectHandle> Handles;
		ActivePeriodicEffects.MultiFind(Tag, Handles);

		for (int32 i = Handles.Num() - 1; i >= 0; --i)
		{
			if (!Handles[i].IsValid())
			{
				ActivePeriodicEffects.RemoveSingle(Tag, Handles[i]);
			}
		}
	}
}


void APHBaseCharacter::RemoveAllPeriodicEffectsByTag(const FGameplayTag EffectTag)
{
	TArray<FActiveGameplayEffectHandle> Handles;
	ActivePeriodicEffects.MultiFind(EffectTag, Handles);

	// Filter and remove invalid handles first
	for (int32 i = Handles.Num() - 1; i >= 0; --i)
	{
		if (!Handles[i].IsValid())
		{
			Handles.RemoveAt(i);
		}
	}

	// Remove valid handles from ASC
	for (const FActiveGameplayEffectHandle& Handle : Handles)
	{
		if (Handle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(Handle);
		}
	}

	// Remove entries from the TMultiMap
	ActivePeriodicEffects.Remove(EffectTag);
}


void APHBaseCharacter::OnRegenTagChanged(FGameplayTag ChangedTag, int32 NewCount)
{
	for (const FInitialGameplayEffectInfo& EffectInfo : StartupEffects)
	{
		if (EffectInfo.TriggerTag == ChangedTag)
		{
			ApplyPeriodicEffectToSelf(EffectInfo);
		}
	}
}

void APHBaseCharacter::OnAnyVitalPeriodicStatChanged(const FOnAttributeChangeData& Data)
{
	for (const FInitialGameplayEffectInfo& EffectInfo : StartupEffects)
	{
		if (EffectInfo.RateAttribute == Data.Attribute || EffectInfo.AmountAttribute == Data.Attribute)
		{
			ApplyPeriodicEffectToSelf(EffectInfo);
		}
	}
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
