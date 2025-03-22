#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PHCharacterEnumLibrary.h"
#include  "Library/PHItemEnumLibrary.h"
#include  "Library/PHItemStructLibrary.h"
#include "PHCombatStructLibrary.generated.h"


USTRUCT(BlueprintType)
struct FCombatHitResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	float FinalDamage = 0.0f;

	UPROPERTY(BlueprintReadOnly)
	bool bCrit = false;

	UPROPERTY(BlueprintReadOnly)
	bool bAppliedStatus = false;

	UPROPERTY(BlueprintReadOnly)
	EDamageTypes PrimaryDamageType = EDamageTypes::DT_None;

	UPROPERTY(BlueprintReadOnly)
	FName StatusEffectName; // "Burn", "Freeze", etc.

	UPROPERTY(BlueprintReadOnly)
	FGameplayTagContainer StatusTags; // optional for flexibility
};


USTRUCT(BlueprintType)
struct FDamageByType
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDamageRange PhysicalDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDamageRange FireDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDamageRange IceDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDamageRange LightningDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDamageRange LightDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FDamageRange CorruptionDamage;

	// Add operator +=
	FDamageByType& operator+=(const FDamageByType& Other)
	{
		PhysicalDamage   += Other.PhysicalDamage;
		FireDamage       += Other.FireDamage;
		IceDamage        += Other.IceDamage;
		LightningDamage  += Other.LightningDamage;
		LightDamage      += Other.LightDamage;
		CorruptionDamage += Other.CorruptionDamage;
		return *this;
	}

	// Add operator *= (scale all damage types by a float)
	FDamageByType& operator*=(float Scale)
	{
		PhysicalDamage   *= Scale;
		FireDamage       *= Scale;
		IceDamage        *= Scale;
		LightningDamage  *= Scale;
		LightDamage      *= Scale;
		CorruptionDamage *= Scale;
		return *this;
	}

	// Optional: support a clean 'FDamageByType * float' usage
	friend FDamageByType operator*(const FDamageByType& Dmg, float Scale)
	{
		FDamageByType Result = Dmg;
		Result *= Scale;
		return Result;
	}

	friend FDamageByType operator+(const FDamageByType& A, const FDamageByType& B)
	{
		FDamageByType Result = A;
		Result += B;
		return Result;
	}

	float GetTotalDamageByType(EDamageTypes DamageType) const
	{
		switch (DamageType)
		{
		case EDamageTypes::DT_Physical:    return (PhysicalDamage.Min + PhysicalDamage.Max) * 0.5f;
		case EDamageTypes::DT_Fire:        return (FireDamage.Min + FireDamage.Max) * 0.5f;
		case EDamageTypes::DT_Ice:         return (IceDamage.Min + IceDamage.Max) * 0.5f;
		case EDamageTypes::DT_Lightning:   return (LightningDamage.Min + LightningDamage.Max) * 0.5f;
		case EDamageTypes::DT_Light:       return (LightDamage.Min + LightDamage.Max) * 0.5f;
		case EDamageTypes::DT_Corruption:  return (CorruptionDamage.Min + CorruptionDamage.Max) * 0.5f;
		default:                           return 0.f;
		}
	}

	float RollDamageByType(EDamageTypes DamageType) const
	{
		switch (DamageType)
		{
		case EDamageTypes::DT_Physical:    return FMath::RandRange(PhysicalDamage.Min,   PhysicalDamage.Max);
		case EDamageTypes::DT_Fire:        return FMath::RandRange(FireDamage.Min,       FireDamage.Max);
		case EDamageTypes::DT_Ice:         return FMath::RandRange(IceDamage.Min,        IceDamage.Max);
		case EDamageTypes::DT_Lightning:   return FMath::RandRange(LightningDamage.Min,  LightningDamage.Max);
		case EDamageTypes::DT_Light:       return FMath::RandRange(LightDamage.Min,      LightDamage.Max);
		case EDamageTypes::DT_Corruption:  return FMath::RandRange(CorruptionDamage.Min, CorruptionDamage.Max);
		default:                           return 0.f;
		}
	}

	FDamageByType RollAllTypes() const
	{
		FDamageByType Rolled;

		Rolled.PhysicalDamage   = { RollDamageByType(EDamageTypes::DT_Physical),   RollDamageByType(EDamageTypes::DT_Physical) };
		Rolled.FireDamage       = { RollDamageByType(EDamageTypes::DT_Fire),       RollDamageByType(EDamageTypes::DT_Fire) };
		Rolled.IceDamage        = { RollDamageByType(EDamageTypes::DT_Ice),        RollDamageByType(EDamageTypes::DT_Ice) };
		Rolled.LightningDamage  = { RollDamageByType(EDamageTypes::DT_Lightning),  RollDamageByType(EDamageTypes::DT_Lightning) };
		Rolled.LightDamage      = { RollDamageByType(EDamageTypes::DT_Light),      RollDamageByType(EDamageTypes::DT_Light) };
		Rolled.CorruptionDamage = { RollDamageByType(EDamageTypes::DT_Corruption), RollDamageByType(EDamageTypes::DT_Corruption) };

		return Rolled;
	}


};


USTRUCT(BlueprintType)
struct FDamageHitResultByType
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	TMap<EDamageTypes, float> FinalDamageByType;

	UPROPERTY(BlueprintReadOnly)
	bool bCrit = false;

	UPROPERTY(BlueprintReadOnly)
	TMap<EDamageTypes, bool> bAppliedStatusEffectPerType;

	UPROPERTY(BlueprintReadOnly)
	TMap<EDamageTypes, FName> StatusEffectNamePerType;

	float GetTotalDamage() const
	{
		float Total = 0.f;
		for (const auto& Pair : FinalDamageByType)
		{
			Total += Pair.Value;
		}
		return Total;
	}
	
	FString ToDebugString() const
	{
		FString Output;

		for (const auto& Pair : FinalDamageByType)
		{
			const EDamageTypes Type = Pair.Key;
			const float Damage = Pair.Value;

			FString TypeName = UEnum::GetValueAsString(Type).RightChop(15); // Trim "EDamageTypes::"
			bool bStatus = bAppliedStatusEffectPerType.Contains(Type) && bAppliedStatusEffectPerType[Type];
			FName StatusName = bStatus ? StatusEffectNamePerType.FindRef(Type) : FName();

			// Build per-line string
			Output += FString::Printf(TEXT("%.0f %s"), Damage, *TypeName);
			if (bCrit) Output += TEXT(" (Crit!)");
			if (bStatus) Output += FString::Printf(TEXT(" (%s)"), *StatusName.ToString());
			Output += TEXT(", ");
		}

		Output += FString::Printf(TEXT("Total: %.0f"), GetTotalDamage());
		return Output;
	}

};

USTRUCT(BlueprintType)
struct FDamageTypeDisplayInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FText TypeName;

	UPROPERTY(BlueprintReadOnly)
	FLinearColor TypeColor;

	UPROPERTY(BlueprintReadOnly)
	EDamageTypes Type = EDamageTypes::DT_None;
};

