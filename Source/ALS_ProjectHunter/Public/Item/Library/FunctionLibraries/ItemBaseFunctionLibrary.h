// Item/Library/FunctionLibraries/ItemBaseFunctionLibrary.h

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Library/Structs/ItemStructs.h"
#include "ItemBaseFunctionLibrary.generated.h"

class AActor;

UCLASS()
class ALS_PROJECTHUNTER_API UItemBaseFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Item|Base")
	static bool IsItemBaseValid(const FItemBase& ItemBase);

	UFUNCTION(BlueprintPure, Category = "Item|Base")
	static bool IsItemBaseValidForInventory(const FItemBase& ItemBase);

	UFUNCTION(BlueprintPure, Category = "Item|Base")
	static bool IsItemBaseWeapon(const FItemBase& ItemBase);

	UFUNCTION(BlueprintPure, Category = "Item|Base")
	static bool IsItemBaseArmor(const FItemBase& ItemBase);

	UFUNCTION(BlueprintPure, Category = "Item|Base")
	static bool IsItemBaseAccessory(const FItemBase& ItemBase);

	UFUNCTION(BlueprintPure, Category = "Item|Base")
	static bool IsItemBaseEquippable(const FItemBase& ItemBase);

	UFUNCTION(BlueprintPure, Category = "Item|Base")
	static bool IsItemBaseConsumable(const FItemBase& ItemBase);

	UFUNCTION(BlueprintPure, Category = "Item|Base")
	static bool IsItemBaseMaterial(const FItemBase& ItemBase);

	UFUNCTION(BlueprintPure, Category = "Item|Base")
	static bool IsItemBaseCurrency(const FItemBase& ItemBase);

	UFUNCTION(BlueprintPure, Category = "Item|Base")
	static bool DoesItemBaseUseRuntimeActor(const FItemBase& ItemBase);

	UFUNCTION(BlueprintPure, Category = "Item|Base")
	static TSubclassOf<AActor> GetItemBaseRuntimeActorClass(const FItemBase& ItemBase);

	UFUNCTION(BlueprintPure, Category = "Item|Base")
	static FName GetItemBaseSocketForContext(const FItemBase& ItemBase, FName Context);

	UFUNCTION(BlueprintPure, Category = "Item|Economy")
	static float GetItemBaseCalculatedValue(
		const FItemBase& ItemBase,
		int32 Quantity = 1,
		EItemRarity InstanceRarity = EItemRarity::IR_None);

	UFUNCTION(BlueprintPure, Category = "Item|Weight")
	static float GetItemBaseTotalWeight(const FItemBase& ItemBase, int32 Quantity = 1);
};
