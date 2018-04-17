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
list(APPEND PP_Tests_List  1523873668701 # D415_Downsample1
                           1523873012723 # D415_Downsample2
                           1523873362088 # D415_Downsample3
                           1523874476600 # D415_Downsample2+Spat(A:0.85/D:32/I:3)
                           1523874595767 # D415_Downsample2+Spat(A:0.3/D:8/I:3)
                           1523889912588 # D415_Downsample2+Temp(A:0.4/D:20/P:0)
                           1523890056362 # D415_Downsample2+Temp(A:0.3/D:10/P:4)
                           1523887243933 # D415_DS:2_Spat(A:0.85/D:32/I:3)_Temp(A:0.25/D:15/P:0)
                           1523889529572) # D415_DS:3_Spat(A:0.3/D:8/I:3)_Temp(A:0.5/D:6/P:4)

# For each post-processing test pattern the following files shall be present
list(APPEND PP_Test_extensions_List .Input.raw .Input.csv .Output.raw .Output.csv)
list(APPEND PP_Test_Sequence_Index_List .0 .1 .2 .3 .4 .5 .6 .7 .8 .9)

set(PP_TESTS_URL http://realsense-hw-public.s3.amazonaws.com/rs-tests/post_processing_test_patterns/)
message(STATUS "Preparing to download Post-processing tests dataset...\nRemote server: ${PP_TESTS_URL}\nTarget Location: ${Deployment_Location}\n")
foreach(i ${PP_Tests_List})
  set(Test_Pattern ${i})
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
          file(DOWNLOAD "${source}" "${destination}" TIMEOUT 30 LOG log STATUS status) # SHOW_PROGRESS
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
