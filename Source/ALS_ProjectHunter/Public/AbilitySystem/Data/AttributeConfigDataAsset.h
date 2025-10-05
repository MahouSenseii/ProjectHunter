// AttributeConfigDataAsset.h
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Library/AttributeStructsLibrary.h"
#include "AttributeConfigDataAsset.generated.h"
 

/**
 * Data Asset that contains all attribute initialization configuration
 * Create ONE instance of this asset and configure all your character's attributes in it
 */

UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UAttributeConfigDataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    UAttributeConfigDataAsset();
 

    // All primary attributes (Strength, Intelligence, etc.)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Primary Attributes")
    TArray<FAttributeInitConfig> Attributes;
    
  

    // Helper function to find a specific attribute config
    UFUNCTION(BlueprintCallable, Category = "Attribute Config")
    FAttributeInitConfig FindAttributeConfig(FGameplayTag AttributeTag, bool& bFound) const;

    // Validation function to check if all required tags are present
    UFUNCTION(BlueprintCallable, Category = "Validation")
    bool ValidateConfiguration(TArray<FString>& OutErrors) const;

#if WITH_EDITOR
    // Editor-only function to auto-populate with default values
    UFUNCTION(CallInEditor, Category = "Editor")
    void AutoPopulateFromGameplayTags();
#endif
};