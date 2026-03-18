// Copyright © 2025 MahouSensei
// Author: Quentin Davis

#include "AbilitySystem/HunterAbilitySystemComponent.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "Character/Component/TagManager.h"
#include "Character/PHBaseCharacter.h"
#include "Engine/Engine.h"
#include "PHGameplayTags.h"

namespace HunterAbilitySystemComponentPrivate
{
	constexpr float SprintStaminaDegenTickInterval = 0.1f;

	UTagManager* ResolveTagManager(const UAbilitySystemComponent* ASC)
	{
		if (!ASC)
		{
			return nullptr;
		}

		if (const APHBaseCharacter* HunterCharacter = Cast<APHBaseCharacter>(ASC->GetAvatarActor()))
		{
			return HunterCharacter->GetTagManager();
		}

		if (const APHBaseCharacter* HunterCharacter = Cast<APHBaseCharacter>(ASC->GetOwner()))
		{
			return HunterCharacter->GetTagManager();
		}

		return nullptr;
	}
}

// Define log category
DEFINE_LOG_CATEGORY(LogHunterGAS);

#if !UE_BUILD_SHIPPING
// Console variable to toggle effect debugging
static TAutoConsoleVariable<int32> CVarDebugEffects(
	TEXT("Hunter.Debug.Effects"),
	0,
	TEXT("Debug gameplay effect applications\n")
	TEXT("0: Disabled (default)\n")
	TEXT("1: Show on-screen messages\n")
	TEXT("2: Show on-screen + log to console"),
	ECVF_Cheat
);

// Console variable for debug message duration
static TAutoConsoleVariable<float> CVarDebugEffectsDuration(
	TEXT("Hunter.Debug.EffectsDuration"),
	3.0f,
	TEXT("Duration in seconds for effect debug messages (default: 3.0)"),
	ECVF_Cheat
);
#endif

UHunterAbilitySystemComponent::UHunterAbilitySystemComponent()
{
	SetIsReplicatedByDefault(true);
	ReplicationMode = EGameplayEffectReplicationMode::Mixed;
}

void UHunterAbilitySystemComponent::AbilityActorInfoSet()
{
	if (!bEffectAppliedDelegateBound)
	{
		OnGameplayEffectAppliedDelegateToSelf.AddUObject(this, &UHunterAbilitySystemComponent::EffectApplied);
		bEffectAppliedDelegateBound = true;
	}

	if (!bSprintingTagDelegateBound)
	{
		const FGameplayTag SprintingTag = FPHGameplayTags::Get().Condition_Sprinting;
		RegisterGameplayTagEvent(SprintingTag, EGameplayTagEventType::NewOrRemoved)
			.AddUObject(this, &UHunterAbilitySystemComponent::HandleSprintingTagChanged);
		bSprintingTagDelegateBound = true;
		HandleSprintingTagChanged(SprintingTag, GetTagCount(SprintingTag));
	}

	UE_LOG(
		LogHunterGAS,
		Verbose,
		TEXT("AbilityActorInfoSet: Initialized runtime delegates for ASC=%s Owner=%s Avatar=%s"),
		*GetName(),
		*GetNameSafe(GetOwner()),
		AbilityActorInfo.IsValid() ? *GetNameSafe(AbilityActorInfo->AvatarActor.Get()) : TEXT("None"));
}

void UHunterAbilitySystemComponent::EffectApplied(UAbilitySystemComponent* AbilitySystemComponent,
	const FGameplayEffectSpec& EffectSpec, FActiveGameplayEffectHandle ActiveEffectHandle)
{
	FGameplayTagContainer TagContainer;
	EffectSpec.GetAllAssetTags(TagContainer);
	EffectAssetTags.Broadcast(TagContainer);

#if !UE_BUILD_SHIPPING
	// Show debug if enabled
	if (CVarDebugEffects.GetValueOnGameThread() > 0)
	{
		ShowEffectDebug(EffectSpec, TagContainer);
	}
#endif
}

void UHunterAbilitySystemComponent::HandleSprintingTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	(void)CallbackTag;

	const AActor* AvatarActorInstance = GetAvatarActor();
	if (!AvatarActorInstance || !AvatarActorInstance->HasAuthority())
	{
		return;
	}

	if (NewCount > 0)
	{
		StartSprintStaminaDegen();
	}
	else
	{
		StopSprintStaminaDegen();
	}
}

void UHunterAbilitySystemComponent::StartSprintStaminaDegen()
{
	UWorld* World = GetWorld();
	const UHunterAttributeSet* AttributeSet = GetHunterAttributeSet();
	if (!World || !AttributeSet)
	{
		return;
	}

	const float DegenRate = FMath::Max(AttributeSet->GetStaminaDegenRate(), 0.f);
	const float DegenAmount = FMath::Max(AttributeSet->GetStaminaDegenAmount(), 0.f);
	if (DegenRate <= 0.f || DegenAmount <= 0.f)
	{
		StopSprintStaminaDegen();
		return;
	}

	if (!World->GetTimerManager().IsTimerActive(SprintStaminaDegenTimerHandle))
	{
		World->GetTimerManager().SetTimer(
			SprintStaminaDegenTimerHandle,
			this,
			&UHunterAbilitySystemComponent::TickSprintStaminaDegen,
			HunterAbilitySystemComponentPrivate::SprintStaminaDegenTickInterval,
			true);
	}

	if (!bSprintDegenEffectTagApplied)
	{
		if (UTagManager* TagManager = HunterAbilitySystemComponentPrivate::ResolveTagManager(this))
		{
			TagManager->SetTagState(FPHGameplayTags::Get().Effect_Stamina_DegenActive, true);
			bSprintDegenEffectTagApplied = true;
		}
	}
	UE_LOG(LogHunterGAS, Verbose, TEXT("StartSprintStaminaDegen: ASC=%s Rate=%.2f Amount=%.2f"), *GetName(), DegenRate, DegenAmount);
}

