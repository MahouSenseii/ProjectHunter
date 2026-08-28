#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PrimaryAttributeRules.generated.h"

class UHunterAttributeSet;

/** Gameplay bonuses contributed by ProjectHunter's seven primary attributes. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHPrimaryAttributeBonuses
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Primary")
	float PhysicalDamagePercent = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Primary")
	float ElementalDamagePercent = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Primary")
	float AttackCastSpeedPercent = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Primary")
	float CriticalDamageBonusPercent = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Primary")
	float AllResistancePoints = 0.f;

	/** Multiplier applied to stamina drain. 1.0 is neutral. */
	UPROPERTY(BlueprintReadOnly, Category = "Stats|Primary")
	float StaminaDegenMultiplier = 1.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Primary")
	float DamageOverTimePercent = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Primary")
	float AilmentDurationBonusSeconds = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Primary")
	float AilmentChanceBonusPercent = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Primary")
	float MinionDamagePercent = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Stats|Primary")
	float MinionHealthPercent = 0.f;
};

/** Stateless source of truth for primary-attribute gameplay scaling. */
class ALS_PROJECTHUNTER_API FPrimaryAttributeRules
{
public:
	static FPHPrimaryAttributeBonuses Resolve(
		float Strength,
		float Intelligence,
		float Dexterity,
		float Endurance,
		float Affliction,
		float Luck,
		float Covenant);

	static FPHPrimaryAttributeBonuses Resolve(const UHunterAttributeSet* AttributeSet);
};

/** Blueprint access for summon owners, UI, and other data consumers. */
UCLASS()
class ALS_PROJECTHUNTER_API UPHPrimaryAttributeFunctionLibrary final : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "ProjectHunter|Stats|Primary")
	static FPHPrimaryAttributeBonuses ResolvePrimaryAttributeBonuses(
		const UHunterAttributeSet* AttributeSet);
};
