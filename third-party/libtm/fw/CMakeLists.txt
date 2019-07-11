# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2019 Intel Corporation. All Rights Reserved.
cmake_minimum_required(VERSION 3.1.3)

set( FW_VERSION "0.0.18.6100")
set( FW_SHA1 858b786215ff66cf8cf93fa85393ca6268dedee4)
set(APP_VERSION "2.0.19.271")
set(APP_SHA1 cab0011e9e18edc8bcca20afb2f944399ac8b81c)
set( BL_VERSION "1.0.1.112")
set( BL_SHA1 d8fb5ba6f7029a3c950bf6b1ef5715efd880ef66)

set( FW_URL "http://realsense-hw-public.s3.amazonaws.com/Releases/TM2/FW/target")
set(APP_URL "http://realsense-hw-public.s3.amazonaws.com/Releases/TM2/FW/app")
set( BL_URL "http://realsense-hw-public.s3.amazonaws.com/Releases/TM2/FW/bl")

add_library(fw STATIC empty.c)

target_include_directories(fw PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>)

function(target_binary url version sha1 symbol ext)
  set(binary "${CMAKE_CURRENT_BINARY_DIR}/${symbol}-${version}${ext}")
  file(DOWNLOAD "${url}/${version}/${symbol}-${version}${ext}" "${binary}"
       EXPECTED_HASH SHA1=${sha1}
       STATUS status)
  list(GET status 0 error_code)
  if (NOT ${error_code} EQUAL 0)
    message(FATAL_ERROR "Download TM2 firmwnare (${status}) - ${url}")
  else()
    message(STATUS "Download TM2 firmware ${status} for ${symbol}-${version}${ext}")
  endif()
  string(TOUPPER ${symbol} SYMBOL)
  string(REPLACE "." "," version_commas ${version})
  string(REPLACE "\\" "\\\\" binary_escaped "${binary}")
  configure_file(fw.c.in  "${CMAKE_CURRENT_BINARY_DIR}/fw_${symbol}.c" @ONLY)
  configure_file(fw.h.in  "${CMAKE_CURRENT_BINARY_DIR}/fw_${symbol}.h" @ONLY)
  configure_file(fw.rc.in "${CMAKE_CURRENT_BINARY_DIR}/fw_${symbol}.rc" @ONLY)
  if (MSVC)
    enable_language(RC)
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/fw.rc" OBJECT_DEPENDS "${binary}")
    set(FW_RC_CONTENT "${FW_RC_CONTENT}${SYMBOL}_DATA ${SYMBOL}_RC \"${binary_escaped}\"\n" PARENT_SCOPE)
  else()
    set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/fw_${symbol}.c" OBJECT_DEPENDS "${binary}")
  endif()
  target_sources(fw PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/fw_${symbol}.c")
endfunction()

target_binary( "${FW_URL}"  "${FW_VERSION}"  "${FW_SHA1}" target .mvcmd)
target_binary("${APP_URL}" "${APP_VERSION}" "${APP_SHA1}" central_app .bin)
target_binary( "${BL_URL}"  "${BL_VERSION}"  "${BL_SHA1}" central_bl .bin)

if (MSVC)
  # lib.exe can't handle multiple .res files, so include them in one.
  configure_file("fw.rc.in" "${CMAKE_CURRENT_BINARY_DIR}/fw.rc")
  target_sources(fw PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/fw.rc")
  # even then, the linker won't grab a .res out of a .lib object, so it needs to be explicitly listed
  target_link_libraries(fw PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/fw.res")
  # and to find the name of the .res file (across cmake generators) we need to create our own rule. :(
  add_custom_command(TARGET fw BYPRODUCTS fw.res COMMAND ${CMAKE_RC_COMPILER} ${CMAKE_RC_FLAGS} /I . /fo fw.res fw.rc)
endif()

install(TARGETS fw EXPORT realsense2Targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
