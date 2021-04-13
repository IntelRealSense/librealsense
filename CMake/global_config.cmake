# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_CXX_STANDARD 11)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(GNUInstallDirs)
# include librealsense helper macros
include(CMake/lrs_macros.cmake)
include(CMake/version_config.cmake)

if(ENABLE_CCACHE)
  find_program(CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
      set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
      set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
  endif(CCACHE_FOUND)
endif()

macro(global_set_flags)
    set(LRS_TARGET realsense2)
    set(LRS_LIB_NAME ${LRS_TARGET})

    add_definitions(-DELPP_THREAD_SAFE)
    add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)

    if (BUILD_GLSL_EXTENSIONS)
        set(LRS_GL_TARGET realsense2-gl)
        set(LRS_GL_LIB_NAME ${LRS_GL_TARGET})
    endif()

    if (ENABLE_ZERO_COPY)
        add_definitions(-DZERO_COPY)
    endif()

    if (BUILD_EASYLOGGINGPP)
        add_definitions(-DBUILD_EASYLOGGINGPP)
    endif()

    if (ENABLE_EASYLOGGINGPP_ASYNC)
        add_definitions(-DEASYLOGGINGPP_ASYNC)
    endif()

    if(TRACE_API)
        add_definitions(-DTRACE_API)
    endif()

    if(HWM_OVER_XU)
        add_definitions(-DHWM_OVER_XU)
    endif()

    if(COM_MULTITHREADED)
        add_definitions(-DCOM_MULTITHREADED)
    endif()

    if (ENFORCE_METADATA)
      add_definitions(-DENFORCE_METADATA)
    endif()

    if (BUILD_WITH_CUDA)
        add_definitions(-DRS2_USE_CUDA)
    endif()

    if (BUILD_SHARED_LIBS)
        add_definitions(-DBUILD_SHARED_LIBS)
    endif()

    if (BUILD_INTERNAL_UNIT_TESTS)
        add_definitions(-DBUILD_INTERNAL_UNIT_TESTS)
    endif()

    if (BUILD_WITH_CUDA)
        include(CMake/cuda_config.cmake)
    endif()

    if(BUILD_PYTHON_BINDINGS)
        include(libusb_config)
    endif()

    if(BUILD_NETWORK_DEVICE)
        add_definitions(-DNET_DEVICE)
        set(LRS_NET_TARGET realsense2-net)
    endif()
    
    if(CHECK_FOR_UPDATES)
        if (ANDROID_NDK_TOOLCHAIN_INCLUDED)
            message(STATUS "Android build do not support CHECK_FOR_UPDATES flag, turning it off..")
            set(CHECK_FOR_UPDATES false)
        elseif (NOT BUILD_GRAPHICAL_EXAMPLES)
            message(STATUS "CHECK_FOR_UPDATES depends on BUILD_GRAPHICAL_EXAMPLES flag, turning it off..")
            set(CHECK_FOR_UPDATES false)
        else()
            include(CMake/external_libcurl.cmake)
            add_definitions(-DCHECK_FOR_UPDATES)
        endif()
    endif()
    
    add_definitions(-D${BACKEND} -DUNICODE)
endmacro()

macro(global_target_config)
    target_link_libraries(${LRS_TARGET} PRIVATE realsense-file ${CMAKE_THREAD_LIBS_INIT})

    set_target_properties (${LRS_TARGET} PROPERTIES FOLDER Library)

    target_include_directories(${LRS_TARGET}
        PRIVATE
            src
            ${ROSBAG_HEADER_DIRS}
            ${BOOST_INCLUDE_PATH}
            ${LZ4_INCLUDE_PATH}
            ${LIBUSB_LOCAL_INCLUDE_PATH}
        PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
            PRIVATE ${USB_INCLUDE_DIRS}
    )



endmacro()

macro(add_tm2)
    message(STATUS "Building with TM2")
    include(libusb_config)
    target_link_libraries(${LRS_TARGET} PRIVATE usb)
    if(USE_EXTERNAL_USB)
        add_dependencies(${LRS_TARGET} libusb)
    endif()
    target_compile_definitions(${LRS_TARGET} PRIVATE WITH_TRACKING=1)
endmacro()
