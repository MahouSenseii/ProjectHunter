// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ALS_ProjectHunter : ModuleRules
{
	public ALS_ProjectHunter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"NavigationSystem",
			"AIModule",
			"GameplayAbilities",
			"PhysicsCore",
			"Niagara",
			"EnhancedInput",
			"ALSV4_CPP",
			"Paper2D" 
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate", 
			"SlateCore", 
			"GameplayTags",
			"GameplayTasks",
			"EditorFramework"
		});
		
		
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"EditorSubsystem",
				"EditorWidgets",
				"ToolMenus",
				"EditorStyle", // For UE4
				"EditorFramework" // For UE5
			});
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
