#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "TagDebugStructs.generated.h"

class UTagManager;
class UObject;

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FTagDebugManager
{
	GENERATED_BODY()

	FTagDebugManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug")
	bool bEnableDebug;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Display")
	bool bDrawToScreen;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Display")
	bool bLogToOutput;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Display", meta = (ClampMin = "0"))
	int32 BaseMessageKey;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Display")
	bool bShowInactiveTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
	bool bShowLifeDeath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
	bool bShowThresholds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
	bool bShowMovement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
	bool bShowCombat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
	bool bShowAilments;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
	bool bShowImmunities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
	bool bShowEffects;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tags|Debug|Groups")
	bool bShowOther;

	void DrawDebug(UTagManager* TagManager, UObject* WorldContext);

private:
	void BuildDisplayLines(
		UTagManager* TagManager,
		const FGameplayTagContainer& OwnedTags,
		TArray<FString>& OutLines,
		TArray<FColor>& OutColors) const;

	bool CheckForTagChanges(UTagManager* TagManager);
	void ClearDrawnMessages();
	static FString GetShortTagName(const FGameplayTag& Tag);

	bool bCacheInitialized;
	FGameplayTagContainer CachedActiveTags;
	TArray<FString> CachedDisplayLines;
	TArray<FColor> CachedLineColors;
	int32 LastDrawnLineCount;
	uint64 LastScreenMessageKeyBase;
};
