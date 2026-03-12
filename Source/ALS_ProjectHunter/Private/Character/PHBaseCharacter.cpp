// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/PHBaseCharacter.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Character/Component/CharacterProgressionManager.h"
#include "Character/Component/StatsManager.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Character/Component/EquipmentManager.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"



APHBaseCharacter::APHBaseCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	// Create Ability System Component
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Create Attribute Set
	AttributeSet = CreateDefaultSubobject<UHunterAttributeSet>(TEXT("AttributeSet"));

	// Create Progression Manager
	ProgressionManager = CreateDefaultSubobject<UCharacterProgressionManager>(TEXT("ProgressionManager"));

	// Create Equipment Component
	EquipmentManager = CreateDefaultSubobject<UEquipmentManager>(TEXT("EquipmentComponent"));

	// Create Stats Manager
	StatsManager = CreateDefaultSubobject<UStatsManager>(TEXT("StatsManager"));
}

void APHBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APHBaseCharacter, CharacterName);
	DOREPLIFETIME(APHBaseCharacter, bIsDead);
	DOREPLIFETIME(APHBaseCharacter, TeamID);
	DOREPLIFETIME(APHBaseCharacter, CombatAffiliation);
}


void APHBaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	
}

void APHBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// Initialize ASC on server
	if (AbilitySystemComponent && !bAbilitySystemInitialized)
	{
		InitializeAbilitySystem();
	}
}

void APHBaseCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Initialize ASC on client
	if (AbilitySystemComponent && !bAbilitySystemInitialized)
	{
		InitializeAbilitySystem();
	}
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* INITIALIZATION */
/* ═══════════════════════════════════════════════════════════════════════ */

void APHBaseCharacter::InitializeAbilitySystem()
{
	if (bAbilitySystemInitialized)
	{
		return;
	}

	if (!AbilitySystemComponent)
	{
		UE_LOG(LogTemp, Error, TEXT("InitializeAbilitySystem: AbilitySystemComponent is null!"));
		return;
	}

	// Initialize ASC with self as owner and avatar
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// Initialize attributes with base values
	InitializeAttributes();

	// Grant default abilities
	if (HasAuthority())
	{
		GiveDefaultAbilities();
		ApplyStartupEffects();
	}

	// Bind to attribute changes
	BindAttributeDelegates();

	bAbilitySystemInitialized = true;

	// Call virtual function for subclass initialization
	OnAbilitySystemInitialized();

	UE_LOG(LogTemp, Log, TEXT("Ability System initialized for %s"), *GetName());
}

void APHBaseCharacter::InitializeAttributes()
{
	if (!AttributeSet)
	{
		return;
	}
	
	// Base attributes will be set by:
	// 1. StartupEffects (for base values)
	// 2. ProgressionManager (for stat points)
	// 3. EquipmentComponent (for item stats)
}

void APHBaseCharacter::BindAttributeDelegates()
{
	if (!AbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	// Bind to health changes
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		AttributeSet->GetHealthAttribute()).AddUObject(this, &APHBaseCharacter::HandleHealthChanged);
}

void APHBaseCharacter::OnAbilitySystemInitialized()

{
	// Override in subclasses for custom initialization
}

