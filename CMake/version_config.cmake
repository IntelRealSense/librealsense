##################################################################
# Parse librealsense version and assign it to CMake variables    #
# This function parses librealsense public API header file, rs.h #
# and retrieves version numbers embedded in the source code.     #
# Since the function relies on hard-coded variables, it is prone #
# for failures should these constants be modified in future      #
##################################################################
function(assign_version_property VER_COMPONENT)
    file(STRINGS "./include/librealsense2/rs.h" REALSENSE_VERSION_${VER_COMPONENT} REGEX "#define RS2_API_${VER_COMPONENT}_VERSION")
    separate_arguments(REALSENSE_VERSION_${VER_COMPONENT})
    list(GET REALSENSE_VERSION_${VER_COMPONENT} -1 tmp)
    if (tmp LESS 0)
        message( FATAL_ERROR "Could not obtain valid Librealsense version ${VER_COMPONENT} component - actual value is ${tmp}" )
    endif()
    set(REALSENSE_VERSION_${VER_COMPONENT} ${tmp} PARENT_SCOPE)
endfunction()

set(REALSENSE_VERSION_MAJOR -1)
set(REALSENSE_VERSION_MINOR -1)
set(REALSENSE_VERSION_PATCH -1)
assign_version_property(MAJOR)
assign_version_property(MINOR)
assign_version_property(PATCH)
set(REALSENSE_VERSION_STRING ${REALSENSE_VERSION_MAJOR}.${REALSENSE_VERSION_MINOR}.${REALSENSE_VERSION_PATCH})
infoValue(REALSENSE_VERSION_STRING)
if (BUILD_GLSL_EXTENSIONS)
    set(REALSENSE-GL_VERSION_STRING ${REALSENSE_VERSION_MAJOR}.${REALSENSE_VERSION_MINOR}.${REALSENSE_VERSION_PATCH})
endif()
