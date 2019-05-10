// Copyright 2016-2018 Chris Conway (Koderz). All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class RuntimeMeshComponent : ModuleRules
{
    public RuntimeMeshComponent(ReadOnlyTargetRules rules) : base(rules)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Private") });
		PublicIncludePaths.AddRange(new string[] { Path.Combine(ModuleDirectory, "Public") });

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
#if !UE_4_22_OR_LATER
				"ShaderCore",  // ShaderCore was Merged into RenderCore in 4.22
#endif
				"RHI",
#if UE_4_20_OR_LATER
				"NavigationSystem"
#endif
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
