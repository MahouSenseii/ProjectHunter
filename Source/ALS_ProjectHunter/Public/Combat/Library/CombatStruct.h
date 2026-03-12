#pragma once
#include "CoreMinimal.h"
#include "CombatEnumLibrary.h"
#include "CombatStruct.generated.h"

USTRUCT(BlueprintType)
struct FCombatAffiliation
{
	GENERATED_BODY()

public:

	// The permanent faction identity of the actor
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	EFaction Faction = EFaction::Neutral;

	// The current alignment state toward a specific context
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Combat")
	ECombatAlignment Alignment = ECombatAlignment::None;
	
	bool operator==(const FCombatAffiliation& Other) const
	{
		return Faction == Other.Faction &&
			   Alignment == Other.Alignment;
	}

	bool operator!=(const FCombatAffiliation& Other) const
	{
		return !(*this == Other);
	}
};