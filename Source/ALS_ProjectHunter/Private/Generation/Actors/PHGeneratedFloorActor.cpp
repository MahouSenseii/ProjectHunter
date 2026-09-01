// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "Generation/Actors/PHGeneratedFloorActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Generation/Generators/PHDungeonGenerator.h"
#include "Generation/Library/FunctionLibraries/PHBlockoutPlanLibrary.h"
#include "AI/Mob/MobManagerActor.h"
#include "Components/BoxComponent.h"
#include "Generation/Library/FunctionLibraries/PHDecorationPlanLibrary.h"
#include "Generation/Library/FunctionLibraries/PHEncounterPlanLibrary.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "AI/Navigation/NavigationBounds.h"
#include "Generation/PHGenerationTags.h"
#include "Tower/Library/FunctionLibraries/RunSeedFunctionLibrary.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Interactable/Actors/Portal/PortalActor.h"
#include "Interactable/Actors/LootChest/LootChest.h"
#include "Tower/Subsystems/RunSubsystem.h"
#include "TimerManager.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY_STATIC(LogPHGeneratedFloor, Log, All);

APHGeneratedFloorActor::APHGeneratedFloorActor()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Static);

	// The blockout kit tiles at 400, so rooms must be whole 400 units to fit it.
	Request.GridSize = 400.0;
	Request.MinRegionSize = FVector2D(800.0, 800.0);
	Request.MaxRegionSize = FVector2D(2000.0, 2000.0);
	Request.RegionSpacing = 800.0;
	Request.AreaSize = FVector2D(12000.0, 12000.0);

	// A floor whose room count is fixed reads as the same building every run even when the rooms
	// move. The range is wide enough that a short floor and a long one are visibly different.
	Request.MinRegionCount = 7;
	Request.MaxRegionCount = 14;
	Request.Seed = 1;
}

void APHGeneratedFloorActor::BeginPlay()
{
	Super::BeginPlay();

	// Delegates do not survive a level save and load, so a portal built in the editor has to be
	// re-bound here or the exit would look right and do nothing.
	if (ExitPortal)
	{
		ExitPortal->OnPortalActivated.AddUniqueDynamic(
			this, &APHGeneratedFloorActor::HandleExitPortalActivated);
	}

	if (bFollowRunSubsystem)
	{
		if (const UGameInstance* GameInstance = GetGameInstance())
		{
			if (URunSubsystem* Run = GameInstance->GetSubsystem<URunSubsystem>())
			{
				Run->OnFloorGenerationRequested.AddUniqueDynamic(
					this, &APHGeneratedFloorActor::HandleFloorGenerationRequested);
				Run->OnFloorRewardReady.AddUniqueDynamic(
					this, &APHGeneratedFloorActor::HandleFloorRewardReady);

				// A GameInstance run may request this floor before its level actor exists.
				if (HasAuthority() && Run->IsRunActive() && Run->GetFloorPhase() == EFloorPhase::Generating)
				{
					HandleFloorGenerationRequested(Run->GetCurrentFloorData());
				}
			}
		}
	}

	// The pawn does not exist yet on this frame. Deferring one tick is what makes the entry pose
	// win over whichever PlayerStart the game mode happened to choose.
	if (HasAuthority() && LastRegionCount > 0)
	{
		MovePlayersTimer = GetWorldTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &APHGeneratedFloorActor::MovePlayersToStart));
	}
}

void APHGeneratedFloorActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ExitTravelTimer);
	GetWorldTimerManager().ClearTimer(MovePlayersTimer);
	if (ExitPortal)
	{
		ExitPortal->OnPortalActivated.RemoveDynamic(
			this, &APHGeneratedFloorActor::HandleExitPortalActivated);
	}

	if (const UGameInstance* GameInstance = GetGameInstance())
	{
		if (URunSubsystem* Run = GameInstance->GetSubsystem<URunSubsystem>())
		{
			Run->OnFloorGenerationRequested.RemoveDynamic(
				this, &APHGeneratedFloorActor::HandleFloorGenerationRequested);
			Run->OnFloorRewardReady.RemoveDynamic(
				this, &APHGeneratedFloorActor::HandleFloorRewardReady);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void APHGeneratedFloorActor::Destroyed()
{
	ClearBuilt();
	Super::Destroyed();
}

UInstancedStaticMeshComponent* APHGeneratedFloorActor::FindOrAddComponentFor(UStaticMesh* Mesh,
	TMap<UStaticMesh*, UInstancedStaticMeshComponent*>& Cache)
{
	if (UInstancedStaticMeshComponent** Existing = Cache.Find(Mesh))
	{
		return *Existing;
	}

	UInstancedStaticMeshComponent* Component = NewObject<UInstancedStaticMeshComponent>(this);
	Component->SetStaticMesh(Mesh);
	// Older saved floor actors have movable roots. A static child cannot attach to those;
	// matching the root preserves the floor transform without rewriting the authored actor.
	Component->SetMobility(RootComponent->Mobility);
	Component->SetupAttachment(RootComponent);
	Component->RegisterComponent();
	AddInstanceComponent(Component);

	BuiltComponents.Add(Component);
	Cache.Add(Mesh, Component);
	return Component;
}

void APHGeneratedFloorActor::ClearBuilt()
{
	GetWorldTimerManager().ClearTimer(ExitTravelTimer);
	GetWorldTimerManager().ClearTimer(MovePlayersTimer);
	if (IsValid(ExitPortal))
	{
		ExitPortal->OnPortalActivated.RemoveDynamic(this, &APHGeneratedFloorActor::HandleExitPortalActivated);
	}

	for (AActor* Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			// Destroying a manager alone only stops its timers. Its live mobs belong to its
			// lifecycle and must be released before their floor and collision disappear.
			if (AMobManagerActor* Manager = Cast<AMobManagerActor>(Actor))
			{
				Manager->StopAndClear();
			}
			Actor->Destroy();
		}
	}
	SpawnedActors.Reset();
	ExitPortal = nullptr;

	for (UInstancedStaticMeshComponent* Component : BuiltComponents)
	{
		if (IsValid(Component))
		{
			Component->DestroyComponent();
		}
	}
	BuiltComponents.Reset();

	LastEncounterCount = 0;
	LastEnemyBudget = 0;
	LastChestCount = 0;
	LastRegionCount = 0;
	LastPlayerStart = FTransform::Identity;
	LastExit = FTransform::Identity;
	bLastBuildHadNavigation = false;
	LastResult = TEXT("Cleared.");
}

void APHGeneratedFloorActor::GenerateNextSeed()
{
	// Hashed rather than incremented. FRandomStream advances linearly from its seed, so literal
	// seeds 1, 2, 3 draw first values a fixed step apart: stepping by one produced a staircase of
	// room counts (9, 9, 9, 10, 10, 10, ...) and made a range look like a constant. A run never
	// hits this because its layout seeds come from DeriveLayoutSeed; only stepping by hand did.
	GenerateForSeed(URunSeedFunctionLibrary::DeriveSeed(Request.Seed, FName(TEXT("NextLayout")), 0));
}

void APHGeneratedFloorActor::GenerateForSeed(const int32 LayoutSeed)
{
	TryGenerateForSeed(LayoutSeed);
}

bool APHGeneratedFloorActor::TryGenerateForSeed(const int32 LayoutSeed)
{
	if (bGenerating) { return false; }
	Request.Seed = LayoutSeed;
	if (!TryGenerate())
	{
		return false;
	}
	MovePlayersToStart();
	return true;
}

void APHGeneratedFloorActor::TravelToNextFloor()
{
	const UGameInstance* GameInstance = GetGameInstance();
	URunSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<URunSubsystem>() : nullptr;

	if (Run && Run->IsRunActive())
	{
		if (!bFollowRunSubsystem || !HasAuthority())
		{
			UE_LOG(LogPHGeneratedFloor, Warning,
				TEXT("Exit refused: this floor is not the authoritative listener for the active run."));
			return;
		}
		// The run owner decides whether the floor is finished. Completing the objective from here
		// to get the portal working would defeat the gate rather than satisfy it.
		if (!Run->CanAdvanceFloor())
		{
			UE_LOG(LogPHGeneratedFloor, Log,
				TEXT("Exit refused: the run owner reports floor %d is not ready to advance."),
				Run->GetCurrentFloor());
			return;
		}

		// Advancing comes back as OnFloorGenerationRequested, which is what rebuilds the floor.
		Run->AdvanceFloor();
		return;
	}

	if (!bRegenerateOnExit)
	{
		return;
	}

	GenerateNextSeed();
}

void APHGeneratedFloorActor::HandleMobDied(APHBaseCharacter* Mob)
{
	// The handler adapts the delegate's signature; the operation lives below.
	ReportMobDeathToRun();
}

void APHGeneratedFloorActor::ReportMobDeathToRun()
{
	if (!bReportKillsToRun)
	{
		return;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	URunSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<URunSubsystem>() : nullptr;
	if (!Run || !Run->IsRunActive())
	{
		return;
	}

	// One death, one owner operation. RegisterKill already advances the objective
	// when the floor is counting kills, so calling AddObjectiveProgress here as
	// well made a single death worth two points and let a clear-all floor finish
	// on half its enemies.
	//
	// Whether a kill counts toward the objective is the run owner's decision, not
	// this actor's: a floor whose objective is not ClearAllEnemies or KillBoss
	// must not gain progress from a kill, and RegisterKill is what knows that.
	Run->RegisterKill();
}

void APHGeneratedFloorActor::HandleFloorRewardReady(FRunFloorData FloorData)
{
	SetExitOpen(true);
}

void APHGeneratedFloorActor::SetExitOpen(const bool bOpen)
{
	if (ExitPortal)
	{
		ExitPortal->SetPortalActive(bOpen);
	}
}

void APHGeneratedFloorActor::HandleExitPortalActivated(APortalActor* Portal, APawn* Traveller)
{
	if (!HasAuthority() || !IsValid(Portal) || Portal != ExitPortal ||
		GetWorldTimerManager().IsTimerActive(ExitTravelTimer))
	{
		return;
	}
	// Deferred a tick on purpose. This runs inside the portal's own OnPortalActivated broadcast,
	// and rebuilding the floor clears everything the last build spawned - the portal included.
	// Destroying it here would leave APortalActor::ExecuteTravel finishing on a dead actor.
	ExitTravelTimer = GetWorldTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateUObject(this, &APHGeneratedFloorActor::TravelToNextFloor));
}

void APHGeneratedFloorActor::HandleFloorGenerationRequested(const FRunFloorData FloorData)
{
	// The run owner hands out a floor seed; the layout stream is its own branch off that, so
	// changing how floors are laid out cannot disturb encounter, reward or loot draws.
	if (!HasAuthority() || !bFollowRunSubsystem)
	{
		return;
	}

	bool bBuilt = TryGenerateForSeed(URunSeedFunctionLibrary::DeriveLayoutSeed(FloorData.FloorSeed));

	// A refusal is legitimate - a required corridor may not fit the frontage two
	// rooms share - but it used to end this handler silently, and the run then sat
	// in Generating forever because nothing else advances that phase. Retry on
	// further layouts derived from the same floor seed, so the recovery is bounded
	// and a seed still replays to the same floor.
	for (int32 Attempt = 1; !bBuilt && Attempt <= MaxLayoutRetries; ++Attempt)
	{
		bBuilt = TryGenerateForSeed(URunSeedFunctionLibrary::DeriveSeed(
			FloorData.FloorSeed, FName(TEXT("LayoutRetry")), Attempt));
	}

	if (!bBuilt)
	{
		// Warning, not Error, and on this actor's own category: an impossible
		// *request* - MinRegionCount above MaxRegionCount, say - can never build at
		// any seed, and that is a configuration mistake the existing
		// FailedRunRequestRemainsGenerating case deliberately exercises. Raising it
		// to Error would fail that test for doing its job. Still loud enough to
		// find, because the alternative symptom is a floor that never appears.
		UE_LOG(LogPHGeneratedFloor, Warning,
			TEXT("[%s] Floor %d could not be built from seed %d after %d retries. The run stays "
			     "in Generating: nothing else advances that phase. If this repeats for every "
			     "seed, check Request for an impossible configuration rather than bad luck."),
			*GetName(), FloorData.FloorNumber, FloorData.FloorSeed, MaxLayoutRetries);
		return;
	}

	const UGameInstance* GameInstance = GetGameInstance();
	if (URunSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<URunSubsystem>() : nullptr)
	{
		// Never acknowledge a refused build as an empty, already-complete floor. A callback may
		// have changed the run while construction finished, so acknowledge only the same request.
		if (Run->GetFloorPhase() == EFloorPhase::Generating && Run->GetFloorSeed() == FloorData.FloorSeed)
		{
			Run->NotifyFloorGenerated(LastEnemyBudget);
		}
	}
}

void APHGeneratedFloorActor::MovePlayersToStart()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || !HasAuthority() || LastRegionCount <= 0)
	{
		return;
	}

	const FVector Destination =
		LastPlayerStart.GetLocation() + FVector(0.0, 0.0, PlayerStartLift);

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* Controller = It->Get();
		APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}

		if (Pawn->TeleportTo(Destination, Pawn->GetActorRotation(), false, true))
		{
			if (UPawnMovementComponent* Movement = Pawn->GetMovementComponent())
			{
				Movement->StopMovementImmediately();
			}
		}
	}
}

