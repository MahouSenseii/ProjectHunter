#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Combat/Library/CombatStruct.h"
#include "CombatManager.generated.h"

class AActor;
class UAbilitySystemComponent;
class UHunterAttributeSet;

DECLARE_LOG_CATEGORY_EXTERN(LogCombatManager, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALS_PROJECTHUNTER_API UCombatManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatManager();

	UFUNCTION(BlueprintCallable, Category = "Combat")
	bool ResolveHit(AActor* SourceActor, AActor* TargetActor, FCombatResolveResult& OutResult);

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FCombatDamagePacket BuildOutgoingDamagePacket(AActor* SourceActor, AActor* TargetActor) const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FCombatDamagePacket ApplyDamageConversion(const FCombatDamagePacket& InPacket, AActor* SourceActor) const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	FCombatResolveResult MitigateDamagePacket(const FCombatDamagePacket& InPacket, AActor* SourceActor, AActor* TargetActor) const;

	UFUNCTION(BlueprintCallable, Category = "Combat")
	void ApplyResolvedDamage(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const;

protected:
	static UAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor);
	static const UHunterAttributeSet* GetHunterAttributeSetFromActor(const AActor* Actor);

	FCombatDamagePacket BuildOutgoingDamagePacketFromAttributes(const UHunterAttributeSet* SourceAttributes, AActor* SourceActor, AActor* TargetActor) const;
	FCombatDamagePacket ApplyDamageConversionFromAttributes(const FCombatDamagePacket& InPacket, const UHunterAttributeSet* SourceAttributes, AActor* SourceActor) const;
	FCombatResolveResult MitigateDamagePacketAgainstAttributes(const FCombatDamagePacket& InPacket, AActor* SourceActor, AActor* TargetActor, const UHunterAttributeSet* SourceAttributes, const UHunterAttributeSet* TargetAttributes) const;

	static float RollDamageRange(float MinDamage, float MaxDamage);
	float CalculateOutgoingDamageForType(EHunterDamageType DamageType, const UHunterAttributeSet* SourceAttributes) const;
	static float GetResistanceValue(EHunterDamageType DamageType, const UHunterAttributeSet* TargetAttributes);
	float GetResistanceCap(EHunterDamageType DamageType, const UHunterAttributeSet* TargetAttributes) const;
	static float GetPierceValue(EHunterDamageType DamageType, const UHunterAttributeSet* SourceAttributes);
	bool IsActorBlocking(AActor* Actor) const;
	bool CanBlockHit(AActor* SourceActor, AActor* TargetActor, const UHunterAttributeSet* TargetAttributes) const;
	float GetBlockTypeMultiplier(EHunterDamageType DamageType, const UHunterAttributeSet* TargetAttributes) const;
	float GetMoreDamageMultiplier(EHunterDamageType DamageType, const UHunterAttributeSet* SourceAttributes) const;
	float GetDamageTakenMultiplier(EHunterDamageType DamageType, const UHunterAttributeSet* TargetAttributes) const;
	void ApplyBlockingToMitigatedResult(AActor* SourceActor, AActor* TargetActor, const UHunterAttributeSet* TargetAttributes, FCombatResolveResult& InOutResult) const;
	void ApplyStaminaBlockCost(const UHunterAttributeSet* TargetAttributes, FCombatResolveResult& InOutResult) const;

	void ResolveCriticalStrike(FCombatDamagePacket& Packet, const UHunterAttributeSet* SourceAttributes) const;
	void ApplyOnHitEffects(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const;
	void ApplyAilments(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const;
	void ApplyReflect(AActor* SourceActor, AActor* TargetActor, const FCombatResolveResult& Result) const;
};
