string(REPLACE ${PROJECT_SOURCE_DIR}/ "" _rel_path ${CMAKE_CURRENT_LIST_DIR})

# ignore warnings on third party files
if (MSVC)
    add_compile_options(/W0) 
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
   add_compile_options(-w)
endif()

include(CMake/external_json.cmake)

add_subdirectory( "${CMAKE_CURRENT_LIST_DIR}/rsutils" )

# Add additional include directories to allow file to include rosbag headers
include(${_rel_path}/realsense-file/config.cmake)

if(BUILD_EASYLOGGINGPP)
    include(${_rel_path}/easyloggingpp/CMakeLists.txt)
endif()

add_subdirectory(${_rel_path}/realsense-file)
add_subdirectory( ${_rel_path}/tclap )

if( BUILD_WITH_DDS )
    add_subdirectory( "${CMAKE_CURRENT_LIST_DIR}/realdds" )
endif()