void APHGeneratedFloorActor::Generate()
{
	TryGenerate();
}

bool APHGeneratedFloorActor::TryGenerate()
{
	if (bGenerating)
	{
		return false;
	}
	TGuardValue<bool> GenerationGuard(bGenerating, true);
	auto Refuse = [this](const FString& Reason)
	{
		LastResult = Reason;
		UE_LOG(LogPHGeneratedFloor, Warning, TEXT("%s"), *LastResult);
		return false;
	};

	if (!GetWorld() || !HasAuthority())
	{
		return Refuse(TEXT("Generation requires an editor world or server authority."));
	}
	if (GetActorTransform().ContainsNaN() || GetActorScale3D().GetAbsMin() <= UE_SMALL_NUMBER ||
		!FMath::IsFinite(PlayerStartLift) || PlayerStartLift < 0.0 ||
		!FMath::IsFinite(LightIntensity) || LightIntensity < 0.0f ||
		!FMath::IsFinite(LightAttenuationRadius) || LightAttenuationRadius < 0.0f ||
		!FMath::IsFinite(LightColor.R) || !FMath::IsFinite(LightColor.G) ||
		!FMath::IsFinite(LightColor.B) || !FMath::IsFinite(LightColor.A))
	{
		return Refuse(TEXT("Floor transform, route lift and lighting settings must be finite; distances and intensity cannot be negative."));
	}

	if (!ModuleSet)
	{
		return Refuse(TEXT("No ModuleSet assigned."));
	}

	TArray<FPHGenerationIssue> Issues;
	if (!ModuleSet->ValidateModuleSet(Issues))
	{
		return Refuse(FString::Printf(TEXT("Module set is invalid: %s"),
			Issues.IsEmpty() ? TEXT("unknown") : *Issues[0].Message));
	}

	// The floor piece defines the tile the whole plan is laid out on, so the layout has to have
	// been generated on the same grid or nothing lines up.
	FPHModuleEntry FloorEntry;
	if (!ModuleSet->ResolvePiece(PHGenerationTags::Piece_Floor.GetTag(), FloorEntry))
	{
		return Refuse(TEXT("Module set has no Piece.Floor."));
	}

	const double TileSize = FloorEntry.Footprint.X;
	if (!FMath::IsNearlyEqual(FloorEntry.Footprint.X, FloorEntry.Footprint.Y))
	{
		return Refuse(FString::Printf(TEXT("Piece.Floor must be square for tiling, got %s."),
			*FloorEntry.Footprint.ToString()));
	}

	if (!FMath::IsNearlyEqual(Request.GridSize, TileSize))
	{
		return Refuse(FString::Printf(
			TEXT("Request.GridSize is %f but the kit tiles at %f; set them equal."),
			Request.GridSize, TileSize));
	}
	FPHModuleEntry WallEntry;
	if (!ModuleSet->ResolvePiece(PHGenerationTags::Piece_Wall.GetTag(), WallEntry) ||
		!FMath::IsNearlyEqual(WallEntry.Footprint.X, TileSize) || WallEntry.Height <= 0.0)
	{
		return Refuse(TEXT("Piece.Wall must span one floor tile and have a positive authored Height."));
	}
	if (bBuildCeiling)
	{
		FPHModuleEntry CeilingEntry;
		if (!ModuleSet->ResolvePiece(PHGenerationTags::Piece_Ceiling.GetTag(), CeilingEntry) ||
			!CeilingEntry.Footprint.Equals(FVector2D(TileSize, TileSize)))
		{
			return Refuse(TEXT("Piece.Ceiling must match the floor tile footprint."));
		}
	}

	const TSubclassOf<UPHLayoutGenerator> Class =
		GeneratorClass ? GeneratorClass : TSubclassOf<UPHLayoutGenerator>(UPHDungeonGenerator::StaticClass());
	if (Class->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
	{
		return Refuse(TEXT("GeneratorClass must be a concrete, current layout generator."));
	}
	UPHLayoutGenerator* Generator = NewObject<UPHLayoutGenerator>(this, Class);

	FPHGeneratedLayout Layout;
	if (!Generator->GenerateLayout(Request, Layout, Issues))
	{
		return Refuse(FString::Printf(TEXT("Generation failed: %s"),
			Issues.IsEmpty() ? TEXT("unknown") : *Issues[0].Message));
	}

	FPHBlockoutPlan Plan;
	if (!UPHBlockoutPlanLibrary::BuildPlan(Layout, TileSize, Plan, Issues, bBuildCeiling,
		MinCorridorWidth, MaxCorridorWidth, WallEntry.Height, LightSpacingTiles))
	{
		return Refuse(FString::Printf(TEXT("Blockout planning failed: %s"),
			Issues.IsEmpty() ? TEXT("unknown") : *Issues[0].Message));
	}
	if (bBuildLighting && !FMath::IsFinite(GetLightRadius(Plan)))
	{
		return Refuse(TEXT("The derived light radius is outside the engine's finite range."));
	}

	FPHEncounterPlan Encounters;
	if (EncounterManagerClass)
	{
		if (EncounterManagerClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists) ||
			!UPHEncounterPlanLibrary::BuildEncounterPlan(Layout, EncounterInset, Encounters, Issues))
		{
			return Refuse(FString::Printf(TEXT("Encounter planning failed: %s"),
				Issues.IsEmpty() ? TEXT("EncounterManagerClass must be concrete and current") : *Issues[0].Message));
		}
		Encounters.Placements.RemoveAll([this](const FPHEncounterPlacement& Placement)
		{
			return bSkipStartRegion && Placement.bIsStartRegion;
		});
		if (!Encounters.Placements.IsEmpty())
		{
			const AMobManagerActor* Defaults = EncounterManagerClass->GetDefaultObject<AMobManagerActor>();
			const bool bHasMobClass = EncounterMobClasses.IsEmpty()
				? Defaults->MobTypes.ContainsByPredicate([](const FMobTypeEntry& Entry)
				{
					return Entry.MobClass && Entry.SpawnWeight > 0 && !Entry.MobClass->HasAnyClassFlags(CLASS_Abstract);
				})
				: EncounterMobClasses.ContainsByPredicate([](const TSubclassOf<APHBaseCharacter>& MobClass)
				{
					return MobClass && !MobClass->HasAnyClassFlags(CLASS_Abstract);
				});
			if (!bHasMobClass)
			{
				return Refuse(TEXT("Encounter regions require at least one spawnable mob class."));
			}
		}
	}
	if (ExitPortalClass && ExitPortalClass->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
	{
		return Refuse(TEXT("ExitPortalClass must be a concrete, current portal actor."));
	}

	struct FResolvedPiece
	{
		double YawOffset = 0.0;
		FVector2D Footprint = FVector2D::ZeroVector;
		double Height = 0.0;
		TArray<UStaticMesh*> Meshes;
	};
	struct FMeshBatch
	{
		UStaticMesh* Mesh = nullptr;
		TArray<FTransform> Transforms;
	};
	TMap<FGameplayTag, FResolvedPiece> ResolvedPieces;
	TMap<UStaticMesh*, int32> BatchByMesh;
	TArray<FMeshBatch> Batches;
	TMap<FGameplayTag, int32> VariantCounter;
	int32 Placed = 0;
	int32 PropCount = 0;
	int32 SkippedProps = 0;
	int64 ClearanceChecks = 0;
	TArray<FBox> WallBounds;
	TArray<FBox> PropBounds;
	TSet<FIntPoint> FloorTiles;
	FloorTiles.Append(Plan.FloorTiles);
	TSet<FIntPoint> CorridorTiles;
	CorridorTiles.Append(Plan.CorridorTiles);
	TSet<FIntPoint> AnchorTiles;
	for (const FPHGeneratedAnchor& Anchor : Layout.Anchors)
	{
		const FVector Position = Anchor.Transform.GetLocation();
		AnchorTiles.Add(FIntPoint(FMath::FloorToInt32(Position.X / TileSize), FMath::FloorToInt32(Position.Y / TileSize)));
	}
	TMap<FIntPoint, double> CeilingByTile;
	for (const FPHPiecePlacement& Placement : Plan.Placements)
	{
		if (Placement.PieceTag == PHGenerationTags::Piece_Ceiling.GetTag())
		{
			const FVector Position = Placement.Transform.GetLocation();
			CeilingByTile.Add(FIntPoint(FMath::RoundToInt32(Position.X / TileSize),
				FMath::RoundToInt32(Position.Y / TileSize)), Position.Z);
		}
	}

	auto PreparePiece = [&](const FGameplayTag& PieceTag, const FTransform& Transform,
		const int32 SourceRuleIndex = INDEX_NONE)
	{
		FResolvedPiece* Resolved = ResolvedPieces.Find(PieceTag);
		if (!Resolved)
		{
			FPHModuleEntry Entry;
			if (!ModuleSet->ResolvePiece(PieceTag, Entry))
			{
				return Refuse(FString::Printf(TEXT("Required piece %s is not mapped; current floor retained."), *PieceTag.ToString()));
			}
			Resolved = &ResolvedPieces.Add(PieceTag);
			Resolved->YawOffset = Entry.YawOffset;
			Resolved->Footprint = Entry.Footprint;
			Resolved->Height = Entry.Height;
			TArray<TSoftObjectPtr<UStaticMesh>> Options;
			Options.Add(Entry.Mesh);
			Options.Append(Entry.Variants);
			for (const TSoftObjectPtr<UStaticMesh>& Option : Options)
			{
				if (Option.IsNull()) { continue; }
				UStaticMesh* Mesh = Option.LoadSynchronous();
				if (!Mesh)
				{
					return Refuse(FString::Printf(TEXT("Required piece %s cannot load %s; current floor retained."),
						*PieceTag.ToString(), *Option.ToSoftObjectPath().ToString()));
				}
				Resolved->Meshes.Add(Mesh);
			}
			if (Resolved->Meshes.IsEmpty())
			{
				return Refuse(FString::Printf(TEXT("Required piece %s has no meshes."), *PieceTag.ToString()));
			}
		}

		// Resolve/load once per tag, retaining the existing per-tag draw sequence and option order.
		const int32 Draw = URunSeedFunctionLibrary::DeriveSeed(
			Layout.Seed, PieceTag.GetTagName(), VariantCounter.FindOrAdd(PieceTag)++);
		UStaticMesh* Mesh = Resolved->Meshes[Draw % Resolved->Meshes.Num()];

		FTransform Final = Transform;
		if (!FMath::IsNearlyZero(Resolved->YawOffset))
		{
			Final.SetRotation((Final.Rotator() + FRotator(0.0, Resolved->YawOffset, 0.0)).Quaternion());
		}
		if (PropRules.IsValidIndex(SourceRuleIndex))
		{
			// Rules work in logical tiles; only construction knows the chosen variant's real mesh
			// bounds and pivot. Do not let a large variant cross walls or cover a reserved route tile.
			const FPHPropRule& Rule = PropRules[SourceRuleIndex];
			const FBox MeshBounds = Mesh->GetBoundingBox();
			FBox Bounds = MeshBounds.TransformBy(Final);
			Final.AddToTranslation(FVector(0.0, 0.0, Layout.Bounds.Min.Z - Bounds.Min.Z));
			Bounds = MeshBounds.TransformBy(Final);
			const FBox AuthoredBounds(
				FVector(-Resolved->Footprint.X * 0.5, -Resolved->Footprint.Y * 0.5, MeshBounds.Min.Z),
				FVector(Resolved->Footprint.X * 0.5, Resolved->Footprint.Y * 0.5,
					FMath::Max(MeshBounds.Max.Z, MeshBounds.Min.Z + Resolved->Height)));
			Bounds += AuthoredBounds.TransformBy(Final);

			// The planner shoves a wall prop a fixed fraction of a tile toward its wall, because it
			// works in logical tiles and cannot know which variant was drawn. Only construction has
			// the real mesh: measured against this kit, a gear is 205 wide where its rule declared
			// 100, and a shove of 0.3 of a 400 tile puts it through the wall. Pulling it back until
			// it is flush with the tile edge leaves it against the wall and inside the room, where
			// clamping the shove away entirely would float every prop in open floor.
			const FIntPoint PropTile(
				FMath::FloorToInt32(Final.GetLocation().X / TileSize),
				FMath::FloorToInt32(Final.GetLocation().Y / TileSize));
			const FVector2D TileMin(PropTile.X * TileSize, PropTile.Y * TileSize);
			const FVector2D TileMax(TileMin.X + TileSize, TileMin.Y + TileSize);

			FVector PullBack = FVector::ZeroVector;
			if (Bounds.Min.X < TileMin.X) { PullBack.X = TileMin.X - Bounds.Min.X; }
			else if (Bounds.Max.X > TileMax.X) { PullBack.X = TileMax.X - Bounds.Max.X; }
			if (Bounds.Min.Y < TileMin.Y) { PullBack.Y = TileMin.Y - Bounds.Min.Y; }
			else if (Bounds.Max.Y > TileMax.Y) { PullBack.Y = TileMax.Y - Bounds.Max.Y; }

			// A prop wider than the tile cannot be saved by shifting it; the clearance test below
			// still rejects it, which is why oversized meshes belong in prefabs, not in a scatter.
			if (!PullBack.IsNearlyZero())
			{
				Final.AddToTranslation(PullBack);
				Bounds = MeshBounds.TransformBy(Final);
				Bounds += AuthoredBounds.TransformBy(Final);
			}
			bool bFits = Bounds.IsValid && !Bounds.Min.ContainsNaN() && !Bounds.Max.ContainsNaN() &&
				Bounds.Min.X >= Layout.Bounds.Min.X && Bounds.Min.Y >= Layout.Bounds.Min.Y &&
				Bounds.Max.X <= Layout.Bounds.Max.X && Bounds.Max.Y <= Layout.Bounds.Max.Y;
			if (bFits)
			{
				const FIntPoint Min(FMath::FloorToInt32(Bounds.Min.X / TileSize), FMath::FloorToInt32(Bounds.Min.Y / TileSize));
				const FIntPoint Max(FMath::CeilToInt32(Bounds.Max.X / TileSize) - 1, FMath::CeilToInt32(Bounds.Max.Y / TileSize) - 1);
				const int64 TilesToCheck = (static_cast<int64>(Max.X) - Min.X + 1) * (static_cast<int64>(Max.Y) - Min.Y + 1);
				bFits = TilesToCheck > 0 && TilesToCheck <= FloorTiles.Num();
				if (bFits)
				{
					ClearanceChecks += TilesToCheck + WallBounds.Num() + PropBounds.Num();
					if (ClearanceChecks > 8000000)
					{
						return Refuse(TEXT("Decoration exceeds the synchronous mesh-clearance work budget; current floor retained."));
					}
					for (int32 Y = Min.Y; bFits && Y <= Max.Y; ++Y)
					{
						for (int32 X = Min.X; bFits && X <= Max.X; ++X)
						{
							const FIntPoint Tile(X, Y);
							const double* Ceiling = CeilingByTile.Find(Tile);
							bFits = FloorTiles.Contains(Tile) && Tile != Plan.PlayerStartTile && Tile != Plan.ExitTile &&
								(!Rule.bAvoidAnchors || !AnchorTiles.Contains(Tile)) &&
								(!Rule.bAvoidCorridors || !CorridorTiles.Contains(Tile)) &&
								(!Ceiling || Bounds.Max.Z <= *Ceiling);
						}
					}
				}
			}
			bFits = bFits && !WallBounds.ContainsByPredicate([&Bounds](const FBox& Other) { return Bounds.Intersect(Other); }) &&
				!PropBounds.ContainsByPredicate([&Bounds](const FBox& Other) { return Bounds.Intersect(Other); });
			if (!bFits)
			{
				++SkippedProps;
				return true;
			}
			PropBounds.Add(Bounds);
			++PropCount;
		}
		else if (PieceTag.MatchesTag(PHGenerationTags::Piece_Wall.GetTag()))
		{
			WallBounds.Add(Mesh->GetBoundingBox().TransformBy(Final));
		}

		int32* BatchIndex = BatchByMesh.Find(Mesh);
		if (!BatchIndex)
		{
			const int32 NewIndex = Batches.AddDefaulted();
			Batches[NewIndex].Mesh = Mesh;
			BatchIndex = &BatchByMesh.Add(Mesh, NewIndex);
		}
		Batches[*BatchIndex].Transforms.Add(Final);
		++Placed;
		return true;
	};

	for (const FPHPiecePlacement& Placement : Plan.Placements)
	{
		if (!PreparePiece(Placement.PieceTag, Placement.Transform)) { return false; }
	}

	if (bDecorate && !PropRules.IsEmpty())
	{
		// Its own labelled branch off the layout seed: decoration must never consume the draws
		// that decide layout, encounters, or loot.
		const int32 DecorationSeed = URunSeedFunctionLibrary::DeriveSeed(
			Layout.Seed, FName(TEXT("Decoration")), 0);

		FPHDecorationPlan Decoration;
		if (UPHDecorationPlanLibrary::BuildDecorationPlan(
			Layout, Plan, PropRules, DecorationSeed, Decoration, Issues))
		{
			for (const FPHPiecePlacement& Placement : Decoration.Placements)
			{
				if (!PreparePiece(Placement.PieceTag, Placement.Transform, Placement.SourceRuleIndex)) { return false; }
			}
		}
		else
		{
			return Refuse(FString::Printf(TEXT("Decoration planning failed: %s"),
				Issues.IsEmpty() ? TEXT("unknown") : *Issues[0].Message));
		}
	}
	// Ceiling fixtures are hung where the lighting pass actually put light, rather than scattered by
	// the prop rules. A ceiling lamp is authored to hang from above; the decoration scatter only
	// knows the floor plane, which is exactly how ceiling lights ended up sitting on the ground.
	if (FPHModuleEntry CeilingLightEntry; ModuleSet->ResolvePiece(
		PHGenerationTags::Prop_Light_Ceiling.GetTag(), CeilingLightEntry))
	{
		for (const FTransform& Pose : Plan.LightPoses)
		{
			if (!PreparePiece(PHGenerationTags::Prop_Light_Ceiling.GetTag(), Pose)) { return false; }
		}
	}

	if (bMarkEndpoints && !ExitPortalClass &&
		!PreparePiece(PHGenerationTags::Piece_Pillar.GetTag(), Plan.Exit))
	{
		return false;
	}

	// All request, planner, class and mesh checks precede this commit point. The previous floor
	// survives bad authoring, instead of leaving the player falling through an empty level.
	ClearBuilt();
	TMap<UStaticMesh*, UInstancedStaticMeshComponent*> Cache;
	for (const FMeshBatch& Batch : Batches)
	{
		// One insertion per mesh also batches instance bounds, physics and navigation updates.
		FindOrAddComponentFor(Batch.Mesh, Cache)->AddInstances(Batch.Transforms, false, false, true);
	}

	if (bBuildLighting)
	{
		PlaceLights(Plan);
	}

	// Published in world space, on the centre of a tile the plan actually built.
	LastPlayerStart = Plan.PlayerStart * GetActorTransform();
	LastExit = Plan.Exit * GetActorTransform();
	LastRegionCount = Layout.Regions.Num();

	if (bBuildNavigation)
	{
		BuildNavigationBounds(Layout.Bounds);
	}

	// Dynamic navigation can still be rebuilding. Encounter owners retain their own candidate
	// projection/retry path; their configuration must be complete before BeginPlay activates it.
	if (!PlaceEncounters(Encounters, Layout.Seed) || !PlaceChests(Layout) || !PlaceRouteActors())
	{
		return Refuse(TEXT("Floor geometry built, but a required encounter or route actor failed to spawn; run was not acknowledged."));
	}

	LastResult = FString::Printf(
		TEXT("Seed %d: %d regions (requested %d-%d), %d floor tiles (%d corridor), %d walls, %d ceilings, %d props, %d instances."),
		Layout.Seed, Layout.Regions.Num(), Request.MinRegionCount, Request.MaxRegionCount,
		Plan.FloorTileCount, Plan.CorridorTileCount,
		Plan.WallCount, Plan.CeilingCount, PropCount, Placed);

	LastResult += FString::Printf(TEXT(" %d encounters, %d enemies, %d chests."),
		LastEncounterCount, LastEnemyBudget, LastChestCount);
	if (SkippedProps > 0)
	{
		LastResult += FString::Printf(TEXT(" %d decoration placements skipped for mesh clearance."), SkippedProps);
	}

	UE_LOG(LogPHGeneratedFloor, Log, TEXT("%s"), *LastResult);
	return true;
}


void APHGeneratedFloorActor::BuildNavigationBounds(const FBox& LayoutBounds)
{
	// Deliberately does not create the volume. Building brush geometry needs UnrealEd, which a
	// runtime module must not depend on, so the NavMeshBoundsVolume is authored in the level and
	// this only reports whether the built floor is actually covered by navigation.
	UWorld* World = GetWorld();
	bLastBuildHadNavigation = false;
	if (!World || !LayoutBounds.IsValid)
	{
		return;
	}

	UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	const ANavigationData* NavData = NavSystem
		? NavSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) : nullptr;
	const FBox WorldBounds = LayoutBounds.TransformBy(GetActorTransform());
	if (!NavData || !NavData->GetDefaultQueryFilter().IsValid())
	{
		UE_LOG(LogPHGeneratedFloor, Warning,
			TEXT("No queryable navigation data in this level. Place a NavMeshBoundsVolume covering %s "
			     "and build Dynamic navigation before testing encounter spawning."), *WorldBounds.ToString());
		return;
	}

	for (const FNavigationBounds& Bounds : NavSystem->GetNavigationBounds())
	{
		if (Bounds.AreaBox.IsValid && Bounds.AreaBox.IsInsideOrOn(WorldBounds))
		{
			bLastBuildHadNavigation = true;
			break;
		}
	}
	if (!bLastBuildHadNavigation)
	{
		UE_LOG(LogPHGeneratedFloor, Warning,
			TEXT("Navigation data exists, but no authored bounds volume covers the floor envelope %s."),
			*WorldBounds.ToString());
	}
	if (World->IsGameWorld() && NavData->GetRuntimeGenerationMode() != ERuntimeGenerationType::Dynamic)
	{
		bLastBuildHadNavigation = false;
		UE_LOG(LogPHGeneratedFloor, Warning,
			TEXT("Runtime floor replacement requires Dynamic navigation; existing baked navigation cannot follow new geometry."));
	}
}

