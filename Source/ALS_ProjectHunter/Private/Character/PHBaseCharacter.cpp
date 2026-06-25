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
#include "Character/Components/PHCharacterMovementComponent.h"
#include "Character/Components/CharacterSystemCoordinatorComponent.h"
#include "Components/ALSMantleComponent.h"
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


APHBaseCharacter::APHBaseCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer.SetDefaultSubobjectClass<UPHCharacterMovementComponent>(
		ACharacter::CharacterMovementComponentName))
{
	PrimaryActorTick.bCanEverTick = true;

	AbilitySystemComponent = CreateDefaultSubobject<UHunterAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UHunterAttributeSet>(TEXT("AttributeSet"));

	ProgressionManager = CreateDefaultSubobject<UCharacterProgressionManager>(TEXT("ProgressionManager"));

	EquipmentManager = CreateDefaultSubobject<UEquipmentManager>(TEXT("EquipmentComponent"));

	StatsManager = CreateDefaultSubobject<UStatsManager>(TEXT("Stats Manager"));

	TagManager = CreateDefaultSubobject<UTagManager>(TEXT("TagManager"));

	// Owns the damage pipeline for this character. Weapons and unarmed hits both route through this.
	// Assign DamageApplicationGE and RecoveryApplicationGE in Blueprint defaults.
	CombatManager = CreateDefaultSubobject<UCombatManager>(TEXT("CombatManager"));

	CombatStatusManager = CreateDefaultSubobject<UCombatStatusManager>(TEXT("CombatStatusManager"));

	CombatSystemManager = CreateDefaultSubobject<UCombatSystemManagerComponent>(TEXT("CombatSystemManager"));

	// Does not replicate; each machine rebuilds presentation from replicated slot state.
	EquipmentPresentation = CreateDefaultSubobject<UEquipmentPresentationComponent>(TEXT("EquipmentPresentation"));

	// Single wiring layer for cross-manager listeners. Orchestration logic must NOT live inline here.
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

	const bool bCanDriveWallTraversal = IsLocallyControlled() || HasAuthority();
	if (bCanDriveWallTraversal && WantsWallTraversal() &&
		GetMovementState() == EALSMovementState::InAir)
	{
		WallAttachRetryAccumulator += DeltaSeconds;
		if (WallAttachRetryAccumulator >= WallAttachRetryInterval)
		{
			WallAttachRetryAccumulator = 0.0f;
			TryStartWallTraversal();
		}
	}
	else
	{
		WallAttachRetryAccumulator = 0.0f;
	}

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

	bWallTraversalHeld = bValue;
	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerSetWallTraversalHeld(bValue);
	}

	if (bValue && GetMovementState() == EALSMovementState::InAir)
	{
		TryStartWallTraversal();
	}

	if (!TagManager)
	{
		return;
	}

	TagManager->SetTagState(FPHGameplayTags::Get().Condition_Sprinting, bValue);
	TagManager->RefreshBaseConditionTags();
}

void APHBaseCharacter::ForwardMovementAction_Implementation(const float Value)
{
	if (UPHCharacterMovementComponent* Movement = GetPHMovementComponent();
		Movement && Movement->IsWallTraversing())
	{
		const FRotator CameraYaw(0.0f, GetAimingRotation().Yaw, 0.0f);
		const FVector CameraForward = CameraYaw.Vector();
		AddMovementInput(
			Movement->ConvertWorldDirectionToWallDirection(CameraForward),
			Value);
		return;
	}

	Super::ForwardMovementAction_Implementation(Value);
}

void APHBaseCharacter::RightMovementAction_Implementation(const float Value)
{
	if (UPHCharacterMovementComponent* Movement = GetPHMovementComponent();
		Movement && Movement->IsWallTraversing())
	{
		const FRotator CameraYaw(0.0f, GetAimingRotation().Yaw, 0.0f);
		const FVector CameraRight = FRotationMatrix(CameraYaw).GetUnitAxis(EAxis::Y);
		AddMovementInput(
			Movement->ConvertWorldDirectionToWallDirection(CameraRight),
			Value);
		return;
	}

	Super::RightMovementAction_Implementation(Value);
}

void APHBaseCharacter::JumpAction_Implementation(const bool bValue)
{
	if (bValue)
	{
		if (UPHCharacterMovementComponent* Movement = GetPHMovementComponent();
			Movement && Movement->IsWallTraversing())
		{
			Movement->JumpOffWall();
			return;
		}
	}

	Super::JumpAction_Implementation(bValue);
}

