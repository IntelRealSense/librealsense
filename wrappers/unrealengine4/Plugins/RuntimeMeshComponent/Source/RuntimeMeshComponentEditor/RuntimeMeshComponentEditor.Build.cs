// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

using UnrealBuildTool;

public class RuntimeMeshComponentEditor : ModuleRules
{
    public RuntimeMeshComponentEditor(ReadOnlyTargetRules rules) : base(rules)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
            new string[] {
                "RuntimeMeshComponentEditor/Public"
				// ... add public include paths required here ...
			}
            );


        PrivateIncludePaths.AddRange(
            new string[] {
                "RuntimeMeshComponentEditor/Private",
				// ... add other private include paths required here ...
			}
            );


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
                "ShaderCore",
                "RHI",
                "Slate",
                "SlateCore",
                "UnrealEd",
                "PropertyEditor",
                "RawMesh",
                "AssetTools",
                "AssetRegistry",

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