bool APHGeneratedFloorActor::PlaceChests(const FPHGeneratedLayout& Layout)
{
	LastChestCount = 0;

	UWorld* World = GetWorld();
	if (!World || !ChestClass)
	{
		return true;
	}

	// Driven by the anchors the layout seated, not by a separate roll here: how many chests a floor
	// carries and which rooms may hold one is authored as anchor rules on the Request, and those
	// draw from the layout stream, so a seed reproduces its chests along with its rooms.
	for (const FPHGeneratedAnchor& Anchor : Layout.Anchors)
	{
		if (Anchor.SemanticTag != PHGenerationTags::Anchor_Chest.GetTag())
		{
			continue;
		}

		// The anchor sits on the floor plane, which is where a chest belongs. This used to add
		// PlayerStartLift, but that value exists to clear a *pawn capsule* and is 100 units, so
		// every chest floated a metre off the ground. Seating happens after spawn, from the
		// actor's own bounds, so it works whatever the chest mesh's pivot is.
		const FTransform Pose = Anchor.Transform * GetActorTransform();

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Owner = this;

		ALootChest* Chest = World->SpawnActor<ALootChest>(ChestClass, Pose, Params);
		if (!Chest)
		{
			continue;
		}

#if WITH_EDITOR
		Chest->SetActorLabel(FString::Printf(TEXT("GeneratedFloor_Chest%d"), Anchor.AnchorID));
#endif

		// Lift by however far the chest's lowest point sits below its pivot, so a
		// centre-pivot mesh rests on the floor instead of sinking half into it. A
		// base-pivot mesh measures zero here and is left where it spawned.
		FVector BoundsOrigin = FVector::ZeroVector;
		FVector BoundsExtent = FVector::ZeroVector;
		Chest->GetActorBounds(/*bOnlyCollidingComponents*/ false, BoundsOrigin, BoundsExtent);
		const double LowestPoint = BoundsOrigin.Z - BoundsExtent.Z;
		const double SinkDepth = Chest->GetActorLocation().Z - LowestPoint;
		if (SinkDepth > 0.0)
		{
			Chest->AddActorWorldOffset(FVector(0.0, 0.0, SinkDepth));
		}

		SpawnedActors.Add(Chest);
		++LastChestCount;
	}

	return true;
}

