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

UENUM(BlueprintType)
enum class EAttributeCategory : uint8
{
	Primary,
	SecondaryMax,
	SecondaryCurrent,
	VitalMax,
	VitalCurrent
};

USTRUCT(BlueprintType)
struct FAttributeInitConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Classification")
	EAttributeCategory Category = EAttributeCategory::Primary;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	FGameplayTag AttributeTag;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	float DefaultValue = 0.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	float MinValue = -9999.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	float MaxValue = 9999.0f;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	FString DisplayName;
    
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	FString Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	bool bEnabled = true;
};

