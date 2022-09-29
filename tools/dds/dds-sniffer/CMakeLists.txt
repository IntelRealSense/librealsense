# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2022 Intel Corporation. All Rights Reserved.


project(rs-dds-sniffer)

set(SRC rs-dds-sniffer.cpp)
set(INC rs-dds-sniffer.h)

add_executable(${PROJECT_NAME} ${INC} ${SRC})
target_link_libraries(${PROJECT_NAME} PRIVATE realdds ${DEPENDENCIES} )
target_include_directories( ${PROJECT_NAME} PRIVATE
    ${REPO_ROOT}/include
    ${REPO_ROOT}/third-party/tclap/include
)
set_target_properties (${PROJECT_NAME} PROPERTIES
                       FOLDER Tools/dds
                       CXX_STANDARD 11)

include("${REPO_ROOT}/include/librealsense2/dds/dds-client.cmake")

include("${REPO_ROOT}/include/librealsense2/utilities/easylogging/easyloggingpp.cmake")
target_sources( ${PROJECT_NAME} PRIVATE ${ELPP_SOURCE_FILES} )
source_group( "EasyLogging++" FILES ${ELPP_SOURCE_FILES}  )

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

