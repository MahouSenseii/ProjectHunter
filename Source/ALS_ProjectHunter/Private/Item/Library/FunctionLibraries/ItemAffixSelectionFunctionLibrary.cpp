#include "Item/Library/FunctionLibraries/ItemAffixSelectionFunctionLibrary.h"

TArray<FPHAttributeData*> UItemAffixSelectionFunctionLibrary::BuildAffixPoolByCorruption(
	const TArray<FPHAttributeData*>& SourceAffixes,
	const EItemType ItemType,
	const EItemSubType ItemSubType,
	const int32 ItemLevel,
	const bool bCorruptedOnly,
	const TSet<FName>& ExcludeAffixes,
	const TSet<FName>& ExcludeGroups)
{
	TArray<FPHAttributeData*> Pool;
	Pool.Reserve(SourceAffixes.Num() / 4);

	for (FPHAttributeData* Affix : SourceAffixes)
	{
		if (!Affix)
		{
			continue;
		}

		if (ExcludeAffixes.Contains(Affix->AttributeName))
		{
			continue;
		}

		if (Affix->AffixGroup != NAME_None && ExcludeGroups.Contains(Affix->AffixGroup))
		{
			continue;
		}

		if (!Affix->IsAllowedOnItemType(ItemType))
		{
			continue;
		}

		if (!Affix->IsAllowedOnSubType(ItemSubType))
		{
			continue;
		}

		if (!Affix->IsValidForItemLevel(ItemLevel))
		{
			continue;
		}

		const bool bIsCorrupted = Affix->IsCorruptedAffix();
		if (bCorruptedOnly && !bIsCorrupted)
		{
			continue;
		}

		if (!bCorruptedOnly && bIsCorrupted)
		{
			continue;
		}

		Pool.Add(Affix);
	}

	return Pool;
}

const FPHAttributeData* UItemAffixSelectionFunctionLibrary::SelectWeightedAffix(
	const TArray<FPHAttributeData*>& AvailableAffixes,
	FRandomStream& RandStream)
{
	if (AvailableAffixes.Num() == 0)
	{
		return nullptr;
	}

	int32 TotalWeight = 0;
	for (const FPHAttributeData* Affix : AvailableAffixes)
	{
		TotalWeight += Affix ? Affix->GetWeight() : 0;
	}

	if (TotalWeight <= 0)
	{
		return AvailableAffixes[RandStream.RandRange(0, AvailableAffixes.Num() - 1)];
	}

	const int32 RandomValue = RandStream.RandRange(0, TotalWeight - 1);
	int32 CurrentWeight = 0;

	for (const FPHAttributeData* Affix : AvailableAffixes)
	{
		if (!Affix)
		{
			continue;
		}

		CurrentWeight += Affix->GetWeight();
		if (RandomValue < CurrentWeight)
		{
			return Affix;
		}
	}

	return AvailableAffixes.Last();
}

FPHAttributeData UItemAffixSelectionFunctionLibrary::CreateRolledAffix(
	const FPHAttributeData& TemplateAffix,
	FRandomStream& RandStream)
{
	FPHAttributeData RolledAffix = TemplateAffix;
	RolledAffix.RollValue(RandStream);
	RolledAffix.GenerateUID();
	return RolledAffix;
}
