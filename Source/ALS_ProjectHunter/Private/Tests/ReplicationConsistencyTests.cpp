// Guards against replication declarations that are never registered.
//
// A UPROPERTY marked Replicated or ReplicatedUsing only actually replicates if
// it also appears in GetLifetimeReplicatedProps. Miss the second half and the
// property silently never reaches clients, its OnRep handler becomes dead code,
// and nothing fails loudly - UE logs a warning that is easy to lose in startup
// spam.
//
// That is exactly how MaxArcaneShieldRegenRate and MaxArcaneShieldRegenAmount
// went unregistered among UHunterAttributeSet's 223 replicated attributes. This
// test turns that class of mistake into a build-visible failure, which matters
// most on the attribute set precisely because it is too large to police by eye.

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Net/UnrealNetwork.h"
#include "UObject/UnrealType.h"

#include "AbilitySystem/HunterAttributeSet.h"
#include "AI/Components/MonsterModifierComponent.h"
#include "Combat/Components/CombatManager.h"
#include "Framework/GameModes/PHGameState.h"
#include "Framework/Player/PHPlayerState.h"
#include "Interactable/Actors/LootChest/LootChest.h"
#include "Progression/Components/CharacterProgressionManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace PHReplicationTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter;

	/**
	 * Collects every property the class declares as replicated but never
	 * registers for replication.
	 */
	TArray<FString> FindUnregisteredReplicatedProperties(UClass* Class)
	{
		TArray<FString> Missing;
		if (!Class)
		{
			return Missing;
		}

		// RepIndex is only assigned once the class has built its replication data.
		Class->SetUpRuntimeReplicationData();

		UObject* CDO = Class->GetDefaultObject();
		if (!CDO)
		{
			return Missing;
		}

		TArray<FLifetimeProperty> LifetimeProps;
		CDO->GetLifetimeReplicatedProps(LifetimeProps);

		TSet<uint16> RegisteredRepIndices;
		RegisteredRepIndices.Reserve(LifetimeProps.Num());
		for (const FLifetimeProperty& Lifetime : LifetimeProps)
		{
			RegisteredRepIndices.Add(Lifetime.RepIndex);
		}

		// Walk this class's own properties. Inherited replicated properties are
		// registered by the parent's GetLifetimeReplicatedProps, so checking them
		// here would produce false positives.
		for (TFieldIterator<FProperty> It(Class, EFieldIterationFlags::None); It; ++It)
		{
			const FProperty* Property = *It;
			if (!Property || !Property->HasAnyPropertyFlags(CPF_Net))
			{
				continue;
			}

			if (!RegisteredRepIndices.Contains(Property->RepIndex))
			{
				Missing.Add(Property->GetName());
			}
		}

		Missing.Sort();
		return Missing;
	}

	ELifetimeCondition FindReplicationCondition(UClass* Class, const FName PropertyName)
	{
		if (!Class)
		{
			return COND_Max;
		}

		Class->SetUpRuntimeReplicationData();
		const FProperty* Property = FindFProperty<FProperty>(Class, PropertyName);
		UObject* CDO = Class->GetDefaultObject();
		if (!Property || !CDO)
		{
			return COND_Max;
		}

		TArray<FLifetimeProperty> LifetimeProps;
		CDO->GetLifetimeReplicatedProps(LifetimeProps);
		for (const FLifetimeProperty& Lifetime : LifetimeProps)
		{
			if (Lifetime.RepIndex == Property->RepIndex)
			{
				return Lifetime.Condition;
			}
		}

		return COND_Max;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHAttributeSetReplicationRegistrationTest,
	"ProjectHunter.Replication.AttributeSetRegistersEveryReplicatedProperty",
	PHReplicationTests::TestFlags)

