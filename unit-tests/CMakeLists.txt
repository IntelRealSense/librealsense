#  minimum required cmake version: 3.1.0
cmake_minimum_required(VERSION 3.1.0)

project(RealsenseUnitTests)

# Save the command line compile commands in the build output
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
# View the makefile commands during build
#set(CMAKE_VERBOSE_MAKEFILE on)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
endif()

set(DEPENDENCIES realsense2)

set (unit_tests_sources unit-tests-live.cpp unit-tests-post-processing.cpp unit-tests-post-processing.h unit-tests-main.cpp unit-tests-common.h)

add_executable(live-test ${unit_tests_sources})
target_link_libraries(live-test ${DEPENDENCIES})

set_target_properties (live-test PROPERTIES
    FOLDER "Unit-Tests"
)

install(
    TARGETS

    live-test

    RUNTIME DESTINATION
    ${CMAKE_INSTALL_PREFIX}/bin
)

#Windows OS will host the files under %TEMP% location
#Unix-like machines will host the tests files under /temp/ directory
if (WIN32)
    set(Deployment_Location "$ENV{TEMP}\\")
else() # Data shall be preserved between reboots. For Linux distributions/ ANDROID_NDK_TOOLCHAIN_INCLUDED/APPLE
    #set(Deployment_Location /var/tmp/) The standard confoiguration currently fails on CI
    set(Deployment_Location /tmp/)
endif()

#Post-Processing data set for unit-tests
#message(STATUS "Post processing deployment directory=${Deployment_Location}")
list(APPEND PP_Tests_List  1525186403504 # D415_DS(2)
                           1525186407536 # D415_DS(3)
                           1525072818314 # D415_DS(1)_HoleFill(0)
                           1525072823227 # D415_DS(1)_HoleFill(1)
                           1524668713358 # D435_DS(3)_HoleFill(2)
                           1525267760676 # D415_DS(2)+Spat(A:0.85/D:32/I:3)
                           1525266028697 # D415_DS(2)+Temp(A:0.25/D:15/P:1)
                           1525265554250 # D415_DS(2)+Temp(A:0.25/D:15/P:3)
                           1525266069476 # D415_DS(2)+Temp(A:0.25/D:15/P:5)
                           1525266120520 # D415_DS(3)+Temp(A:0.25/D:15/P:7)
                           1525267168585 # D415_DS(2)_Spat(A:0.85/D:32/I:3)_Temp(A:0.25/D:15/P:0)
                           1525089539880) # D415_DS(2)_Spat(A:0.85/D:32/I:3)_Temp(A:0.25/D:15/P:0)_HoleFill(1)

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
      #message(STATUS "Checking for a required test record ${Test_File_Name}")
      if(NOT EXISTS "${destination}")
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
