# For debian based systems, the following addition to the CMake process, with additional script in the scripts folder,
# allows a package creator to generate a .deb file using the following command:
#   sudo cpack -G DEB
# This may also work for other package managers (RPM and others) because cpack also supports those, but it hasn't been tested.

# these are cache variables, so they could be overwritten with -D,
set(CPACK_PACKAGE_NAME ${PROJECT_NAME}
    CACHE STRING "librealsense2"
)
# which is useful in case of packing only selected components instead of the whole thing
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "librealsense2"
    CACHE STRING "librealsense2 package"
)
set(CPACK_PACKAGE_VENDOR "librealsense2")

set(CPACK_VERBATIM_VARIABLES YES)

set(CPACK_PACKAGE_INSTALL_DIRECTORY ${CPACK_PACKAGE_NAME})
SET(CPACK_OUTPUT_FILE_PREFIX "${CMAKE_SOURCE_DIR}/_packages")

# https://unix.stackexchange.com/a/11552/254512
#set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/librealsense2/${CMAKE_PROJECT_VERSION}")

set(CPACK_PACKAGE_VERSION_MAJOR ${REALSENSE_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${REALSENSE_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${REALSENSE_VERSION_PATCH})

set(CPACK_PACKAGE_CONTACT "jpswensen@gmail.com")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "John Swensen")

# package name for deb
# if set, then instead of some-application-0.9.2-Linux.deb
# you'll get some-application_0.9.2_amd64.deb (note the underscores too)
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
# if you want every group to have its own package,
# although the same happens if this is not sent (so it defaults to ONE_PER_GROUP)
# and CPACK_DEB_COMPONENT_INSTALL is set to YES
set(CPACK_COMPONENTS_GROUPING ALL_COMPONENTS_IN_ONE)#ONE_PER_GROUP)
# without this you won't be able to pack only specified component
set(CPACK_DEB_COMPONENT_INSTALL YES)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_GENERATE_SHLIBS ON)

set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA
	"${CMAKE_CURRENT_SOURCE_DIR}/scripts/postinst")

include(CPack)
