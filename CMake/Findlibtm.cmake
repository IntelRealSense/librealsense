# - Find libtm
# Find the LIBTM headers and libraries.
# Set root directory of libtm as ENVIRONMENT VARIABLE: LIBTM_DIR
# Then use with find_package(libtm) in your project


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

#try to search for the LIBTM_DIR enviroment VARIABLE
set(LIBTM_DIR
	"$ENV{LIBTM_DIR}"
	CACHE
	PATH
	"Root directory to search for LIBTM")

# Look for the header file.
find_path(LIBTM_INCLUDE_DIR
	NAMES
	TrackingManager.h
	HINTS
	"${LIBTM_DIR}/libtm/include"
	)

# Look for the library.
find_library(LIBTM_LIBRARY
	NAMES
	libtm	
	HINTS
	"${LIBTM_DIR}/lib/${TARGET_ARCH}"
	"${LIBTM_DIR}/lib/"	
	)
# Look for the library debug flavor.
find_library(LIBTM_LIBRARY_DEBUG
	NAMES		
	libtm_debug
	HINTS
	"${LIBTM_DIR}/lib/${TARGET_ARCH}"	
	)

# handle the LIBTM and REQUIRED arguments and set LIBTM_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBTM
	DEFAULT_MSG
	LIBTM_LIBRARY
	LIBTM_LIBRARY_DEBUG
	LIBTM_INCLUDE_DIR)

if (LIBTM_FOUND AND NOT TARGET libtm)
	message(STATUS "LIBTM_INCLUDE_DIR: ${LIBTM_INCLUDE_DIR} LIBTM_LIBRARY: ${LIBTM_LIBRARY}")	
	add_library(libtm INTERFACE)
	set_property(TARGET libtm PROPERTY INTERFACE_INCLUDE_DIRECTORIES ${LIBTM_INCLUDE_DIR})
  	target_include_directories(libtm INTERFACE ${LIBTM_INCLUDE_DIR})  	
  	target_link_libraries(libtm INTERFACE optimized ${LIBTM_LIBRARY} debug ${LIBTM_LIBRARY_DEBUG})
	mark_as_advanced(LIBTM_DIR LIBTM_LIBRARY LIBTM_INCLUDE_DIR)
endif()
