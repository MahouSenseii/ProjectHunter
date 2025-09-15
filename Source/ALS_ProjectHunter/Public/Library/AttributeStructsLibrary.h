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
struct FAttributeInitConfig
{
	GENERATED_BODY()

	// The gameplay tag that identifies this attribute
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	FGameplayTag AttributeTag;
	
	// Default value for this attribute
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Attribute")
	float DefaultValue = 0.0f;
	
	// Minimum allowed value (for clamping)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	float MinValue = -9999.0f;
	
	// Maximum allowed value (for clamping)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	float MaxValue = 9999.0f;
	
	// Display name for debugging/logging
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	FString DisplayName;
	
	// Optional description
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Display")
	FString Description;

	// Whether this attribute should be validated/applied
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Validation")
	bool bEnabled = true;
};

