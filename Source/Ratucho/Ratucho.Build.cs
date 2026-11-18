using UnrealBuildTool;

public class Ratucho : ModuleRules
{
	public Ratucho(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory + "/Public");

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"NetCore",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",
			"Slate",
			"SlateCore"
		});
	}
}