bool APHGeneratedFloorActor::PlaceRouteActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;

	if (bPlacePlayerStart)
	{
		const FVector Location = LastPlayerStart.GetLocation() + FVector(0.0, 0.0, PlayerStartLift);
		if (APlayerStart* Start =
			World->SpawnActor<APlayerStart>(Location, LastPlayerStart.Rotator(), Params))
		{
#if WITH_EDITOR
			Start->SetActorLabel(TEXT("GeneratedFloor_PlayerStart"));
#endif
			SpawnedActors.Add(Start);
		}
		else
		{
			return false;
		}
	}

	if (!ExitPortalClass)
	{
		return true;
	}

	const FVector PortalLocation = LastExit.GetLocation() + FVector(0.0, 0.0, PlayerStartLift);
	const FTransform PortalPose(LastExit.GetRotation(), PortalLocation);
	Params.bDeferConstruction = true;
	ExitPortal = World->SpawnActor<APortalActor>(
		ExitPortalClass, PortalPose, Params);
	if (!ExitPortal)
	{
		return false;
	}

#if WITH_EDITOR
	ExitPortal->SetActorLabel(TEXT("GeneratedFloor_ExitPortal"));
#endif

	// A generated exit has no twin to link to: this actor decides where it leads by rebuilding the
	// floor, so the portal must not go looking for a destination portal that will never exist.
	ExitPortal->PortalID = FName(TEXT("GeneratedFloor_Exit"));
	ExitPortal->bDestinationHandledByListener = true;
	ExitPortal->OnPortalActivated.AddUniqueDynamic(
		this, &APHGeneratedFloorActor::HandleExitPortalActivated);

	// The exit starts shut while a run is gating it, and opens when the run owner reports the floor
	// finished. Left open it would look usable on arrival and simply refuse, which reads as a bug
	// rather than as a locked door. With no run there is nothing to clear, so it opens immediately.
	const UGameInstance* GameInstance = GetGameInstance();
	const URunSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<URunSubsystem>() : nullptr;
	const bool bGated = bGateExitOnObjective && Run && Run->IsRunActive();
	ExitPortal->bActiveOnBeginPlay = !bGated;

	SpawnedActors.Add(ExitPortal);
	// Portal BeginPlay registers PortalID and evaluates travel behavior immediately.
	ExitPortal->FinishSpawning(PortalPose);
	return IsValid(ExitPortal);
}

