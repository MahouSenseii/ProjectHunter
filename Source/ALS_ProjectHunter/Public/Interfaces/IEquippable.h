#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Library/PHItemStructLibrary.h"
#include "IEquippable.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UEquippable : public UInterface
{
	GENERATED_BODY()
};

class ALS_PROJECTHUNTER_API IEquippable
{
	GENERATED_BODY()

public:
	// === Core Equippable Functions ===
    
	/** Called when the item is equipped */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equippable")
	void OnEquipped(AActor* NewOwner);
    
	/** Called when item is unequipped */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equippable")
	void OnUnequipped();
    
	/** Check if item can be equipped by the actor */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equippable")
	bool CanBeEquippedBy(AActor* Actor) const;
    
	/** Get the socket name for attachment */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equippable")
	FName GetAttachmentSocket() const;
    
	/** Get the item information */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equippable")
	 UItemDefinitionAsset* GetEquippableItemInfo() const;
    
	/** Apply item stats to owner */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equippable")
	void ApplyStatsToOwner(AActor* Owner);
    
	/** Remove item stats from owner */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equippable")
	void RemoveStatsFromOwner(AActor* Owner);
    
	/** Get equipped location type */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Equippable")
	EEquipmentSlot GetEquipmentSlot() const;
};