#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Loot/Library/Structs/LootStructs.h"
#include "LootComponent.generated.h"

class ULootSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogLootComponent, Log, All);

UCLASS(ClassGroup = (Loot), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API ULootComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULootComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config")
	FName SourceID;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config")
	FLootSpawnSettings DefaultSpawnSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config")
	bool bUseOverrideSettings = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config", meta = (EditCondition = "bUseOverrideSettings"))
	FLootDropSettings OverrideSettings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot|Config", meta = (ClampMin = "0", ClampMax = "100"))
	int32 LevelOverride = 0;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
	FLootResultBatch DropLoot(float PlayerLuck = 0.0f, float PlayerMagicFind = 0.0f);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
	FLootResultBatch DropLootAtLocation(
		FVector Location,
		float PlayerLuck = 0.0f,
		float PlayerMagicFind = 0.0f);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
	FLootResultBatch GenerateLoot(float PlayerLuck = 0.0f, float PlayerMagicFind = 0.0f);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Loot")
	void SpawnLoot(const FLootResultBatch& Results, FVector Location = FVector::ZeroVector);

	UFUNCTION(BlueprintPure, Category = "Loot")
	bool IsSourceValid() const;

	UFUNCTION(BlueprintPure, Category = "Loot")
	bool GetSourceEntry(FLootSourceEntry& OutEntry) const;

	UFUNCTION(BlueprintPure, Category = "Loot")
	ULootSubsystem* GetLootSubsystem() const;

protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	mutable ULootSubsystem* CachedLootSubsystem = nullptr;

	FLootRequest BuildRequest(float PlayerLuck, float PlayerMagicFind) const;
	bool EnsureSubsystem() const;
	bool HasLootAuthority(const TCHAR* FunctionName) const;
};
