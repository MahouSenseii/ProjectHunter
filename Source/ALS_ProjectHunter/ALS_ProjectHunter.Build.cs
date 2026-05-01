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
			"GameplayTags",
			"PhysicsCore",
			"Niagara",
			"EnhancedInput",
			"ALSV4_CPP",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"GameplayTasks"
		});
	}
}
