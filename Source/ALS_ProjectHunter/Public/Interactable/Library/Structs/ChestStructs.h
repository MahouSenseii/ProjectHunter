// Interactable/Library/Structs/ChestStructs.h
#pragma once

#include "CoreMinimal.h"
#include "Loot/Library/LootStructs.h"
#include "ChestStructs.generated.h"

class UAnimSequence;
class UNiagaraSystem;
class USoundBase;
class USkeletalMesh;
class UStaticMesh;

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FChestVisualConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals")
	bool bUseStaticMesh = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals|Static Mesh",
		meta = (EditCondition = "bUseStaticMesh", EditConditionHides))
	TObjectPtr<UStaticMesh> ClosedMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals|Static Mesh",
		meta = (EditCondition = "bUseStaticMesh", EditConditionHides))
	TObjectPtr<UStaticMesh> OpenMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals|Skeletal Mesh",
		meta = (EditCondition = "!bUseStaticMesh", EditConditionHides))
	TObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visuals|Skeletal Mesh",
		meta = (EditCondition = "!bUseStaticMesh", EditConditionHides))
	TObjectPtr<UAnimSequence> OpenAnimation = nullptr;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FChestCollisionConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	bool bBlockPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	bool bBlockInteractable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	bool bBlockCamera = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Collision")
	bool bGenerateOverlapEvents = false;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FChestAnimationConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	bool bPlayOpenAnimation = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation",
		meta = (EditCondition = "bPlayOpenAnimation", ClampMin = "0.1"))
	float OpenAnimationDuration = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation",
		meta = (EditCondition = "bPlayOpenAnimation", ClampMin = "0.1", ClampMax = "5.0"))
	float AnimationPlayRate = 1.0f;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FChestFeedbackConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> OpenSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio")
	TObjectPtr<USoundBase> CloseSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VFX")
	TObjectPtr<UNiagaraSystem> OpenNiagaraEffect = nullptr;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FChestRespawnConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn")
	bool bCanRespawn = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn",
		meta = (EditCondition = "bCanRespawn", ClampMin = "0.0"))
	float RespawnTime = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn",
		meta = (EditCondition = "bCanRespawn"))
	bool bRerollLootOnRespawn = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn",
		meta = (EditCondition = "bCanRespawn"))
	bool bPlayCloseAnimationOnRespawn = true;
};

USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FChestSpawnConfig
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn", meta = (ClampMin = "0.0"))
	float ScatterRadius = 150.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	float SpawnHeightOffset = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawn")
	bool bRandomScatter = true;

	FLootSpawnSettings ToSpawnSettings(FVector BaseLocation) const
	{
		FLootSpawnSettings Settings;
		Settings.SpawnLocation = BaseLocation;
		Settings.ScatterRadius = ScatterRadius;
		Settings.HeightOffset = SpawnHeightOffset;
		Settings.bRandomScatter = bRandomScatter;
		return Settings;
	}
};
