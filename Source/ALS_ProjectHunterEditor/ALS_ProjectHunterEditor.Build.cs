using UnrealBuildTool;

public class ALS_ProjectHunterEditor : ModuleRules
{
	public ALS_ProjectHunterEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"ALS_ProjectHunter",
			"GameplayAbilities",
			"GameplayTags",
			"GameplayTasks"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"PropertyEditor",
			"UnrealEd",
			"EditorFramework",
			"DetailCustomizations",
			"InputCore"
		});
	}
}