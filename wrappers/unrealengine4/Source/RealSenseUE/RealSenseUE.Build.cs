using UnrealBuildTool;

public class RealSenseUE : ModuleRules
{
	public RealSenseUE(ReadOnlyTargetRules Target) : base(Target)
	{
		bEnableExceptions = true;
		bUseRTTI = false;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore" });
		PrivateDependencyModuleNames.AddRange(new string[] {  });

		PublicDependencyModuleNames.AddRange(new string[] { "RealSense" });
		PublicIncludePathModuleNames.AddRange(new string[] { "RealSense" });
	}
}
