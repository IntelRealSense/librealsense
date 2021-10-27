find_package(OpenGL REQUIRED)
set(DEPENDENCIES realsense2 glfw ${OPENGL_LIBRARIES})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_definitions(-DGL_SILENCE_DEPRECATION)
endif()
