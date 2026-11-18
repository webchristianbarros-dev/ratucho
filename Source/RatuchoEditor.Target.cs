using UnrealBuildTool;

public class RatuchoEditorTarget : TargetRules
{
	public RatuchoEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("Ratucho");
	}
}