bool FPHAttributeSetReplicationRegistrationTest::RunTest(const FString&)
{
	const TArray<FString> Missing =
		PHReplicationTests::FindUnregisteredReplicatedProperties(UHunterAttributeSet::StaticClass());

	if (Missing.Num() > 0)
	{
		AddError(FString::Printf(
			TEXT("UHunterAttributeSet declares %d replicated propert%s that GetLifetimeReplicatedProps never registers, ")
			TEXT("so they will never reach clients: %s"),
			Missing.Num(),
			Missing.Num() == 1 ? TEXT("y") : TEXT("ies"),
			*FString::Join(Missing, TEXT(", "))));
	}

	TestEqual(TEXT("Every replicated attribute is registered"), Missing.Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHGameplayClassReplicationRegistrationTest,
	"ProjectHunter.Replication.GameplayClassesRegisterEveryReplicatedProperty",
	PHReplicationTests::TestFlags)

bool FPHGameplayClassReplicationRegistrationTest::RunTest(const FString&)
{
	// The run-critical replicated classes. Monster presentation and party run
	// state are both new replication surfaces, and both are the kind of thing
	// that only fails visibly in a multiplayer session.
	UClass* Classes[] =
	{
		UMonsterModifierComponent::StaticClass(),
		APHPlayerState::StaticClass(),
		APHGameState::StaticClass(),
		UCharacterProgressionManager::StaticClass()
	};

	for (UClass* Class : Classes)
	{
		const TArray<FString> Missing = PHReplicationTests::FindUnregisteredReplicatedProperties(Class);

		if (Missing.Num() > 0)
		{
			AddError(FString::Printf(
				TEXT("%s declares replicated properties that GetLifetimeReplicatedProps never registers: %s"),
				*GetNameSafe(Class),
				*FString::Join(Missing, TEXT(", "))));
		}

		TestEqual(
			*FString::Printf(TEXT("%s registers every replicated property"), *GetNameSafe(Class)),
			Missing.Num(), 0);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FPHReplicationSemanticsTest,
	"ProjectHunter.Replication.NetworkSemantics",
	PHReplicationTests::TestFlags)

bool FPHReplicationSemanticsTest::RunTest(const FString&)
{
	TestEqual(
		TEXT("Character save-slot name only replicates to its owning player"),
		PHReplicationTests::FindReplicationCondition(
			APHPlayerState::StaticClass(),
			GET_MEMBER_NAME_CHECKED(APHPlayerState, CharacterSlotName)),
		COND_OwnerOnly);

	const FProperty* RunStateProperty = FindFProperty<FProperty>(
		APHGameState::StaticClass(), GET_MEMBER_NAME_CHECKED(APHGameState, RunState));
	const FProperty* RunSessionProperty = FindFProperty<FProperty>(
		APHGameState::StaticClass(), GET_MEMBER_NAME_CHECKED(APHGameState, RunSession));
	const FProperty* RunRevisionProperty = FindFProperty<FProperty>(
		APHGameState::StaticClass(), GET_MEMBER_NAME_CHECKED(APHGameState, RunSnapshotRevision));

	TestTrue(TEXT("RunState property exists"), RunStateProperty != nullptr);
	TestTrue(TEXT("RunSession property exists"), RunSessionProperty != nullptr);
	TestTrue(TEXT("RunSnapshotRevision property exists"), RunRevisionProperty != nullptr);
	if (RunStateProperty && RunSessionProperty && RunRevisionProperty)
	{
		TestTrue(TEXT("RunState does not independently fire the snapshot callback"), RunStateProperty->RepNotifyFunc.IsNone());
		TestTrue(TEXT("RunSession does not independently fire the snapshot callback"), RunSessionProperty->RepNotifyFunc.IsNone());
		TestEqual(
			TEXT("One revision property owns the coherent snapshot callback"),
			RunRevisionProperty->RepNotifyFunc,
			FName(TEXT("OnRep_RunSnapshot")));
	}

	const FProperty* MatchPhaseProperty = FindFProperty<FProperty>(
		APHGameState::StaticClass(), GET_MEMBER_NAME_CHECKED(APHGameState, MatchPhase));
	const FProperty* MatchRevisionProperty = FindFProperty<FProperty>(
		APHGameState::StaticClass(), GET_MEMBER_NAME_CHECKED(APHGameState, MatchSnapshotRevision));
	TestTrue(TEXT("MatchPhase property exists"), MatchPhaseProperty != nullptr);
	TestTrue(TEXT("MatchSnapshotRevision property exists"), MatchRevisionProperty != nullptr);
	if (MatchPhaseProperty && MatchRevisionProperty)
	{
		TestTrue(TEXT("MatchPhase waits for its clock data before notifying"), MatchPhaseProperty->RepNotifyFunc.IsNone());
		TestEqual(
			TEXT("Match revision owns the coherent phase callback"),
			MatchRevisionProperty->RepNotifyFunc,
			FName(TEXT("OnRep_MatchPhase")));
	}

	const FName ProgressionProperties[] =
	{
		GET_MEMBER_NAME_CHECKED(UCharacterProgressionManager, UnspentStatPoints),
		GET_MEMBER_NAME_CHECKED(UCharacterProgressionManager, TotalStatPoints),
		GET_MEMBER_NAME_CHECKED(UCharacterProgressionManager, UnspentSkillPoints)
	};
	for (const FName PropertyName : ProgressionProperties)
	{
		const FProperty* ProgressionProperty = FindFProperty<FProperty>(
			UCharacterProgressionManager::StaticClass(), PropertyName);
		TestTrue(*FString::Printf(TEXT("%s property exists"), *PropertyName.ToString()), ProgressionProperty != nullptr);
		if (ProgressionProperty)
		{
			TestEqual(
				*FString::Printf(TEXT("%s replication notifies Blueprint UI"), *PropertyName.ToString()),
				ProgressionProperty->RepNotifyFunc,
				FName(TEXT("OnRep_ProgressionValue")));
		}
	}

	const UFunction* OpenChestFunction = ALootChest::StaticClass()->FindFunctionByName(TEXT("OpenChest"));
	TestTrue(TEXT("OpenChest remains Blueprint-callable"), OpenChestFunction != nullptr);
	if (OpenChestFunction)
	{
		TestTrue(
			TEXT("OpenChest is visibly server-only in Blueprint"),
			OpenChestFunction->HasAnyFunctionFlags(FUNC_BlueprintAuthorityOnly));
	}
	TestNull(
		TEXT("Loot chest no longer exposes an invalid RPC on an unowned world actor"),
		ALootChest::StaticClass()->FindFunctionByName(TEXT("ServerOpenChest")));

	const UFunction* DamagePopupFunction = UCombatManager::StaticClass()->FindFunctionByName(
		TEXT("ClientReceiveDamagePopup"));
	TestTrue(TEXT("CombatManager exposes an owning-client damage popup bridge"), DamagePopupFunction != nullptr);
	if (DamagePopupFunction)
	{
		TestTrue(
			TEXT("Damage popup bridge is a client RPC"),
			DamagePopupFunction->HasAllFunctionFlags(FUNC_Net | FUNC_NetClient));
		TestFalse(
			TEXT("Cosmetic damage popup delivery does not block reliable gameplay traffic"),
			DamagePopupFunction->HasAnyFunctionFlags(FUNC_NetReliable));
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
