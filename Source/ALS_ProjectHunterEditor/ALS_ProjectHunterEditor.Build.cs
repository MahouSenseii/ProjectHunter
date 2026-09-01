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
			"ALSV4_CPP",
			"AIModule",
			"NavigationSystem",
			"Slate",
			"SlateCore",
			"PropertyEditor",
			"UnrealEd",
			"UMG",
			"UMGEditor",
			"MovieScene",
			"MovieSceneTracks",
			"AssetRegistry",
			"AssetTools",
			"Json",
			"RenderCore",
			"RHI",
			"LevelEditor",
			"EditorFramework",
			"DetailCustomizations",
			"InputCore"
		});
	}
}
