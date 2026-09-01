// Copyright:   Copyright (C) 2022 Quentin Davis
// Source Code: https://github.com/MahouSenseii/ProjectHunter

#include "CoreMinimal.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Character/Components/PHCharacterMovementComponent.h"
#include "Character/PHBaseCharacter.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPHWallTraversalBlueprintConsumersTest,
	"ProjectHunter.Movement.WallTraversal.BlueprintConsumersCompile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPHWallTraversalBlueprintConsumersTest::RunTest(const FString&)
{
	const TCHAR* AssetPaths[] = {
		TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_BaseCharacterBP"),
		TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/ALS_PlayerCharacterBP"),
		TEXT("/ALSV4_CPP/AdvancedLocomotionV4/CharacterAssets/MannequinSkeleton/ALS_AnimBP"),
		TEXT("/ALSV4_CPP/AdvancedLocomotionV4/Blueprints/CharacterLogic/BP_ALS_Player_Controller")
	};
	const FName ContractFunctions[] = {
		TEXT("SprintAction"), TEXT("SprintAction_Completed"), TEXT("SprintAction_Canceled"),
		TEXT("JumpAction"), TEXT("JumpAction_Completed"), TEXT("TryStartWallTraversal"),
		TEXT("StopWallTraversal"), TEXT("CompleteWallToGroundTransition"),
		TEXT("GetWallTraversalWeight"), TEXT("SelectWallTraversalState"),
		TEXT("GetFootIKSurfaceNormal"), TEXT("GetWallTransitionData")
	};

	for (int32 AssetIndex = 0; AssetIndex < UE_ARRAY_COUNT(AssetPaths); ++AssetIndex)
	{
		const TCHAR* AssetPath = AssetPaths[AssetIndex];
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, AssetPath);
		if (!TestNotNull(FString::Printf(TEXT("Traversal consumer loads: %s"), AssetPath), Blueprint))
		{
			continue;
		}

		// Compile in memory; validation must never resave the user's character or animation assets.
		FKismetEditorUtilities::CompileBlueprint(Blueprint,
			EBlueprintCompileOptions::SkipSave | EBlueprintCompileOptions::SkipGarbageCollection |
			EBlueprintCompileOptions::SkipFiBSearchMetaUpdate);
		TestTrue(FString::Printf(TEXT("Traversal consumer compiles: %s"), AssetPath),
			Blueprint->Status == BS_UpToDate || Blueprint->Status == BS_UpToDateWithWarnings);
		UClass* GeneratedClass = Blueprint->GeneratedClass;
		if (!TestNotNull(TEXT("Compilation retains the generated class"), GeneratedClass))
		{
			continue;
		}

		if (AssetIndex < 2)
		{
			const APHBaseCharacter* Character = Cast<APHBaseCharacter>(GeneratedClass->GetDefaultObject());
			if (!TestNotNull(TEXT("The authored character retains its ProjectHunter parent"), Character))
			{
				continue;
			}
			TestNotNull(TEXT("The authored character uses ProjectHunter wall movement"),
				Character->GetPHMovementComponent());
			TestNotNull(TEXT("The authored character retains its stamina owner"),
				Character->GetAbilitySystemComponent());
		}

		for (const FName FunctionName : ContractFunctions)
		{
			if (const UFunction* Function = GeneratedClass->FindFunctionByName(FunctionName))
			{
				AddInfo(FString::Printf(TEXT("%s: %s is declared by %s"),
					*Blueprint->GetName(), *FunctionName.ToString(), *Function->GetOuter()->GetPathName()));
			}
		}

		TArray<UEdGraph*> Graphs;
		Blueprint->GetAllGraphs(Graphs);
		for (const UEdGraph* Graph : Graphs)
		{
			for (const UEdGraphNode* Node : Graph->Nodes)
			{
				if (!Node)
				{
					continue;
				}
				const FString Title = Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
				if (Graph->GetName() == TEXT("InputGraph") || Title.Contains(TEXT("Wall")) || Title.Contains(TEXT("Sprint")) ||
					Title.Contains(TEXT("JumpAction")))
				{
					AddInfo(FString::Printf(TEXT("%s / %s: %s"),
						*Blueprint->GetName(), *Graph->GetName(), *Title));
					for (const UEdGraphPin* Pin : Node->Pins)
					{
						if (Pin && Pin->Direction == EGPD_Output)
						{
							for (const UEdGraphPin* Link : Pin->LinkedTo)
							{
								AddInfo(FString::Printf(TEXT("  %s -> %s.%s"), *Pin->PinName.ToString(),
									*Link->GetOwningNode()->GetNodeTitle(ENodeTitleType::ListView).ToString(),
									*Link->PinName.ToString()));
							}
						}
					}
				}
			}
		}
	}
	return true;
}

#endif
