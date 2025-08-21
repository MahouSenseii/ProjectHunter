#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "AttributeStructsLibrary.generated.h"


USTRUCT()
struct FEffectProperties
{
	GENERATED_BODY()


	FEffectProperties(){}

	FGameplayEffectContextHandle EffectContextHandle;

	UPROPERTY() UAbilitySystemComponent* SourceAsc = nullptr;
	UPROPERTY() AActor* SourceAvatarActor = nullptr;
	UPROPERTY() AController* SourceController = nullptr;
	UPROPERTY() ACharacter* SourceCharacter = nullptr;

	UPROPERTY() UAbilitySystemComponent* TargetASC = nullptr;
	UPROPERTY() AActor* TargetAvatarActor = nullptr;
	UPROPERTY() AController* TargetController = nullptr;
	UPROPERTY() ACharacter* TargetCharacter = nullptr;
};

USTRUCT(BlueprintType)
struct FPHAttributeInfo
{
	GENERATED_BODY()

	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag AttributeTag = FGameplayTag();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText AttributeName = FText();

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText AttributeDescription = FText();

	UPROPERTY(BlueprintReadOnly)
	float AttributeValue = 0.0f;
};

USTRUCT(BlueprintType)
struct FInitialPrimaryAttributes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Strength = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Intelligence = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Dexterity = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Endurance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Affliction = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Luck = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Covenant = 0.f;
};

USTRUCT(BlueprintType)
struct FInitialSecondaryAttributes
{
	GENERATED_BODY()

	// === Regen === //
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen")
	float HealthRegenRate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen")
	float ManaRegenRate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen")
	float StaminaRegenRate = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen")
	float ArcaneShieldRegenRate = 0.f;

	// === Regen Amounts === //
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen")
	float HealthRegenAmount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen")
	float ManaRegenAmount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen")
	float StaminaRegenAmount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Regen")
	float ArcaneShieldRegenAmount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Degen")
	float StaminaDegenAmount = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Degen")
	float StaminaDegenRate = 0.f; 

	// === Reserved (Flat) === //
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reserved")
	float FlatReservedHealth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reserved")
	float FlatReservedMana = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reserved")
	float FlatReservedStamina = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reserved")
	float FlatReservedArcaneShield = 0.f;

	// === Reserved (Percentage) === //
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reserved")
	float PercentageReservedHealth = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reserved")
	float PercentageReservedMana = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reserved")
	float PercentageReservedStamina = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reserved")
	float PercentageReservedArcaneShield = 0.f;

	// === Resistances (Flat) === //
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistances")
	float FireResistanceFlat = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistances")
	float IceResistanceFlat = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistances")
	float LightningResistanceFlat = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistances")
	float LightResistanceFlat = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistances")
	float CorruptionResistanceFlat = 0.f;

	// === Resistances (Percent) === //
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistances")
	float FireResistancePercent = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistances")
	float IceResistancePercent = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistances")
	float LightningResistancePercent = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistances")
	float LightResistancePercent = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resistances")
	float CorruptionResistancePercent = 0.f;

	// === Utility === //
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility")
	float Gems = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility")
	float LifeLeech = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility")
	float ManaLeech = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility")
	float MovementSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility")
	float CritChance = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility")
	float CritMultiplier = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility")
	float Poise = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Utility")
	float StunRecovery = 0.f;
};

USTRUCT(BlueprintType)
struct FInitialVitalAttributes
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vital")
	float Health = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vital")
	float Stamina = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Vital")
	float Mana = 100.f;
};