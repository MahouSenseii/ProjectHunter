// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Generation/Library/Structs/GenerationValidationStructs.h"
#include "PHBiomeModuleSet.generated.h"

class UStaticMesh;

/** One authored mapping from a logical piece to a real asset. */
USTRUCT(BlueprintType)
struct ALS_PROJECTHUNTER_API FPHModuleEntry
{
	GENERATED_BODY()

	/** Soft so validating a set never loads its meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/**
	 * Interchangeable alternatives to Mesh. One logical piece with several meshes is what stops a
	 * floor reading as the same crate repeated; the construction stage picks among them
	 * deterministically. Empty means Mesh is the only option.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
	TArray<TSoftObjectPtr<UStaticMesh>> Variants;

	/**
	 * Span claimed on the layout grid, in Unreal units — not the raw mesh bounds. A wall is 400
	 * long and claims one module of depth even though its mesh is 20 thick.
	 *
	 * `Piece.*` entries must be whole modules of the set's GridSize, since they tile. `Prop.*`
	 * entries need only be positive; their footprint feeds spacing and clearance, not tiling.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module", meta = (ClampMin = "0.0"))
	FVector2D Footprint = FVector2D(100.0, 100.0);

	/** Height in units, for wall and pillar pieces. Zero means the piece is flat. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module", meta = (ClampMin = "0.0"))
	double Height = 0.0;

	/** Yaw applied when placing, for kits whose pieces are not authored facing +X. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
	double YawOffset = 0.0;
};

/**
 * Resolves logical construction pieces to one art pack's assets.
 *
 * This is the seam that keeps generation independent of any particular kit: generators and
 * construction code request `Piece.*` tags, and only this asset knows an `SM_*` name exists.
 * Swapping to a different pack is an authoring change, not a code change (GAME_DESIGN §11).
 */
UCLASS(BlueprintType)
class ALS_PROJECTHUNTER_API UPHBiomeModuleSet : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Identifies the biome or kit this set represents, for selection by data. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome")
	FGameplayTag BiomeTag;

	/**
	 * Module size every piece in this kit is authored against. BlockingStarterPack is 100.
	 * A layout built with this set must use a matching FPHLayoutRequest::GridSize.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome", meta = (ClampMin = "1.0"))
	double GridSize = 100.0;

	/**
	 * Keys must be registered descendants of `Piece` or `Prop`. Both live here because they differ
	 * in when they are placed, not in how an art pack resolves them.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Biome")
	TMap<FGameplayTag, FPHModuleEntry> Modules;

	/**
	 * Exact match first, then the nearest authored ancestor, so a kit may map `Piece.Wall` once
	 * instead of every wall variant. Returns false when nothing in the chain is authored.
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation|Biome")
	bool ResolvePiece(FGameplayTag PieceTag, FPHModuleEntry& OutEntry) const;

	/**
	 * Structural check of the authored data: valid piece tags, assigned meshes, and footprints
	 * that are whole modules. Replaces OutIssues. Does not judge which pieces a consumer needs.
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation|Biome")
	bool ValidateModuleSet(TArray<FPHGenerationIssue>& OutIssues) const;

	/**
	 * Reports which of the requested pieces this set cannot resolve. The consumer states its own
	 * needs, because a dungeon, a trap course, and a town do not need the same kit.
	 */
	UFUNCTION(BlueprintCallable, Category = "Generation|Biome")
	bool HasPieces(const TArray<FGameplayTag>& RequiredPieces, TArray<FGameplayTag>& OutMissing) const;
};
