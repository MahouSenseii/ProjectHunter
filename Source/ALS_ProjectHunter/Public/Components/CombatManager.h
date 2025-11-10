// CombatManager.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Library/PHCombatStructLibrary.h"
#include "Library/PHItemEnumLibrary.h"
#include "Library/PHCharacterEnumLibrary.h"
#include "CombatManager.generated.h"

class APHBaseCharacter;
class UPHAttributeSet;
class UDamagePopup;
class UWidgetComponent;
class UAnimMontage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnDamageApplied, float, DamageAmount, 
    EDamageTypes, DamageType, bool, bIsCriticalHit);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnCombatStatusChanged, ECombatStatus, OldStatus, 
    ECombatStatus, NewStatus);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatEntered);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCombatExited);

DECLARE_LOG_CATEGORY_EXTERN(LogCombatManager, Log, All);

/**
 * Combat Manager Component - Path of Exile 2 Style Combat System
 * 
 * Damage Calculation Order (following PoE2):
 * 1. Base Damage from weapon/spell
 * 2. Damage Conversions (Physical -> Elemental -> Chaos)
 * 3. Added Damage (flat bonuses)
 * 4. Increased/More Damage multipliers
 * 5. Critical Strike multipliers
 * 6. Enemy Defenses (Armor, Resistances, Block)
 * 7. Damage Taken modifiers on enemy
 * 
 * Features:
 * - Complete damage conversion system
 * - Life/Mana leech
 * - Damage reflection and thorns
 * - Status effect application
 * - Resistance penetration
 * - Blocking and poise system
 * - Combat status tracking and management
 * - Out-of-combat regeneration
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UCombatManager : public UActorComponent
{
    GENERATED_BODY()

public:
    UCombatManager();
    
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, 
                              FActorComponentTickFunction* ThisTickFunction) override;


    UFUNCTION(BlueprintCallable, Category = "Combat|Setup")
    APHBaseCharacter* GetOwnerCharacter() const {return OwnerCharacter; }
    
protected:
    virtual void BeginPlay() override;

    /* ================================ */
    /* === DAMAGE CALCULATION ========= */
    /* ================================ */
    
    /**
     * Main damage calculation function following PoE2 mechanics
     * @param Attacker - Character dealing damage
     * @param Defender - Character receiving damage
     * @return Complete damage breakdown by type with crit and status info
     */
    UFUNCTION(BlueprintCallable, Category = "Combat|Calculation")
    FDamageHitResultByType CalculateDamage(const APHBaseCharacter* Attacker, 
                                          const APHBaseCharacter* Defender) const;
    
    /**
     * Get base damage ranges from attacker's attributes
     */
    static FDamageByType GetBaseDamage(const UPHAttributeSet* AttAtt);
    
    /**
     * Apply damage conversion mechanics (Physical -> Elemental -> Chaos)
     * Conversions are capped at 100% per damage type
     */
    static FDamageByType ApplyDamageConversions(const FDamageByType& InDamage, 
                                                const UPHAttributeSet* AttAtt);
    
    /**
     * Roll random values within damage ranges
     */
    static TMap<EDamageTypes, float> RollDamageValues(const FDamageByType& DamageRanges);
    
    /**
     * Apply damage increases, more multipliers, and conditional bonuses
     */
    static TMap<EDamageTypes, float> ApplyDamageModifiers(const TMap<EDamageTypes, float>& RolledDamage,
                                                          const UPHAttributeSet* AttAtt);
    
    /**
     * Apply defender's resistances, armor, and other defenses
     * Includes penetration mechanics
     */
    static TMap<EDamageTypes, float> ApplyDefenses(const TMap<EDamageTypes, float>& IncomingDamage,
                                                   const UPHAttributeSet* AttAtt,
                                                   const UPHAttributeSet* DefAtt);
    
    /**
     * Roll for critical strike
     */
    static bool RollCriticalStrike(const UPHAttributeSet* AttAtt);
    
    /**
     * Roll for status effect application per damage type
     */
    static TMap<EDamageTypes, bool> RollStatusEffects(const UPHAttributeSet* AttAtt,
                                                      const TMap<EDamageTypes, float>& FinalDamage);

    /* ================================ */
    /* === DAMAGE APPLICATION ========= */
    /* ================================ */
    
    /**
     * Apply calculated damage to defender
     * Handles blocking, leech, reflection, and status effects
     */
    UFUNCTION(BlueprintCallable, Category = "Combat|Application")
    void ApplyDamage(const APHBaseCharacter* Attacker, APHBaseCharacter* Defender);
    
    /**
     * Apply life and mana leech to attacker
     */
    static void ApplyLeech(const APHBaseCharacter* Attacker, float DamageDealt);
    
    /**
     * Process damage reflection and thorn damage
     */
    static void ProcessReflectionAndThorns(const APHBaseCharacter* Attacker,
                                           const APHBaseCharacter* Defender,
                                           const FDamageHitResultByType& IncomingHit);
    
    /**
     * Apply status effects based on damage types
     */
    static void ApplyStatusEffects(const APHBaseCharacter* Target, 
                                   const FDamageHitResultByType& HitResult);

    /* ================================ */
    /* === UTILITY FUNCTIONS ========== */
    /* ================================ */
    
    /**
     * Check if the character should die and trigger a ragdoll
     */
    void CheckAliveStatus() const;
    
    /**
     * Get the damage type that dealt the most damage
     */
    static EDamageTypes GetHighestDamageType(const FDamageHitResultByType& HitResult);
    
    /**
     * Initialize damage popup widget
     */
    UFUNCTION()
    void InitPopup(float DamageAmount, EDamageTypes DamageType, bool bIsCrit);

    /* ================================ */
    /* === COMBO SYSTEM =============== */
    /* ================================ */
    
    UFUNCTION(BlueprintCallable, Category = "Combat|Combo")
    void IncreaseComboCounter(float Amount);
    
    void ResetComboCounter() const;

    /* ================================ */
    /* === BLOCKING =================== */
    /* ================================ */
    
    UFUNCTION(BlueprintPure, Category = "Combat|Defense")
    bool IsBlocking() const { return bIsBlocking; }
    
    UFUNCTION(BlueprintCallable, Category = "Combat|Defense")
    void SetBlocking(bool bNewBlocking) { bIsBlocking = bNewBlocking; }
    
    /* ================================ */
    /* === COMBAT STATUS ============== */
    /* ================================ */
    
    UFUNCTION(BlueprintPure, Category = "Combat|Status")
    ECombatStatus GetCombatStatus() const { return CurrentCombatStatus; }
    
    UFUNCTION(BlueprintPure, Category = "Combat|Status")
    bool IsInCombat() const { return CurrentCombatStatus == ECombatStatus::InCombat; }
    
    UFUNCTION(BlueprintCallable, Category = "Combat|Status")
    void SetCombatStatus(ECombatStatus NewStatus);
    
    UFUNCTION(BlueprintCallable, Category = "Combat|Status")
    void EnterCombat(APHBaseCharacter* Enemy);
    
    UFUNCTION(BlueprintCallable, Category = "Combat|Status")
    void ExitCombat();
    
    UFUNCTION(BlueprintCallable, Category = "Combat|Status")
    void ForceCombatExit();
    
    UFUNCTION(BlueprintPure, Category = "Combat|Status")
    bool IsInCombatWith(const APHBaseCharacter* Target) const;
    
    UFUNCTION(BlueprintPure, Category = "Combat|Status")
    TArray<APHBaseCharacter*> GetActiveCombatTargets() const;
    
    UFUNCTION(BlueprintPure, Category = "Combat|Status")
    float GetTimeSinceLastCombatActivity() const 
    { 
        return GetWorld() ? GetWorld()->GetTimeSeconds() - LastCombatActivityTime : 0.f; 
    }
    