FVector APHBaseCharacter::GetFootIKSurfaceNormal_Implementation() const
{
	const UPHCharacterMovementComponent* Movement = GetPHMovementComponent();
	if (Movement && Movement->IsWallTraversing())
	{
		const FVector SurfaceNormal = Movement->GetWallNormal().GetSafeNormal();
		if (!SurfaceNormal.IsNearlyZero())
		{
			return SurfaceNormal;
		}
	}

	return Super::GetFootIKSurfaceNormal_Implementation();
}

FALSWallTransitionData APHBaseCharacter::GetWallTransitionData_Implementation() const
{
	const UPHCharacterMovementComponent* Movement = GetPHMovementComponent();
	return Movement
		? Movement->GetWallTransitionData()
		: Super::GetWallTransitionData_Implementation();
}

UPHCharacterMovementComponent* APHBaseCharacter::GetPHMovementComponent() const
{
	return Cast<UPHCharacterMovementComponent>(GetCharacterMovement());
}

bool APHBaseCharacter::TryStartWallTraversal()
{
	UPHCharacterMovementComponent* Movement = GetPHMovementComponent();
	if (!Movement || Movement->IsWallTraversing() ||
		GetMovementState() != EALSMovementState::InAir)
	{
		return false;
	}

	const EALSMovementState RequestedState = SelectWallTraversalState(GetWallTraversalWeight());
	if (RequestedState != EALSMovementState::WallRunning &&
		RequestedState != EALSMovementState::WallClimbing)
	{
		return false;
	}

	return Movement->TryStartWallTraversal(RequestedState);
}

void APHBaseCharacter::StopWallTraversal()
{
	if (UPHCharacterMovementComponent* Movement = GetPHMovementComponent())
	{
		Movement->StopWallTraversal();
	}
}

void APHBaseCharacter::CompleteWallToGroundTransition()
{
	if (UPHCharacterMovementComponent* Movement = GetPHMovementComponent())
	{
		Movement->CompleteWallToGroundTransition();
	}

	if (GetLocalRole() == ROLE_AutonomousProxy)
	{
		ServerCompleteWallToGroundTransition();
	}
}

bool APHBaseCharacter::IsWallTraversing() const
{
	const UPHCharacterMovementComponent* Movement = GetPHMovementComponent();
	return Movement && Movement->IsWallTraversing();
}

bool APHBaseCharacter::IsWallRunning() const
{
	const UPHCharacterMovementComponent* Movement = GetPHMovementComponent();
	return Movement && Movement->IsWallRunning();
}

bool APHBaseCharacter::IsWallClimbing() const
{
	const UPHCharacterMovementComponent* Movement = GetPHMovementComponent();
	return Movement && Movement->IsWallClimbing();
}

float APHBaseCharacter::GetWallTraversalWeight_Implementation() const
{
	return AttributeSet ? FMath::Max(AttributeSet->GetWeight(), 0.0f) : 0.0f;
}

EALSMovementState APHBaseCharacter::SelectWallTraversalState_Implementation(const float CurrentWeight) const
{
	if (CurrentWeight <= MaxWallRunningWeight)
	{
		return EALSMovementState::WallRunning;
	}

	if (CurrentWeight <= MaxWallClimbingWeight)
	{
		return EALSMovementState::WallClimbing;
	}

	return EALSMovementState::None;
}

bool APHBaseCharacter::TryWallTopMantle()
{
	UALSMantleComponent* MantleComponent = FindComponentByClass<UALSMantleComponent>();
	if (!MantleComponent)
	{
		return false;
	}

	MantleComponent->OnOwnerJumpInput();
	return GetMovementState() == EALSMovementState::Mantling;
}

