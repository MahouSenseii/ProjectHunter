// InteractionManager.cpp - CORRECTED VERSION
#include "Components/InteractionManager.h"
#include "Character/PHBaseCharacter.h"
#include "Character/Player/PHPlayerController.h"
#include "Components/InteractableManager.h"
#include "GameFramework/CharacterMovementComponent.h"

DEFINE_LOG_CATEGORY(LogInteract);

// FSpatialGrid Implementation
void FSpatialGrid::Add(UInteractableManager* Interactable)
{
    if (!Interactable || !Interactable->GetOwner()) return;
    
    FIntPoint Cell = GetCell(Interactable->GetOwner()->GetActorLocation());
    Grid.FindOrAdd(Cell).Add(Interactable);
}

void FSpatialGrid::Remove(UInteractableManager* Interactable)
{
    if (!Interactable || !Interactable->GetOwner()) return;
    
    FIntPoint Cell = GetCell(Interactable->GetOwner()->GetActorLocation());
    if (auto* CellArray = Grid.Find(Cell))
    {
        CellArray->Remove(Interactable);
        if (CellArray->Num() == 0)
        {
            Grid.Remove(Cell);
        }
    }
}

void FSpatialGrid::Move(UInteractableManager* Interactable, const FVector& OldLocation, const FVector& NewLocation)
{
    FIntPoint OldCell = GetCell(OldLocation);
    FIntPoint NewCell = GetCell(NewLocation);
    
    if (OldCell != NewCell)
    {
        // Remove from old cell
        if (auto* OldCellArray = Grid.Find(OldCell))
        {
            OldCellArray->Remove(Interactable);
            if (OldCellArray->Num() == 0)
            {
                Grid.Remove(OldCell);
            }
        }
        
        // Add to a new cell
        Grid.FindOrAdd(NewCell).Add(Interactable);
    }
}

TArray<UInteractableManager*> FSpatialGrid::GetNearby(const FVector& Location, float Radius) const
{
    TArray<UInteractableManager*> Result;
    
    const int32 CellRadius = FMath::CeilToInt(Radius / CellSize);
    const FIntPoint CenterCell = GetCell(Location);
    
    // Pre-allocate based on estimated cell count
    const int32 EstimatedCells = (CellRadius * 2 + 1) * (CellRadius * 2 + 1);
    Result.Reserve(EstimatedCells * 5); // Estimate 5 items per cell
    
    for (int32 X = -CellRadius; X <= CellRadius; X++)
    {
        for (int32 Y = -CellRadius; Y <= CellRadius; Y++)
        {
            const FIntPoint Cell(CenterCell.X + X, CenterCell.Y + Y);
            if (const auto* CellArray = Grid.Find(Cell))
            {
                Result.Append(*CellArray);
            }
        }
    }
    
    return Result;
}

// UInteractionManager Implementation
UInteractionManager::UInteractionManager(): CachedPlayerLocation(), CachedLookDirection(), LastLookDirection()
{
    PrimaryComponentTick.bCanEverTick = false;

    // Pre-allocate arrays for performance
    CandidateCache.Reserve(50);
    NearbyCache.Reserve(100);
}

void UInteractionManager::BeginPlay()
{
    Super::BeginPlay();
    
    // Initialize controller and cache camera manager
    OwnerController = Cast<APHPlayerController>(GetOwner());
    if (OwnerController)
    {
        // Cache camera manager for performance
        CachedCamera = OwnerController->PlayerCameraManager;
    }
    
    // Configure spatial grid
    SpatialGrid.CellSize = SpatialGridCellSize;
    
    ScheduleNextUpdate();
}

void UInteractionManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UpdateTimerHandle);
    }
}

void UInteractionManager::ScheduleNextUpdate()
{
    float UpdateInterval; // Default
    
    // Adaptive timing based on nearby item count
    const int32 NearbyCount = bUseSpatialPartitioning ? 
        SpatialGrid.GetNearby(CachedPlayerLocation, InteractMaxDistance * 1.5f).Num() :
        InteractableList.Num();
    
    if (NearbyCount == 0)
    {
        UpdateInterval = 0.5f; // Very slow when nothing around
    }
    else if (NearbyCount < 5)
    {
        UpdateInterval = 0.2f; // Moderate for a few items
    }
    else if (NearbyCount < 15)
    {
        UpdateInterval = 0.1f; // Normal for several items
    }
    else
    {
        UpdateInterval = 0.05f; // Fast for high density
    }
    
    GetWorld()->GetTimerManager().SetTimer(
        UpdateTimerHandle,
        this,
        &UInteractionManager::UpdateInteractionOptimized,
        UpdateInterval,
        false
    );
}

void UInteractionManager::UpdateInteractionOptimized()
{
    // Cache player data once per update
    CachePlayerData();
    
    // Check if an update is needed
    if (ShouldUpdateInteraction())
    {
        AssignCurrentInteraction();
    }
    
    ScheduleNextUpdate();
}

