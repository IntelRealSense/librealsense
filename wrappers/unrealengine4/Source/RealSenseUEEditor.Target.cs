using UnrealBuildTool;
using System.Collections.Generic;

public class RealSenseUEEditorTarget : TargetRules
{
	public RealSenseUEEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

        ExtraModuleNames.AddRange( new string[] { "RealSenseUE" } );
	}
}
