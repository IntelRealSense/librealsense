message(STATUS "Setting Apple configurations")

macro(os_set_flags)
    set(FORCE_LIBUVC ON)
    set(BUILD_WITH_TM2 ON)
endmacro()

macro(os_target_config)
endmacro()
