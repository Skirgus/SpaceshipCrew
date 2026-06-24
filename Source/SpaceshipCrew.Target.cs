using UnrealBuildTool;
using System.Collections.Generic;

public class SpaceshipCrewTarget : TargetRules
{
	public SpaceshipCrewTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("SpaceshipCrew");
	}
}
