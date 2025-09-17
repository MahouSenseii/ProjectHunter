// ItemInstanceObject.h
#pragma once

#include "CoreMinimal.h"
#include "Library/PHItemStructLibrary.h"      // for FItemInformation, FItemBase, FEquippableItemData, etc.
#include "UObject/NoExportTypes.h"
#include "ItemInstance.generated.h"

class UItemDefinitionAsset;   // forward declare the asset
struct FItemInstance;         // forward declare the runtime struct if declared elsewhere

UCLASS(BlueprintType, Blueprintable)
class ALS_PROJECTHUNTER_API UItemInstanceObject : public UObject
{
	GENERATED_BODY()

public:
	// Public API stays stable: the rest of your code can keep reading this “view”
	UFUNCTION(BlueprintCallable, BlueprintPure)
	const FItemInformation& GetItemInfo() const { return ItemInfoView; }

	UFUNCTION(BlueprintCallable)
	void RebuildItemInfoView();

public:
	// Static/base definition (Data Asset)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Definition")
	TSoftObjectPtr<UItemDefinitionAsset> BaseDef;

	// Runtime rolled data (keep VisibleAnywhere so designers don’t hand-edit)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Runtime")
	FItemInstance Instance;

private:
	// Back-compat cache: what the rest of your systems already expect to read
	UPROPERTY(Transient)
	FItemInformation ItemInfoView;
};
