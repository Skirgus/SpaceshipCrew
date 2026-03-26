// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SpaceshipCrew : ModuleRules
{
	public SpaceshipCrew(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"NavigationSystem"
		});

		PublicIncludePaths.AddRange(new string[] {
			"SpaceshipCrew",
			"SpaceshipCrew/Variant_Platforming",
			"SpaceshipCrew/Variant_Platforming/Animation",
			"SpaceshipCrew/Variant_Combat",
			"SpaceshipCrew/Variant_Combat/AI",
			"SpaceshipCrew/Variant_Combat/Animation",
			"SpaceshipCrew/Variant_Combat/Gameplay",
			"SpaceshipCrew/Variant_Combat/Interfaces",
			"SpaceshipCrew/Variant_Combat/UI",
			"SpaceshipCrew/Variant_SideScrolling",
			"SpaceshipCrew/Variant_SideScrolling/AI",
			"SpaceshipCrew/Variant_SideScrolling/Gameplay",
			"SpaceshipCrew/Variant_SideScrolling/Interfaces",
			"SpaceshipCrew/Variant_SideScrolling/UI",
			"SpaceshipCrew/Ship"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
