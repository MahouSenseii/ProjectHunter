#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "Tags/PHGameplayTags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHGameplayTagAttributeRegistryCompletenessTest,
	"ProjectHunter.Tags.Registry.AllAttributeTagsResolve",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FPHGameplayTagAttributeRegistryCompletenessTest::RunTest(const FString&)
{
	TestEqual(
		TEXT("Every registered attribute entry has a tag-to-attribute lookup"),
		FPHGameplayTags::TagToAttributeMap.Num(),
		FPHGameplayTags::AllAttributesMap.Num());

	TSet<FGameplayAttribute> UniqueAttributes;

	for (const TPair<FString, FGameplayAttribute>& Pair : FPHGameplayTags::AllAttributesMap)
	{
		UniqueAttributes.Add(Pair.Value);
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*Pair.Key), false);
		const bool bTagValid = Tag.IsValid();
		TestTrue(*FString::Printf(TEXT("Native tag exists: %s"), *Pair.Key), bTagValid);
		if (!bTagValid)
		{
			continue;
		}

		const FGameplayAttribute ResolvedAttribute = FPHGameplayTags::GetAttributeFromTag(Tag);
		TestTrue(
			*FString::Printf(TEXT("Tag resolves to its registered GAS attribute: %s"), *Pair.Key),
			ResolvedAttribute.IsValid() && ResolvedAttribute == Pair.Value);

		const FGameplayTag ReverseTag = FPHGameplayTags::GetTagFromAttribute(Pair.Value);
		TestTrue(
			*FString::Printf(TEXT("Attribute has a canonical reverse tag: %s"), *Pair.Key),
			ReverseTag.IsValid());
		if (ReverseTag.IsValid())
		{
			TestEqual(
				*FString::Printf(TEXT("Reverse tag resolves back to the same attribute: %s"), *Pair.Key),
				FPHGameplayTags::GetAttributeFromTag(ReverseTag),
				Pair.Value);
		}
	}

	TestEqual(
		TEXT("Reverse registry contains every unique GAS attribute"),
		FPHGameplayTags::AttributeToTagMap.Num(),
		UniqueAttributes.Num());

	const TMap<FGameplayAttribute, FGameplayTag> OriginalReverseMap = FPHGameplayTags::AttributeToTagMap;
	FPHGameplayTags::RegisterAttributeToTagMappings();
	TestEqual(
		TEXT("Rebuilding the reverse registry is idempotent"),
		FPHGameplayTags::AttributeToTagMap.Num(),
		OriginalReverseMap.Num());
	for (const TPair<FGameplayAttribute, FGameplayTag>& Pair : OriginalReverseMap)
	{
		TestEqual(
			*FString::Printf(TEXT("Canonical reverse mapping stays deterministic: %s"), *Pair.Key.GetName()),
			FPHGameplayTags::GetTagFromAttribute(Pair.Key),
			Pair.Value);
	}

	TestTrue(TEXT("Weight has a native gameplay tag"), FPHGameplayTags::Attributes_Secondary_Misc_Weight.IsValid());
	TestTrue(
		TEXT("Weight tag resolves to the Weight GAS attribute"),
		FPHGameplayTags::GetAttributeFromTag(FPHGameplayTags::Attributes_Secondary_Misc_Weight)
		== UHunterAttributeSet::GetWeightAttribute());
	return true;
}

#endif