bool APHGeneratedFloorActor::PlaceEncounters(const FPHEncounterPlan& Plan, const int32 LayoutSeed)
{
	LastEncounterCount = 0;
	LastEnemyBudget = 0;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	if (!EncounterManagerClass)
	{
		return true;
	}
	const UGameInstance* GameInstance = GetGameInstance();
	const URunSubsystem* Run = GameInstance ? GameInstance->GetSubsystem<URunSubsystem>() : nullptr;

	for (const FPHEncounterPlacement& Placement : Plan.Placements)
	{
		const FTransform Pose = FTransform(Placement.SpawnBounds.GetCenter()) * GetActorTransform();
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Owner = this;
		Params.bDeferConstruction = true;

		AMobManagerActor* Manager = World->SpawnActor<AMobManagerActor>(
			EncounterManagerClass, Pose, Params);
		if (!Manager)
		{
			return false;
		}

#if WITH_EDITOR
		Manager->SetActorLabel(FString::Printf(TEXT("Encounter_Region%d"), Placement.RegionID));
#endif

		// The manager scatters within its own SpawnArea, so the region volume is handed to it
		// rather than a list of exact positions: it owns nav projection and collision checks.
		if (UBoxComponent* Area = Manager->SpawnArea)
		{
			Area->SetBoxExtent(Placement.SpawnBounds.GetExtent(), false);
		}

		// The anchor count is the budget generation asked for.
		Manager->MaxNumOfMobs = FMath::Max(1, Placement.EnemyCount);
		Manager->EncounterIndex = Placement.RegionID;
		Manager->EncounterSeedOverride = bFollowRunSubsystem && Run && Run->IsRunActive()
			? Run->GetEncounterSeed(Placement.RegionID)
			: URunSeedFunctionLibrary::DeriveSeed(LayoutSeed, FName(TEXT("Encounter")), Placement.RegionID);

		if (!EncounterMobClasses.IsEmpty())
		{
			Manager->MobTypes.Reset();
			for (const TSubclassOf<APHBaseCharacter>& MobClass : EncounterMobClasses)
			{
				if (!MobClass || MobClass->HasAnyClassFlags(CLASS_Abstract))
				{
					continue;
				}
				FMobTypeEntry& Entry = Manager->MobTypes.AddDefaulted_GetRef();
				Entry.MobClass = MobClass;
			}
		}

		SpawnedActors.Add(Manager);

		// Bound before FinishSpawning, not after: auto activation can start spawning inside that
		// call, and a mob that dies before the binding exists is a kill the floor never counts.
		Manager->OnMobDied.AddUniqueDynamic(this, &APHGeneratedFloorActor::HandleMobDied);

		// Auto activation now sees its final volume, classes, budget and independent seed.
		Manager->FinishSpawning(Pose);
		if (!IsValid(Manager)) { return false; }
		LastEnemyBudget += Manager->MaxNumOfMobs;
		++LastEncounterCount;
	}
	return true;
}


