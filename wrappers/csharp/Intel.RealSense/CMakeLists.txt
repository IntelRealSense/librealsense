cmake_minimum_required( VERSION 3.8.2 )

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
	.nuget/Intel.RealSense.targets
)

csharp_set_designer_cs_properties(
	.nuget/Intel.RealSense.targets
	Properties/AssemblyInfo.cs
)

set_property(SOURCE App.xaml PROPERTY VS_XAML_TYPE "ApplicationDefinition")

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v${DOTNET_TARGET_FRAMEWORK_VERSION}")

add_dependencies(${PROJECT_NAME} realsense2)

set_target_properties (${PROJECT_NAME} PROPERTIES
	FOLDER Library/CSharp
)