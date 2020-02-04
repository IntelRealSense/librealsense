#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseUnitTests)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

set(DEPENDENCIES realsense2)

find_package (Python3 COMPONENTS Interpreter Development)
if(Python3_FOUND)
    execute_process(
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/unit-test-config.py ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
        RESULT_VARIABLE rv
    )
    if( NOT ${rv} EQUAL 0 )
        message(FATAL_ERROR "unit-test-config has failed with status = ${rv}")
    endif()
    add_subdirectory( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR}/build )
else()
    message(WARNING "Python 3 was not found; Unit tests will be limited!")
endif()

set (unit_tests_sources
    unit-tests-live.cpp
    unit-tests-post-processing.cpp
    unit-tests-post-processing.h
    unit-tests-main.cpp
    unit-tests-common.h
    unit-tests-post-processing-from-bag.cpp
)

add_executable(live-test ${unit_tests_sources})
set_property(TARGET live-test PROPERTY CXX_STANDARD 11)
target_link_libraries(live-test ${DEPENDENCIES} Threads::Threads)

set_target_properties (live-test PROPERTIES
    FOLDER "Unit-Tests"
)

install(
    TARGETS
    live-test
    RUNTIME DESTINATION
    ${CMAKE_INSTALL_PREFIX}/bin
)

if(TESTDATA_LOCATION)
    set(Deployment_Location ${TESTDATA_LOCATION})
else()
    #Windows OS will host the files under %TEMP% location
    #Unix-like machines will host the tests files under /tmp/ directory
    if (WIN32)
        set(Deployment_Location "$ENV{TEMP}\\")
    else() # Data shall be preserved between reboots. For Linux distributions/ ANDROID_NDK_TOOLCHAIN_INCLUDED/APPLE
        #set(Deployment_Location /var/tmp/) The standard configuration currently fails on CI
        set(Deployment_Location /tmp/)
    endif()
endif()

add_custom_command(TARGET live-test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
    ${Deployment_Location}
)

# copy wheel odometry calibration file to build folder
add_custom_command(TARGET live-test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
    ${CMAKE_CURRENT_SOURCE_DIR}/resources/calibration_odometry.json
    ${CMAKE_CURRENT_BINARY_DIR}/calibration_odometry.json
)

#Post-Processing data set for unit-tests
#message(STATUS "Post processing deployment directory=${Deployment_Location}")
list(APPEND PP_Tests_List  1551257764229 # D415_DS(2)
                           1551257812956 # D415_DS(3)
                           1551257880762 # D415_DS(2)_HoleFill(0)
                           1551257882796 # D415_DS(2)_HoleFill(1)
                           1551257884097 # D435_DS(2)_HoleFill(2)
                           1551257987255 # D435_DS(2)+Spat(A:0.85/D:32/I:3)
                           1551259481873 # D435_DS(2)+Spat(A:0.5/D:15/I:2)
                           1551261946511 # D435_DS(2)+Temp(A:0.25/D:15/P:0)
                           1551262153516 # D435_DS(2)+Temp(A:0.45/D:25/P:1)
                           1551262256875 # D435_DS(2)+Temp(A:0.5/D:30/P:4)
                           1551262841203 # D435_DS(2)+Temp(A:0.5/D:30/P:6)
                           1551262772964 # D435_DS(2)+Temp(A:0.5/D:30/P:8)
                           1551262971309 # D435_DS(2)_Spat(A:0.7/D:25/I:2)_Temp(A:0.6/D:15/P:6)
                           1551263177558) # D435_DS(2)_Spat(A:0.7/D:25/I:2)_Temp(A:0.6/D:15/P:6))_HoleFill(1)

# For each post-processing test pattern the following files shall be present
list(APPEND PP_Test_extensions_List .Input.raw .Input.csv .Output.raw .Output.csv)

set_property(GLOBAL PROPERTY sequence_extensions_list)
function(produce_sequence_extensions source target)
    # Check if metadata file exist, and download if needed
    if(NOT EXISTS "${target}")
          file(DOWNLOAD "${source}" "${target}" LOG log STATUS status TIMEOUT 300) # SHOW_PROGRESS
          list(GET status 0 op_return_value)
          if (NOT op_return_value MATCHES "0")
              list(GET status 1 description)
              message(STATUS "Operation failed: opcode= ${status}")
              message(STATUS "Log: opcode= ${log}")
          endif()
      endif()

    FILE(READ "${target}" contents)
    # Convert file contents into a CMake list
    set(sequence_length 1)
    STRING(REGEX REPLACE ";" "\\\\;" contents "${contents}")
    STRING(REGEX REPLACE "\n" ";" contents "${contents}")
    foreach(i ${contents})
        if ("${i}" MATCHES "^Frames sequence length")
            string(REPLACE "," ";" line ${i})
            # use of unsubstituted variables is requred
            set(my_seq ${line})
            list(GET my_seq 1 result)
            if (result MATCHES "^[0-9]+$")
                set(sequence_length ${result})
            endif()
        endif()
    endforeach()
    set(index 0)
    #Reset the list of appendexes
    set(PP_Test_Sequence_Index_List)
    while (${index} LESS ${sequence_length} )
        list(APPEND PP_Test_Sequence_Index_List .${index})
        MATH(EXPR index "${index} + 1")
    endwhile()
    get_property(local_list GLOBAL PROPERTY sequence_extensions_list)
    set(local_list ${PP_Test_Sequence_Index_List})
    set_property(GLOBAL PROPERTY sequence_extensions_list "${local_list}")

