// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/PHBaseCharacter.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "AbilitySystem/HunterAbilitySystemComponent.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystem/Effects/HunterGE_DerivedPrimaryVitals.h"
#include "AbilitySystemComponent.h"
#include "Progression/Components/CharacterProgressionManager.h"
#include "Stats/Components/StatsManager.h"
#include "Tags/Components/TagManager.h"
#include "Combat/Components/CombatStatusManager.h"
#include "Combat/Components/CombatSystemManagerComponent.h"
#include "Combat/Components/CombatManager.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Equipment/Components/EquipmentPresentationComponent.h"
#include "Character/Components/CharacterSystemCoordinatorComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "PHGameplayTags.h"

DEFINE_LOG_CATEGORY(LogPHBaseCharacter);

namespace PHBaseCharacterPrivate
{
	static bool HasDerivedVitalsStartupEffect(const TArray<TSubclassOf<UGameplayEffect>>& EffectClasses)
	{
		for (const TSubclassOf<UGameplayEffect>& EffectClass : EffectClasses)
		{
			if (EffectClass && EffectClass->IsChildOf(UHunterGE_DerivedPrimaryVitals::StaticClass()))
			{
				return true;
			}
		}

		return false;
	}
}


APHBaseCharacter::APHBaseCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	// Create Ability System Component
	AbilitySystemComponent = CreateDefaultSubobject<UHunterAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Create Attribute Set
	AttributeSet = CreateDefaultSubobject<UHunterAttributeSet>(TEXT("AttributeSet"));

	// Create Progression Manager
	ProgressionManager = CreateDefaultSubobject<UCharacterProgressionManager>(TEXT("ProgressionManager"));

	// Create Equipment Component
	EquipmentManager = CreateDefaultSubobject<UEquipmentManager>(TEXT("EquipmentComponent"));

	// Create Stats Manager
	StatsManager = CreateDefaultSubobject<UStatsManager>(TEXT("Stats Manager"));
	
	// Create Tag Manager
	TagManager = CreateDefaultSubobject<UTagManager>(TEXT("TagManager"));
	
	
	// Create Combat Manager — owns the damage pipeline for this character.
	// Weapons and unarmed hits both route through this. Assign DamageApplicationGE
	// and RecoveryApplicationGE in Blueprint defaults.
	CombatManager = CreateDefaultSubobject<UCombatManager>(TEXT("CombatManager"));

	// Create Combat Status Manager - assign GE classes in Blueprint defaults.
	CombatStatusManager = CreateDefaultSubobject<UCombatStatusManager>(TEXT("CombatStatusManager"));

	// Combat front door for Blueprint and C++ callers. Heavy math stays in CombatManager.
	CombatSystemManager = CreateDefaultSubobject<UCombatSystemManagerComponent>(TEXT("CombatSystemManager"));

	// PH-0.4: Equipment Presentation Component — visual weapon actors and mesh attachment.
	// Does not replicate; each machine rebuilds presentation from replicated slot state.
	EquipmentPresentation = CreateDefaultSubobject<UEquipmentPresentationComponent>(TEXT("EquipmentPresentation"));

	// PH-0.4: Character System Coordinator — single wiring layer for cross-manager listeners.
	// APHBaseCharacter is a composition root; orchestration logic must NOT live inline here.
	SystemCoordinator = CreateDefaultSubobject<UCharacterSystemCoordinatorComponent>(TEXT("SystemCoordinator"));
}

void APHBaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}


void APHBaseCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	// BLUEPRINT-CDO RECOVERY
	// ─────────────────────────────────────────────────────────────────────────
	// PostInitializeComponents fires AFTER the Blueprint CDO has stamped its
	// serialized property overrides onto this instance. If any Blueprint in the
	// hierarchy (ALS_BaseCharacterBP, ALS_PlayerCharacterBP, etc.) was saved
	// before these UPROPERTY members existed on PHBaseCharacter — or was saved
	// during a class reparent — the CDO carries null for those properties and
	// overwrites the valid TObjectPtrs the C++ constructor just set.
	//
	// The underlying UObjects still exist (CreateDefaultSubobject creates them
	// and registers them regardless), so we can recover the pointers here:
	//   • UActorComponents  → FindComponentByClass
	//   • Plain UObject subobjects (AttributeSet) → FindObject with outer = this
	//
	// PERMANENT FIX: Open every Blueprint child of PHBaseCharacter in the editor,
	// click Compile, then Save. The Blueprint CDO will regenerate from the current
	// C++ defaults and stop serializing null for these members. Once all Blueprints
	// are resaved these recovery blocks become permanent no-ops.

