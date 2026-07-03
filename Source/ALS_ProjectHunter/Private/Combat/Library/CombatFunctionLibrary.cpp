#include "Combat/Library/CombatFunctionLibrary.h"
#include "Character/PHBaseCharacter.h"


float UCombatFunctionLibrary::GetHealthPercent(const APHBaseCharacter* Character)
{
	if (!Character )
	{
		return 0.f;
	}
	return Character->GetHealthPercent();
}

float UCombatFunctionLibrary::GetResolvedDamageByType(
	const FCombatResolveResult& Result,
	const EHunterDamageType DamageType)
{
	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		return Result.PhysicalTaken;
	case EHunterDamageType::Fire:
		return Result.FireTaken;
	case EHunterDamageType::Ice:
		return Result.IceTaken;
	case EHunterDamageType::Lightning:
		return Result.LightningTaken;
	case EHunterDamageType::Light:
		return Result.LightTaken;
	case EHunterDamageType::Corruption:
		return Result.CorruptionTaken;
	default:
		return 0.f;
	}
}

EHunterDamageType UCombatFunctionLibrary::GetDominantDamageTypeFromResolveResult(
	const FCombatResolveResult& Result)
{
	EHunterDamageType DominantDamageType = EHunterDamageType::Physical;
	float HighestDamage = GetResolvedDamageByType(Result, DominantDamageType);

	auto CheckDamageType = [&](const EHunterDamageType DamageType)
	{
		const float Damage = GetResolvedDamageByType(Result, DamageType);
		if (Damage > HighestDamage + KINDA_SMALL_NUMBER)
		{
			HighestDamage = Damage;
			DominantDamageType = DamageType;
		}
	};

	CheckDamageType(EHunterDamageType::Fire);
	CheckDamageType(EHunterDamageType::Ice);
	CheckDamageType(EHunterDamageType::Lightning);
	CheckDamageType(EHunterDamageType::Light);
	CheckDamageType(EHunterDamageType::Corruption);

	return DominantDamageType;
}

FLinearColor UCombatFunctionLibrary::GetDefaultDamageTypeColor(const EHunterDamageType DamageType)
{
	switch (DamageType)
	{
	case EHunterDamageType::Physical:
		return FLinearColor::White;
	case EHunterDamageType::Fire:
		return FLinearColor(1.0f, 0.22f, 0.06f, 1.0f);
	case EHunterDamageType::Ice:
		return FLinearColor(0.30f, 0.85f, 1.0f, 1.0f);
	case EHunterDamageType::Lightning:
		return FLinearColor(1.0f, 0.88f, 0.12f, 1.0f);
	case EHunterDamageType::Light:
		return FLinearColor(1.0f, 0.96f, 0.55f, 1.0f);
	case EHunterDamageType::Corruption:
		return FLinearColor(0.65f, 0.18f, 1.0f, 1.0f);
	default:
		return FLinearColor::White;
	}
}

FText UCombatFunctionLibrary::FormatDamagePopupAmount(const float DamageAmount)
{
	return FText::AsNumber(FMath::RoundToInt(FMath::Max(DamageAmount, 0.f)));
}
