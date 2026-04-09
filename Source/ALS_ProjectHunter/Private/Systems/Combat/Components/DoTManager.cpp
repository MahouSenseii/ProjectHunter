// Character/Component/DoTManager.cpp
#include "Systems/Combat/Components/DoTManager.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"

DEFINE_LOG_CATEGORY(LogDoTManager);

UDoTManager::UDoTManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Apply functions
// ─────────────────────────────────────────────────────────────────────────────

FDoTApplyResult UDoTManager::ApplyBleed(AActor* Target, float DamagePerTick,
	float Duration, AActor* Instigator)
{
	if (!BleedEffectClass)
	{
		PH_LOG_WARNING(LogDoTManager, "ApplyBleed failed: BleedEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(BleedEffectClass, Target,
		DamagePerTick, DoTSetByCallerTags::Bleed_DamagePerTick, Duration, Instigator);
}

FDoTApplyResult UDoTManager::ApplyIgnite(AActor* Target, float DamagePerTick,
	float Duration, AActor* Instigator)
{
	if (!IgniteEffectClass)
	{
		PH_LOG_WARNING(LogDoTManager, "ApplyIgnite failed: IgniteEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(IgniteEffectClass, Target,
		DamagePerTick, DoTSetByCallerTags::Ignite_DamagePerTick, Duration, Instigator);
}

FDoTApplyResult UDoTManager::ApplyPoison(AActor* Target, float DamagePerTick,
	float Duration, AActor* Instigator)
{
	if (!PoisonEffectClass)
	{
		PH_LOG_WARNING(LogDoTManager, "ApplyPoison failed: PoisonEffectClass was not configured.");
		return {};
	}
	return ApplyDoTEffect(PoisonEffectClass, Target,
		DamagePerTick, DoTSetByCallerTags::Poison_DamagePerTick, Duration, Instigator);
}

FDoTApplyResult UDoTManager::ApplyChill(AActor* Target, float SlowFraction,
	float Duration, AActor* Instigator)
{
	if (!ChillEffectClass)
	{
		PH_LOG_WARNING(LogDoTManager, "ApplyChill failed: ChillEffectClass was not configured.");
		return {};
	}
	const float ClampedFraction = FMath::Clamp(SlowFraction, 0.0f, 0.7f);
	return ApplyDoTEffect(ChillEffectClass, Target,
		ClampedFraction, DoTSetByCallerTags::Chill_SlowFraction, Duration, Instigator);
}

FDoTApplyResult UDoTManager::ApplyFreeze(AActor* Target, float Duration,
	AActor* Instigator)
{
	if (!FreezeEffectClass)
	{
		PH_LOG_WARNING(LogDoTManager, "ApplyFreeze failed: FreezeEffectClass was not configured.");
		return {};
	}
	// Freeze uses no SetByCaller — it's a pure CC, magnitude is always 1
	return ApplyDoTEffect(FreezeEffectClass, Target,
		1.0f, NAME_None, Duration, Instigator);
}

FDoTApplyResult UDoTManager::ApplyShock(AActor* Target, float AmpFraction,
	float Duration, AActor* Instigator)
{
	if (!ShockEffectClass)
	{
		PH_LOG_WARNING(LogDoTManager, "ApplyShock failed: ShockEffectClass was not configured.");
		return {};
	}
	const float ClampedFraction = FMath::Clamp(AmpFraction, 0.0f, 0.5f);
	return ApplyDoTEffect(ShockEffectClass, Target,
		ClampedFraction, DoTSetByCallerTags::Shock_AmpFraction, Duration, Instigator);
}

// ─────────────────────────────────────────────────────────────────────────────
// Query
// ─────────────────────────────────────────────────────────────────────────────

bool UDoTManager::IsBleeding(AActor* Target) const
{
	return BleedEffectClass && HasActiveEffect(Target, BleedEffectClass);
}

bool UDoTManager::IsIgnited(AActor* Target) const
{
	return IgniteEffectClass && HasActiveEffect(Target, IgniteEffectClass);
}

int32 UDoTManager::GetPoisonStacks(AActor* Target) const
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

bool UDoTManager::IsChilled(AActor* Target) const
{
	return ChillEffectClass && HasActiveEffect(Target, ChillEffectClass);
}

bool UDoTManager::IsFrozen(AActor* Target) const
{
	return FreezeEffectClass && HasActiveEffect(Target, FreezeEffectClass);
}

bool UDoTManager::IsShocked(AActor* Target) const
{
	return ShockEffectClass && HasActiveEffect(Target, ShockEffectClass);
}

// ─────────────────────────────────────────────────────────────────────────────
// Removal
// ─────────────────────────────────────────────────────────────────────────────

void UDoTManager::CureBleed(AActor* Target)
{
	if (BleedEffectClass)
	{
		RemoveEffectByClass(Target, BleedEffectClass);
	}
}

void UDoTManager::CureIgnite(AActor* Target)
{
	if (IgniteEffectClass)
	{
		RemoveEffectByClass(Target, IgniteEffectClass);
	}
}

void UDoTManager::CurePoison(AActor* Target)
{
	if (PoisonEffectClass)
	{
		RemoveEffectByClass(Target, PoisonEffectClass);
	}
}

void UDoTManager::RemoveChill(AActor* Target)
{
	if (ChillEffectClass)
	{
		RemoveEffectByClass(Target, ChillEffectClass);
	}
}

void UDoTManager::RemoveFreeze(AActor* Target)
{
	if (FreezeEffectClass)
	{
		RemoveEffectByClass(Target, FreezeEffectClass);
	}
}

void UDoTManager::RemoveShock(AActor* Target)
{
	if (ShockEffectClass)
	{
		RemoveEffectByClass(Target, ShockEffectClass);
	}
}

void UDoTManager::CleanseAll(AActor* Target)
{
	CureBleed(Target);
	CureIgnite(Target);
	CurePoison(Target);
	RemoveChill(Target);
	RemoveFreeze(Target);
	RemoveShock(Target);
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

FDoTApplyResult UDoTManager::ApplyDoTEffect(
	TSubclassOf<UGameplayEffect> EffectClass,
	AActor* Target,
	float SetByCallerValue,
	FName SetByCallerTag,
	float Duration,
	AActor* Instigator) const
{
	FDoTApplyResult Result;

	if (!EffectClass || !Target)
	{
		return Result;
	}

	UAbilitySystemComponent* TargetASC = GetTargetASC(Target);
	if (!TargetASC)
	{
		UE_LOG(LogDoTManager, Warning,
			TEXT("ApplyDoTEffect: Target '%s' has no ASC"), *Target->GetName());
		return Result;
	}

	UAbilitySystemComponent* SourceASC = GetOwnerASC();
	if (!SourceASC)
	{
		UE_LOG(LogDoTManager, Warning,
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
		SpecHandle.Data->SetSetByCallerMagnitude(
			FGameplayTag::RequestGameplayTag(SetByCallerTag),
			SetByCallerValue);
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

		UE_LOG(LogDoTManager, Log,
			TEXT("ApplyDoTEffect: Applied '%s' to '%s' (%.1fs, %.2f/tick)"),
			*EffectClass->GetName(), *Target->GetName(),
			Duration, SetByCallerValue);
	}

	return Result;
}

void UDoTManager::RemoveEffectByClass(AActor* Target,
	TSubclassOf<UGameplayEffect> EffectClass) const
{
	UAbilitySystemComponent* ASC = GetTargetASC(Target);
	if (!ASC || !EffectClass)
	{
		return;
	}

	// Use class-based removal to strip all stacks of the given GE class.
	ASC->RemoveActiveGameplayEffectBySourceEffect(EffectClass, nullptr, -1);

	UE_LOG(LogDoTManager, Log,
		TEXT("RemoveEffectByClass: Removed '%s' from '%s'"),
		*EffectClass->GetName(), *Target->GetName());
}

bool UDoTManager::HasActiveEffect(AActor* Target,
	TSubclassOf<UGameplayEffect> EffectClass) const
{
	UAbilitySystemComponent* ASC = GetTargetASC(Target);
	if (!ASC || !EffectClass)
	{
		return false;
	}

	return ASC->GetGameplayEffectCount(EffectClass, nullptr) > 0;
}

UAbilitySystemComponent* UDoTManager::GetTargetASC(AActor* Target)
{
	if (!Target)
	{
		return nullptr;
	}
	IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Target);
	return ASCInterface ? ASCInterface->GetAbilitySystemComponent() : nullptr;
}

UAbilitySystemComponent* UDoTManager::GetOwnerASC() const
{
	return GetTargetASC(GetOwner());
}
