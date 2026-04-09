using System.IO;
using UnrealBuildTool;

public class SpaceshipCrew : ModuleRules
{
	public SpaceshipCrew(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		CppStandard = CppStandardVersion.Cpp20;

		PrivateIncludePaths.AddRange(new string[]
		{
			Path.Combine(ModuleDirectory, "Menu"),
			Path.Combine(ModuleDirectory, "UI")
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Slate",
			"SlateCore",
			"ApplicationCore"
		});

		if (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test)
		{
			PrivateDependencyModuleNames.Add("AutomationTest");
		}
	}
}
