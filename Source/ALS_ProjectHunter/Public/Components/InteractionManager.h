// Copyright@2024 Quentin Davis 
#pragma once

#include "CoreMinimal.h"
#include "InteractableManager.h"
#include "Components/ActorComponent.h"
#include "InteractionManager.generated.h"

class APHPlayerController;
class APHPlayerCharacter;
class UInteractableManager;
class AALSPlayerCameraManager;

DECLARE_LOG_CATEGORY_EXTERN(LogInteract, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnNewInteractableAssigned, UInteractableManager*, NewInteractable);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemoveCurrentInteractable, UInteractableManager*, RemovedIntercatable);

// Optimized spatial partitioning system
USTRUCT()
struct FSpatialGrid
{
    GENERATED_BODY()
    
    TMap<FIntPoint, TArray<UInteractableManager*>> Grid;
    float CellSize = 200.0f;
    
    FIntPoint GetCell(const FVector& Location) const
    {
        return FIntPoint(
            FMath::FloorToInt(Location.X / CellSize),
            FMath::FloorToInt(Location.Y / CellSize)
        );
    }
    
    void Add(UInteractableManager* Interactable);
    void Remove(UInteractableManager* Interactable);
    void Move(UInteractableManager* Interactable, const FVector& OldLocation, const FVector& NewLocation);
    TArray<UInteractableManager*> GetNearby(const FVector& Location, float Radius) const;
    void Clear() { Grid.Empty(); }
};

// Cached interaction candidate for performance
USTRUCT()
struct FInteractionCandidate
{
    GENERATED_BODY()
    
    UPROPERTY()
    UInteractableManager* Interactable = nullptr;
    
    float DistanceSq = 0.0f;
    float DotProduct = 0.0f;
    float Score = 0.0f;
    
    FInteractionCandidate() = default;
    FInteractionCandidate(UInteractableManager* InInteractable, float InDistanceSq, float InDotProduct)
        : Interactable(InInteractable), DistanceSq(InDistanceSq), DotProduct(InDotProduct) {}
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UInteractionManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UInteractionManager();

    // Configuration
    UPROPERTY(EditAnywhere, Category = "Interaction")
    float InteractThreshold = 0.25f;

    UPROPERTY(EditAnywhere, Category = "Interaction")
    float InteractMaxDistance = 400.0f;

    UPROPERTY(EditAnywhere, Category = "Scoring")
    float DotWeight = 0.9f;

    UPROPERTY(EditAnywhere, Category = "Scoring")
    float DistanceWeight = 0.1f;

    UPROPERTY(EditAnywhere, Category = "Debug")
    bool bDebugMode = false;

    // Performance tuning
    UPROPERTY(EditAnywhere, Category = "Optimization")
    float LookSensitivity = 0.98f;
    
    UPROPERTY(EditAnywhere, Category = "Optimization")
    float SpatialGridCellSize = 200.0f;
    
    UPROPERTY(EditAnywhere, Category = "Optimization")
    int32 MaxCandidatesPerFrame = 20;
    
    UPROPERTY(EditAnywhere, Category = "Optimization")
    bool bUseSpatialPartitioning = true;

    UPROPERTY(BlueprintAssignable, Category="Events") 
    FOnNewInteractableAssigned OnNewInteractableAssigned;
    
    UPROPERTY(BlueprintAssignable, Category="Events") 
    FOnRemoveCurrentInteractable OnRemoveCurrentInteractable;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // Core system
    UPROPERTY() 
    TObjectPtr<APHPlayerController> OwnerController;
    
    UPROPERTY(BlueprintReadWrite) 
    TObjectPtr<UInteractableManager> CurrentInteractable;

    // Spatial optimization
    FSpatialGrid SpatialGrid;
    
    // Keep the original InteractableList for fallback
    UPROPERTY(BlueprintReadOnly) 
    TArray<UInteractableManager*> InteractableList;
    
    // Caching for performance
    FVector CachedPlayerLocation;
    FVector CachedLookDirection;
    FVector LastLookDirection;

    UPROPERTY()
    TObjectPtr<APlayerCameraManager> CachedCamera;
    
    // Update management
    FTimerHandle UpdateTimerHandle;
    int32 LastListVersion = 0;
    int32 CurrentListVersion = 0;
    
    // Candidate caching to reduce allocations
    UPROPERTY()
    TArray<FInteractionCandidate> CandidateCache;

    UPROPERTY()
    TArray<UInteractableManager*> NearbyCache;

    void ScheduleNextUpdate();
    void UpdateInteractionOptimized();
    void CachePlayerData();
    bool ShouldUpdateInteraction();
    void AssignCurrentInteraction();

public:
    // Public interface
    UFUNCTION(BlueprintCallable, Category = "Interaction")
    UInteractableManager* GetCurrentInteractable() const { return CurrentInteractable; }

    UFUNCTION(BlueprintCallable, Category = "InteractionControl")
    void SetCurrentInteraction(UInteractableManager* NewInteractable);

    UFUNCTION(BlueprintCallable, Category = "InteractionControl")
    void AddToInteractionList(UInteractableManager* InInteractable);

    UFUNCTION(BlueprintCallable, Category = "InteractionControl")
    void RemoveFromInteractionList(UInteractableManager* InInteractable);
    
    UFUNCTION(BlueprintCallable, Category = "InteractionControl")
    void UpdateInteractableLocation(UInteractableManager* Interactable, const FVector& OldLocation);

    UFUNCTION(BlueprintCallable, Category = "Interaction")
    void ToggleHighlight(bool bShouldHighlight) const;

    // Utility functions
    UFUNCTION(BlueprintCallable, Category = "Math Utils")
    static void MaxOfFloatArray(const TArray<float>& FloatArray, float& OutMaxValue, int32& OutMaxIndex);
    
    bool IsValidForInteraction() const;

private:
    void RemoveInteractionFromCurrent(UInteractableManager* Interactable);
    void AddInteraction(UInteractableManager* Interactable);
    
    // Optimized scoring
    float CalculateInteractionScore(const FInteractionCandidate& Candidate) const;
    
    FInteractionCandidate CreateCandidate(UInteractableManager* Interactable) const;
};