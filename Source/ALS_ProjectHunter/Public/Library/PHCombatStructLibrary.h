#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include  "Library/PHItemEnumLibrary.h"
#include  "Library/PHItemStructLibrary.h"
#include "Library/PHDamageTypeUtils.h"
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite) FDamageRange PhysicalDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FDamageRange FireDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FDamageRange IceDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FDamageRange LightningDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FDamageRange LightDamage;
	UPROPERTY(EditAnywhere, BlueprintReadWrite) FDamageRange CorruptionDamage;

	/* -------- helpers on FDamageRange -------- */
	static FORCEINLINE FDamageRange AddRanges(const FDamageRange& A, const FDamageRange& B)
	{
		return { A.Min + B.Min, A.Max + B.Max };
	}

	static FORCEINLINE FDamageRange ScaleRange(const FDamageRange& R, float S)
	{
		return { R.Min * S, R.Max * S };
	}

	/* -------- compound ops on THIS struct -------- */
	FORCEINLINE FDamageByType& operator+=(const FDamageByType& Other)
	{
		PhysicalDamage   = AddRanges(PhysicalDamage,   Other.PhysicalDamage);
		FireDamage       = AddRanges(FireDamage,       Other.FireDamage);
		IceDamage        = AddRanges(IceDamage,        Other.IceDamage);
		LightningDamage  = AddRanges(LightningDamage,  Other.LightningDamage);
		LightDamage      = AddRanges(LightDamage,      Other.LightDamage);
		CorruptionDamage = AddRanges(CorruptionDamage, Other.CorruptionDamage);
		return *this;
	}

	FORCEINLINE FDamageByType& operator*=(float Scale)
	{
		PhysicalDamage   = ScaleRange(PhysicalDamage,   Scale);
		FireDamage       = ScaleRange(FireDamage,       Scale);
		IceDamage        = ScaleRange(IceDamage,        Scale);
		LightningDamage  = ScaleRange(LightningDamage,  Scale);
		LightDamage      = ScaleRange(LightDamage,      Scale);
		CorruptionDamage = ScaleRange(CorruptionDamage, Scale);
		return *this;
	}

	/* -------- non-compound ops -------- */
	friend FORCEINLINE FDamageByType operator+(FDamageByType A, const FDamageByType& B)
	{
		A += B;
		return A;
	}
	friend FORCEINLINE FDamageByType operator*(FDamageByType A, float Scale)
	{
		A *= Scale;
		return A;
	}
	friend FORCEINLINE FDamageByType operator*(float Scale, FDamageByType A)
	{
		A *= Scale;
		return A;
	}

	/* -------- queries / rolls -------- */
	FORCEINLINE float GetTotalDamageByType(EDamageTypes Type) const
	{
		const auto Avg = [](const FDamageRange& R){ return (R.Min + R.Max) * 0.5f; };
		switch (Type)
		{
			case EDamageTypes::DT_Physical:   return Avg(PhysicalDamage);
			case EDamageTypes::DT_Fire:       return Avg(FireDamage);
			case EDamageTypes::DT_Ice:        return Avg(IceDamage);
			case EDamageTypes::DT_Lightning:  return Avg(LightningDamage);
			case EDamageTypes::DT_Light:      return Avg(LightDamage);
			case EDamageTypes::DT_Corruption:    return Avg(CorruptionDamage); // rename it if you standardized to Corruption
			default:                          return 0.f;
		}
	}

	FORCEINLINE float RollDamageByType(EDamageTypes Type) const
	{
		switch (Type)
		{
			case EDamageTypes::DT_Physical:   return FMath::FRandRange(PhysicalDamage.Min,   PhysicalDamage.Max);
			case EDamageTypes::DT_Fire:       return FMath::FRandRange(FireDamage.Min,       FireDamage.Max);
			case EDamageTypes::DT_Ice:        return FMath::FRandRange(IceDamage.Min,        IceDamage.Max);
			case EDamageTypes::DT_Lightning:  return FMath::FRandRange(LightningDamage.Min,  LightningDamage.Max);
			case EDamageTypes::DT_Light:      return FMath::FRandRange(LightDamage.Min,      LightDamage.Max);
			case EDamageTypes::DT_Corruption:    return FMath::FRandRange(CorruptionDamage.Min, CorruptionDamage.Max);
			default:                          return 0.f;
		}
	}

	FORCEINLINE FDamageByType RollAllTypes() const
	{
		FDamageByType R;
		// If a “roll” should return a *point* damage, mirroring to Min/Max is fine:
		R.PhysicalDamage   = { RollDamageByType(EDamageTypes::DT_Physical),   RollDamageByType(EDamageTypes::DT_Physical) };
		R.FireDamage       = { RollDamageByType(EDamageTypes::DT_Fire),       RollDamageByType(EDamageTypes::DT_Fire) };
		R.IceDamage        = { RollDamageByType(EDamageTypes::DT_Ice),        RollDamageByType(EDamageTypes::DT_Ice) };
		R.LightningDamage  = { RollDamageByType(EDamageTypes::DT_Lightning),  RollDamageByType(EDamageTypes::DT_Lightning) };
		R.LightDamage      = { RollDamageByType(EDamageTypes::DT_Light),      RollDamageByType(EDamageTypes::DT_Light) };
		R.CorruptionDamage = { RollDamageByType(EDamageTypes::DT_Corruption),    RollDamageByType(EDamageTypes::DT_Corruption) };
		return R;
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

                        FString TypeName = DamageTypeToString(Type);
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

