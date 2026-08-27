#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Engine/DataTable.h"
#include "Item/Generation/AffixGenerator.h"
#include "Item/Library/FunctionLibraries/ItemPowerFunctionLibrary.h"
#include "Item/Library/Structs/AffixStructs.h"
#include "Item/Settings/ItemPowerSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FItemPowerIncludesEveryModifierTest,
	"ProjectHunter.Item.Power.IncludesEveryModifierCollection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemPowerIncludesEveryModifierTest::RunTest(const FString& Parameters)
{
	FPHItemStats Stats;
	FPHAttributeData Implicit; Implicit.PowerValue = 1.0f; Stats.Implicits.Add(Implicit);
	FPHAttributeData Prefix; Prefix.PowerValue = 2.0f; Stats.Prefixes.Add(Prefix);
	FPHAttributeData Suffix; Suffix.PowerValue = 3.0f; Stats.Suffixes.Add(Suffix);
	FPHAttributeData Crafted; Crafted.PowerValue = 4.0f; Stats.Crafted.Add(Crafted);
	FPHAttributeData Enchant; Enchant.PowerValue = 5.0f; Stats.Enchants.Add(Enchant);

	TestEqual(TEXT("Base + all modifier collections"),
		UItemPowerFunctionLibrary::CalculateItemPower(10.0f, Stats), 25.0f);

	Prefix.PowerValue = -50.0f;
	Stats.Prefixes[0] = Prefix;
	TestEqual(TEXT("Negative/corrupted power cannot make an item score negative"),
		UItemPowerFunctionLibrary::CalculateItemPower(0.0f, Stats), 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FItemPowerGradeBoundaryTest,
	"ProjectHunter.Item.Power.GradeBoundaries",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FItemPowerGradeBoundaryTest::RunTest(const FString& Parameters)
{
	const UItemPowerSettings* Settings = GetDefault<UItemPowerSettings>();
	TestEqual(TEXT("Below E is F"), UItemPowerFunctionLibrary::GetGradeForPower(Settings->GradeE - 0.01f), EItemRarity::IR_GradeF);
	TestEqual(TEXT("E threshold"), UItemPowerFunctionLibrary::GetGradeForPower(Settings->GradeE), EItemRarity::IR_GradeE);
	TestEqual(TEXT("SS threshold"), UItemPowerFunctionLibrary::GetGradeForPower(Settings->GradeSS), EItemRarity::IR_GradeSS);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTieredAffixAuthoringTest,
	"ProjectHunter.Item.Affix.TieredDefinitionGeneratesRuntimeAffix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTieredAffixAuthoringTest::RunTest(const FString& Parameters)
{
	UDataTable* PrefixTable = NewObject<UDataTable>();
	PrefixTable->RowStruct = FAffixData::StaticStruct();

	FAffixData Definition;
	Definition.AffixID = TEXT("TestPowerAffix");
	Definition.AttributeName = TEXT("TestPowerAttribute");
	Definition.AffixType = EAffixes::AF_Prefix;
	Definition.AffixName = FText::FromString(TEXT("Tested"));
	Definition.Weight = 321;

	FAffixTier Tier;
	Tier.TierNumber = 4;
	Tier.MinItemLevel = 1;
	Tier.MaxItemLevel = 100;
	Tier.MinValue = 7.0f;
	Tier.MaxValue = 7.0f;
	Tier.PowerValue = 37.0f;
	Definition.Tiers.Add(Tier);
	PrefixTable->AddRow(TEXT("TestPowerAffix"), Definition);

	FItemBase Base;
	Base.ItemID = TEXT("TestBase");
	Base.ItemType = EItemType::IT_Weapon;
	Base.ItemSubType = EItemSubType::IST_Sword;
	Base.PrefixAffixTable = PrefixTable;

	FAffixGenerator Generator;
	const FPHItemStats Stats = Generator.GenerateAffixes(Base, 50, EItemRarity::IR_GradeD, 12345);
	TestEqual(TEXT("Exactly one guaranteed Grade-D prefix"), Stats.Prefixes.Num(), 1);
	if (Stats.Prefixes.Num() == 1)
	{
		const FPHAttributeData& Rolled = Stats.Prefixes[0];
		TestEqual(TEXT("Stable affix ID"), Rolled.AffixID, FName(TEXT("TestPowerAffix")));
		TestEqual(TEXT("Authored tier"), Rolled.TierNumber, 4);
		TestEqual(TEXT("Authored item power"), Rolled.PowerValue, 37.0f);
		TestEqual(TEXT("Explicit spawn weight"), Rolled.SpawnWeight, 321);
		TestEqual(TEXT("Rolled stat value"), Rolled.RolledStatValue, 7.0f);
	}
	return true;
}

#endif