void APHBaseCharacter::OnRep_CombatAffiliation(FCombatAffiliation OldAffiliation)
{
	if (OldAffiliation != CombatAffiliation)
	{
		OnCombatAffiliationChanged.Broadcast(CombatAffiliation);
	}
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* PROGRESSION */
/* ═══════════════════════════════════════════════════════════════════════ */

void APHBaseCharacter::SetCombatAffiliation(const FCombatAffiliation& NewAffiliation)
{
	if (!HasAuthority())
	{
		return;
	}

	if (CombatAffiliation == NewAffiliation)
	{
		return;
	}
	CombatAffiliation = NewAffiliation;
	OnCombatAffiliationChanged.Broadcast(CombatAffiliation);
}

int32 APHBaseCharacter::GetCharacterLevel() const
{
	if (ProgressionManager)
	{
		return ProgressionManager->Level;
	}
	return 1;
}

int64 APHBaseCharacter::GetXPReward() const
{
	return GetCharacterLevel() * 100;
}

void APHBaseCharacter::AwardExperienceFromKill(APHBaseCharacter* KilledCharacter)
{
	if (!ProgressionManager || !KilledCharacter)
	{
		return;
	}

	// Delegate to ProgressionManager
	ProgressionManager->AwardExperienceFromKill(KilledCharacter);
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* COMBAT & HEALTH */
/* ═══════════════════════════════════════════════════════════════════════ */

void APHBaseCharacter::OnRep_IsDead()
{
	if (!bIsDead)
	{
		return;
	}

	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (GetCharacterMovement())
	{
		GetCharacterMovement()->DisableMovement();
	}

	PlayDeathAnimation();
	OnDeathEvent.Broadcast(this, LastKillerActor);
}

float APHBaseCharacter::GetHealth() const
{
	if (StatsManager)
	{
		return StatsManager->GetHealth();
	}
	return 0.0f;
}

float APHBaseCharacter::GetMaxHealth() const
{
	if (StatsManager)
	{
		return StatsManager->GetMaxHealth();
	}
	return 1.0f;
}

float APHBaseCharacter::GetHealthPercent() const
{
	if (StatsManager)
	{
		return StatsManager->GetHealthPercent();
	}
	return 0.0f;
}




void APHBaseCharacter::OnDeath_Implementation(AController* Killer, AActor* DamageCauser)
{
	if (bIsDead)
	{
		return;
	}

	bIsDead = true;
	LastKillerActor = DamageCauser;

	UE_LOG(LogTemp, Log, TEXT("%s died"), *GetName());

	PlayDeathAnimation();
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCharacterMovement()->DisableMovement();
	
	OnDeathEvent.Broadcast(this, DamageCauser);
}

void APHBaseCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
}

void APHBaseCharacter::PlayDeathAnimation()
{
	if (DeathMontage)
	{
		PlayAnimMontage(DeathMontage);
	}
}

void APHBaseCharacter::PlayHitReactAnimation()
{
	if (HitReactMontage)
	{
		PlayAnimMontage(HitReactMontage);
	}
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* ABILITIES */
/* ═══════════════════════════════════════════════════════════════════════ */

void APHBaseCharacter::GiveDefaultAbilities()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	// Grant all default abilities
	for (TSubclassOf<UGameplayAbility>& AbilityClass : DefaultAbilities)
	{
		if (AbilityClass)
		{
			FGameplayAbilitySpec AbilitySpec(AbilityClass, 1, INDEX_NONE, this);
			FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(AbilitySpec);
			GrantedAbilityHandles.Add(Handle);
		}
	}
}

void APHBaseCharacter::ApplyStartupEffects()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	// Apply all startup effects
	for (TSubclassOf<UGameplayEffect>& EffectClass : StartupEffects)
	{
		if (EffectClass)
		{
			FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(
				EffectClass, 1, EffectContext);
			
			if (SpecHandle.IsValid())
			{
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}

void APHBaseCharacter::RemoveAllAbilities()
{
	if (!HasAuthority() || !AbilitySystemComponent)
	{
		return;
	}

	// Remove all granted abilities
	for (FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
	{
		AbilitySystemComponent->ClearAbility(Handle);
	}

	GrantedAbilityHandles.Empty();
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* TEAM & TARGETING */
/* ═══════════════════════════════════════════════════════════════════════ */

bool APHBaseCharacter::IsSameTeam(const APHBaseCharacter* OtherCharacter) const
{
	if (!OtherCharacter)
	{
		return false;
	}

	return TeamID == OtherCharacter->TeamID;
}

bool APHBaseCharacter::IsHostile(const APHBaseCharacter* OtherCharacter) const
{
	if (!OtherCharacter || OtherCharacter == this)
	{
		return false;
	}

	// Example override
	if (CombatAffiliation.Faction == EFaction::PlayerKillers ||
		OtherCharacter->GetCombatAffiliation().Faction == EFaction::PlayerKillers)
	{
		return true;
	}

	// TODO: temporary hostility / guard aggro checks here

	return TeamID != OtherCharacter->TeamID;
}