float APHGeneratedFloorActor::GetLightRadius(const FPHBlockoutPlan& Plan) const
{
	if (LightAttenuationRadius > 0.0f)
	{
		return LightAttenuationRadius;
	}

	// The planner guarantees XY coverage. Include mounting height and actor scale in the sphere
	// radius, otherwise high ceilings or scaled floors can put every floor tile outside its lights.
	double MaxHeight = 0.0;
	for (const FTransform& Pose : Plan.LightPoses)
	{
		MaxHeight = FMath::Max(MaxHeight, FMath::Abs(Pose.GetLocation().Z - Plan.PlayerStart.GetLocation().Z));
	}
	const double HorizontalReach = FMath::Max(1, LightSpacingTiles) * Plan.TileSize;
	return static_cast<float>(FMath::Sqrt(HorizontalReach * HorizontalReach + MaxHeight * MaxHeight) *
		1.75 * GetActorScale3D().GetAbsMax());
}

void APHGeneratedFloorActor::PlaceLights(const FPHBlockoutPlan& Plan)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	const float Radius = GetLightRadius(Plan);

	for (const FTransform& Pose : Plan.LightPoses)
	{
		// The pose is the ceiling plane, where the fixture mesh mounts. Only the
		// light source drops, and only far enough to clear the ceiling.
		const FTransform WorldPose = Pose * GetActorTransform();
		const FVector LightLocation =
			WorldPose.GetLocation() - FVector(0.0, 0.0, LightDropBelowCeiling);

		FActorSpawnParameters Params;
		Params.Owner = this;
		APointLight* Light = World->SpawnActor<APointLight>(LightLocation, FRotator::ZeroRotator, Params);
		if (!Light)
		{
			continue;
		}

		if (UPointLightComponent* Component = Cast<UPointLightComponent>(Light->GetLightComponent()))
		{
			Component->SetMobility(EComponentMobility::Movable);
			Component->SetIntensity(LightIntensity);
			Component->SetAttenuationRadius(Radius);
			Component->SetLightColor(LightColor);
			Component->SetCastShadows(false);
		}

		SpawnedActors.Add(Light);
	}
}
