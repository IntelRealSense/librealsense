# - Find librealsense
# Find the LIBREALSENSE headers and libraries.
#
#  LIBREALSENSE_INCLUDE_DIRS - where to find rs.h
#  LIBREALSENSE_LIBRARIES    - List of libraries when using LIBREALSENSE.
#  LIBREALSENSE_FOUND        - True if LIBREALSENSE found.
#

#set target architeture variable 
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(TARGET_ARCH x64)
else()
	set(TARGET_ARCH x86)
endif()
#add windows/linux SUFFIX for libraries
if(WIN32)
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib" ".dll")
else()	
	set(CMAKE_FIND_LIBRARY_SUFFIXES ".so" ".a")
endif()

#try to search for the LIBREALSENSE_DIR enviroment VARIABLE
set(LIBREALSENSE_DIR
	"$ENV{LIBREALSENSE_DIR}"
	CACHE
	PATH
	"Root directory to search for LIBREALSENSE")

# Look for the header file.
find_path(LIBREALSENSE_INCLUDE_DIR
	NAMES
	rs.h
	HINTS
	"${LIBREALSENSE_DIR}/include/librealsense"
	)

# Look for the library.
find_library(LIBREALSENSE_LIBRARY
	NAMES
	realsense	
	HINTS
	"${LIBREALSENSE_DIR}/lib/${TARGET_ARCH}"
	"${LIBREALSENSE_DIR}/lib/"	
	)
# Look for the library debug flavor.
find_library(LIBREALSENSE_LIBRARY_DEBUG
	NAMES		
	realsense_debug
	HINTS
	"${LIBREALSENSE_DIR}/lib/${TARGET_ARCH}"	
	)

# handle the LIBREALSENSE and REQUIRED arguments and set LIBREALSENSE_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBREALSENSE
	DEFAULT_MSG
	LIBREALSENSE_LIBRARY
	LIBREALSENSE_LIBRARY_DEBUG
	LIBREALSENSE_INCLUDE_DIR)

if (LIBREALSENSE_FOUND AND NOT TARGET realsense)
	message(STATUS "LIBREALSENSE_INCLUDE_DIR: ${LIBREALSENSE_INCLUDE_DIR} LIBREALSENSE_LIBRARY: ${LIBREALSENSE_LIBRARY}")	
	add_library(realsense INTERFACE)
	set_property(TARGET realsense PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${LIBREALSENSE_INCLUDE_DIR})
  	target_include_directories(realsense INTERFACE ${LIBREALSENSE_DIR}/include)  	
  	target_link_libraries(realsense INTERFACE optimized ${LIBREALSENSE_LIBRARY} debug ${LIBREALSENSE_LIBRARY_DEBUG})
	mark_as_advanced(LIBREALSENSE_DIR LIBREALSENSE_LIBRARY LIBREALSENSE_INCLUDE_DIR)
endif()