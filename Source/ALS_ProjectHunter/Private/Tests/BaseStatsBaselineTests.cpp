// Author: Quentin Davis

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "AbilitySystem/Effects/HunterGE_DerivedPrimaryVitals.h"
#include "AbilitySystem/HunterAttributeSet.h"
#include "GameplayEffect.h"
#include "Stats/Data/BaseStatsData.h"

namespace BaseStatsBaselineTestPrivate
{
	/** A fresh asset reflected off the real AttributeSet, then reset. */
	UBaseStatsData* MakeResetAsset()
	{
		UBaseStatsData* Data = NewObject<UBaseStatsData>(GetTransientPackage());
		Data->SourceAttributeSetClass = UHunterAttributeSet::StaticClass();
		Data->InitializationEffects.Add(UHunterGE_DerivedPrimaryVitals::StaticClass());
		Data->RefreshFromAttributeSetDefinition();
		Data->ResetToBaseline();
		return Data;
	}

	/** Authored means present in the runtime map; unauthored rows are absent. */
	bool TryGetAuthored(const TMap<FName, float>& Authored, const TCHAR* StatName, float& OutValue)
	{
		if (const float* Found = Authored.Find(FName(StatName)))
		{
			OutValue = *Found;
			return true;
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHBaseStatsResetProducesPlayableDefaultsTest,
	"ProjectHunter.Stats.BaseStatsData.ResetToBaselineIsPlayable",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FPHBaseStatsResetProducesPlayableDefaultsTest::RunTest(const FString&)
{
	using namespace BaseStatsBaselineTestPrivate;

	const UBaseStatsData* Data = MakeResetAsset();
	if (!TestNotNull(TEXT("Reflected stats data was created"), Data))
	{
		return false;
	}

	TestTrue(TEXT("Reflection produced stat rows"), Data->GetBaseAttributes().Num() > 0);

	const TMap<FName, float> Authored = Data->GetAllStatsAsMap();

	// Regeneration and drain are Rate * Amount. A zero on either side disables
	// the resource silently, which is exactly how this broke once already.
	const TCHAR* FlowPairs[][2] = {
		{TEXT("HealthRegenRate"),        TEXT("HealthRegenAmount")},
		{TEXT("ManaRegenRate"),          TEXT("ManaRegenAmount")},
		{TEXT("StaminaRegenRate"),       TEXT("StaminaRegenAmount")},
		{TEXT("StaminaDegenRate"),       TEXT("StaminaDegenAmount")},
	};

	for (const TCHAR* const* Pair : FlowPairs)
	{
		float Rate = 0.0f;
		float Amount = 0.0f;
		const bool bHasRate = TryGetAuthored(Authored, Pair[0], Rate);
		const bool bHasAmount = TryGetAuthored(Authored, Pair[1], Amount);

		TestTrue(*FString::Printf(TEXT("%s is authored"), Pair[0]), bHasRate);
		TestTrue(*FString::Printf(TEXT("%s is authored"), Pair[1]), bHasAmount);
		TestTrue(*FString::Printf(TEXT("%s * %s is non-zero, so the resource actually moves"),
			Pair[0], Pair[1]), Rate * Amount > 0.0f);
	}

	// Characters begin unspent: level 0, no primaries, so a pool equals its
	// authored base until something is invested.
	for (const TCHAR* ZeroStat : {TEXT("PlayerLevel"), TEXT("Strength"), TEXT("Intelligence"),
		TEXT("Dexterity"), TEXT("Endurance"), TEXT("Affliction"), TEXT("Luck"), TEXT("Covenant")})
	{
		float Value = -1.0f;
		const bool bAuthored = TryGetAuthored(Authored, ZeroStat, Value);
		TestTrue(*FString::Printf(TEXT("%s is authored"), ZeroStat), bAuthored);
		TestEqual(*FString::Printf(TEXT("%s starts at zero"), ZeroStat), Value, 0.0f);
	}

	// Sprinting has to cost more than standing still returns, or stamina rises
	// while it is being spent.
	{
		float RegenAmount = 0.0f;
		float DegenAmount = 0.0f;
		TryGetAuthored(Authored, TEXT("StaminaRegenAmount"), RegenAmount);
		TryGetAuthored(Authored, TEXT("StaminaDegenAmount"), DegenAmount);
		TestTrue(TEXT("Stamina drain outweighs stamina regeneration"), DegenAmount > RegenAmount);
	}

	// The max values are the per-character base the effect adds onto, so they
	// have to survive the reset.
	for (const TCHAR* MaxStat : {TEXT("MaxHealth"), TEXT("MaxMana"), TEXT("MaxStamina")})
	{
		float Value = 0.0f;
		TestTrue(*FString::Printf(TEXT("%s is authored as a base"), MaxStat),
			TryGetAuthored(Authored, MaxStat, Value) && Value > 0.0f);
	}

	// These are recomputed by UpdateHealthDerivedAttributes and friends, so
	// authoring them achieves nothing and only creates confusion.
	for (const TCHAR* Computed : {TEXT("MaxEffectiveHealth"), TEXT("MaxEffectiveMana"),
		TEXT("MaxEffectiveStamina"), TEXT("MaxEffectiveArcaneShield")})
	{
		TestFalse(*FString::Printf(TEXT("%s is left unauthored"), Computed),
			Authored.Contains(FName(Computed)));
	}

	// Nothing the initialization effect Overrides may be authored, or that
	// effect is skipped in its entirety at runtime.
	const UGameplayEffect* EffectCDO =
		UHunterGE_DerivedPrimaryVitals::StaticClass()->GetDefaultObject<UGameplayEffect>();
	if (TestNotNull(TEXT("Derived vitals effect CDO resolved"), EffectCDO))
	{
		for (const FGameplayModifierInfo& Modifier : EffectCDO->Modifiers)
		{
			if (Modifier.ModifierOp != EGameplayModOp::Override || !Modifier.Attribute.IsValid())
			{
				continue;
			}

			if (const FProperty* AttributeProperty = Modifier.Attribute.GetUProperty())
			{
				const FName AttributeName = AttributeProperty->GetFName();
				TestFalse(*FString::Printf(
					TEXT("%s is not authored; an Override modifier would block the whole effect"),
					*AttributeName.ToString()), Authored.Contains(AttributeName));
			}
		}
	}

	return true;
}

#endif
