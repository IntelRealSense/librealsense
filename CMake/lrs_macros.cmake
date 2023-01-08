macro(info msg)
    message(STATUS "Info: ${msg}")
endmacro()

macro(infoValue variableName)
    info("${variableName}=\${${variableName}}")
endmacro()

macro(config_cxx_flags)
    include(CheckCXXCompilerFlag)
    if(MSVC OR MSVC_IDE)
        check_cxx_compiler_flag(/std:c++14 SUPPORTS_CXX14)
    else()
        check_cxx_compiler_flag(-std=c++14 SUPPORTS_CXX14)
    endif()
    if( NOT SUPPORTS_CXX14 )
        message(FATAL_ERROR "Project '${PROJECT_NAME}' requires C++14 or higher")
    endif()
    if( NOT CMAKE_CXX_STANDARD )
        set( CMAKE_CXX_STANDARD 14 )
    endif()
    # We require that the current project (e.g., librealsense) use C++14. However, projects using
    # the library don't need to be C++14 -- they can use C++11. Hence this is PRIVATE and not PUBLIC:
    target_compile_features( ${PROJECT_NAME} PRIVATE cxx_std_${CMAKE_CXX_STANDARD} )
    #set( CMAKE_CUDA_STANDARD ${LRS_CXX_STANDARD} )
endmacro()

macro(config_crt)
    if(BUILD_WITH_STATIC_CRT)
        foreach(flag_var
                CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO
                CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
                CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO)
            if(${flag_var} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
            endif(${flag_var} MATCHES "/MD")
        endforeach(flag_var)
    endif(BUILD_WITH_STATIC_CRT)
endmacro()
