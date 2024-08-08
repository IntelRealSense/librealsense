cmake_minimum_required( VERSION 3.8.0 )

project(Intel.RealSense)

set(LRS_DOTNET_TARGET ${PROJECT_NAME})

add_library(${PROJECT_NAME}
    Context.cs
    NativeMethods.cs
    .nuget/Intel.RealSense.targets
    GlobalSuppressions.cs
    Properties/AssemblyInfo.cs
    packages.config
    stylecop.json
)

include(Base/CMakeLists.txt)
include(Devices/CMakeLists.txt)
include(Frames/CMakeLists.txt)
include(Pipeline/CMakeLists.txt)
include(Processing/CMakeLists.txt)
include(StreamProfiles/CMakeLists.txt)
include(Sensors/CMakeLists.txt)
include(Types/CMakeLists.txt)
include(Helpers/CMakeLists.txt)
include(Options/CMakeLists.txt)

csharp_set_designer_cs_properties(
    .nuget/Intel.RealSense.targets
)

if(BUILD_UNITY_BINDINGS)
	string(REPLACE "/" "\\" CMAKE_BINARY_WIN_DIR ${CMAKE_BINARY_DIR})
	string(REPLACE "/" "\\" CMAKE_SOURCE_WIN_DIR ${CMAKE_SOURCE_DIR})
	file(MAKE_DIRECTORY ${CMAKE_BINARY_WIN_DIR}/wrappers)
	if(NOT EXISTS ${CMAKE_BINARY_WIN_DIR}/wrappers/unity)
		execute_process(COMMAND cmd "/C mklink /D ${CMAKE_BINARY_WIN_DIR}\\wrappers\\unity ${CMAKE_SOURCE_WIN_DIR}\\wrappers\\unity")
	endif()
    add_custom_command(TARGET ${PROJECT_NAME}
               POST_BUILD
               COMMAND XCOPY /y /s "$(OutDir)Intel.RealSense.dll" "${CMAKE_BINARY_WIN_DIR}\\wrappers\\unity\\Assets\\RealSenseSDK2.0\\Plugins\\"
               COMMENT "Copy Intel.RealSense.dll to Unity plugins folder")
    add_custom_command(TARGET ${PROJECT_NAME}
               POST_BUILD
               COMMAND XCOPY /y /s "$(OutDir)realsense2.dll" "${CMAKE_BINARY_WIN_DIR}\\wrappers\\unity\\Assets\\RealSenseSDK2.0\\Plugins\\"
               COMMENT "Copy realsense2.dll to Unity plugins folder")

    find_program (UNITY_PATH Unity\\Editor\\Unity.exe)
    if(EXISTS ${UNITY_PATH})
        message(STATUS "Found Unity at - ${UNITY_PATH}, will try to generate RealSense Unity package on C# wrapper build")
        string(REPLACE "/" "\\" UNITY_WIN_PATH ${UNITY_PATH})
        add_custom_command(TARGET ${PROJECT_NAME}
                   POST_BUILD
                   COMMAND ${UNITY_WIN_PATH} -quit -batchmode -logFile "$(OutDir)Unity.log" -projectPath "${CMAKE_BINARY_WIN_DIR}\\wrappers\\unity" -exportPackage "Assets" "$(OutDir)realsense.unitypackage" || EXIT 0
                   COMMENT "Try to generate a Unity package")
    else()
        message(WARNING "Couldn't locate Unity.exe")
    endif()
endif()

set_property(SOURCE App.xaml PROPERTY VS_XAML_TYPE "ApplicationDefinition")

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v${DOTNET_VERSION_LIBRARY}")

add_dependencies(${PROJECT_NAME} realsense2)

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
