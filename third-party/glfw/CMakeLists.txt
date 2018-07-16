# ubuntu 12.04 LTS cmake version 2.8.7
# ubuntu 14.04 LTS cmake version 2.8.12.2
# ubuntu 16.04 LTS cmake version 3.5.1	
cmake_minimum_required(VERSION 2.8.3)

project(GLFW3)

set(GLFW3_CPP
    src/context.c
    src/init.c
    src/input.c
    src/monitor.c
    src/wgl_context.c
    src/win32_init.c
    src/win32_monitor.c
    src/win32_time.c
    src/win32_tls.c
    src/win32_tls.h
    src/win32_window.c
    src/window.c
    src/winmm_joystick.c
)

set(GLFW3_HPP
    src/glfw_config.h
    src/internal.h
    src/wgl_context.h
    src/win32_platform.h
    src/winmm_joystick.h
)

if(WIN32)
    add_definitions(-D_GLFW_USE_CONFIG_H)
    add_library(glfw3 STATIC ${GLFW3_CPP} ${GLFW3_HPP})

    set_target_properties (glfw3 PROPERTIES
        FOLDER "3rd Party"
    )
else()
    add_library(glfw3 ${GLFW3_CPP} ${GLFW3_HPP})
endif()

target_include_directories(glfw3 PUBLIC include/)
