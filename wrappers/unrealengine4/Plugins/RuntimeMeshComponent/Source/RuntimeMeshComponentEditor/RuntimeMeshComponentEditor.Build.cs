// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class RuntimeMeshComponentEditor : ModuleRules
{
    public RuntimeMeshComponentEditor(ReadOnlyTargetRules rules) : base(rules)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // HORU: this was throwing warnings
        //     PublicIncludePaths.AddRange(
        //         new string[] {
        //             "RuntimeMeshComponentEditor/Public"
        //	// ... add public include paths required here ...
        //}
        //         );

        // HORU: this was throwing warnings
        //     PrivateIncludePaths.AddRange(
        //         new string[] {
        //             "RuntimeMeshComponentEditor/Private",
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
                // ... add private dependencies that you statically link with here ...	
                "Engine",
                "Slate",
                "SlateCore",
                "RenderCore",
                "RHI",
                "NavigationSystem",
                "UnrealEd",
                "LevelEditor",
                "PropertyEditor",
                "RawMesh",
                "AssetTools",
                "AssetRegistry",
                "Projects",
                "EditorStyle",
                "InputCore",

                "RuntimeMeshComponent",
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