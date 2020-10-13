using UnrealBuildTool;
using System.Collections.Generic;

public class RealSenseUETarget : TargetRules
{
	public RealSenseUETarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

        ExtraModuleNames.AddRange( new string[] { "RealSenseUE" } );
	}
}
