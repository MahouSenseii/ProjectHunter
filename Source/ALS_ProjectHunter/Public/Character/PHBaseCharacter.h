// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Core/Logging/ProjectHunterLogMacros.h"
#include "Character/ALSCharacter.h"
#include "Character/Library/PHCharacterLog.h"
#include "Combat/Library/CombatStructs.h"
#include "AbilitySystem/HunterAbilitySystemComponent.h"
#include "AbilitySystem/PHAbilitySet.h"
#include "PHBaseCharacter.generated.h"

struct FGameplayAbilitySpecHandle;
// Forward declarations
class UAbilitySystemComponent;
class UHunterAttributeSet;
class UHunterAbilitySystemComponent;
class UCharacterProgressionManager;
class UCombatManager;
class UCombatSystemManagerComponent;
class UEquipmentManager;
class UEquipmentPresentationComponent;
class UCharacterSystemCoordinatorComponent;
class UStatsManager;
class UTagManager;
class UCombatStatusManager;
class UBaseStatsData;
class UGameplayEffect;
class UGameplayAbility;
struct FOnAttributeChangeData; // Add this forward declaration

/**
 * Base character class shared by players and NPCs
 * Contains all core combat systems, attributes, and progression
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCombatAffiliationChanged, const FCombatAffiliation&, NewAffiliation);

// If APHBaseCharacter please cass for information 
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeath, APHBaseCharacter*, DeadCharacter, AActor*, KillerActor);

UCLASS(Abstract)
class ALS_PROJECTHUNTER_API APHBaseCharacter : public AALSCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APHBaseCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void SprintAction_Implementation(bool bValue) override;
	virtual void OnRep_PlayerState() override;
	virtual void OnRep_Controller() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/* ═══════════════════════════════════════════════════════════════════════ */
	/* CORE COMPONENTS (Shared by Players and NPCs) */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Ability System Component - Handles all GAS functionality */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	/** Main attribute set - All combat stats (Health, Damage, Resistances, etc.) */
	UPROPERTY()
	TObjectPtr<UHunterAttributeSet> AttributeSet;

	/** Progression Manager - XP, Level, Stat Points */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Progression", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterProgressionManager> ProgressionManager;

	/** Equipment Component - Items, gear, inventory */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEquipmentManager> EquipmentManager;

	/** Stats Manager - All stat queries and calculations */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStatsManager> StatsManager;

	/** Tag Manager - Central runtime gameplay tag state for this character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tags", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTagManager> TagManager;
	

	/**
	 * Combat Manager — resolves all hit/damage interactions for this character.
	 * Weapons (and fists) report hits here; the manager owns the damage pipeline.
	 * Configure DamageApplicationGE and RecoveryApplicationGE in Blueprint defaults.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatManager> CombatManager;

	/**
	 * Combat Status Manager - applies and tracks DoTs, statuses, buffs, debuffs,
	 * cleanses, and GAS-backed status queries.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatStatusManager> CombatStatusManager;

	/**
	 * Combat System Manager - front door for Blueprint and C++ combat calls.
	 * It routes hit intent to CombatManager and status calls to CombatStatusManager.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCombatSystemManagerComponent> CombatSystemManager;

	/**
	 * Equipment Presentation Component — owns visual weapon actors and mesh
	 * attachment. Bound to EquipmentManager::OnEquipmentChanged by the coordinator.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UEquipmentPresentationComponent> EquipmentPresentation;

	/**
	 * Character System Coordinator — single point of cross-system listener wiring.
	 * PH-0.4: APHBaseCharacter is a composition root only; orchestration lives here.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Coordinator", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCharacterSystemCoordinatorComponent> SystemCoordinator;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* ABILITY SYSTEM INTERFACE */
	/* ═══════════════════════════════════════════════════════════════════════ */

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{
		// Fast path: member pointer is valid (normal case).
		if (AbilitySystemComponent)
		{
			return AbilitySystemComponent;
		}

		// Safety net: the member was cleared, which happens when a Blueprint child
		// class adds a component with the same display name as the C++ CreateDefaultSubobject
		// ("AbilitySystemComponent"). UE5 evicts the C++ component from the TObjectPtr
		// during Blueprint component reconciliation, but the component object itself
		// survives on the actor. FindComponentByClass recovers it.
		//
		// ROOT FIX: open the Blueprint's Components panel, right-click the "Ability
		// System Component" entry, and if Delete is available it is Blueprint-added
		// and must be removed. The C++ CreateDefaultSubobject is the authoritative one.
		UHunterAbilitySystemComponent* FoundASC = FindComponentByClass<UHunterAbilitySystemComponent>();

		PH_LOG_WARNING(LogPHBaseCharacter, "GetAbilitySystemComponent recovered a null member pointer for Character=%s via ASC=%s. A Blueprint child class likely has a duplicate Ability System Component in its Components panel.", *GetName(), *GetNameSafe(FoundASC));

		return FoundASC;
	}

	UFUNCTION(BlueprintPure, Category = "Abilities")
	UHunterAttributeSet* GetAttributeSet() const
	{
		return AttributeSet;
	}

	UFUNCTION(BlueprintPure, Category = "Progression")
	UCharacterProgressionManager* GetProgressionManager() const
	{
		return ProgressionManager;
	}

	UFUNCTION(BlueprintPure, Category = "Equipment")
	UEquipmentManager* GetEquipmentManager() const
	{
		return EquipmentManager;
	}
	
	

	UFUNCTION(BlueprintPure, Category = "Stats")
	UStatsManager* GetStatsManager() const
	{
		return StatsManager;
	}

	UFUNCTION(BlueprintPure, Category = "Tags")
	UTagManager* GetTagManager() const
	{
		return TagManager;
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	UCombatManager* GetCombatManager() const
	{
		return CombatManager;
	}

	UFUNCTION(BlueprintPure, Category = "Combat")
	UCombatSystemManagerComponent* GetCombatSystemManager() const
	{
		return CombatSystemManager;
	}

	UFUNCTION(BlueprintPure, Category = "Combat|Status")
	UCombatStatusManager* GetCombatStatusManager() const
	{
		return CombatStatusManager;
	}

	UFUNCTION(BlueprintPure, Category = "Combat|Status", meta = (DeprecatedFunction, DeprecationMessage = "Use GetCombatStatusManager."))
	UCombatStatusManager* GetDoTManager() const
	{
		return GetCombatStatusManager();
	}

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* CHARACTER INFO */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Character level (cached from ProgressionManager for convenience) */
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	int32 CachedLevel = 1;

	/** Is this character a player? */
	UFUNCTION(BlueprintPure, Category = "Character")
	virtual bool IsPlayer() const { return false; }

	/** Is this character an NPC? */
	UFUNCTION(BlueprintPure, Category = "Character")
	virtual bool IsNPC() const { return false; }


	/* ═══════════════════════════════════════════════════════════════════════ */
	/* DEATH (Blueprint-driven) */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/**
	 * Broadcast exactly once when this character dies.
	 *
	 * Blueprint death logic stays the orchestrator: when your BP decides the
	 * character is dead (ragdoll, montage, loot, etc.), call NotifyDeath(Killer)
	 * as one extra node. That single call is what lets C++ systems react —
	 * AMobManagerActor binds here for kill counts, OnMobDied, and pool recycling.
	 *
	 * Do not broadcast this delegate directly; NotifyDeath guards against
	 * double-fire and enforces server authority.
	 */
	UPROPERTY(BlueprintAssignable, Category = "Character|Death")
	FOnDeath OnDeath;

	/**
	 * Single entry point for declaring this character dead.
	 * Call from your Blueprint death event (server side). Safe to call multiple
	 * times — only the first call broadcasts OnDeath.
	 * @param Killer  The actor credited with the kill (may be null).
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Death", BlueprintAuthorityOnly)
	virtual void NotifyDeath(AActor* Killer);

	/** True once NotifyDeath has fired (until ResetDeathState, e.g. pool reuse). */
	UFUNCTION(BlueprintPure, Category = "Character|Death")
	bool IsDead() const { return bHasDied; }

	/**
	 * Clears the death latch so a pooled/recycled actor can die again.
	 * Called by AMobManagerActor::FinalizeSpawn and UMobPoolSubsystem::ResetMobState.
	 */
	UFUNCTION(BlueprintCallable, Category = "Character|Death")
	virtual void ResetDeathState() { bHasDied = false; }

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* PROGRESSION (Shared System) */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Get current level */
	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCharacterLevel() const;

	/** Get base XP reward (for when this character is killed) */
	UFUNCTION(BlueprintPure, Category = "Progression")
	virtual int64 GetXPReward() const;

	/** Multiplier applied to the base XP reward calculation. Override in subclasses or set in Blueprint. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Progression", meta = (ClampMin = "0.0"))
	float XPRewardMultiplier = 1.0f;

	/** Award XP to this character from killing another character */
	UFUNCTION(BlueprintCallable, Category = "Progression", BlueprintAuthorityOnly)
	void AwardExperienceFromKill(APHBaseCharacter* KilledCharacter);

	/** Get current health (delegates to StatsManager) */
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetHealth() const;

	/** Get max health (delegates to StatsManager) */
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetMaxHealth() const;

	/** Get health percent (delegates to StatsManager) */
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetHealthPercent() const;
	

	/** 
	 * Called when health changes
	 */
	virtual void HandleHealthChanged(const FOnAttributeChangeData& Data);

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* ABILITIES */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Lyra-style ability sets granted to this character on first ASC initialization. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TObjectPtr<UPHAbilitySet>> DefaultAbilitySets;

	/** Legacy direct ability grants. Prefer DefaultAbilitySets for new abilities. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities|Legacy")
	TArray<TSubclassOf<UGameplayAbility>> DefaultAbilities;

	/** Startup effects applied on spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayEffect>> StartupEffects;

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void GiveDefaultAbilities();

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void ApplyStartupEffects();

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void RemoveAllAbilities();
	

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* INITIALIZATION (public so MobPoolSubsystem can reinitialize recycled actors) */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Reinitialize base attributes from the data asset / startup GEs. */
	virtual void InitializeAttributes();

protected:
	virtual void InitializeAbilitySystem();
	virtual void BindAttributeDelegates();
	virtual void OnAbilitySystemInitialized();
	bool EnsureAttributeSetRegisteredWithAbilitySystem();

	UPROPERTY()
	bool bAbilitySystemInitialized = false;

	/** Death latch — set by NotifyDeath, cleared by ResetDeathState. */
	UPROPERTY(Transient)
	bool bHasDied = false;
	
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;

	UPROPERTY()
	TArray<FPHAbilitySet_GrantedHandles> GrantedAbilitySetHandles;
};
