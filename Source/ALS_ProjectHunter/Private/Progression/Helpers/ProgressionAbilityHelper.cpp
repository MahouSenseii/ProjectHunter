#include "Progression/Helpers/ProgressionAbilityHelper.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "Progression/Components/CharacterProgressionManager.h"

UAbilitySystemComponent* FProgressionAbilityHelper::GetAbilitySystemComponent(const UCharacterProgressionManager& Manager)
{
	if (Manager.CachedASC)
	{
		return Manager.CachedASC.Get();
	}

	if (AActor* Owner = Manager.GetOwner())
	{
		return Owner->FindComponentByClass<UAbilitySystemComponent>();
	}

	return nullptr;
}

UHunterAttributeSet* FProgressionAbilityHelper::GetAttributeSet(const UCharacterProgressionManager& Manager)
{
	if (Manager.CachedAttributeSet)
	{
		return Manager.CachedAttributeSet.Get();
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent(Manager);
	return ASC ? const_cast<UHunterAttributeSet*>(ASC->GetSet<UHunterAttributeSet>()) : nullptr;
}

void FProgressionAbilityHelper::TrySyncPlayerLevelAttribute(UAbilitySystemComponent* ASC, const int32 Level)
{
	if (!ASC)
	{
		return;
	}

	const UHunterAttributeSet* AttributeSet = ASC->GetSet<UHunterAttributeSet>();
	if (!AttributeSet)
	{
		UE_LOG(
			LogCharacterProgressionManager,
			Verbose,
			TEXT("TrySyncPlayerLevelAttribute: Skipping level sync because the live HunterAttributeSet is not registered on ASC=%s yet"),
			*GetNameSafe(ASC));
		return;
	}

	// Floor of 0, not 1: an unspent character legitimately sits at level 0, and
	// the old clamp made the data asset's 0 indistinguishable from 1.
	ASC->SetNumericAttributeBase(
		UHunterAttributeSet::GetPlayerLevelAttribute(),
		static_cast<float>(FMath::Max(Level, 0)));
}
