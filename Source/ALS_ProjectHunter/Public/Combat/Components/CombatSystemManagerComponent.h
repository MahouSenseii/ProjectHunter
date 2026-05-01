// ProjectHunter combat front door. Routes combat intent without owning combat math.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Library/CombatStructs.h"
#include "CombatSystemManagerComponent.generated.h"

class UCombatManager;
class UCombatStatusManager;

DECLARE_LOG_CATEGORY_EXTERN(LogCombatSystemManager, Log, All);

UCLASS(ClassGroup=(ProjectHunter), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UCombatSystemManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatSystemManagerComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Combat")
	UCombatManager* GetCombatManager() const { return CombatManager; }

	UFUNCTION(BlueprintPure, Category = "Combat|Status")
	UCombatStatusManager* GetCombatStatusManager() const { return CombatStatusManager; }

	UFUNCTION(BlueprintPure, Category = "Combat|Status")
	UCombatStatusManager* GetStatusManager() const { return GetCombatStatusManager(); }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool ApplyHit(AActor* AttackerActor, AActor* DefenderActor,
		const FCombatHitPacket& HitPacket, FCombatResolveResult& OutResult);

	UFUNCTION(BlueprintCallable, Category = "Combat|Status")
	void CleanseAll(AActor* Target);

private:
	void CacheCombatManagers();

	UPROPERTY(Transient)
	TObjectPtr<UCombatManager> CombatManager = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCombatStatusManager> CombatStatusManager = nullptr;
};
