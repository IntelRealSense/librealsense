# This file is only used to enable the building librealsense
# as a debian package for use with ROS (http://ros.org)
# via catkin_make (which invokes cmake)

cmake_minimum_required(VERSION 2.8.3)

project(librealsense)

include(ExternalProject)

find_package(catkin REQUIRED)

externalproject_add(${PROJECT_NAME}_extern
  PREFIX "external_proj"

  SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}

  CONFIGURE_COMMAND echo "No Configuration Step"

  BUILD_IN_SOURCE 1
  BUILD_COMMAND $(MAKE) lib/${PROJECT_NAME}.so

  INSTALL_DIR ${CATKIN_DEVEL_PREFIX}
  INSTALL_COMMAND cp <SOURCE_DIR>/lib/${PROJECT_NAME}.so <INSTALL_DIR>/lib/ COMMAND cp -r <SOURCE_DIR>/include/${PROJECT_NAME} <INSTALL_DIR>/include/
)

# Need to ensure the include directory exists before the catkin_package macro
execute_process(COMMAND mkdir -p ${CATKIN_DEVEL_PREFIX}/include/${PROJECT_NAME})

# Declare this project as a catkin package
# Export the include files and library for other packages in this workspace
catkin_package(
  INCLUDE_DIRS ${CATKIN_DEVEL_PREFIX}/include/${PROJECT_NAME}
  LIBRARIES realsense
)

# Install library
install(FILES ${CATKIN_DEVEL_PREFIX}/lib/${PROJECT_NAME}.so
  DESTINATION ${CATKIN_PACKAGE_LIB_DESTINATION}
)

# Install include files
install(DIRECTORY ${CATKIN_DEVEL_PREFIX}/include/${PROJECT_NAME}/
  DESTINATION ${CATKIN_PACKAGE_INCLUDE_DESTINATION})