void APHBaseCharacter::OnMovementModeChanged(
	const EMovementMode PrevMovementMode,
	const uint8 PreviousCustomMode)
{
	if (PrevMovementMode == MOVE_Custom &&
		(PreviousCustomMode == static_cast<uint8>(EPHCustomMovementMode::WallRunning) ||
			PreviousCustomMode == static_cast<uint8>(EPHCustomMovementMode::WallClimbing) ||
			PreviousCustomMode == static_cast<uint8>(EPHCustomMovementMode::WallToGround)))
	{
		if (UPHCharacterMovementComponent* Movement = GetPHMovementComponent();
			Movement && !Movement->IsWallTraversing())
		{
			Movement->RestoreWorldUpRotation();
		}
	}

	Super::OnMovementModeChanged(PrevMovementMode, PreviousCustomMode);

	const UPHCharacterMovementComponent* Movement = GetPHMovementComponent();
	if (Movement && Movement->MovementMode == MOVE_Falling &&
		WantsWallTraversal() && (IsLocallyControlled() || HasAuthority()))
	{
		// Jump may begin while sprint was already held, so there may be no new
		// sprint input event. Search immediately on entering falling and retain
		// the periodic retry in Tick until a wall is found.
		TryStartWallTraversal();
	}

	if (!Movement || Movement->MovementMode != MOVE_Custom)
	{
		if (PrevMovementMode == MOVE_Custom)
		{
			SetGait(GetDesiredGait());
		}
		return;
	}

	switch (static_cast<EPHCustomMovementMode>(Movement->CustomMovementMode))
	{
	case EPHCustomMovementMode::WallRunning:
		SetMovementState(EALSMovementState::WallRunning);
		SetGait(EALSGait::Sprinting);
		break;

	case EPHCustomMovementMode::WallClimbing:
		SetMovementState(EALSMovementState::WallClimbing);
		SetGait(EALSGait::Running);
		break;

	case EPHCustomMovementMode::WallToGround:
		if (Movement->GetWallTransitionData().bStartedFromWallClimbing)
		{
			SetMovementState(EALSMovementState::WallClimbing);
			SetGait(EALSGait::Running);
		}
		else
		{
			SetMovementState(EALSMovementState::WallRunning);
			SetGait(EALSGait::Sprinting);
		}
		break;

	default:
		break;
	}
}

void APHBaseCharacter::ServerSetWallTraversalHeld_Implementation(const bool bHeld)
{
	bWallTraversalHeld = bHeld;

	if (bHeld && GetMovementState() == EALSMovementState::InAir)
	{
		TryStartWallTraversal();
	}
}

void APHBaseCharacter::ServerCompleteWallToGroundTransition_Implementation()
{
	if (UPHCharacterMovementComponent* Movement = GetPHMovementComponent())
	{
		Movement->CompleteWallToGroundTransition();
	}
}


void APHBaseCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AbilitySystemComponent)
	{
		InitializeAbilitySystem();

		// Same safety net as PossessedBy — for clients where PlayerState replication makes the ASC available.
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

	if (TagManager)
	{
		TagManager->Initialize(AbilitySystemComponent);
	}

	InitializeAttributes();

	if (HasAuthority())
	{
		GiveDefaultAbilities();
		ApplyStartupEffects();
	}

	BindAttributeDelegates();

	bAbilitySystemInitialized = true;

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
	
}

void APHBaseCharacter::BindAttributeDelegates()
{
	if (!AbilitySystemComponent || !AttributeSet)
	{
		return;
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		AttributeSet->GetHealthAttribute()).AddUObject(this, &APHBaseCharacter::HandleHealthChanged);
}

void APHBaseCharacter::OnAbilitySystemInitialized()
{
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
	return static_cast<int64>(GetCharacterLevel() * 100 * XPRewardMultiplier);
}

void APHBaseCharacter::AwardExperienceFromKill(APHBaseCharacter* KilledCharacter)
{
	if (!ProgressionManager || !KilledCharacter)
	{
		return;
	}

	ProgressionManager->AwardExperienceFromKill(KilledCharacter);
}

void APHBaseCharacter::NotifyDeath(AActor* Killer)
{
	// Server authoritative: BlueprintAuthorityOnly already blocks client BP calls,
	// this guards direct C++ callers as well.
	if (!HasAuthority())
	{
		PH_LOG_WARNING(LogPHBaseCharacter,
			"NotifyDeath called without authority on %s - ignored.", *GetName());
		return;
	}

	// Latch: BP death flows can fire from several places (health delegate,
	// montage notify, ability). Only the first call broadcasts.
	if (bHasDied)
	{
		return;
	}
	bHasDied = true;

	PH_LOG(LogPHBaseCharacter, Log, "NotifyDeath: %s killed by %s.",
		*GetName(), *GetNameSafe(Killer));

	OnDeath.Broadcast(this, Killer);
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




void APHBaseCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	(void)Data;

	if (TagManager)
	{
		TagManager->RefreshBaseConditionTags();
	}
}


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

	for (FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
	{
		AbilitySystemComponent->ClearAbility(Handle);
	}

	GrantedAbilityHandles.Empty();
}


