// Item/Library/Structs/ItemRuneStructs.h
#pragma once

#include "CoreMinimal.h"
#include "ItemRuneStructs.generated.h"

USTRUCT(BlueprintType)
struct FRuneSocket
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Rune")
	bool bIsSocketed = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Rune")
	FName RuneID = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Rune")
	int32 RuneLevel = 0;

	FRuneSocket() = default;
};

USTRUCT(BlueprintType)
struct FRuneCraftingData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Rune")
	TArray<FRuneSocket> RuneSockets;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Rune")
	int32 EnhancementLevel = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, SaveGame, Category = "Rune")
	int32 MaxEnhancementLevel = 15;

	FRuneCraftingData() = default;

	int32 GetSocketCount() const { return RuneSockets.Num(); }
	int32 GetSocketedRuneCount() const
	{
		return RuneSockets.FilterByPredicate([](const FRuneSocket& Socket) {
			return Socket.bIsSocketed;
		}).Num();
	}
};
