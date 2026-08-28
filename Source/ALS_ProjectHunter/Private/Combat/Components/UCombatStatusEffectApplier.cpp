#include "Combat/Components/UCombatStatusEffectApplier.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"

DEFINE_LOG_CATEGORY(LogCombatStatusEffectApplier);

UCombatStatusEffectApplier::UCombatStatusEffectApplier()
{
	// Never registered as an actual scene/actor component (CombatManager owns
	// this as a plain sub-object), but keep the explicit default for clarity
	// and in case anything ever does register it.
	PrimaryComponentTick.bCanEverTick = false;
}

FCombatStatusApplyResult UCombatStatusEffectApplier::ApplyBleed(AActor* Target, float DamagePerTick,
	float Duration, AActor* Instigator)
{
	if (!BleedEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusEffectApplier, "ApplyBleed failed: BleedEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(BleedEffectClass, Target,
		DamagePerTick, CombatStatusSetByCallerTags::Bleed_DamagePerTick, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusEffectApplier::ApplyIgnite(AActor* Target, float DamagePerTick,
	float Duration, AActor* Instigator)
{
	if (!IgniteEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusEffectApplier, "ApplyIgnite failed: IgniteEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(IgniteEffectClass, Target,
		DamagePerTick, CombatStatusSetByCallerTags::Ignite_DamagePerTick, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusEffectApplier::ApplyPoison(AActor* Target, float DamagePerTick,
	float Duration, AActor* Instigator)
{
	if (!PoisonEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusEffectApplier, "ApplyPoison failed: PoisonEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(PoisonEffectClass, Target,
		DamagePerTick, CombatStatusSetByCallerTags::Poison_DamagePerTick, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusEffectApplier::ApplyCorruption(AActor* Target, float DamagePerTick,
	float Duration, AActor* Instigator)
{
	if (!CorruptionEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusEffectApplier, "ApplyCorruption failed: CorruptionEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(CorruptionEffectClass, Target,
		DamagePerTick, CombatStatusSetByCallerTags::Corruption_DamagePerTick, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusEffectApplier::ApplyChill(AActor* Target, float SlowFraction,
	float Duration, AActor* Instigator)
{
	if (!ChillEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusEffectApplier, "ApplyChill failed: ChillEffectClass was not configured.");
		return {};
	}
	const float ClampedFraction = FMath::Clamp(SlowFraction, 0.0f, 0.7f);
	return ApplyDoTEffect(ChillEffectClass, Target,
		ClampedFraction, CombatStatusSetByCallerTags::Chill_SlowFraction, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusEffectApplier::ApplyFreeze(AActor* Target, float Duration,
	AActor* Instigator)
{
	if (!FreezeEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusEffectApplier, "ApplyFreeze failed: FreezeEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(FreezeEffectClass, Target,
		1.0f, NAME_None, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusEffectApplier::ApplyPetrify(AActor* Target, float Duration,
	AActor* Instigator)
{
	if (!PetrifyEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusEffectApplier, "ApplyPetrify failed: PetrifyEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(PetrifyEffectClass, Target,
		1.0f, NAME_None, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusEffectApplier::ApplyShock(AActor* Target, float AmpFraction,
	float Duration, AActor* Instigator)
{
	if (!ShockEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusEffectApplier, "ApplyShock failed: ShockEffectClass was not configured.");
		return {};
	}
	const float ClampedFraction = FMath::Clamp(AmpFraction, 0.0f, 0.5f);
	return ApplyDoTEffect(ShockEffectClass, Target,
		ClampedFraction, CombatStatusSetByCallerTags::Shock_AmpFraction, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusEffectApplier::ApplyStun(
	AActor* Target,
	const float Duration,
	AActor* Instigator)
{
	if (!StunEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusEffectApplier, "ApplyStun failed: StunEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(StunEffectClass, Target, 1.f, NAME_None, Duration, Instigator);
}

FCombatStatusApplyResult UCombatStatusEffectApplier::ApplyPurify(
	AActor* Target,
	const float Duration,
	AActor* Instigator)
{
	if (!PurifyEffectClass)
	{
		PH_LOG_WARNING(LogCombatStatusEffectApplier, "ApplyPurify failed: PurifyEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(PurifyEffectClass, Target, 1.f, NAME_None, Duration, Instigator);
}

bool UCombatStatusEffectApplier::IsBleeding(AActor* Target) const
{
	return BleedEffectClass && HasActiveEffect(Target, BleedEffectClass);
}

bool UCombatStatusEffectApplier::IsIgnited(AActor* Target) const
{
	return IgniteEffectClass && HasActiveEffect(Target, IgniteEffectClass);
}

int32 UCombatStatusEffectApplier::GetPoisonStacks(AActor* Target) const
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
	return ASC->GetGameplayEffectCount(PoisonEffectClass, nullptr);
}

bool UCombatStatusEffectApplier::IsCorrupted(AActor* Target) const
{
	return CorruptionEffectClass && HasActiveEffect(Target, CorruptionEffectClass);
}

bool UCombatStatusEffectApplier::IsChilled(AActor* Target) const
{
	return ChillEffectClass && HasActiveEffect(Target, ChillEffectClass);
}

bool UCombatStatusEffectApplier::IsFrozen(AActor* Target) const
{
	return FreezeEffectClass && HasActiveEffect(Target, FreezeEffectClass);
}

bool UCombatStatusEffectApplier::IsPetrified(AActor* Target) const
{
	return PetrifyEffectClass && HasActiveEffect(Target, PetrifyEffectClass);
}

bool UCombatStatusEffectApplier::IsShocked(AActor* Target) const
{
	return ShockEffectClass && HasActiveEffect(Target, ShockEffectClass);
}

bool UCombatStatusEffectApplier::IsStunned(AActor* Target) const
{
	return StunEffectClass && HasActiveEffect(Target, StunEffectClass);
}

bool UCombatStatusEffectApplier::IsPurified(AActor* Target) const
{
	return PurifyEffectClass && HasActiveEffect(Target, PurifyEffectClass);
}

void UCombatStatusEffectApplier::CureBleed(AActor* Target)
{
	if (BleedEffectClass)
	{
		RemoveEffectByClass(Target, BleedEffectClass);
	}
}

void UCombatStatusEffectApplier::CureIgnite(AActor* Target)
{
	if (IgniteEffectClass)
	{
		RemoveEffectByClass(Target, IgniteEffectClass);
	}
}

void UCombatStatusEffectApplier::CurePoison(AActor* Target)
{
	if (PoisonEffectClass)
	{
		RemoveEffectByClass(Target, PoisonEffectClass);
	}
}

void UCombatStatusEffectApplier::CureCorruption(AActor* Target)
{
	if (CorruptionEffectClass)
	{
		RemoveEffectByClass(Target, CorruptionEffectClass);
	}
}

void UCombatStatusEffectApplier::RemoveChill(AActor* Target)
{
	if (ChillEffectClass)
	{
		RemoveEffectByClass(Target, ChillEffectClass);
	}
}

void UCombatStatusEffectApplier::RemoveFreeze(AActor* Target)
{
	if (FreezeEffectClass)
	{
		RemoveEffectByClass(Target, FreezeEffectClass);
	}
}

void UCombatStatusEffectApplier::RemovePetrify(AActor* Target)
{
	if (PetrifyEffectClass)
	{
		RemoveEffectByClass(Target, PetrifyEffectClass);
	}
}

void UCombatStatusEffectApplier::RemoveShock(AActor* Target)
{
	if (ShockEffectClass)
	{
		RemoveEffectByClass(Target, ShockEffectClass);
	}
}

void UCombatStatusEffectApplier::RemoveStun(AActor* Target)
{
	if (StunEffectClass)
	{
		RemoveEffectByClass(Target, StunEffectClass);
	}
}

void UCombatStatusEffectApplier::RemovePurify(AActor* Target)
{
	if (PurifyEffectClass)
	{
		RemoveEffectByClass(Target, PurifyEffectClass);
	}
}

void UCombatStatusEffectApplier::CleanseAll(AActor* Target)
{
	CureBleed(Target);
	CureIgnite(Target);
	CurePoison(Target);
	CureCorruption(Target);
	RemoveChill(Target);
	RemoveFreeze(Target);
	RemovePetrify(Target);
	RemoveShock(Target);
	RemoveStun(Target);
	RemovePurify(Target);
}

FCombatStatusApplyResult UCombatStatusEffectApplier::ApplyDoTEffect(
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
		UE_LOG(LogCombatStatusEffectApplier, Warning,
			TEXT("ApplyDoTEffect: Target '%s' has no ASC"), *Target->GetName());
		return Result;
	}

	// Source comes from the Instigator CombatManager passed in (the attacker),
	// not from self-discovery. CombatStatusEffectApplier never looks up its own
	// owner: environmental/self-inflicted effects with no instigator fall
	// back to the target applying the effect to itself.
	AActor* SourceActor = IsValid(Instigator) ? Instigator : Target;
	UAbilitySystemComponent* SourceASC = GetTargetASC(SourceActor);
	if (!SourceASC)
	{
		UE_LOG(LogCombatStatusEffectApplier, Warning,
			TEXT("ApplyDoTEffect: Source '%s' has no ASC"), *GetNameSafe(SourceActor));
		return Result;
	}

	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(SourceActor, SourceActor);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(
		EffectClass, 1.0f, Context);

	if (!SpecHandle.IsValid())
	{
		return Result;
	}

	if (Duration > 0.0f)
	{
		SpecHandle.Data->SetDuration(Duration, true);
	}

	if (SetByCallerTag != NAME_None && SetByCallerValue != 0.0f)
	{
		const FGameplayTag GameplayTag = FGameplayTag::RequestGameplayTag(SetByCallerTag, false);
		if (GameplayTag.IsValid())
		{
			SpecHandle.Data->SetSetByCallerMagnitude(GameplayTag, SetByCallerValue);
		}
		else
		{
			UE_LOG(LogCombatStatusEffectApplier, Warning,
				TEXT("ApplyDoTEffect: SetByCaller tag '%s' is not registered."),
				*SetByCallerTag.ToString());
		}
	}

	FActiveGameplayEffectHandle ActiveHandle =
		SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);

	if (ActiveHandle.IsValid())
	{
		Result.bApplied     = true;
		Result.EffectHandle = ActiveHandle;

		UE_LOG(LogCombatStatusEffectApplier, Log,
			TEXT("ApplyDoTEffect: Applied '%s' to '%s' (%.1fs, %.2f/tick)"),
			*EffectClass->GetName(), *Target->GetName(),
			Duration, SetByCallerValue);
	}

	return Result;
}

void UCombatStatusEffectApplier::RemoveEffectByClass(AActor* Target,
	TSubclassOf<UGameplayEffect> EffectClass) const
{
	UAbilitySystemComponent* ASC = GetTargetASC(Target);
	if (!ASC || !EffectClass)
	{
		return;
	}

	ASC->RemoveActiveGameplayEffectBySourceEffect(EffectClass, nullptr, -1);

	UE_LOG(LogCombatStatusEffectApplier, Log,
		TEXT("RemoveEffectByClass: Removed '%s' from '%s'"),
		*EffectClass->GetName(), *Target->GetName());
}

bool UCombatStatusEffectApplier::HasActiveEffect(AActor* Target,
	TSubclassOf<UGameplayEffect> EffectClass) const
{
	UAbilitySystemComponent* ASC = GetTargetASC(Target);
	if (!ASC || !EffectClass)
	{
		return false;
	}

	return ASC->GetGameplayEffectCount(EffectClass, nullptr) > 0;
}

UAbilitySystemComponent* UCombatStatusEffectApplier::GetTargetASC(AActor* Target)
{
	if (!Target)
	{
		return nullptr;
	}
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Target);
	return ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
}
