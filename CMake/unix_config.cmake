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
    if(${MACHINE} MATCHES "arm64-*" OR ${MACHINE} MATCHES "aarch64-*")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mstrict-align -ftree-vectorize")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstrict-align -ftree-vectorize")
    elseif(${MACHINE} MATCHES "arm-*")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mfpu=neon -mfloat-abi=hard -ftree-vectorize -latomic")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpu=neon -mfloat-abi=hard -ftree-vectorize -latomic")
    elseif(${MACHINE} MATCHES "powerpc64(le)?-linux-gnu")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -ftree-vectorize")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ftree-vectorize")
    elseif(${MACHINE} MATCHES "riscv64-*")
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mstrict-align -ftree-vectorize")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstrict-align -ftree-vectorize")
    else()
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -mssse3")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mssse3")
        set(LRS_TRY_USE_AVX true)
    endif(${MACHINE} MATCHES "arm64-*" OR ${MACHINE} MATCHES "aarch64-*")

    if(BUILD_WITH_OPENMP)
        find_package(OpenMP REQUIRED)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    else()
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    endif()
    
    ###############
    # According to SDLE we need to add the following flags for additional security:
    # Debug & Release:
    # -Wformat: Checks for format string vulnerabilities.
    # -Wformat-security: Ensures format strings are not vulnerable to attacks.
    # -fPIC: Generates position-independent code (PIC) suitable for shared libraries.
    # -fPIE: Generates position-independent executable (PIE) code.
    # -pie: Links the output as a position-independent executable.
    # -D_FORTIFY_SOURCE=2: Adds extra checks for buffer overflows.
    # -mfunction-return=thunk: Mitigates return-oriented programming (ROP) attacks. (Added flag -fcf-protection=none to allow it)
    # -mindirect-branch=thunk: Mitigates indirect branch attacks.
    # -mindirect-branch-register: Uses registers for indirect branches to mitigate attacks.
    # -fstack-protector: Adds stack protection to detect buffer overflows.

    # Release only
    # -Werror: Treats all warnings as errors.
    # -Werror=format-security: Treats format security warnings as errors.
    # -z noexecstack: Marks the stack as non-executable to prevent certain types of attacks.
    # -Wl,-z,relro,-z,now: Enables read-only relocations and immediate binding for security.
    # -fstack-protector-strong: Provides stronger stack protection than -fstack-protector.
	
    # see https://readthedocs.intel.com/SecureCodingStandards/2023.Q2.0/compiler/c-cpp/ for more details

    if (CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|armv7l" OR APPLE OR  # Some flags are not recognized or some systems / gcc versions
       (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "9.0")) # 
        set(ADDITIONAL_COMPILER_FLAGS "-Wformat -Wformat-security -fPIC -fstack-protector")
    else()
        #‘-mfunction-return’ and ‘-fcf-protection’ are not compatible, so specifing -fcf-protection=none
        set(ADDITIONAL_COMPILER_FLAGS "-Wformat -Wformat-security -fPIC -fcf-protection=none -mfunction-return=thunk -mindirect-branch=thunk -mindirect-branch-register -fstack-protector")
    endif()
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -pie")
	
	set(ADDITIONAL_COMPILER_FLAGS "${ADDITIONAL_COMPILER_FLAGS} -Wno-error=stringop-overflow")

    string(FIND "${CMAKE_CXX_FLAGS}" "-D_FORTIFY_SOURCE" _index)
    if (${_index} EQUAL -1) # Define D_FORTIFY_SOURCE is undefined
        set(ADDITIONAL_COMPILER_FLAGS "${ADDITIONAL_COMPILER_FLAGS} -D_FORTIFY_SOURCE=2")
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Configuring for Debug build")
    else() # Release, RelWithDebInfo, or multi configuration generator is being used (aka not specifing build type, or building with VS)
        message(STATUS "Configuring for Release build")
        set(ADDITIONAL_COMPILER_FLAGS "${ADDITIONAL_COMPILER_FLAGS} -Werror -z noexecstack -Wl,-z,relro,-z,now -fstack-protector-strong")
    endif()
	
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ADDITIONAL_COMPILER_FLAGS}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ADDITIONAL_COMPILER_FLAGS}")
	
	
    set_directory_properties(PROPERTIES DIRECTORY third-party/ COMPILE_OPTIONS "-w")
    set_source_files_properties(third-party/*.* PROPERTIES COMPILE_OPTIONS "-w")

    #################
	
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
