cmake_minimum_required( VERSION 3.8.0 )

project(cs-tutorial-load-json)

add_executable(${PROJECT_NAME}
	Program.cs
	Window.xaml
	Window.xaml.cs
	ADP_TestLoadSettingsJson.cs

	Properties/AssemblyInfo.cs
)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v${DOTNET_VERSION_EXAMPLES}")
# set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE TRUE)

add_dependencies(${PROJECT_NAME} Intel.RealSense)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_REFERENCES
	"System"
	"System.Xaml"
	"PresentationCore"
	"PresentationFramework"
	"WindowsBase"
)

set_target_properties (${PROJECT_NAME} PROPERTIES
	FOLDER Wrappers/csharp
)

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_CURRENT_SOURCE_DIR}/JsonConfigs $<TARGET_FILE_DIR:${PROJECT_NAME}>/JsonConfigs)

