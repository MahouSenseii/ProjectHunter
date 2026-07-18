// Compatibility shim for saved Blueprints that still serialize this component.
// New combat work should use UCombatManager directly.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Library/Structs/CombatStructs.h"
#include "CombatSystemManagerComponent.generated.h"

class UCombatManager;
class UCombatStatusEffectApplier;

DECLARE_LOG_CATEGORY_EXTERN(LogCombatSystemManager, Log, All);

UCLASS(ClassGroup=(ProjectHunter))
class ALS_PROJECTHUNTER_API UCombatSystemManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatSystemManagerComponent();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Combat")
	UCombatManager* GetCombatManager() const { return CombatManager; }

	UFUNCTION(BlueprintPure, Category = "Combat|Status")
	UCombatStatusEffectApplier* GetCombatStatusManager() const { return CombatStatusEffectApplier; }

	UFUNCTION(BlueprintPure, Category = "Combat|Status")
	UCombatStatusEffectApplier* GetStatusManager() const { return GetCombatStatusManager(); }

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool ApplyHit(AActor* AttackerActor, AActor* DefenderActor,
		const FAnimationDamageInfo& DamageInfo, FCombatResolveResult& OutResult,
		EHitResponse HitResponse = EHitResponse::Normal, bool bCanApplyAilments = true);

	UFUNCTION(BlueprintCallable, Category = "Combat|Status")
	void CleanseAll(AActor* Target);

private:
	void CacheCombatManagers();

	UPROPERTY(Transient)
	TObjectPtr<UCombatManager> CombatManager = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UCombatStatusEffectApplier> CombatStatusEffectApplier = nullptr;
};
