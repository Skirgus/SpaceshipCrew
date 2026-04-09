using UnrealBuildTool;
using System.Collections.Generic;

public class SpaceshipCrewTarget : TargetRules
{
	public SpaceshipCrewTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("SpaceshipCrew");
	}
}
