
# recordings URL is same as in live-test.cmake
set(PP_Rosbag_Recordings_URL https://librealsense.intel.com/rs-tests/Rosbag_unit_test_records)

# for rec-play/test-non-realtime.py
dl_file( ${PP_Rosbag_Recordings_URL} recordings recording_deadlock.bag OFF )