#define PH_RECOVER_COMPONENT(Member, Type) \
	if (!Member) \
	{ \
			Member = FindComponentByClass<Type>(); \
			PH_LOG_WARNING(LogPHBaseCharacter, "PostInitializeComponents recovered a null %s pointer on Character=%s with RecoveredComponent=%s. Compile and save this Blueprint.", TEXT(#Member), *GetName(), *GetNameSafe(Member)); \
	}

	// UActorComponent subobjects — FindComponentByClass recovers each one.
	PH_RECOVER_COMPONENT(AbilitySystemComponent, UHunterAbilitySystemComponent)
	PH_RECOVER_COMPONENT(ProgressionManager,     UCharacterProgressionManager)
	PH_RECOVER_COMPONENT(EquipmentManager,       UEquipmentManager)
	PH_RECOVER_COMPONENT(StatsManager,           UStatsManager)
	PH_RECOVER_COMPONENT(TagManager,             UTagManager)
	PH_RECOVER_COMPONENT(CombatManager,          UCombatManager)
	PH_RECOVER_COMPONENT(CombatStatusManager,    UCombatStatusManager)
	PH_RECOVER_COMPONENT(CombatSystemManager,    UCombatSystemManagerComponent)
	PH_RECOVER_COMPONENT(EquipmentPresentation,  UEquipmentPresentationComponent)
	PH_RECOVER_COMPONENT(SystemCoordinator,      UCharacterSystemCoordinatorComponent)

#undef PH_RECOVER_COMPONENT

	// AttributeSet is a plain UObject subobject (not UActorComponent), so
	// FindObject<T>(outer, name) is used with the exact CreateDefaultSubobject name.
	if (!AttributeSet)
	{
		AttributeSet = FindObject<UHunterAttributeSet>(this, TEXT("AttributeSet"));
		if (AttributeSet)
		{
			PH_LOG_WARNING(LogPHBaseCharacter, "PostInitializeComponents recovered a null AttributeSet pointer on Character=%s with AttributeSet=%s. Compile and save this Blueprint.", *GetName(), *GetNameSafe(AttributeSet));
		}
		else
		{
			AttributeSet = NewObject<UHunterAttributeSet>(this, UHunterAttributeSet::StaticClass(), TEXT("AttributeSet"));
			if (AttributeSet)
			{
				PH_LOG_WARNING(LogPHBaseCharacter, "PostInitializeComponents created a runtime AttributeSet for Character=%s because Blueprint recovery via FindObject failed. Compile and save this Blueprint.", *GetName());
			}
			else
			{
				PH_LOG_ERROR(LogPHBaseCharacter, "PostInitializeComponents failed: Character=%s had a null AttributeSet and recovery via FindObject/NewObject also failed.", *GetName());
			}
		}
	}

	if (AbilitySystemComponent && AttributeSet && !AbilitySystemComponent->GetSet<UHunterAttributeSet>())
	{
		AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet.Get());

		if (AbilitySystemComponent->GetSet<UHunterAttributeSet>() == AttributeSet.Get())
		{
			PH_LOG_WARNING(LogPHBaseCharacter, "PostInitializeComponents registered AttributeSet=%s with ASC=%s for Character=%s after Blueprint recovery. Compile and save this Blueprint.", *GetNameSafe(AttributeSet), *GetNameSafe(AbilitySystemComponent), *GetName());
		}
		else
		{
			PH_LOG_ERROR(LogPHBaseCharacter, "PostInitializeComponents failed to register AttributeSet=%s with ASC=%s for Character=%s after Blueprint recovery.", *GetNameSafe(AttributeSet), *GetNameSafe(AbilitySystemComponent), *GetName());
		}
	}
}

void APHBaseCharacter::BeginPlay()
{

	if (AbilitySystemComponent)
	{
		InitializeAbilitySystem();
	}

	Super::BeginPlay();
}

void APHBaseCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!IsLocallyControlled())
	{
		return;
	}

	UHunterAbilitySystemComponent* HunterASC = Cast<UHunterAbilitySystemComponent>(AbilitySystemComponent);
	if (!HunterASC)
	{
		return;
	}

	const bool bGamePaused = GetWorld() ? GetWorld()->IsPaused() : false;
	HunterASC->ProcessAbilityInput(DeltaSeconds, bGamePaused);
}

void APHBaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (AbilitySystemComponent)
	{
		InitializeAbilitySystem();


		if (TagManager && !TagManager->IsInitialized())
		{
			TagManager->Initialize(AbilitySystemComponent);
		}
	}
}

void APHBaseCharacter::SprintAction_Implementation(bool bValue)
{
	Super::SprintAction_Implementation(bValue);

	if (!TagManager)
	{
		return;
	}

	TagManager->SetTagState(FPHGameplayTags::Get().Condition_Sprinting, bValue);
	TagManager->RefreshBaseConditionTags();
}


void APHBaseCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AbilitySystemComponent)
	{
		InitializeAbilitySystem();

		// Same safety net as PossessedBy — for clients where PlayerState replication
		// is what makes the ASC available.
		if (TagManager && !TagManager->IsInitialized())
		{
			TagManager->Initialize(AbilitySystemComponent);
		}
	}
}

void APHBaseCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	if (AbilitySystemComponent)
	{
		InitializeAbilitySystem();
	}
}

/* ═══════════════════════════════════════════════════════════════════════ */
/* INITIALIZATION */
/* ═══════════════════════════════════════════════════════════════════════ */

bool APHBaseCharacter::EnsureAttributeSetRegisteredWithAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		PH_LOG_ERROR(LogPHBaseCharacter, "EnsureAttributeSetRegisteredWithAbilitySystem failed: %s has no ASC.", *GetName());
		return false;
	}

	if (!AttributeSet)
	{
		PH_LOG_ERROR(LogPHBaseCharacter, "EnsureAttributeSetRegisteredWithAbilitySystem failed: %s has no AttributeSet.", *GetName());
		return false;
	}

	const UAttributeSet* RegisteredAttributeSet = nullptr;
	for (const UAttributeSet* Candidate : AbilitySystemComponent->GetSpawnedAttributes())
	{
		if (Candidate && Candidate->IsA(AttributeSet->GetClass()))
		{
			RegisteredAttributeSet = Candidate;
			break;
		}
	}

	if (RegisteredAttributeSet && RegisteredAttributeSet != AttributeSet)
	{
		PH_LOG_WARNING(LogPHBaseCharacter, "EnsureAttributeSetRegisteredWithAbilitySystem adopted LiveAttributeSet=%s on ASC=%s for Character=%s instead of stale AttributeSet=%s.", *GetNameSafe(RegisteredAttributeSet), *GetNameSafe(AbilitySystemComponent), *GetName(), *GetNameSafe(AttributeSet));

		AttributeSet = Cast<UHunterAttributeSet>(const_cast<UAttributeSet*>(RegisteredAttributeSet));
	}
	else if (!RegisteredAttributeSet)
	{

		AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet.Get());

		for (const UAttributeSet* Candidate : AbilitySystemComponent->GetSpawnedAttributes())
		{
			if (Candidate && Candidate->IsA(AttributeSet->GetClass()))
			{
				RegisteredAttributeSet = Candidate;
				break;
			}
		}
	}

	const bool bRegisteredCorrectly = (RegisteredAttributeSet == AttributeSet);
	if (!bRegisteredCorrectly)
	{
		PH_LOG_ERROR(LogPHBaseCharacter, "EnsureAttributeSetRegisteredWithAbilitySystem failed for Character=%s. ASC=%s AttributeSet=%s LiveAttributeSet=%s.", *GetName(), *GetNameSafe(AbilitySystemComponent), *GetNameSafe(AttributeSet), *GetNameSafe(RegisteredAttributeSet));
	}

	return bRegisteredCorrectly;
}

