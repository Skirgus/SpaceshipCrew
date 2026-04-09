using UnrealBuildTool;
using System.Collections.Generic;

public class SpaceshipCrewEditorTarget : TargetRules
{
	public SpaceshipCrewEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("SpaceshipCrew");
	}
}
