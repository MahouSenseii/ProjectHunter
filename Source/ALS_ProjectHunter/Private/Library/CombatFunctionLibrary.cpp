// Fill out your copyright notice in the Description page of Project Settings.


#include "Library/CombatFunctionLibrary.h"

#include "Library/PHCombatStructLibrary.h"
#include "Library/PHItemEnumLibrary.h"
#include "Library/PHDamageTypeUtils.h"

FText UCombatFunctionLibrary::FormatDamageHitResult(const FDamageHitResultByType& HitResult)
{
	FString Output;
	const FString Comma = TEXT(", ");

	for (const auto& Pair : HitResult.FinalDamageByType)
	{
		const EDamageTypes Type = Pair.Key;
		const float Damage = Pair.Value;

                // Get name and color
                const FString TypeName = DamageTypeToString(Type);
		const FLinearColor Color = GetDamageColor(Type);
		const FString ColorHex = Color.ToFColor(true).ToHex();

		FString Segment = FString::Printf(TEXT("<span color=\"#%s\">%.0f %s</span>"), *ColorHex, Damage, *TypeName);

		if (HitResult.bCrit)
		{
			Segment += TEXT(" <span color=\"#FFD700\">(Crit!)</span>");
		}

		if (HitResult.bAppliedStatusEffectPerType.Contains(Type) && HitResult.bAppliedStatusEffectPerType[Type])
		{
			const FName StatusName = HitResult.StatusEffectNamePerType.FindRef(Type);
			Segment += FString::Printf(TEXT(" <span color=\"#B400FF\">(%s)</span>"), *StatusName.ToString());
		}

		Output += Segment + Comma;
	}

	Output += FString::Printf(TEXT("Total: <b>%.0f</b>"), HitResult.GetTotalDamage());

	return FText::FromString(Output);
}

FLinearColor UCombatFunctionLibrary::GetDamageColor(EDamageTypes Type)
{
	switch (Type)
	{
	case EDamageTypes::DT_Physical:    return FLinearColor(0.85f, 0.85f, 0.85f); // Gray
	case EDamageTypes::DT_Fire:        return FLinearColor::Red;
	case EDamageTypes::DT_Ice:         return FLinearColor(0.3f, 0.6f, 1.0f);   // Blue
	case EDamageTypes::DT_Lightning:   return FLinearColor(1.0f, 1.0f, 0.2f);   // Yellow
	case EDamageTypes::DT_Light:       return FLinearColor(1.0f, 1.0f, 1.0f);   // White
	case EDamageTypes::DT_Corruption:  return FLinearColor(0.5f, 0.0f, 0.5f);   // Purple
	default:                           return FLinearColor::White;
	}
}

FText UCombatFunctionLibrary::FormatPopupDamageText(const FDamageHitResultByType& HitResult)
{
	if (HitResult.FinalDamageByType.Num() == 0)
	{
		return FText::FromString(TEXT("0"));
	}

	// Find the highest-damage type
	EDamageTypes TopType = EDamageTypes::DT_None;
	float TopValue = -1.f;

	for (const auto& Pair : HitResult.FinalDamageByType)
	{
		if (Pair.Value > TopValue)
		{
			TopValue = Pair.Value;
			TopType = Pair.Key;
		}
	}

	const FLinearColor Color = GetDamageColor(TopType);
	const FString HexColor = Color.ToFColor(true).ToHex();
	const float Total = HitResult.GetTotalDamage();

	FString Text = FString::Printf(TEXT("<span color=\"#%s\">"), *HexColor);

	if (HitResult.bCrit)
	{
		Text += FString::Printf(TEXT("<b>%.0f</b>!"), Total);
	}
	else
	{
		Text += FString::Printf(TEXT("%.0f"), Total);
	}

	Text += TEXT("</span>");

	return FText::FromString(Text);
}

FText UCombatFunctionLibrary::GetStrongestDamageTypeName(const FDamageHitResultByType& Hit)
{
	EDamageTypes MaxType = EDamageTypes::DT_None;
	float MaxValue = 0.f;

	for (const auto& Pair : Hit.FinalDamageByType)
	{
		if (Pair.Value > MaxValue)
		{
			MaxType = Pair.Key;
			MaxValue = Pair.Value;
		}
	}

        const FString TypeName = DamageTypeToString(MaxType);
        return FText::FromString(TypeName);
}

FDamageTypeDisplayInfo UCombatFunctionLibrary::GetStrongestDamageTypeInfo(const FDamageHitResultByType& HitResult)
{
	FDamageTypeDisplayInfo Info;

	EDamageTypes MaxType = EDamageTypes::DT_None;
	float MaxValue = 0.f;

	for (const auto& Pair : HitResult.FinalDamageByType)
	{
		if (Pair.Value > MaxValue)
		{
			MaxType = Pair.Key;
			MaxValue = Pair.Value;
		}
	}

        if (MaxType != EDamageTypes::DT_None)
        {
                const FString Name = DamageTypeToString(MaxType);
                Info.TypeName = FText::FromString(Name);
		Info.TypeColor = GetDamageColor(MaxType); // Reuse your existing color helper
		Info.Type = MaxType;
	}

	return Info;
}
