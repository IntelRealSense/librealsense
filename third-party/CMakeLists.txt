string(REPLACE ${PROJECT_SOURCE_DIR}/ "" _rel_path ${CMAKE_CURRENT_LIST_DIR})

add_subdirectory( "${CMAKE_CURRENT_LIST_DIR}/rsutils" )

# The NEW policy flag set OpenGL_GL_PREFERENCE variable to GLVND if it was founds.
if (POLICY CMP0072 AND UNIX AND NOT APPLE)
	set(CMAKE_POLICY_DEFAULT_CMP0072 NEW)
endif()

# Add additional include directories to allow file to include rosbag headers
include(${_rel_path}/realsense-file/config.cmake)

if(BUILD_EASYLOGGINGPP)
    include(${_rel_path}/easyloggingpp/CMakeLists.txt)
endif()

add_subdirectory(${_rel_path}/realsense-file)

