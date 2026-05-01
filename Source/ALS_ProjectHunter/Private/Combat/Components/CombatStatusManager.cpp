// Character/Component/CombatStatusManager.cpp
#include "Combat/Components/CombatStatusManager.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"

DEFINE_LOG_CATEGORY(LogCombatStatusManager);

UCombatStatusManager::UCombatStatusManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Apply functions
// ─────────────────────────────────────────────────────────────────────────────

FCombatStatusApplyResult UCombatStatusManager::ApplyBleed(AActor* Target, float DamagePerTick,
	float Duration, AActor* Instigator)
{
	if (!BleedEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusManager, "ApplyBleed failed: BleedEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(BleedEffectClass, Target,
		DamagePerTick, CombatStatusSetByCallerTags::Bleed_DamagePerTick, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusManager::ApplyIgnite(AActor* Target, float DamagePerTick,
	float Duration, AActor* Instigator)
{
	if (!IgniteEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusManager, "ApplyIgnite failed: IgniteEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(IgniteEffectClass, Target,
		DamagePerTick, CombatStatusSetByCallerTags::Ignite_DamagePerTick, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusManager::ApplyPoison(AActor* Target, float DamagePerTick,
	float Duration, AActor* Instigator)
{
	if (!PoisonEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusManager, "ApplyPoison failed: PoisonEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(PoisonEffectClass, Target,
		DamagePerTick, CombatStatusSetByCallerTags::Poison_DamagePerTick, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusManager::ApplyCorruption(AActor* Target, float DamagePerTick,
	float Duration, AActor* Instigator)
{
	if (!CorruptionEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusManager, "ApplyCorruption failed: CorruptionEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(CorruptionEffectClass, Target,
		DamagePerTick, CombatStatusSetByCallerTags::Corruption_DamagePerTick, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusManager::ApplyChill(AActor* Target, float SlowFraction,
	float Duration, AActor* Instigator)
{
	if (!ChillEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusManager, "ApplyChill failed: ChillEffectClass was not configured.");
		return {};
	}
	const float ClampedFraction = FMath::Clamp(SlowFraction, 0.0f, 0.7f);
	return ApplyDoTEffect(ChillEffectClass, Target,
		ClampedFraction, CombatStatusSetByCallerTags::Chill_SlowFraction, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusManager::ApplyFreeze(AActor* Target, float Duration,
	AActor* Instigator)
{
	if (!FreezeEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusManager, "ApplyFreeze failed: FreezeEffectClass was not configured.");
		return {};
	}
	// Freeze uses no SetByCaller — it's a pure CC, magnitude is always 1
	return ApplyDoTEffect(FreezeEffectClass, Target,
		1.0f, NAME_None, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusManager::ApplyPetrify(AActor* Target, float Duration,
	AActor* Instigator)
{
	if (!PetrifyEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusManager, "ApplyPetrify failed: PetrifyEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(PetrifyEffectClass, Target,
		1.0f, NAME_None, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusManager::ApplyShock(AActor* Target, float AmpFraction,
	float Duration, AActor* Instigator)
{
	if (!ShockEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusManager, "ApplyShock failed: ShockEffectClass was not configured.");
		return {};
	}
	const float ClampedFraction = FMath::Clamp(AmpFraction, 0.0f, 0.5f);
	return ApplyDoTEffect(ShockEffectClass, Target,
		ClampedFraction, CombatStatusSetByCallerTags::Shock_AmpFraction, Duration, Instigator);
}

// ─────────────────────────────────────────────────────────────────────────────
// Query
// ─────────────────────────────────────────────────────────────────────────────

bool UCombatStatusManager::IsBleeding(AActor* Target) const
{
	return BleedEffectClass && HasActiveEffect(Target, BleedEffectClass);
}

bool UCombatStatusManager::IsIgnited(AActor* Target) const
{
	return IgniteEffectClass && HasActiveEffect(Target, IgniteEffectClass);
}

int32 UCombatStatusManager::GetPoisonStacks(AActor* Target) const
{
	if (!PoisonEffectClass)
	{
		return 0;
	}
	UAbilitySystemComponent* ASC = GetTargetASC(Target);
	if (!ASC)
	{
		return 0;
	}
	// GetGameplayEffectCount returns the number of active stacks for the given GE class.
	return ASC->GetGameplayEffectCount(PoisonEffectClass, nullptr);
}

bool UCombatStatusManager::IsCorrupted(AActor* Target) const
{
	return CorruptionEffectClass && HasActiveEffect(Target, CorruptionEffectClass);
}

bool UCombatStatusManager::IsChilled(AActor* Target) const
{
	return ChillEffectClass && HasActiveEffect(Target, ChillEffectClass);
}

bool UCombatStatusManager::IsFrozen(AActor* Target) const
{
	return FreezeEffectClass && HasActiveEffect(Target, FreezeEffectClass);
}

bool UCombatStatusManager::IsPetrified(AActor* Target) const
{
	return PetrifyEffectClass && HasActiveEffect(Target, PetrifyEffectClass);
}

bool UCombatStatusManager::IsShocked(AActor* Target) const
{
	return ShockEffectClass && HasActiveEffect(Target, ShockEffectClass);
}

// ─────────────────────────────────────────────────────────────────────────────
// Removal
// ─────────────────────────────────────────────────────────────────────────────

void UCombatStatusManager::CureBleed(AActor* Target)
{
	if (BleedEffectClass)
	{
		RemoveEffectByClass(Target, BleedEffectClass);
	}
}

void UCombatStatusManager::CureIgnite(AActor* Target)
{
	if (IgniteEffectClass)
	{
		RemoveEffectByClass(Target, IgniteEffectClass);
	}
}

void UCombatStatusManager::CurePoison(AActor* Target)
{
	if (PoisonEffectClass)
	{
		RemoveEffectByClass(Target, PoisonEffectClass);
	}
}

void UCombatStatusManager::CureCorruption(AActor* Target)
{
	if (CorruptionEffectClass)
	{
		RemoveEffectByClass(Target, CorruptionEffectClass);
	}
}

void UCombatStatusManager::RemoveChill(AActor* Target)
{
	if (ChillEffectClass)
	{
		RemoveEffectByClass(Target, ChillEffectClass);
	}
}

void UCombatStatusManager::RemoveFreeze(AActor* Target)
{
	if (FreezeEffectClass)
	{
		RemoveEffectByClass(Target, FreezeEffectClass);
	}
}

void UCombatStatusManager::RemovePetrify(AActor* Target)
{
	if (PetrifyEffectClass)
	{
		RemoveEffectByClass(Target, PetrifyEffectClass);
	}
}

void UCombatStatusManager::RemoveShock(AActor* Target)
{
	if (ShockEffectClass)
	{
		RemoveEffectByClass(Target, ShockEffectClass);
	}
}

void UCombatStatusManager::CleanseAll(AActor* Target)
{
	CureBleed(Target);
	CureIgnite(Target);
	CurePoison(Target);
	CureCorruption(Target);
	RemoveChill(Target);
	RemoveFreeze(Target);
	RemovePetrify(Target);
	RemoveShock(Target);
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

FCombatStatusApplyResult UCombatStatusManager::ApplyDoTEffect(
	TSubclassOf<UGameplayEffect> EffectClass,
	AActor* Target,
	float SetByCallerValue,
	FName SetByCallerTag,
	float Duration,
	AActor* Instigator) const
{
	FCombatStatusApplyResult Result;

	if (!EffectClass || !Target)
	{
		return Result;
	}

	UAbilitySystemComponent* TargetASC = GetTargetASC(Target);
	if (!TargetASC)
	{
		UE_LOG(LogCombatStatusManager, Warning,
			TEXT("ApplyDoTEffect: Target '%s' has no ASC"), *Target->GetName());
		return Result;
	}

	UAbilitySystemComponent* SourceASC = GetOwnerASC();
	if (!SourceASC)
	{
		UE_LOG(LogCombatStatusManager, Warning,
			TEXT("ApplyDoTEffect: Owner '%s' has no ASC"), *GetOwner()->GetName());
		return Result;
	}

	// Build effect context — tracks instigator for source-aggregated stacking
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(Instigator ? Instigator : GetOwner(), GetOwner());

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		EffectClass, 1.0f, Context);

	if (!SpecHandle.IsValid())
	{
		return Result;
	}

	// Override duration
	if (Duration > 0.0f)
	{
		SpecHandle.Data->SetDuration(Duration, true);
	}

	// Set magnitude via SetByCaller (skip if no tag provided — e.g., Freeze)
	if (SetByCallerTag != NAME_None && SetByCallerValue != 0.0f)
	{
		const FGameplayTag GameplayTag = FGameplayTag::RequestGameplayTag(SetByCallerTag, false);
		if (GameplayTag.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(GameplayTag, SetByCallerValue);
		}
		else
		{
			UE_LOG(LogCombatStatusManager, Warning,
				TEXT("ApplyDoTEffect: SetByCaller tag '%s' is not registered."),
				*SetByCallerTag.ToString());
		}
	}

	// Apply to target.
	// BUG FIX: Must call ApplyGameplayEffectSpecToTarget on the SOURCE ASC, not the target.
	// Calling TargetASC->ApplyGameplayEffectSpecToTarget(..., TargetASC) made the target
	// the applier of its own effect, breaking source-aggregated stacking and network
	// prediction ownership. The spec context (instigator) is already correct because it
	// was built from SourceASC->MakeEffectContext().
	FActiveGameplayEffectHandle ActiveHandle =
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	if (ActiveHandle.IsValid())
	{
		Result.bApplied     = true;
		Result.EffectHandle = ActiveHandle;

		UE_LOG(LogCombatStatusManager, Log,
			TEXT("ApplyDoTEffect: Applied '%s' to '%s' (%.1fs, %.2f/tick)"),
			*EffectClass->GetName(), *Target->GetName(),
			Duration, SetByCallerValue);
	}

	return Result;
}

void UCombatStatusManager::RemoveEffectByClass(AActor* Target,
	TSubclassOf<UGameplayEffect> EffectClass) const
{
	UAbilitySystemComponent* ASC = GetTargetASC(Target);
	if (!ASC || !EffectClass)
	{
		return;
	}

	// Use class-based removal to strip all stacks of the given GE class.
	ASC->RemoveActiveGameplayEffectBySourceEffect(EffectClass, nullptr, -1);

	UE_LOG(LogCombatStatusManager, Log,
		TEXT("RemoveEffectByClass: Removed '%s' from '%s'"),
		*EffectClass->GetName(), *Target->GetName());
}

bool UCombatStatusManager::HasActiveEffect(AActor* Target,
	TSubclassOf<UGameplayEffect> EffectClass) const
{
	UAbilitySystemComponent* ASC = GetTargetASC(Target);
	if (!ASC || !EffectClass)
	{
		return false;
	}

	return ASC->GetGameplayEffectCount(EffectClass, nullptr) > 0;
}

UAbilitySystemComponent* UCombatStatusManager::GetTargetASC(AActor* Target)
{
	if (!Target)
	{
		return nullptr;
	}
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Target);
	return ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
}

UAbilitySystemComponent* UCombatStatusManager::GetOwnerASC() const
{
	return GetTargetASC(GetOwner());
}
