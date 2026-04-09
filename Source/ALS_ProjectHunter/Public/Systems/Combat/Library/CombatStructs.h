#pragma once
#include "CoreMinimal.h"
#include "Combat/Library/CombatEnumLibrary.h"
#include "CombatStructs.generated.h"

USTRUCT(BlueprintType)
struct FCombatAffiliation
{
	GENERATED_BODY()

public:

	// The permanent faction identity of the actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	EFaction Faction = EFaction::Neutral;

	// The current alignment state toward a specific context
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	ECombatAlignment Alignment = ECombatAlignment::None;
	
	bool operator==(const FCombatAffiliation& Other) const
	{
		return Faction == Other.Faction &&
			   Alignment == Other.Alignment;
	}

	bool operator!=(const FCombatAffiliation& Other) const
	{
		return !(*this == Other);
	}
};


UENUM(BlueprintType)
enum class EHunterDamageType : uint8
{
	Physical UMETA(DisplayName = "Physical"),
	Fire UMETA(DisplayName = "Fire"),
	Ice UMETA(DisplayName = "Ice"),
	Lightning UMETA(DisplayName = "Lightning"),
	Light UMETA(DisplayName = "Light"),
	Corruption UMETA(DisplayName = "Corruption")
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatDamagePacket
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Physical = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Fire = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Ice = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Lightning = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Light = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float Corruption = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bCrit = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float CritMultiplierApplied = 1.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float TotalPreMitigation = 0.f;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatResolveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	FCombatDamagePacket PreMitigationPacket;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float PhysicalTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float FireTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float IceTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float LightningTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float LightTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float CorruptionTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float PhysicalBlocked = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float FireBlocked = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float IceBlocked = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float LightningBlocked = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float LightBlocked = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float CorruptionBlocked = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float TotalDamageBeforeBlock = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float TotalDamageAfterBlock = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float TotalBlockedAmount = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float DamageToStamina = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float DamageToArcaneShield = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float DamageToHealth = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float TotalDamageTaken = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bKilledTarget = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bWasCrit = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bWasBlocked = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bGuardBroken = false;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FCombatHitContext
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	TObjectPtr<AActor> SourceActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	TObjectPtr<AActor> TargetActor = nullptr;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	bool bCanCrit = true;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Combat")
	float CritRollOverride = -1.f;
};
