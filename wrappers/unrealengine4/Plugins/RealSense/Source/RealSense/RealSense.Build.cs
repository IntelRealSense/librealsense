using UnrealBuildTool;
using System.IO;

public class RealSense : ModuleRules
{
	public RealSense(ReadOnlyTargetRules Target) : base(Target)
	{
		bEnableExceptions = true;
		bUseRTTI = false;

		PrivateIncludePaths.AddRange(new string[] { Path.Combine( ModuleDirectory, "Private") });
		PublicIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Public") });

		PrivatePCHHeaderFile = "Private/PCH.h";

		PublicDependencyModuleNames.AddRange(
			new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"RHI",
				"RenderCore",
				"Slate",
				"SlateCore",
				"RuntimeMeshComponent"
			}
		);

		if (Target.bBuildEditor)
		{
			PublicDependencyModuleNames.AddRange(
			new string[] {
				"UnrealEd",
				"EditorStyle",
				"DesktopWidgets",
				"DesktopPlatform"
			});
		}

		//string RealSenseDirectory = Environment.GetEnvironmentVariable("RSSDK_DIR");
		string RealSenseDirectory = "C:/Program Files (x86)/Intel RealSense SDK 2.0";

		PublicIncludePaths.Add(Path.Combine(RealSenseDirectory, "include"));
		PublicLibraryPaths.Add(Path.Combine(RealSenseDirectory, "lib", "x64"));
		PublicLibraryPaths.Add(Path.Combine(RealSenseDirectory, "bin", "x64"));
		PublicAdditionalLibraries.Add("realsense2.lib");
	}
}