void APHBaseCharacter::InitializeAbilitySystem()
{
	if (!AbilitySystemComponent)
	{
		PH_LOG_ERROR(LogPHBaseCharacter, "InitializeAbilitySystem failed: %s has no ASC.", *GetName());
		return;
	}

	const bool bHasLiveAttributeSet = EnsureAttributeSetRegisteredWithAbilitySystem();
	if (!bHasLiveAttributeSet)
	{
		PH_LOG_ERROR(LogPHBaseCharacter, "InitializeAbilitySystem failed for Character=%s because the live AttributeSet was missing on ASC=%s.", *GetName(), *GetNameSafe(AbilitySystemComponent));
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (UHunterAbilitySystemComponent* HunterASC = Cast<UHunterAbilitySystemComponent>(AbilitySystemComponent))
	{
		HunterASC->AbilityActorInfoSet();
	}

	CachedLevel = FMath::Max(GetCharacterLevel(), 1);

	if (HasAuthority())
	{
		AbilitySystemComponent->SetNumericAttributeBase(
			UHunterAttributeSet::GetPlayerLevelAttribute(),
			static_cast<float>(CachedLevel));
	}


	if (bAbilitySystemInitialized)
	{
		if (TagManager)
		{
			TagManager->RefreshBaseConditionTags();
		}

		if (StatsManager)
		{
			StatsManager->NotifyAbilitySystemReady();
		}

		UE_LOG(
			LogPHBaseCharacter,
			Verbose,
			TEXT("InitializeAbilitySystem: Refreshed actor info for %s. ASC=%s LiveAttributeSet=%s"),
			*GetName(),
			*GetNameSafe(AbilitySystemComponent),
			*GetNameSafe(AttributeSet));
		return;
	}

	// --- First-time initialization from here down ---

	if (TagManager)
	{
		TagManager->Initialize(AbilitySystemComponent);
	}

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

	if (TagManager)
	{
		TagManager->RefreshBaseConditionTags();
	}

	if (StatsManager)
	{
		StatsManager->NotifyAbilitySystemReady();
	}

	UE_LOG(
			LogPHBaseCharacter,
		Log,
		TEXT("Ability System initialized for %s. ASC=%s LiveAttributeSet=%s"),
		*GetName(),
		*GetNameSafe(AbilitySystemComponent),
		*GetNameSafe(AttributeSet));
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
	// I-10 FIX: Apply XPRewardMultiplier so designers can tune per-enemy XP budgets
	// in Blueprint/DataAsset without touching code. Previously hard-coded to Level*100.
	return static_cast<int64>(GetCharacterLevel() * 100 * XPRewardMultiplier);
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




void APHBaseCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	(void)Data;

	if (TagManager)
	{
		TagManager->RefreshBaseConditionTags();
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

	if (UHunterAbilitySystemComponent* HunterASC = Cast<UHunterAbilitySystemComponent>(AbilitySystemComponent))
	{
		for (UPHAbilitySet* AbilitySet : DefaultAbilitySets)
		{
			if (!AbilitySet)
			{
				continue;
			}

			FPHAbilitySet_GrantedHandles GrantedHandles;
			AbilitySet->GiveToAbilitySystem(HunterASC, &GrantedHandles, this);
			if (!GrantedHandles.IsEmpty())
			{
				GrantedAbilitySetHandles.Add(GrantedHandles);
			}
		}
	}
	else if (!DefaultAbilitySets.IsEmpty())
	{
		UE_LOG(LogPHBaseCharacter, Error, TEXT("GiveDefaultAbilities: Character=%s has DefaultAbilitySets but ASC=%s is not UHunterAbilitySystemComponent."), *GetName(), *GetNameSafe(AbilitySystemComponent));
	}

	// Legacy direct grants. New abilities should prefer DefaultAbilitySets.
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

	TArray<TSubclassOf<UGameplayEffect>> EffectiveStartupEffects = StartupEffects;
	if (!PHBaseCharacterPrivate::HasDerivedVitalsStartupEffect(EffectiveStartupEffects))
	{
		EffectiveStartupEffects.Add(UHunterGE_DerivedPrimaryVitals::StaticClass());
	}

	// Apply all startup effects
	for (TSubclassOf<UGameplayEffect>& EffectClass : EffectiveStartupEffects)
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

	if (UHunterAbilitySystemComponent* HunterASC = Cast<UHunterAbilitySystemComponent>(AbilitySystemComponent))
	{
		for (FPHAbilitySet_GrantedHandles& GrantedHandles : GrantedAbilitySetHandles)
		{
			GrantedHandles.TakeFromAbilitySystem(HunterASC);
		}

		GrantedAbilitySetHandles.Empty();
	}

	// Remove legacy direct grants.
	for (FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
	{
		AbilitySystemComponent->ClearAbility(Handle);
	}

	GrantedAbilityHandles.Empty();
}


