message(STATUS "Setting Windows configurations")

config_crt()

macro(os_set_flags)
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    # Makes VS15 find the DLL when trying to run examples/tests
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
    # build with multiple cores
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /MP")
    ## Check for Windows Version ##
    if( (${CMAKE_SYSTEM_VERSION} EQUAL 6.1) OR (FORCE_WINUSB_UVC) ) # Windows 7
        message(STATUS "Build for Win7")
        set(BUILD_FOR_WIN7 ON)
        set(FORCE_WINUSB_UVC ON)
    else() # Some other windows version
        message(STATUS "Build for Windows > Win7")
        set(BUILD_FOR_OTHER_WIN ON)
        set(BACKEND RS2_USE_WMF_BACKEND)
    endif()

    if(FORCE_WINUSB_UVC)
        set(BACKEND RS2_USE_WINUSB_UVC_BACKEND)
    endif()

    if (MSVC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /bigobj /wd4819")
        set(LRS_TRY_USE_AVX true)
        add_definitions(-D_UNICODE)
    endif()
    set(DOTNET_VERSION_LIBRARY "3.5" CACHE STRING ".Net Version, defaulting to '3.5', the Unity wrapper currently supports only .NET 3.5")
    set(DOTNET_VERSION_EXAMPLES "4.0" CACHE STRING ".Net Version, defaulting to '4.0'")

    if(BUILD_EASYLOGGINGPP)
        add_definitions(-DNOMINMAX)
    endif()

    include(CMake/external_libusb.cmake)
endmacro()

macro(os_target_config)
    add_definitions(-D__SSSE3__ -D_CRT_SECURE_NO_WARNINGS)

    if(FORCE_WINUSB_UVC)
        target_sources(${LRS_TARGET}
            PRIVATE
            "${CMAKE_CURRENT_LIST_DIR}/src/win7/winusb_uvc/winusb_uvc.cpp"
            "${CMAKE_CURRENT_LIST_DIR}/src/libuvc/utlist.h"
            "${CMAKE_CURRENT_LIST_DIR}/src/win7/winusb_uvc/winusb_uvc.h"
        )
    endif()

    if(BUILD_FOR_WIN7)
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
