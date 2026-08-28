#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Item/Library/Enums/ItemTooltipEnums.h"
#include "Item/Library/FunctionLibraries/ItemTooltipLineFunctionLibrary.h"
#include "Item/Library/FunctionLibraries/ItemTooltipSectionFunctionLibrary.h"
#include "Item/Library/Structs/ItemRequirementStructs.h"
#include "Item/Library/Structs/ItemTooltipStructs.h"

namespace PHItemRequirementTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHItemRequirementFailureDetailsTest,
	"ProjectHunter.Item.Requirements.ReportsEveryMissingStat",
	PHItemRequirementTests::TestFlags)

bool FPHItemRequirementFailureDetailsTest::RunTest(const FString&)
{
	FItemStatRequirement Requirements;
	Requirements.RequiredLevel = 10;
	Requirements.RequiredStrength = 20;
	Requirements.RequiredDexterity = 15;
	Requirements.RequiredIntelligence = 30;

	const FItemRequirementCheckResult Result = Requirements.EvaluateRequirements(
		9.0f,
		20.0f,
		14.5f,
		25.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f);

	TestTrue(TEXT("Requirement data is valid"), Result.bItemValid);
	TestTrue(TEXT("Current stats were supplied"), Result.bStatsAvailable);
	TestFalse(TEXT("Missing requirements reject the item"), Result.bMeetsRequirements);
	TestEqual(TEXT("Every missing requirement is reported"), Result.Failures.Num(), 3);
	TestEqual(TEXT("Passing and failing authored requirements are retained for presentation"), Result.Checks.Num(), 4);

	if (Result.Failures.Num() == 3)
	{
		TestEqual(TEXT("Level failure type"), Result.Failures[0].RequirementType, EItemRequirementType::Level);
		TestEqual(TEXT("Level current value"), Result.Failures[0].CurrentValue, 9.0f);
		TestEqual(TEXT("Level required value"), Result.Failures[0].RequiredValue, 10.0f);
		TestEqual(TEXT("Level missing value"), Result.Failures[0].MissingValue, 1.0f);

		TestEqual(TEXT("Dexterity failure type"), Result.Failures[1].RequirementType, EItemRequirementType::Dexterity);
		TestEqual(TEXT("Dexterity missing value"), Result.Failures[1].MissingValue, 0.5f);

		TestEqual(TEXT("Intelligence failure type"), Result.Failures[2].RequirementType, EItemRequirementType::Intelligence);
		TestEqual(TEXT("Intelligence missing value"), Result.Failures[2].MissingValue, 5.0f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHItemRequirementBoundaryTest,
	"ProjectHunter.Item.Requirements.ExactValuesPass",
	PHItemRequirementTests::TestFlags)

bool FPHItemRequirementBoundaryTest::RunTest(const FString&)
{
	FItemStatRequirement Requirements;
	Requirements.RequiredLevel = 10;
	Requirements.RequiredStrength = 20;
	Requirements.RequiredDexterity = 15;
	Requirements.RequiredIntelligence = 30;
	Requirements.RequiredEndurance = 7;
	Requirements.RequiredAffliction = 8;
	Requirements.RequiredLuck = 9;
	Requirements.RequiredCovenant = 11;

	const FItemRequirementCheckResult Result = Requirements.EvaluateRequirements(
		10.0f,
		20.0f,
		15.0f,
		30.0f,
		7.0f,
		8.0f,
		9.0f,
		11.0f);

	TestTrue(TEXT("Exact values meet every requirement"), Result.bMeetsRequirements);
	TestTrue(TEXT("Passing result has no failures"), Result.Failures.IsEmpty());
	TestTrue(TEXT("Legacy Boolean API remains compatible"), Requirements.MeetsRequirements(10, 20, 15, 30, 7, 8, 9, 11));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHItemRequirementTooltipPresentationTest,
	"ProjectHunter.Item.Requirements.TooltipShowsCurrentRequiredAndMissingValues",
	PHItemRequirementTests::TestFlags)

bool FPHItemRequirementTooltipPresentationTest::RunTest(const FString&)
{
	FItemStatRequirement Requirements;
	Requirements.RequiredLevel = 10;
	Requirements.RequiredStrength = 20;
	Requirements.RequiredDexterity = 15;
	Requirements.RequiredIntelligence = 0;

	const FItemRequirementCheckResult Result = Requirements.EvaluateRequirements(
		9.0f,
		20.0f,
		12.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f,
		0.0f);

	FItemTooltipData TooltipData;
	UItemTooltipSectionFunctionLibrary::AddRequirementsSection(TooltipData, Requirements, &Result);

	TestEqual(TEXT("Tooltip contains one requirements section"), TooltipData.Sections.Num(), 1);
	if (TooltipData.Sections.Num() != 1)
	{
		return false;
	}

	const FItemTooltipSection& Section = TooltipData.Sections[0];
	TestEqual(TEXT("Every visible authored requirement gets a row"), Section.Lines.Num(), 3);
	if (Section.Lines.Num() != 3)
	{
		return false;
	}

	TestEqual(TEXT("Failed level row shows current, required, and missing"), Section.Lines[0].Value.ToString(), FString(TEXT("9 / 10 (1 missing)")));
	TestEqual(TEXT("Failed level row uses warning styling"), Section.Lines[0].Style, EItemTooltipLineStyle::Warning);
	TestTrue(TEXT("Failed level row is emphasized"), Section.Lines[0].bEmphasized);

	TestEqual(TEXT("Passing strength row shows current and required"), Section.Lines[1].Value.ToString(), FString(TEXT("20 / 20")));
	TestEqual(TEXT("Passing strength row uses normal property styling"), Section.Lines[1].Style, EItemTooltipLineStyle::Property);
	TestFalse(TEXT("Passing strength row is not emphasized"), Section.Lines[1].bEmphasized);

	TestEqual(TEXT("Failed dexterity row shows fractional deficit"), Section.Lines[2].Value.ToString(), FString(TEXT("12 / 15 (3 missing)")));
	TestEqual(TEXT("Failed dexterity row uses the shared negative color"), Section.Lines[2].TextColor, UItemTooltipLineFunctionLibrary::GetNegativeTextColor());
	return true;
}

#endif
