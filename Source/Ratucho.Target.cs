using UnrealBuildTool;

public class RatuchoTarget : TargetRules
{
	public RatuchoTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Ratucho");
	}
}
