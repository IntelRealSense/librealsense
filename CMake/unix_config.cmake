message(STATUS "Setting Unix configurations")

macro(os_set_flags)

    # Put all the collaterals together, so we can find when trying to run examples/tests
    # Note: this puts the outputs under <binary>/<build-type>
    if( "${CMAKE_BUILD_TYPE}" STREQUAL "" )
        # This can happen according to the docs -- and in GHA...
        message( STATUS "No output directory set; using ${CMAKE_BINARY_DIR}/Release/" )
        set( CMAKE_BUILD_TYPE "Release" )
    endif()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE})

    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -pedantic -D_DEFAULT_SOURCE")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pedantic -Wno-missing-field-initializers")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-switch -Wno-multichar -Wsequence-point -Wformat -Wformat-security")

    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpmachine OUTPUT_VARIABLE MACHINE)
    if(${MACHINE} MATCHES "arm-*")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mfpu=neon -mfloat-abi=hard -ftree-vectorize -latomic")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon -mfloat-abi=hard -ftree-vectorize -latomic")
    elseif(${MACHINE} MATCHES "aarch64-*")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mstrict-align -ftree-vectorize")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstrict-align -ftree-vectorize")
    elseif(${MACHINE} MATCHES "powerpc64(le)?-linux-gnu")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -ftree-vectorize")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftree-vectorize")
    else()
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mssse3")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3")
        set(LRS_TRY_USE_AVX true)
    endif(${MACHINE} MATCHES "arm-*")

    if(BUILD_WITH_OPENMP)
        find_package(OpenMP REQUIRED)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    endif()
    
    if(APPLE)
        set(FORCE_RSUSB_BACKEND ON)
    endif()
    
    if(FORCE_RSUSB_BACKEND)
        set(BACKEND RS2_USE_LIBUVC_BACKEND)
    else()
        set(BACKEND RS2_USE_V4L2_BACKEND)
    endif()
endmacro()

macro(os_target_config)
endmacro()
