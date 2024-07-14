
# recordings URL is same as in live-test.cmake
set(PP_Rosbag_Recordings_URL https://librealsense.intel.com/rs-tests/Rosbag_unit_test_records)

# for rec-play/test-non-realtime.py
dl_file( ${PP_Rosbag_Recordings_URL} recordings recording_deadlock.bag OFF )

# for rec-play/test-playback-stress.py and for post-processing/post-processing-from-bag.py
dl_file( ${PP_Rosbag_Recordings_URL} recordings all_combinations_depth_color.bag OFF )

# for post-processing/post-processing-from-bag.py
dl_file( ${PP_Rosbag_Recordings_URL} recordings [aligned_2c]_all_combinations_depth_color.bag OFF )

# for post-processing/post-processing-from-bag.py
dl_file( ${PP_Rosbag_Recordings_URL} recordings [aligned_2d]_all_combinations_depth_color.bag OFF )

# for post-processing/post-processing-from-bag.py
dl_file( ${PP_Rosbag_Recordings_URL} recordings [pointcloud]_all_combinations_depth_color.bag OFF )

# for 3D/projection-from-recording.bag
dl_file( ${PP_Rosbag_Recordings_URL} recordings single_depth_color_640x480.bag OFF )

# for post-processing/post-processing-filters.py
# Define a function to download files
function(dl_file url subdir filename)
    set(destination ${CMAKE_BINARY_DIR}/${subdir}/${filename})
    if(NOT EXISTS "${destination}")
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/${subdir})
        message(STATUS "Downloading ${url}/${filename} to ${destination}")
        file(DOWNLOAD "${url}/${filename}" "${destination}" LOG log STATUS status TIMEOUT 300) # SHOW_PROGRESS
        list(GET status 0 op_return_value)
        if (NOT op_return_value MATCHES "0")
            list(GET status 1 description)
            message(STATUS "Operation failed: opcode= ${status}")
            message(STATUS "Log: ${log}")
        endif()
    endif()
endfunction()

# Specify the folder for recordings
set(RECORDINGS_FOLDER "unit-tests/recordings")
# Set deployment location based on TESTDATA_LOCATION or system-specific defaults
if(TESTDATA_LOCATION)
    set(Deployment_Location ${TESTDATA_LOCATION})
else()
    if (WIN32)
        set(Deployment_Location "$ENV{TEMP}/")
    else()
        set(Deployment_Location /tmp/)
    endif()
endif()

# List of post-processing test patterns
list(APPEND PP_Tests_List  
    1551257764229 # D415_DS(2)
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
    1551263177558  # D435_DS(2)_Spat(A:0.7/D:25/I:2)_Temp(A:0.6/D:15/P:6)_HoleFill(1)
)

# Extensions for post-processing test files
list(APPEND PP_Test_extensions_List .Input.raw .Input.csv .Output.raw .Output.csv)

# Function to handle sequence extensions and downloads
function(produce_sequence_extensions source target)
    if(NOT EXISTS "${target}")
        file(DOWNLOAD "${source}" "${target}" LOG log STATUS status TIMEOUT 300) # SHOW_PROGRESS
        list(GET status 0 op_return_value)
        if (NOT op_return_value MATCHES "0")
            list(GET status 1 description)
            message(STATUS "Operation failed: opcode= ${status}")
            message(STATUS "Log: ${log}")
        endif()
    endif()

    FILE(READ "${target}" contents)
    set(sequence_length 1)
    STRING(REGEX REPLACE ";" "\\\\;" contents "${contents}")
    STRING(REGEX REPLACE "\n" ";" contents "${contents}")
    foreach(i ${contents})
        if ("${i}" MATCHES "^Frames sequence length")
            string(REPLACE "," ";" line ${i})
            set(my_seq ${line})
            list(GET my_seq 1 result)
            if (result MATCHES "^[0-9]+$")
                set(sequence_length ${result})
            endif()
        endif()
    endforeach()
    
    set(index 0)
    set(PP_Test_Sequence_Index_List)
    while (${index} LESS ${sequence_length} )
        list(APPEND PP_Test_Sequence_Index_List .${index})
        MATH(EXPR index "${index} + 1")
    endwhile()
    set_property(GLOBAL PROPERTY sequence_extensions_list "${PP_Test_Sequence_Index_List}")
endfunction()

set(PP_TESTS_URL https://librealsense.intel.com/rs-tests/post_processing_tests_2018_ww18/)
message(STATUS "Preparing to download Post-processing tests dataset...\nRemote server: ${PP_TESTS_URL}\nTarget Location: ${CMAKE_BINARY_DIR}/${RECORDINGS_FOLDER}")

foreach(i ${PP_Tests_List})
    set(Test_Pattern ${i})
    set(sequence_meta_file "${Test_Pattern}.0.Output.csv")
    set(source ${PP_TESTS_URL}${sequence_meta_file})
    set(destination ${CMAKE_BINARY_DIR}/${RECORDINGS_FOLDER}/${sequence_meta_file})
    produce_sequence_extensions(${source} ${destination})
    get_property(PP_Test_Sequence_Index_List GLOBAL PROPERTY sequence_extensions_list)

    foreach(ext ${PP_Test_extensions_List})
        foreach(idx ${PP_Test_Sequence_Index_List})
            set(Test_File_Name "${Test_Pattern}${idx}${ext}")
            set(source ${PP_TESTS_URL}${Test_File_Name})
            set(destination ${CMAKE_BINARY_DIR}/${RECORDINGS_FOLDER}/${Test_File_Name})
            if(NOT EXISTS "${destination}")
                message(STATUS "Downloading ${source}")
                file(DOWNLOAD "${source}" "${destination}" LOG log STATUS status TIMEOUT 300) # SHOW_PROGRESS
                list(GET status 0 op_return_value)
                if (NOT op_return_value MATCHES "0")
                    list(GET status 1 description)
                    message(STATUS "Operation failed: opcode= ${status}")
                    message(STATUS "Log: ${log}")
                endif()
            endif()
        endforeach()
    endforeach()
endforeach()

message(STATUS "Post processing deployment directory=${Deployment_Location}")
