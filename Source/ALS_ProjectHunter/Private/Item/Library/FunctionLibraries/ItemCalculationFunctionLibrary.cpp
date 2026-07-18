#include "Item/Library/FunctionLibraries/ItemCalculationFunctionLibrary.h"

FDamageRange UItemCalculationFunctionLibrary::CalculateFinalDamage(
	FDamageRange BaseDamage,
	float FlatAdded,
	float IncreasedPercent,
	float MorePercent)
{
	float FinalMin = BaseDamage.MinDamage + FlatAdded;
	float FinalMax = BaseDamage.MaxDamage + FlatAdded;

	if (IncreasedPercent != 0.0f)
	{
		const float IncreasedMultiplier = 1.0f + (IncreasedPercent / 100.0f);
		FinalMin *= IncreasedMultiplier;
		FinalMax *= IncreasedMultiplier;
	}

	if (MorePercent != 0.0f)
	{
		const float MoreMultiplier = 1.0f + (MorePercent / 100.0f);
		FinalMin *= MoreMultiplier;
		FinalMax *= MoreMultiplier;
	}

	return FDamageRange(FinalMin, FinalMax);
}

float UItemCalculationFunctionLibrary::CalculateDPS(FDamageRange DamageRange, float AttackSpeed)
{
	return DamageRange.GetAverage() * AttackSpeed;
}

FDamageRange UItemCalculationFunctionLibrary::CalculateCriticalDamage(
	FDamageRange BaseDamage,
	float CritMultiplier)
{
	return FDamageRange(
		BaseDamage.MinDamage * CritMultiplier,
		BaseDamage.MaxDamage * CritMultiplier);
}

float UItemCalculationFunctionLibrary::CalculateFinalResistance(
	float BaseResistance,
	float FlatAdded,
	float IncreasedPercent)
{
	float FinalResistance = BaseResistance + FlatAdded;

	if (IncreasedPercent != 0.0f)
	{
		FinalResistance *= (1.0f + IncreasedPercent / 100.0f);
	}

	return FMath::Clamp(FinalResistance, 0.0f, 100.0f);
}

float UItemCalculationFunctionLibrary::CalculateArmorReduction(float Armor, float IncomingDamage)
{
	if (IncomingDamage <= 0.0f)
	{
		return 1.0f;
	}

	const float Reduction = Armor / (Armor + 10.0f * IncomingDamage);
	return FMath::Clamp(Reduction, 0.0f, 0.9f);
}

float UItemCalculationFunctionLibrary::CalculateMaxWeightFromStrength(
	int32 Strength,
	float WeightPerStrength)
{
	return Strength * WeightPerStrength;
}

float UItemCalculationFunctionLibrary::GetOverweightPercentage(float CurrentWeight, float MaxWeight)
{
	if (MaxWeight <= 0.0f)
	{
		return 0.0f;
	}

	if (CurrentWeight <= MaxWeight)
	{
		return 0.0f;
	}

	return (CurrentWeight - MaxWeight) / MaxWeight;
}

float UItemCalculationFunctionLibrary::GetRarityValueMultiplier(EItemRarity Rarity)
{
	switch (Rarity)
	{
		case EItemRarity::IR_GradeF:  return 1.0f;
		case EItemRarity::IR_GradeE:  return 1.5f;
		case EItemRarity::IR_GradeD:  return 2.5f;
		case EItemRarity::IR_GradeC:  return 5.0f;
		case EItemRarity::IR_GradeB:  return 10.0f;
		case EItemRarity::IR_GradeA:  return 25.0f;
		case EItemRarity::IR_GradeS:  return 100.0f;
		case EItemRarity::IR_GradeSS: return 1000.0f;
		default: return 1.0f;
	}
}
