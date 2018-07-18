cmake_minimum_required( VERSION 3.8.0 )

project(Intel.RealSense)

add_library(${PROJECT_NAME} 
	Context.cs
	Device.cs
	Frame.cs
	FrameQueue.cs
	FrameSet.cs
	Helpers.cs
	NativeMethods.cs
	Pipeline.cs
	Processing.cs
	Sensor.cs
	StreamProfile.cs
	Types.cs
	SoftwareDevice.cs
	.nuget/Intel.RealSense.targets
)

csharp_set_designer_cs_properties(
	.nuget/Intel.RealSense.targets
	Properties/AssemblyInfo.cs
)

if(BUILD_UNITY_BINDINGS)
	add_custom_command(TARGET ${PROJECT_NAME}
			   POST_BUILD
			   COMMAND XCOPY /y /s "$(OutDir)*.dll" "${CMAKE_BINARY_WIN_DIR}\\wrappers\\unity\\Assets\\RealSenseSDK2.0\\Plugins\\"
			   COMMENT "Copy DLLs to Unity plugins folder")
			   
	find_program (UNITY_PATH Unity\\Editor\\Unity.exe)
	if(EXISTS ${UNITY_PATH})
		message(STATUS "Found Unity at - ${UNITY_PATH}, will try to generate RealSense Unity package on C# wrapper build")
		string(REPLACE "/" "\\" UNITY_WIN_PATH ${UNITY_PATH})
		add_custom_command(TARGET ${PROJECT_NAME}
				   POST_BUILD
				   COMMAND ${UNITY_WIN_PATH} -quit -batchmode -projectPath "${CMAKE_BINARY_WIN_DIR}\\wrappers\\unity" -exportPackage "Assets" "$(OutDir)realsense.unitypackage" || EXIT 0
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