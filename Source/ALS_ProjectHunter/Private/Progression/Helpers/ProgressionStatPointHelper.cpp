#include "Progression/Helpers/ProgressionStatPointHelper.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GameplayEffectTypes.h"
#include "Progression/Components/CharacterProgressionManager.h"
#include "Progression/Helpers/ProgressionAbilityHelper.h"
#include "Progression/Library/FunctionLibraries/ProgressionStatFunctionLibrary.h"

bool FProgressionStatPointHelper::ApplyStatPointToAttribute(UCharacterProgressionManager& Manager, const FName AttributeName)
{
	UAbilitySystemComponent* ASC = FProgressionAbilityHelper::GetAbilitySystemComponent(Manager);
	if (!ASC)
	{
		UE_LOG(LogCharacterProgressionManager, Error, TEXT("ApplyStatPointToAttribute: ASC is null"));
		return false;
	}

	const FGameplayAttribute Attribute = UProgressionStatFunctionLibrary::GetAttributeForStatName(AttributeName);
	if (!Attribute.IsValid())
	{
		UE_LOG(
			LogCharacterProgressionManager,
			Error,
			TEXT("ApplyStatPointToAttribute: Unknown primary attribute '%s'"),
			*AttributeName.ToString());
		return false;
	}

	const TSubclassOf<UGameplayEffect>* GEClassPtr = Manager.StatPointGEClasses.Find(AttributeName);
	if (!GEClassPtr || !(*GEClassPtr))
	{
		UE_LOG(
			LogCharacterProgressionManager,
			Warning,
			TEXT("ApplyStatPointToAttribute: No GE class configured for attribute '%s'. Add an entry to StatPointGEClasses in the Blueprint defaults."),
			*AttributeName.ToString());
		return false;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(Manager.GetOwner());

	const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectToSelf(
		(*GEClassPtr)->GetDefaultObject<UGameplayEffect>(),
		1.0f,
		Context);

	if (Handle.IsValid())
	{
		Manager.StatPointGEHandles.FindOrAdd(AttributeName).Add(Handle);
		UE_LOG(
			LogCharacterProgressionManager,
			Log,
			TEXT("ApplyStatPointToAttribute: Applied +1 to '%s' (handle %s)"),
			*AttributeName.ToString(),
			*Handle.ToString());
		return true;
	}

	UE_LOG(
		LogCharacterProgressionManager,
		Error,
		TEXT("ApplyStatPointToAttribute: GE application failed for '%s'"),
		*AttributeName.ToString());

	return false;
}

void FProgressionStatPointHelper::RemoveStatPointFromAttribute(
	UCharacterProgressionManager& Manager,
	const FName AttributeName,
	const int32 PointsToRemove)
{
	UAbilitySystemComponent* ASC = FProgressionAbilityHelper::GetAbilitySystemComponent(Manager);
	if (!ASC)
	{
		return;
	}

	TArray<FActiveGameplayEffectHandle>* Handles = Manager.StatPointGEHandles.Find(AttributeName);
	if (!Handles || Handles->Num() == 0)
	{
		UE_LOG(
			LogCharacterProgressionManager,
			Warning,
			TEXT("RemoveStatPointFromAttribute: No tracked GE handles for '%s'. Attribute will not be adjusted."),
			*AttributeName.ToString());
		return;
	}

	const int32 ToRemove = FMath::Min(PointsToRemove, Handles->Num());
	for (int32 Index = 0; Index < ToRemove; ++Index)
	{
		const FActiveGameplayEffectHandle Handle = Handles->Pop(EAllowShrinking::No);
		if (Handle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(Handle);
		}
	}

	UE_LOG(
		LogCharacterProgressionManager,
		Log,
		TEXT("RemoveStatPointFromAttribute: Removed %d point(s) from '%s'"),
		ToRemove,
		*AttributeName.ToString());
}

void FProgressionStatPointHelper::RebuildSpentStatPointsCache(UCharacterProgressionManager& Manager)
{
	Manager.SpentStatPointsCache.Empty(Manager.SpentStatPoints.Num());
	for (const FStatPointSpending& Entry : Manager.SpentStatPoints)
	{
		Manager.SpentStatPointsCache.Add(Entry.AttributeName, Entry.PointsSpent);
	}
}
