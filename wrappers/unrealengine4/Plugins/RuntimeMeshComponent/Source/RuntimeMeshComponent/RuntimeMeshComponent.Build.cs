// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class RuntimeMeshComponent : ModuleRules
{
    public RuntimeMeshComponent(ReadOnlyTargetRules rules) : base(rules)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bFasterWithoutUnity = true;

        // HORU: this was throwing warnings
        //     PublicIncludePaths.AddRange(
        //         new string[] {
        //             "RuntimeMeshComponent/Public"
        //	// ... add public include paths required here ...
        //}
        //         );

        // HORU: this was throwing warnings
        //     PrivateIncludePaths.AddRange(
        //         new string[] {
        //             "RuntimeMeshComponent/Private",
        //	// ... add other private include paths required here ...
        //}
        //         );


        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
				// ... add other public dependencies that you statically link with here ...
			}
            );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
				// ... add private dependencies that you statically link with here ...	
                "RenderCore",
                "RHI",
                "NavigationSystem"
            }
            );


        DynamicallyLoadedModuleNames.AddRange(
            new string[]
            {
				// ... add any modules that your module loads dynamically here ...
			}
            );
    }
}