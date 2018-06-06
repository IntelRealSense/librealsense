cmake_minimum_required( VERSION 3.8.2 )

project(cs-tutorial-1-depth)

add_executable(${PROJECT_NAME} Program.cs)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_TARGET_FRAMEWORK_VERSION "v${DOTNET_TARGET_FRAMEWORK_VERSION}")
# set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE TRUE)

add_dependencies(${PROJECT_NAME} Intel.RealSense)

set_property(TARGET ${PROJECT_NAME} PROPERTY VS_DOTNET_REFERENCES
	"System"
)

set_target_properties (${PROJECT_NAME} PROPERTIES
	FOLDER Examples/CSharp
)