void UInteractionManager::CachePlayerData()
{
    if (!IsValidForInteraction()) return;
    
    const APHBaseCharacter* Character = Cast<APHBaseCharacter>(OwnerController->PossessedCharacter);
    CachedPlayerLocation = Character->GetActorLocation();
    
    // Use cached camera manager reference
    if (CachedCamera)
    {
        CachedLookDirection = CachedCamera->GetActorForwardVector();
    }
    else
    {
        CachedLookDirection = Character->GetActorForwardVector();
    }
}

bool UInteractionManager::ShouldUpdateInteraction()
{
    if (!IsValidForInteraction()) return false;
    
    const APHBaseCharacter* Character = Cast<APHBaseCharacter>(OwnerController->PossessedCharacter);
    
    // Don't update if falling
    if (Character->GetCharacterMovement()->IsFalling()) return false;
    
    // Check if a list version changed
    if (CurrentListVersion != LastListVersion)
    {
        LastListVersion = CurrentListVersion;
        return true;
    }
    
    // Check if you look a direction changed significantly
    const float DotProduct = FVector::DotProduct(CachedLookDirection, LastLookDirection);
    if (DotProduct < LookSensitivity)
    {
        LastLookDirection = CachedLookDirection;
        return true;
    }
    
    return false;
}

void UInteractionManager::AssignCurrentInteraction()
{
    if (!IsValidForInteraction()) return;
    
    // Clear candidate cache
    CandidateCache.Reset();
    
    // Get nearby interactables
    if (bUseSpatialPartitioning)
    {
        NearbyCache = SpatialGrid.GetNearby(CachedPlayerLocation, InteractMaxDistance);
    }
    else
    {
        // Fallback to using the maintained InteractableList
        NearbyCache.Reset();
        const float MaxDistanceSq = FMath::Square(InteractMaxDistance);
        
        for (UInteractableManager* Interactable : InteractableList)
        {
            if (!IsValid(Interactable) || !Interactable->IsInteractable) continue;
            
            const float DistanceSq = FVector::DistSquared(CachedPlayerLocation, 
                Interactable->GetOwner()->GetActorLocation());
                
            if (DistanceSq <= MaxDistanceSq)
            {
                NearbyCache.Add(Interactable);
            }
        }
    }
    
    // SOLUTION 1: Pre-filter and sort candidates by distance
    TArray<TPair<UInteractableManager*, float>> CandidatesWithDistance;
    CandidatesWithDistance.Reserve(NearbyCache.Num());
    
    // Create distance-sorted candidates
    for (UInteractableManager* Interactable : NearbyCache)
    {
        if (!IsValid(Interactable) || !Interactable->IsInteractable) continue;
        
        const float DistanceSq = FVector::DistSquared(CachedPlayerLocation, 
            Interactable->GetOwner()->GetActorLocation());
            
        if (DistanceSq <= FMath::Square(InteractMaxDistance))
        {
            CandidatesWithDistance.Add(TPair<UInteractableManager*, float>(Interactable, DistanceSq));
        }
    }
    
    // Sort by distance (closest first)
    CandidatesWithDistance.Sort([](const TPair<UInteractableManager*, float>& A, 
                                   const TPair<UInteractableManager*, float>& B)
    {
        return A.Value < B.Value;
    });
    
    // SOLUTION 2: Evaluate best candidates (closest first)
    const int32 MaxCandidates = FMath::Min(CandidatesWithDistance.Num(), MaxCandidatesPerFrame);
    
    float BestScore = 0.0f;
    UInteractableManager* BestInteractable = nullptr;
    
    for (int32 i = 0; i < MaxCandidates; ++i)
    {
        UInteractableManager* Interactable = CandidatesWithDistance[i].Key;
        
        FInteractionCandidate Candidate = CreateCandidate(Interactable);
        const float Score = CalculateInteractionScore(Candidate);
        
        if (Score > BestScore && Score >= InteractThreshold)
        {
            BestScore = Score;
            BestInteractable = Interactable;
        }
    }
    
    SetCurrentInteraction(BestInteractable);
    
    if (bDebugMode)
    {
        UE_LOG(LogInteract, Log, TEXT("Evaluated %d/%d candidates, selected: %s (Score: %.3f)"),
            MaxCandidates, CandidatesWithDistance.Num(),
            BestInteractable ? *BestInteractable->GetOwner()->GetName() : TEXT("None"),
            BestScore);
    }
}

FInteractionCandidate UInteractionManager::CreateCandidate(UInteractableManager* Interactable) const
{
    const FVector InteractableLocation = Interactable->GetOwner()->GetActorLocation();
    const FVector ToInteractable = (InteractableLocation - CachedPlayerLocation).GetSafeNormal();
    
    return FInteractionCandidate(
        Interactable,
        FVector::DistSquared(CachedPlayerLocation, InteractableLocation),
        FVector::DotProduct(CachedLookDirection, ToInteractable)
    );
}

