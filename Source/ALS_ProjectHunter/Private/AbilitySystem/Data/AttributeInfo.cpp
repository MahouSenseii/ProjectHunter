#include "AbilitySystem/Data/AttributeInfo.h"

#if WITH_EDITOR
#include "AbilitySystem/Data/AttributeConfigDataAsset.h"
#endif

FPHAttributeInfo UAttributeInfo::FindAttributeInfoForTag(const FGameplayTag& AttributeTag, bool bLogNotFound) const
{
	for (const FPHAttributeInfo& Info : AttributeInformation)
	{
		if(Info.AttributeTag.MatchesTagExact(AttributeTag))
		{
			return Info;
		}
	}

	if(bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't find Info for AttributeTag [%s] on AttributeInfo [%s]"),
			*AttributeTag.ToString(), *GetNameSafe(this));
	}
	return FPHAttributeInfo();
}

#if WITH_EDITOR
void UAttributeInfo::PopulateFromConfig()
{
	if (!SourceConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("SourceConfig is not set! Please assign an AttributeConfigDataAsset."));
		return;
	}

	// Clear existing data
	AttributeInformation.Empty();

	// Populate from config
	for (const FAttributeInitConfig& Config : SourceConfig->Attributes)
	{
		if (!Config.bEnabled || !Config.AttributeTag.IsValid())
		{
			continue;
		}

		FPHAttributeInfo NewInfo;
		NewInfo.AttributeTag = Config.AttributeTag;
		NewInfo.AttributeName = FText::FromString(Config.DisplayName);
		NewInfo.AttributeDescription = FText::FromString(Config.Description);
		NewInfo.AttributeValue = 0.0f; // Runtime value, set to 0 by default

		AttributeInformation.Add(NewInfo);
	}

	// Mark as modified to save
	Modify();

	UE_LOG(LogTemp, Log, TEXT("Successfully populated %d attributes from config!"), AttributeInformation.Num());
}

void UAttributeInfo::ClearAllAttributes()
{
	AttributeInformation.Empty();
	Modify();
	
	UE_LOG(LogTemp, Log, TEXT("Cleared all attribute information."));
}
#endif