void UHunterAbilitySystemComponent::StopSprintStaminaDegen()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SprintStaminaDegenTimerHandle);
	}

	if (bSprintDegenEffectTagApplied)
	{
		if (UTagManager* TagManager = HunterAbilitySystemComponentPrivate::ResolveTagManager(this))
		{
			TagManager->SetTagState(FPHGameplayTags::Get().Effect_Stamina_DegenActive, false);
		}
		bSprintDegenEffectTagApplied = false;
	}
	UE_LOG(LogHunterGAS, Verbose, TEXT("StopSprintStaminaDegen: ASC=%s"), *GetName());
}

void UHunterAbilitySystemComponent::TickSprintStaminaDegen()
{
	const AActor* AvatarActorInstance = GetAvatarActor();
	if (!AvatarActorInstance || !AvatarActorInstance->HasAuthority())
	{
		StopSprintStaminaDegen();
		return;
	}

	const FPHGameplayTags& GameplayTags = FPHGameplayTags::Get();
	if (!HasMatchingGameplayTag(GameplayTags.Condition_Sprinting))
	{
		StopSprintStaminaDegen();
		return;
	}

	const UHunterAttributeSet* AttributeSet = GetHunterAttributeSet();
	if (!AttributeSet)
	{
		StopSprintStaminaDegen();
		return;
	}

	const float DegenRate = FMath::Max(AttributeSet->GetStaminaDegenRate(), 0.f);
	const float DegenAmount = FMath::Max(AttributeSet->GetStaminaDegenAmount(), 0.f);
	if (DegenRate <= 0.f || DegenAmount <= 0.f)
	{
		StopSprintStaminaDegen();
		return;
	}

	const float CurrentStamina = FMath::Max(AttributeSet->GetStamina(), 0.f);
	const float DrainAmount = DegenRate * DegenAmount * HunterAbilitySystemComponentPrivate::SprintStaminaDegenTickInterval;
	if (DrainAmount <= 0.f)
	{
		return;
	}

	const float NewStamina = FMath::Max(0.f, CurrentStamina - DrainAmount);
	if (!FMath::IsNearlyEqual(CurrentStamina, NewStamina))
	{
		SetNumericAttributeBase(UHunterAttributeSet::GetStaminaAttribute(), NewStamina);

		if (UTagManager* TagManager = HunterAbilitySystemComponentPrivate::ResolveTagManager(this))
		{
			TagManager->RefreshBaseConditionTags();
		}
	}
}

const UHunterAttributeSet* UHunterAbilitySystemComponent::GetHunterAttributeSet() const
{
	return GetSet<UHunterAttributeSet>();
}

#if !UE_BUILD_SHIPPING
void UHunterAbilitySystemComponent::ShowEffectDebug(const FGameplayEffectSpec& EffectSpec,
	const FGameplayTagContainer& TagContainer) const
{
	const int32 DebugLevel = CVarDebugEffects.GetValueOnGameThread();
	if (DebugLevel <= 0) return;

	const UGameplayEffect* EffectDef = EffectSpec.Def;
	if (!EffectDef) return;

	const FString EffectName = EffectDef->GetName();
	const AActor* HunterActor = GetOwner();
	const FString OwnerName = HunterActor ? HunterActor->GetName() : TEXT("Unknown");
    
	// Get magnitude info
	FString MagnitudeInfo;
	for (const FGameplayModifierInfo& Modifier : EffectDef->Modifiers)
	{
		float Magnitude = 0.0f;
		if (Modifier.ModifierMagnitude.AttemptCalculateMagnitude(EffectSpec, Magnitude))
		{
			const FString AttributeName = Modifier.Attribute.GetName();
			MagnitudeInfo += FString::Printf(TEXT("\n  - %s: %.2f"), *AttributeName, Magnitude);
		}
	}

	// Build debug message (using ASCII for compatibility)
	const FString DebugMessage = FString::Printf(
		TEXT("[EFFECT APPLIED] %s\nEffect: %s\nTags: %s%s"),
		*OwnerName,
		*EffectName,
		*TagContainer.ToStringSimple(),
		*MagnitudeInfo
	);

	// On-screen message
	if (GEngine)
	{
		const float Duration = CVarDebugEffectsDuration.GetValueOnGameThread();
		const FColor Color = FColor::Cyan;
        
		GEngine->AddOnScreenDebugMessage(
			INDEX_NONE,
			Duration,
			Color,
			DebugMessage
		);
	}

	// Console log if level 2+
	if (DebugLevel >= 2)
	{
		UE_LOG(LogHunterGAS, Log, TEXT("%s"), *DebugMessage);
	}
}
#endif
