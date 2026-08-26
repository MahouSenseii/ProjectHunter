#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "Stats/Debug/StatsDebugManager.h"
#include "Stats/Library/Enums/StatsEnumLibrary.h"
#include "StatsManager.generated.h"

class FEquipmentStatsApplier;
class FStatsAttributeResolver;
class FStatsInitializer;
class UAbilitySystemComponent;
class UAttributeSet;
class UBaseStatsData;
class UGameplayEffect;
class UHunterAttributeSet;
class UItemInstance;
enum class EEquipmentSlot : uint8;
struct FGameplayAttribute;
struct FPHAttributeData;
struct FStatInitializationEntry;

DECLARE_LOG_CATEGORY_EXTERN(LogStatsManager, Log, All);

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UStatsManager : public UActorComponent
{
	GENERATED_BODY()

	friend class FEquipmentStatsApplier;
	friend class FStatsAttributeResolver;
	friend class FStatsInitializer;

public:
	UStatsManager();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void NotifyAbilitySystemReady();

	// Required for recycled actors that need their authored base stats applied again.
	UFUNCTION(BlueprintCallable, Category = "Stats")
	void ResetStatsInitialization();

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool HasInitializedStats() const { return bHasInitializedConfiguredStats; }

	UFUNCTION(BlueprintCallable, Category = "Stats|Equipment")
	void ApplyEquipmentStats(UItemInstance* Item);

	UFUNCTION(BlueprintCallable, Category = "Stats|Equipment")
	void RemoveEquipmentStats(UItemInstance* Item);

	UFUNCTION(BlueprintCallable, Category = "Stats|Equipment")
	void RefreshEquipmentStats();

	UFUNCTION()
	void HandleEquipmentChanged(EEquipmentSlot Slot, UItemInstance* NewItem, UItemInstance* OldItem);

	UFUNCTION(BlueprintPure, Category = "Stats|Equipment")
	bool HasEquipmentStatsApplied(UItemInstance* Item) const;

	UFUNCTION(BlueprintCallable, Category = "Stats|Effects")
	bool ApplyGameplayEffectToSelf(TSubclassOf<UGameplayEffect> EffectClass, float Level = 1.0f);

	UFUNCTION(BlueprintCallable, Category = "Stats|Effects")
	bool ApplyGameplayEffectToTarget(AActor* TargetActor, TSubclassOf<UGameplayEffect> EffectClass, float Level = 1.0f);

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetStrength() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetIntelligence() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetDexterity() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetEndurance() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetAffliction() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetLuck() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Primary")
	float GetCovenant() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Secondary")
	float GetMagicFind() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Secondary")
	float GetItemFind() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Secondary")
	float GetGoldFind() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Secondary")
	float GetExperienceBonus() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetHealthPercent() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetMana() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetMaxMana() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetManaPercent() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetStamina() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetMaxStamina() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Vitals")
	float GetStaminaPercent() const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetAttributeByType(EHunterAttribute AttributeType) const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	float GetAttributeByName(FName AttributeName) const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	bool MeetsStatRequirements(const TMap<FName, float>& Requirements) const;

	float GetAttributeValue(const FGameplayAttribute& Attribute) const;
	bool HasLiveAttribute(const FGameplayAttribute& Attribute) const;
	bool ResolveAttributeByName(FName AttributeName, FGameplayAttribute& OutAttribute) const;
	bool ResolveAttributeByName(FName AttributeName, FGameplayAttribute& OutAttribute, FStatInitializationEntry* OutDefinition) const;
	void GatherStatDefinitions(TArray<FStatInitializationEntry>& OutDefinitions) const;

	UFUNCTION(BlueprintPure, Category = "Stats")
	TSubclassOf<UAttributeSet> GetSourceAttributeSetClass() const;

	const UBaseStatsData* GetStatsDataAsset() const { return StatsData; }

	UFUNCTION(BlueprintCallable, Category = "Stats|Debug")
	void SetStatsDebugEnabled(bool bEnable);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats|Debug")
	FStatsDebugManager DebugManager;

	UFUNCTION(BlueprintPure, Category = "Stats|Power")
	float GetPowerLevel() const;

	UFUNCTION(BlueprintPure, Category = "Stats|Power")
	float GetPowerRatioAgainst(AActor* OtherActor) const;

	void InitializeFromDataAsset(UBaseStatsData* InStatsData);
	void InitializeFromMap(const TMap<FName, float>& StatsMap);
	void SetStatValue(FName AttributeName, float Value);

protected:
	UHunterAttributeSet* GetAttributeSet() const;
	UAbilitySystemComponent* GetAbilitySystemComponent() const;
	const UAttributeSet* GetLiveSourceAttributeSet(UAbilitySystemComponent* ASC, const UClass* DesiredClass, bool bLogIfMissing, FName AttributeName = NAME_None) const;
	bool HasExpectedLiveAttributeSet(bool bLogIfMissing, FName AttributeName = NAME_None) const;
	void RefreshCachedAbilitySystemState(const TCHAR* Context) const;
	bool TryInitializeConfiguredStats(const TCHAR* Context);
	void LogAbilitySystemState(const TCHAR* Context, UAbilitySystemComponent* ASC, const UAttributeSet* LiveAttributeSet) const;
	void LogWarningOnce(const FString& Key, const FString& Message) const;

	bool SetNumericAttributeByName(FName AttributeName, float Value, bool bAutoInitializeCurrentFromMax = true);
	bool TryGetStatValueForInitialization(const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName StatName, float& OutValue) const;
	bool ApplyStatIfPresent(const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName StatName, bool bAutoInitializeCurrentFromMax = true);
	bool ApplyCurrentVitalWithClamp(const UBaseStatsData* InStatsData, const TMap<FName, float>& StatsMap, FName CurrentStatName, FName MaxStatName, FName StarterPropertyName);
	FGameplayEffectSpecHandle CreateEquipmentEffect(UItemInstance* Item, const TArray<FPHAttributeData>& Stats);
	bool ApplyStatModifier(UGameplayEffect* Effect, const FPHAttributeData& Stat, const FGameplayAttribute& Attribute);

	UPROPERTY()
	mutable TObjectPtr<UHunterAttributeSet> CachedAttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats|Config")
	TObjectPtr<UBaseStatsData> StatsData;

	UPROPERTY()
	mutable TObjectPtr<UAbilitySystemComponent> CachedASC;

	bool bHasInitializedConfiguredStats = false;
	mutable TSet<FString> EmittedWarningKeys;

	// One aggregate numeric effect plus any authored complex/conditional effects.
	// This is transient runtime state; it intentionally is not serialized.
	TMap<FGuid, TArray<FActiveGameplayEffectHandle>> ActiveEquipmentEffects;

	/** One consolidated effect preserves true product ordering for all equipment More/Less modifiers. */
	FActiveGameplayEffectHandle ActiveEquipmentProductEffect;

	UPROPERTY()
	TMap<FGuid, TObjectPtr<UItemInstance>> ActiveEquipmentItems;
};
