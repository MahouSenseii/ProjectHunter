#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "PHCharacterStructLibrary.generated.h"

class UGameplayEffect;

USTRUCT(BlueprintType)
struct FPHLevelUpInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	int32 LevelUpRequirement = 0;

	UPROPERTY(EditDefaultsOnly)
	int32 AttributePointAward = 1;
	
};


UENUM(BlueprintType)
enum class EPassiveType : uint8
{
	PT_None UMETA(DisplayName = "None"),
	PT_StatBonus UMETA(DisplayName = "Stat Bonus"),
	PT_DamageOverTime UMETA(DisplayName = "Damage Over Time"),
	PT_Resistance UMETA(DisplayName = "Resistance Bonus"),
	PT_LifeLeech UMETA(DisplayName = "Life Leech"),
	PT_Utility UMETA(DisplayName = "Utility"),
	PT_Other UMETA(DisplayName = "Other")
};

UENUM(BlueprintType)
enum class EPassiveSource : uint8
{
	None           UMETA(DisplayName = "None"),
	PassiveTree    UMETA(DisplayName = "Passive Tree"),
	Title          UMETA(DisplayName = "Title"),
	Quest          UMETA(DisplayName = "Quest"),
};


USTRUCT(BlueprintType)
struct FPassiveEffectInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	FName PassiveID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	FText PassiveName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	EPassiveType PassiveType = EPassiveType::PT_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	EPassiveSource SourceType = EPassiveSource::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	TSubclassOf<UGameplayEffect> PassiveEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	FGameplayTagContainer ActivationTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	int32 RequiredLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	bool bUnlocked = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive")
	FText DisplayName;

	// Optional future: icon, description, UI category, etc.
};

USTRUCT(BlueprintType)
struct FPHCharacterPassive
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive Group")
	FName PassiveNodeID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive Group")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive Group")
	EPassiveSource SourceType = EPassiveSource::PassiveTree;

	/** All effects granted by this passive node */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Passive Group")
	TArray<FPassiveEffectInfo> PassiveEffects;
};

USTRUCT(BlueprintType)
struct FLevelUpResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	bool bLeveledUp = false;

	UPROPERTY(BlueprintReadOnly)
	int32 XPLeft = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 AttributePointsAwarded = 0;
};

USTRUCT(BlueprintType)
struct FInitialGameplayEffectInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayEffect> EffectClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefaultRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayAttribute RateAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DefaultAmount = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayAttribute AmountAttribute;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "Attributes.Secondary.Vital"))
	FGameplayTag SetByCallerTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Level = 1.0f;
};