protected:
    void RefreshCombatTimer();
    void CheckCombatTimeout();
    void OnEnterCombat();
    void OnExitCombat();

public:
    /* ================================ */
    /* === EVENTS ===================== */
    /* ================================ */
    
    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnDamageApplied OnDamageApplied;
    
    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnCombatStatusChanged OnCombatStatusChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnCombatEntered OnCombatEntered;
    
    UPROPERTY(BlueprintAssignable, Category = "Combat|Events")
    FOnCombatExited OnCombatExited;

protected:
    /* ================================ */
    /* === PROPERTIES ================= */
    /* ================================ */
    
    UPROPERTY()
    APHBaseCharacter* OwnerCharacter;
    
    UPROPERTY(EditDefaultsOnly, Category = "Combat|UI")
    TSubclassOf<UDamagePopup> DamagePopupClass;
    
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Animation")
    UAnimMontage* StaggerMontage;
    
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Combo")
    float ComboResetTime = 2.0f;
    
    /* === Combat Status Properties === */
    
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat|Status")
    ECombatStatus CurrentCombatStatus = ECombatStatus::OutOfCombat;
    
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Status")
    float CombatTimeout = 5.0f; // Time before exiting combat after the last activity
    
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Status")
    float MaxCombatDistance = 3000.0f; // Max distance to maintain combat with a target
    
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Status")
    bool bUseTransitions = false; // Whether to use entering/leaving combat transitions
    
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Status", meta = (EditCondition = "bUseTransitions"))
    float CombatEnterTransitionTime = 0.5f;
    
    UPROPERTY(EditDefaultsOnly, Category = "Combat|Status", meta = (EditCondition = "bUseTransitions"))
    float CombatExitTransitionTime = 1.0f;
    

    
    /* === Runtime Properties === */
    
    UPROPERTY()
    TArray<APHBaseCharacter*> CombatTargets;
    
    float LastCombatActivityTime = 0.f;
    bool bCanRegenerate = true;
    
    FTimerHandle ComboResetTimerHandle;
    FTimerHandle CombatExitTimer;
    FTimerHandle CombatTransitionTimer;
    FTimerHandle RegenerationDelayTimer;
    
    bool bIsBlocking = false;
    
    // Track hits for NPC hostility (optional feature from original code)
    int32 TimesHitBeforeEnemy = 0;
};