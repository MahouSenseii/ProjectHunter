using UnrealBuildTool;

public class ALS_ProjectHunter : ModuleRules
{
	public ALS_ProjectHunter(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{ 
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
			"Paper2D",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"GameplayTags",
			"GameplayTasks"
		});
		
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"EditorFramework",
				"UnrealEd",
				"EditorSubsystem",
				"EditorWidgets",
				"ToolMenus"
			});
		}
	}
}
