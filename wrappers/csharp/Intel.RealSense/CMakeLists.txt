cmake_minimum_required( VERSION 3.8.0 )

project(Intel.RealSense)

add_library(${PROJECT_NAME} 	
	Config.cs
	Context.cs
	FrameQueue.cs
	FrameSet.cs
	FrameSetMarshaler.cs
	FramesReleaser.cs
	Helpers.cs
	Log.cs
	NativeMethods.cs
	Pipeline.cs
	PipelineProfile.cs
	Sensor.cs
	SensorList.cs
	SoftwareDevice.cs
	SoftwareSensor.cs
	Devices\AdvancedDevice.cs
	Devices\Device.cs
	Devices\DeviceList.cs
	Devices\PlaybackDevice.cs
	Devices\RecordDevice.cs
	Frames\DepthFrame.cs
	Frames\Frame.cs
	Frames\FrameMarshaler.cs
	Frames\Points.cs
	Frames\VideoFrame.cs
	Processing\Align.cs
	Processing\Colorizer.cs
	Processing\CustomProcessingBlock.cs
	Processing\DecimationFilter.cs
	Processing\DisparityTransform.cs
	Processing\FrameSource.cs
	Processing\HoleFillingFilter.cs
	Processing\PointCloud.cs
	Processing\ProcessingBlock.cs
	Processing\SpatialFilter.cs
	Processing\Syncer.cs
	Processing\TemporalFilter.cs
	Profiles\StreamProfile.cs
	Profiles\StreamProfileList.cs
	Profiles\VideoStreamProfile.cs
	Types\CameraInfo.cs
	Types\Delegates.cs
	Types\Distortion.cs
	Types\ExceptionType.cs
	Types\Extension.cs
	Types\Extrinsics.cs
	Types\Format.cs
	Types\FrameMetadataValue.cs
	Types\Intrinsics.cs
	Types\LogSeverity.cs
	Types\Matchers.cs
	Types\NotificationCategory.cs
	Types\Option.cs
	Types\PlaybackStatus.cs
	Types\RecordingMode.cs
	Types\Rs400VisualPreset.cs
	Types\SoftwareVideoFrame.cs
	Types\SoftwareVideoStream.cs
	Types\Sr300VisualPreset.cs
	Types\Stream.cs
	Types\TimestampDomain.cs
	Types\VideoStream.cs


	Option.cs
	.nuget/Intel.RealSense.targets
)

csharp_set_designer_cs_properties(
	.nuget/Intel.RealSense.targets
)

if(BUILD_UNITY_BINDINGS)
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

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_REFERENCES
	"System"
)
