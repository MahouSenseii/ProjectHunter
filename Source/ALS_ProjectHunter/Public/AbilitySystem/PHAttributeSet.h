// Copyright@2024 Quentin Davis

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "Library/AttributeStructsLibrary.h"
#include "Library/PHCharacterEnumLibrary.h"
#include "PHAttributeSet.generated.h"


	#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeChangeSignature, float, NewValue);

template<class T>
using TStaticFuncPtr = TBaseStaticDelegateInstance<FGameplayAttribute(), FDefaultTSDelegateUserPolicy>::FFuncPtr;

/**
 * 
 */
UCLASS()
class ALS_PROJECTHUNTER_API UPHAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	
	UPHAttributeSet();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category = "Combat Alignment")
	ECombatAlignment GetCombatAlignmentBP() const;

	UFUNCTION(BlueprintCallable, Category = "Combat Alignment")
	void SetCombatAlignmentBP(ECombatAlignment NewAlignment);

	UPROPERTY(BlueprintAssignable, Category = "Combat Alignment")
	FOnAttributeChangeSignature OnCombatAlignmentChange;
	
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	TMap<FGameplayTag, TStaticFuncPtr<FGameplayAttribute()>> TagsToAttributes;

	UPROPERTY(BlueprintReadOnly)
	TMap<FGameplayTag, FGameplayTag> TagsMinMax;


	/*
	 *Combat Indecatorss 
	 */

	UPROPERTY(BlueprintReadOnly, Category = "Combat Alignment", ReplicatedUsing = OnRep_CombatAlignment)
	FGameplayAttributeData CombatAlignment;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, CombatAlignment)
 
	/*
	 * Primary Attributes 
	 */
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Strength, Category = "Primary Attribute")
	FGameplayAttributeData Strength;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Strength); //+5 max health, 2% physical damage.

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Intelligence, Category = "Primary Attribute")
	FGameplayAttributeData Intelligence;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Intelligence); // +5 mana , 1.3% elemental damage.

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Dexterity, Category = "Primary Attribute")
	FGameplayAttributeData Dexterity;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Dexterity); // +.5 Crit multiplier , +.5 Attack/Cast Speed

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Endurance, Category = "Primary Attribute")
	FGameplayAttributeData Endurance;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Endurance); // +10 Stamina  +.01 Resistance;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Affliction, Category = "Primary Attribute")
	FGameplayAttributeData Affliction;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Affliction); 

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Luck, Category = "Primary Attribute")
	FGameplayAttributeData Luck;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Luck);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Covenant, Category = "Primary Attribute")
	FGameplayAttributeData Covenant;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Covenant);


	/*
	 * Secondary Attribute
	 */

	//Gems
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Gems, Category = "Vital Attribute|Gems")
	FGameplayAttributeData Gems;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Gems);
	
	/*
	 * Vital Attributes 
	 */
	
	// Health
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Health, Category = "Vital Attribute|Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Health);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxHealth);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData  MaxEffectiveHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxEffectiveHealth);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_HealthRegenRate, Category = "Vital Attribute|Health")
	FGameplayAttributeData HealthRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, HealthRegenRate);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxHealthRegenRate, Category = "Vital Attribute|Health")
	FGameplayAttributeData MaxHealthRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxHealthRegenRate);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_HealthRegenAmount, Category = "Vital Attribute|Health")
	FGameplayAttributeData HealthRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, HealthRegenAmount);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxHealthRegenAmount, Category = "Vital Attribute|Health")
	FGameplayAttributeData MaxHealthRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxHealthRegenAmount);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData ReservedHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ReservedHealth);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData MaxReservedHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxReservedHealth);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData FlatReservedHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FlatReservedHealth);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing =  OnRep_PercentageReservedHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData PercentageReservedHealth;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PercentageReservedHealth);
	
	//Stamina
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Stamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Stamina);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxStamina);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveStamina, Category = "Vital Attribute|Health")
	FGameplayAttributeData  MaxEffectiveStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxEffectiveStamina);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaRegenRate, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData StaminaRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, StaminaRegenRate);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxStaminaRegenRate, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData MaxStaminaRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxStaminaRegenRate);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_StaminaRegenAmount, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData StaminaRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, StaminaRegenAmount);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxStaminaRegenAmount, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData MaxStaminaRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxStaminaRegenAmount);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData ReservedStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ReservedStamina);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData MaxReservedStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxReservedStamina);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData FlatReservedStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FlatReservedStamina);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing =  OnRep_PercentageReservedStamina, Category = "Vital Attribute|Stamina")
	FGameplayAttributeData PercentageReservedStamina;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PercentageReservedStamina);

	//Mana
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Mana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, Mana);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxMana);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxEffectiveHealth, Category = "Vital Attribute|Health")
	FGameplayAttributeData  MaxEffectiveMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxEffectiveMana);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaRegenRate, Category = "Vital Attribute|Mana")
	FGameplayAttributeData ManaRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ManaRegenRate);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxManaRegenRate, Category = "Vital Attribute|Mana")
	FGameplayAttributeData MaxManaRegenRate;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxManaRegenRate);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ManaRegenAmount, Category = "Vital Attribute|Mana")
	FGameplayAttributeData ManaRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ManaRegenAmount);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxManaRegenAmount, Category = "Vital Attribute|Mana")
	FGameplayAttributeData MaxManaRegenAmount;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxManaRegenAmount);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_ReservedMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData ReservedMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, ReservedMana);

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_MaxReservedMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData MaxReservedMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, MaxReservedMana);
	
	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_FlatReservedMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData FlatReservedMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, FlatReservedMana);


	UPROPERTY(BlueprintReadWrite, ReplicatedUsing =  OnRep_PercentageReservedMana, Category = "Vital Attribute|Mana")
	FGameplayAttributeData PercentageReservedMana;
	ATTRIBUTE_ACCESSORS(UPHAttributeSet, PercentageReservedMana);

	/**
	 * Meta Attributes
	 */

	/*
	 *Combat Indicators
	 */

	UFUNCTION()
	void OnRep_CombatAlignment(const FGameplayAttributeData& OldCombatAlignment) const;

	
	// Health 
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldHealth)const ;

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData&  OldMaxHealth)const;

	UFUNCTION()
	void OnRep_MaxEffectiveHealth(const FGameplayAttributeData& OldAmount)const;

	UFUNCTION()
	void OnRep_HealthRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxHealthRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_HealthRegenAmount(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxHealthRegenAmount(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_ReservedHealth(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxReservedHealth(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_FlatReservedHealth(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_PercentageReservedHealth(const FGameplayAttributeData& OldAmount) const;
	
	// Stamina
	UFUNCTION()
	void OnRep_Stamina(const FGameplayAttributeData& OldStamina)const ;

	UFUNCTION()
	void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)const ;

	UFUNCTION()
	void OnRep_MaxEffectiveStamina(const FGameplayAttributeData& OldAmount)const;

	UFUNCTION()
	void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxStaminaRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_StaminaRegenAmount(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxStaminaRegenAmount(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_ReservedStamina(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxReservedStamina(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_FlatReservedStamina(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_PercentageReservedStamina(const FGameplayAttributeData& OldAmount) const;
	

	//Mana
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldMana)const ;

	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana)const ;

	UFUNCTION()
	void OnRep_MaxEffectiveMana(const FGameplayAttributeData& OldAmount)const;

	UFUNCTION()
	void OnRep_ManaRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxManaRegenRate(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_ManaRegenAmount(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxManaRegenAmount(const FGameplayAttributeData& OldAmount) const;
	
	UFUNCTION()
	void OnRep_ReservedMana(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_MaxReservedMana(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_FlatReservedMana(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_PercentageReservedMana(const FGameplayAttributeData& OldAmount) const;
	
	
	UFUNCTION()
	void OnRep_Gems(const FGameplayAttributeData& OldGems)const ;
	
	UFUNCTION()
	void OnRep_Strength(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_Intelligence(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_Dexterity(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_Endurance(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_Affliction(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_Luck(const FGameplayAttributeData& OldAmount) const;

	UFUNCTION()
	void OnRep_Covenant(const FGameplayAttributeData& OldAmount) const;


private:
	static void SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props);
	
	void SetAttributeValue(const FGameplayAttribute& Attribute, float NewValue);
};
