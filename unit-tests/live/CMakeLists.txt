
# recordings URL is same as in live-test.cmake
set(PP_Rosbag_Recordings_URL https://librealsense.intel.com/rs-tests/Rosbag_unit_test_records)

# for rec-play/test-non-realtime.py
dl_file( ${PP_Rosbag_Recordings_URL} recordings recording_deadlock.bag OFF )

# for rec-play/test-playback-stress.py
dl_file( ${PP_Rosbag_Recordings_URL} recordings all_combinations_depth_color.bag OFF )

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

list(APPEND PP_Rosbag_Recordings_List
    [aligned_2c]_all_combinations_depth_color.bag
    [aligned_2d]_all_combinations_depth_color.bag
    all_combinations_depth_color.bag
    )
	
foreach(i ${PP_Rosbag_Recordings_List})
    dl_file( ${PP_Rosbag_Recordings_URL} ${Deployment_Location} ${i} OFF )
endforeach(i)
