// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Character/ALSCharacter.h"
#include "Combat/Library/CombatStruct.h"
#include "PHBaseCharacter.generated.h"

struct FGameplayAbilitySpecHandle;
// Forward declarations
class UAbilitySystemComponent;
class UHunterAttributeSet;
class UHunterAbilitySystemComponent;
class UCharacterProgressionManager;
class UEquipmentManager;
class UStatsManager;
class UTagManager;
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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDeath, APHBaseCharacter*, DeadCharacter,AActor*, KillerActor); 

UCLASS(Abstract)
class ALS_PROJECTHUNTER_API APHBaseCharacter : public AALSCharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APHBaseCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void OnRep_Controller() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void SprintAction_Implementation(bool bValue) override;
	UPROPERTY(BlueprintAssignable, Category="Combat")
	FOnCombatAffiliationChanged OnCombatAffiliationChanged;
	
	UPROPERTY(BlueprintAssignable, Category="Combat")
	FOnDeath OnDeathEvent;
	
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
	 * Base stats data asset
	 * Defines starting attribute values for this character
	 * Set in subclasses or Blueprint (e.g., DA_WarriorBaseStats, DA_GoblinBaseStats)
	 */
	/*UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	TObjectPtr<UBaseStatsData> BaseStatsData;*/

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* ABILITY SYSTEM INTERFACE */
	/* ═══════════════════════════════════════════════════════════════════════ */

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override
	{
		return AbilitySystemComponent;
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

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* CHARACTER INFO */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Character display name */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Character")
	FText CharacterName;

	/** Character level (cached from ProgressionManager for convenience) */
	UPROPERTY(BlueprintReadOnly, Category = "Character")
	int32 CachedLevel = 1;

	/** Is this character a player? */
	UFUNCTION(BlueprintPure, Category = "Character")
	virtual bool IsPlayer() const { return false; }

	/** Is this character an NPC? */
	UFUNCTION(BlueprintPure, Category = "Character")
	virtual bool IsNPC() const { return false; }
	
	UFUNCTION(BlueprintPure, Category = "Character")
	FCombatAffiliation GetCombatAffiliation() const { return CombatAffiliation; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Character")
	void SetCombatAffiliation(const FCombatAffiliation& NewAffiliation);

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* PROGRESSION (Shared System) */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Get current level */
	UFUNCTION(BlueprintPure, Category = "Progression")
	int32 GetCharacterLevel() const;

	/** Get base XP reward (for when this character is killed) */
	UFUNCTION(BlueprintPure, Category = "Progression")
	virtual int64 GetXPReward() const;

	/** Award XP to this character from killing another character */
	UFUNCTION(BlueprintCallable, Category = "Progression", BlueprintAuthorityOnly)
	void AwardExperienceFromKill(APHBaseCharacter* KilledCharacter);

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* COMBAT & HEALTH */
	/* ═══════════════════════════════════════════════════════════════════════ */

	UPROPERTY(ReplicatedUsing=OnRep_IsDead, BlueprintReadOnly, Category="Combat")
	bool bIsDead = false;

	UFUNCTION()
	void OnRep_IsDead();

	/** Get current health (delegates to StatsManager) */
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetHealth() const;

	/** Get max health (delegates to StatsManager) */
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetMaxHealth() const;

	/** Get health percent (delegates to StatsManager) */
	UFUNCTION(BlueprintPure, Category = "Combat")
	float GetHealthPercent() const;

	/** Called when this character dies */
	UFUNCTION(BlueprintNativeEvent, Category = "Combat")
	void OnDeath(AController* Killer, AActor* DamageCauser);
	virtual void OnDeath_Implementation(AController* Killer, AActor* DamageCauser);

	/** 
	 * Called when health changes
	 */
	virtual void HandleHealthChanged(const FOnAttributeChangeData& Data);

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* ABILITIES */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Abilities granted to this character on spawn */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
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
	/* TEAM & TARGETING */
	/* ═══════════════════════════════════════════════════════════════════════ */

	/** Team ID (for friendly fire, targeting, etc.) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Team")
	uint8 TeamID = 0;

	UFUNCTION(BlueprintPure, Category = "Team")
	uint8 GetTeamID() const { return TeamID; }

	UFUNCTION(BlueprintPure, Category = "Team")
	bool IsSameTeam(const APHBaseCharacter* OtherCharacter) const;

	UFUNCTION(BlueprintPure, Category = "Team")
	bool IsHostile(const APHBaseCharacter* OtherCharacter) const;

	/* ═══════════════════════════════════════════════════════════════════════ */
	/* VISUAL & ANIMATION */
	/* ═══════════════════════════════════════════════════════════════════════ */

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Animation")
	TObjectPtr<UAnimMontage> HitReactMontage;

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void PlayDeathAnimation();

	UFUNCTION(BlueprintCallable, Category = "Animation")
	void PlayHitReactAnimation();

protected:
	/* ═══════════════════════════════════════════════════════════════════════ */
	/* INITIALIZATION */
	/* ═══════════════════════════════════════════════════════════════════════ */

	virtual void InitializeAbilitySystem();
	virtual void InitializeAttributes();
	virtual void BindAttributeDelegates();
	virtual void OnAbilitySystemInitialized();
	bool EnsureAttributeSetRegisteredWithAbilitySystem();
	
	UPROPERTY(Replicated)
	TObjectPtr<AActor> LastKillerActor = nullptr;

	UPROPERTY()
	bool bAbilitySystemInitialized = false;
	
	UPROPERTY(ReplicatedUsing=OnRep_CombatAffiliation)
	FCombatAffiliation CombatAffiliation;
	
	UFUNCTION()
	void OnRep_CombatAffiliation(FCombatAffiliation OldAffiliation);

	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> GrantedAbilityHandles;
};
