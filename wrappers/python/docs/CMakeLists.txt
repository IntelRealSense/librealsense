cmake_minimum_required(VERSION 3.8)

project(RealsensePythonDocs)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/CMake")
find_package(Sphinx REQUIRED)
 
if(NOT DEFINED SPHINX_THEME)
	set(SPHINX_THEME default)
endif()
 
if(NOT DEFINED SPHINX_THEME_DIR)
	set(SPHINX_THEME_DIR)
endif()
 
# configured documentation tools and intermediate build results
set(BINARY_BUILD_DIR "${CMAKE_CURRENT_BINARY_DIR}/_build")
 
# Sphinx cache with pickled ReST documents
set(SPHINX_CACHE_DIR "${CMAKE_CURRENT_BINARY_DIR}/_doctrees")
 
# HTML output directory
set(SPHINX_HTML_DIR "${CMAKE_CURRENT_BINARY_DIR}/html")

# Sphinx Template directory
set(SPHINX_TEMPLATE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/_templates")
 
configure_file(
	"${CMAKE_CURRENT_SOURCE_DIR}/conf.py.in"
	"${BINARY_BUILD_DIR}/conf.py"
	@ONLY)
 
 # if sphinx is causing issues building, replace -q with -v for a verbose error
add_custom_target(pyrealsense2_docs ALL
	${SPHINX_EXECUTABLE}
		-q -b html
		-c "${BINARY_BUILD_DIR}"
		-d "${SPHINX_CACHE_DIR}"
		"${CMAKE_CURRENT_SOURCE_DIR}"
		"${SPHINX_HTML_DIR}"
	COMMENT "Building HTML documentation for pyrealsense2 with Sphinx")

add_dependencies(pyrealsense2_docs pyrealsense2)