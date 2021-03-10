message(STATUS "Setting Windows configurations")
cmake_minimum_required(VERSION 3.6.0) #Required by list(FILTER ...
config_crt()

macro(os_set_flags)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    # Makes VS15 find the DLL when trying to run examples/tests
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

    if(BUILD_WITH_OPENMP)
        find_package(OpenMP REQUIRED)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    endif()

    ## Check for Windows Version ##
    if(${CMAKE_SYSTEM_VERSION} EQUAL 6.1) # Windows 7
        set(FORCE_RSUSB_BACKEND ON)
    endif()

    if(FORCE_RSUSB_BACKEND)
        set(BACKEND RS2_USE_WINUSB_UVC_BACKEND)
    else()
        set(BACKEND RS2_USE_WMF_BACKEND)
    endif()

    if(MSVC)
        # Set CMAKE_DEBUG_POSTFIX to "d" to add a trailing "d" to library
        # built in debug mode. In this Windows user can compile, build and install the
        # library in both Release and Debug configuration avoiding naming clashes in the
        # installation directories.
        set(CMAKE_DEBUG_POSTFIX "d")

        # build with multiple cores
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")

        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /wd4819")
        set(LRS_TRY_USE_AVX true)
        add_definitions(-D_UNICODE)
    endif()
    set(DOTNET_VERSION_LIBRARY "3.5" CACHE STRING ".Net Version, defaulting to '3.5', the Unity wrapper currently supports only .NET 3.5")
    set(DOTNET_VERSION_EXAMPLES "4.0" CACHE STRING ".Net Version, defaulting to '4.0'")

    if(BUILD_EASYLOGGINGPP)
        add_definitions(-DNOMINMAX)
    endif()
endmacro()

macro(os_target_config)
    add_definitions(-D__SSSE3__ -D_CRT_SECURE_NO_WARNINGS)

    if(FORCE_RSUSB_BACKEND)
        if (NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
            message("Preparing Windows 7 drivers" )
            make_directory(${CMAKE_CURRENT_BINARY_DIR}/drivers/)
            file(GLOB DRIVERS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}/src/win7/drivers/" "${CMAKE_CURRENT_SOURCE_DIR}/src/win7/drivers/*")
            foreach(item IN LISTS DRIVERS)
                message("Copying ${CMAKE_CURRENT_SOURCE_DIR}/src/win7/drivers/${item} to ${CMAKE_CURRENT_BINARY_DIR}/drivers/" )
                configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/win7/drivers/${item}" "${CMAKE_CURRENT_BINARY_DIR}/drivers/${item}" COPYONLY)
            endforeach()
        endif()

        add_custom_target(realsense-driver ALL DEPENDS ${DRIVERS})
        add_dependencies(${LRS_TARGET} realsense-driver)
        set_target_properties (realsense-driver PROPERTIES FOLDER Library)
    endif()

    get_target_property(LRS_FILES ${LRS_TARGET} SOURCES)
    list(APPEND LRS_HEADERS ${LRS_FILES})
    list(APPEND LRS_SOURCES ${LRS_FILES})
    list(FILTER LRS_HEADERS INCLUDE REGEX ".\\.h$|.\\.hpp$|.\\.def$|.\\.cuh$")
    list(FILTER LRS_SOURCES INCLUDE REGEX ".\\.c$|.\\.cpp$|.\\.cc$|.\\.cu$")

    foreach(_file IN ITEMS ${LRS_HEADERS})
        string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/ "" _relative_file ${_file})
        get_filename_component(_file_path "${_relative_file}" PATH)
        string(REPLACE "/" "\\" _file_path_msvc "${_file_path}")
        source_group("Header Files\\${_file_path_msvc}" FILES "${_relative_file}")
    endforeach()

    foreach(_file IN ITEMS ${LRS_SOURCES})
        string(REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/ "" _relative_file ${_file})
        get_filename_component(_file_path "${_relative_file}" PATH)
        string(REPLACE "/" "\\" _file_path_msvc "${_file_path}")
        source_group("Source Files\\${_file_path_msvc}" FILES "${_relative_file}")
    endforeach()
endmacro()

#modify variable with prefix. Mimics list(TRANSFORM ... PREPEND introduced with cmake 3.12
FUNCTION(PREPEND var prefix)
   SET(listVar "")
   FOREACH(f ${ARGN})
      LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${var} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)
