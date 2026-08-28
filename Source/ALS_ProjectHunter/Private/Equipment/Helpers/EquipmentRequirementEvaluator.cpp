#include "Equipment/Helpers/EquipmentRequirementEvaluator.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "Equipment/Components/EquipmentManager.h"
#include "Item/ItemInstance.h"
#include "Item/Library/Structs/ItemStructs.h"
#include "Stats/Components/StatsManager.h"
#include "Stats/Library/Enums/StatsEnumLibrary.h"

FItemRequirementCheckResult FEquipmentRequirementEvaluator::Evaluate(
	const UEquipmentManager& Manager,
	const UItemInstance* Item)
{
	FItemRequirementCheckResult Result;
	if (!IsValid(Item))
	{
		return Result;
	}

	const FItemBase* BaseData = Item->GetBaseData();
	if (!BaseData)
	{
		return Result;
	}

	Result.bItemValid = true;

	const AActor* Owner = Manager.GetOwner();
	const UStatsManager* StatsManager = Owner ? Owner->FindComponentByClass<UStatsManager>() : nullptr;
	if (!StatsManager)
	{
		return Result;
	}

	const bool bHasRequirementAttributes =
		StatsManager->HasLiveAttribute(UHunterAttributeSet::GetPlayerLevelAttribute())
		&& StatsManager->HasLiveAttribute(UHunterAttributeSet::GetStrengthAttribute())
		&& StatsManager->HasLiveAttribute(UHunterAttributeSet::GetDexterityAttribute())
		&& StatsManager->HasLiveAttribute(UHunterAttributeSet::GetIntelligenceAttribute())
		&& StatsManager->HasLiveAttribute(UHunterAttributeSet::GetEnduranceAttribute())
		&& StatsManager->HasLiveAttribute(UHunterAttributeSet::GetAfflictionAttribute())
		&& StatsManager->HasLiveAttribute(UHunterAttributeSet::GetLuckAttribute())
		&& StatsManager->HasLiveAttribute(UHunterAttributeSet::GetCovenantAttribute());
	if (!bHasRequirementAttributes)
	{
		return Result;
	}

	return BaseData->StatRequirements.EvaluateRequirements(
		StatsManager->GetAttributeByType(EHunterAttribute::PlayerLevel),
		StatsManager->GetAttributeByType(EHunterAttribute::Strength),
		StatsManager->GetAttributeByType(EHunterAttribute::Dexterity),
		StatsManager->GetAttributeByType(EHunterAttribute::Intelligence),
		StatsManager->GetAttributeByType(EHunterAttribute::Endurance),
		StatsManager->GetAttributeByType(EHunterAttribute::Affliction),
		StatsManager->GetAttributeByType(EHunterAttribute::Luck),
		StatsManager->GetAttributeByType(EHunterAttribute::Covenant));
}
