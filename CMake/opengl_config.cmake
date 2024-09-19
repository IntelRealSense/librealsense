# The NEW policy flag set OpenGL_GL_PREFERENCE variable to GLVND.
if (POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif()

find_package(OpenGL REQUIRED)
list( APPEND DEPENDENCIES glfw ${OPENGL_LIBRARIES} )
