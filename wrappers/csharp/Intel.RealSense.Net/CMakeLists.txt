cmake_minimum_required( VERSION 3.8.0 )

project(Intel.RealSense.Net)

set(LRS_DOTNET_TARGET ${PROJECT_NAME})

add_library(${PROJECT_NAME}
    NativeMethods.cs
    .nuget/Intel.RealSense.targets
    GlobalSuppressions.cs
    Properties/AssemblyInfo.cs
    packages.config
    stylecop.json
)

include(Devices/CMakeLists.txt)

csharp_set_designer_cs_properties(
    .nuget/Intel.RealSense.Net.targets
)


set_property(SOURCE App.xaml PROPERTY VS_XAML_TYPE "ApplicationDefinition")

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v${DOTNET_VERSION_LIBRARY}")

add_dependencies(${PROJECT_NAME} Intel.RealSense)

set_target_properties (${PROJECT_NAME} PROPERTIES
    FOLDER Wrappers/csharp
)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_REFERENCES
    "System"
)

target_compile_options(${PROJECT_NAME} PRIVATE /define:${CMAKE_SYSTEM_NAME})

set_source_files_properties(stylecop.json PROPERTIES VS_TOOL_OVERRIDE "AdditionalFiles")

install(TARGETS ${PROJECT_NAME}
    EXPORT realsense2Targets
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    #LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    #ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