endfunction()

function(is_file_empty empty filename)
    set (data "")
    if(EXISTS ${filename})
        file(STRINGS  ${filename} data)
    endif()
    string(LENGTH "${data}" data_length)
    #message(STATUS "Read ${data_length}  bytes from  ${filename}")
    set(res TRUE)
    if (NOT ${data_length} STREQUAL "0")
        set(res FALSE)
    endif()
    set(empty ${res} PARENT_SCOPE)
endfunction()

set(PP_TESTS_URL http://realsense-hw-public.s3.amazonaws.com/rs-tests/post_processing_tests_2018_ww18/)
message(STATUS "Preparing to download Post-processing tests dataset...\nRemote server: ${PP_TESTS_URL}\nTarget Location: ${Deployment_Location}\n")
foreach(i ${PP_Tests_List})
  set(Test_Pattern ${i})
  #Download and parse the output metafile to retrieve the number of snapshots in sequence
  set(sequence_meta_file "${Test_Pattern}.0.Output.csv")
  set(source ${PP_TESTS_URL}${sequence_meta_file})
  set(destination ${Deployment_Location}${sequence_meta_file})
  produce_sequence_extensions(${source} ${destination})
  # Copy the assigned variables to local variable
  get_property(PP_Test_Sequence_Index_List GLOBAL PROPERTY sequence_extensions_list)

  #Iterate over test pattern extension to download the test files.
  foreach(ext ${PP_Test_extensions_List})
    #Each test comprise of a sequence of frame indexed according to the following
    foreach(idx ${PP_Test_Sequence_Index_List})
      # Calculate the target full path name for deployment
      set(Test_File_Name "${Test_Pattern}${idx}${ext}")
      set(source ${PP_TESTS_URL}${Test_File_Name})
      set(destination ${Deployment_Location}${Test_File_Name})
      set (empty FALSE)
      is_file_empty(empty, ${destination} )
      #message(STATUS "Checked " ${empty} " for " ${Test_File_Name} " in " ${destination})
      if(NOT EXISTS "${destination}" OR ${empty})
          message(STATUS "Downloading ${source}")
          file(DOWNLOAD "${source}" "${destination}" LOG log STATUS status TIMEOUT 300) # SHOW_PROGRESS LOG log STATUS status
          list(GET status 0 op_return_value)
          if (NOT op_return_value MATCHES "0")
              list(GET status 1 description)
              message(STATUS "Operation failed: opcode= ${status}")
              message(STATUS "Log: opcode= ${log}")
          endif()
      endif()
    endforeach(idx)
  endforeach(ext)
endforeach(i)

list(APPEND PP_Rosbag_Recordings_List
    [aligned_2c]_all_combinations_depth_color.bag
    [aligned_2d]_all_combinations_depth_color.bag
    all_combinations_depth_color.bag
    [pointcloud]_all_combinations_depth_color.bag
    single_depth_color_640x480.bag
    D435i_Depth_and_IMU.bag
)
 set(PP_Rosbag_Recordings_URL http://realsense-hw-public.s3.amazonaws.com/rs-tests/Rosbag_unit_test_records/)
foreach(i ${PP_Rosbag_Recordings_List})
    set (empty FALSE)
    is_file_empty(empty, ${Deployment_Location}${i} )
    if(NOT EXISTS "${Deployment_Location}${i}" OR ${empty})
        message(STATUS "Target not exist or empty - ${Deployment_Location}${i}")
        message(STATUS "Downloading ${PP_Rosbag_Recordings_URL}${i}")
        file(DOWNLOAD "${PP_Rosbag_Recordings_URL}${i}" "${Deployment_Location}${i}" TIMEOUT 600 STATUS status LOG log)
    endif()
endforeach(i)

if(NOT ${BUILD_SHARED_LIBS} AND ${BUILD_INTERNAL_UNIT_TESTS})
    add_subdirectory(internal)
endif()
