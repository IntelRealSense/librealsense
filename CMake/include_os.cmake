    if (WIN32)
        include(CMake/windows_config.cmake)
    endif()

    if(UNIX)
        include(CMake/unix_config.cmake)
    endif()

    if(ANDROID_NDK_TOOLCHAIN_INCLUDED)
        include(CMake/android_config.cmake)
    endif()