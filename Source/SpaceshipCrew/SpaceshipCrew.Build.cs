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
			ModuleDirectory,
			Path.Combine(ModuleDirectory, "Menu"),
			Path.Combine(ModuleDirectory, "UI"),
			Path.Combine(ModuleDirectory, "ShipModule"),
			Path.Combine(ModuleDirectory, "ShipBuilder")
		});

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore",
			"ApplicationCore",
			"AssetRegistry"
		});

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"AssetTools",
				"ContentBrowser",
				"PropertyEditor",
				"ToolMenus",
				"AdvancedPreviewScene"
			});
		}

		if (Target.Configuration != UnrealTargetConfiguration.Shipping && Target.Configuration != UnrealTargetConfiguration.Test)
		{
			PrivateDependencyModuleNames.Add("AutomationTest");
		}
	}
}
