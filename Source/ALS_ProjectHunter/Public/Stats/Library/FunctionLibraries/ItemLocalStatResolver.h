#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Stats/Library/Structs/ResolvedItemStats.h"
#include "ItemLocalStatResolver.generated.h"

class UItemInstance;
struct FPHItemStats;

/** Stateless local-item folding used by equipment, combat, tooltips, and skills. */
class ALS_PROJECTHUNTER_API FItemLocalStatResolver
{
public:
	static FResolvedWeaponStats ResolveWeapon(
		const FBaseWeaponStats& BaseStats,
		const FPHItemStats& ItemStats);

	static FResolvedArmorStats ResolveArmor(
		const FBaseArmorStats& BaseStats,
		const FPHItemStats& ItemStats);

	static bool ResolveWeapon(const UItemInstance* Item, FResolvedWeaponStats& OutStats);
	static bool ResolveArmor(const UItemInstance* Item, FResolvedArmorStats& OutStats);
};

/** Blueprint access to the same resolved local snapshots used by combat. */
UCLASS()
class ALS_PROJECTHUNTER_API UItemLocalStatFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Item|Resolved Stats")
	static bool ResolveWeaponStats(const UItemInstance* Item, FResolvedWeaponStats& OutStats);

	UFUNCTION(BlueprintPure, Category = "Item|Resolved Stats")
	static bool ResolveArmorStats(const UItemInstance* Item, FResolvedArmorStats& OutStats);
};
