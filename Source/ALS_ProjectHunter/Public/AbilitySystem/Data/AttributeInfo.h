// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeConfigDataAsset.h"
#include "Engine/DataAsset.h"
#include "Library/AttributeStructsLibrary.h"

#include "AttributeInfo.generated.h"

/**
 *  Will hold basic information for Attribute.
 */
UCLASS()
class ALS_PROJECTHUNTER_API UAttributeInfo : public UDataAsset
{
	GENERATED_BODY()

public:

	FPHAttributeInfo FindAttributeInfoForTag(const FGameplayTag& AttributeTag, bool bLogNotFound = false ) const;

	/** Reference to AttributeConfigDataAsset to populate from */
	UPROPERTY(EditAnywhere, Category = "Editor")
	TObjectPtr<UAttributeConfigDataAsset> SourceConfig;
	

	/** Auto-populate from AttributeConfigDataAsset */
	UFUNCTION(CallInEditor, Category = "Editor")
	void PopulateFromConfig();

	/** Clear all attribute information */
	UFUNCTION(CallInEditor, Category = "Editor")
	void ClearAllAttributes();
	

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FPHAttributeInfo> AttributeInformation;
	
};