float UInteractionManager::CalculateInteractionScore(const FInteractionCandidate& Candidate) const
{
    // Normalize distance (closer = higher score)
    const float MaxDistanceSq = FMath::Square(InteractMaxDistance);
    const float NormalizedDistance = 1.0f - (Candidate.DistanceSq / MaxDistanceSq);
    
    // Dot product is already normalized (-1 to 1), convert to (0 to 1)
    const float NormalizedDot = (Candidate.DotProduct + 1.0f) * 0.5f;
    
    // Weighted combination
    return (NormalizedDot * DotWeight) + (NormalizedDistance * DistanceWeight);
}

void UInteractionManager::AddToInteractionList(UInteractableManager* InInteractable)
{
    if (!InInteractable) return;
    
    // Always maintain the list for fallback
    InteractableList.AddUnique(InInteractable);
    
    // Also add to spatial grid if enabled
    if (bUseSpatialPartitioning)
    {
        SpatialGrid.Add(InInteractable);
    }
    
    CurrentListVersion++;
    
    UE_LOG(LogInteract, Verbose, TEXT("Added %s to interaction system (count: %d)"), 
        *InInteractable->GetOwner()->GetName(), InteractableList.Num());
}

void UInteractionManager::RemoveFromInteractionList(UInteractableManager* InInteractable)
{
    if (!InInteractable) return;
    
    // Always maintain the list for fallback
    InteractableList.Remove(InInteractable);
    
    // Also remove from spatial grid if enabled
    if (bUseSpatialPartitioning)
    {
        SpatialGrid.Remove(InInteractable);
    }
    
    if (CurrentInteractable == InInteractable)
    {
        SetCurrentInteraction(nullptr);
    }
    
    CurrentListVersion++;
    
    UE_LOG(LogInteract, Verbose, TEXT("Removed %s from interaction system (count: %d)"), 
        *InInteractable->GetOwner()->GetName(), InteractableList.Num());
}

void UInteractionManager::UpdateInteractableLocation(UInteractableManager* Interactable, const FVector& OldLocation)
{
    if (!Interactable || !bUseSpatialPartitioning) return;
    
    const FVector NewLocation = Interactable->GetOwner()->GetActorLocation();
    SpatialGrid.Move(Interactable, OldLocation, NewLocation);
}

void UInteractionManager::SetCurrentInteraction(UInteractableManager* NewInteractable)
{
    if (NewInteractable == CurrentInteractable) return;
    
    if (IsValid(CurrentInteractable))
    {
        RemoveInteractionFromCurrent(CurrentInteractable);
    }
    
    CurrentInteractable = NewInteractable;
    
    if (IsValid(CurrentInteractable))
    {
        AddInteraction(CurrentInteractable);
    }
}

void UInteractionManager::AddInteraction(UInteractableManager* Interactable)
{
    if (!IsValid(Interactable)) return;
    
    if (CurrentInteractable != Interactable)
    {
        OnRemoveCurrentInteractable.Broadcast(CurrentInteractable);
    }
    
    Interactable->ToggleHighlight(true, OwnerController);
    CurrentInteractable = Interactable;
    OnNewInteractableAssigned.Broadcast(Interactable);
}

void UInteractionManager::RemoveInteractionFromCurrent(UInteractableManager* Interactable)
{
    if (!IsValid(Interactable) || !IsValid(OwnerController) || CurrentInteractable != Interactable) return;
    
    ToggleHighlight(false);
    CurrentInteractable = nullptr;
    OnRemoveCurrentInteractable.Broadcast(Interactable);
}

void UInteractionManager::ToggleHighlight(const bool bShouldHighlight) const
{
    if (IsValid(CurrentInteractable) && IsValid(OwnerController))
    {
        CurrentInteractable->ToggleHighlight(bShouldHighlight, OwnerController);
    }
}

bool UInteractionManager::IsValidForInteraction() const
{
    return IsValid(OwnerController) && 
           IsValid(OwnerController->PossessedCharacter);
}

void UInteractionManager::MaxOfFloatArray(const TArray<float>& FloatArray, float& OutMaxValue, int32& OutMaxIndex)
{
    if (FloatArray.Num() == 0)
    {
        OutMaxValue = -FLT_MAX;
        OutMaxIndex = -1;
        return;
    }
    
    OutMaxValue = FloatArray[0];
    OutMaxIndex = 0;
    
    for (int32 Index = 1; Index < FloatArray.Num(); ++Index)
    {
        if (FloatArray[Index] > OutMaxValue)
        {
            OutMaxValue = FloatArray[Index];
            OutMaxIndex = Index;
        }
    }
}