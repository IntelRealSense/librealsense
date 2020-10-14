import os, sys, subprocess,  test_tools.common as test
# add these 2 lines to run independently and not through run-unit-tests.py
path = "C:/Users/mmirbach/git/librealsense/build/Debug"
sys.path.append(path)

import pyrealsense2 as rs
from time import sleep

# If this is the first run we need to set the correct enviroment variables and rerun the script
if len(sys.argv) < 2:
    os.environ["RS2_AC_DISABLE_CONDITIONS"] = "1"
    os.environ["RS2_AC_DISABLE_RETRIES"] = "1"
    os.environ["RS2_AC_FORCE_BAD_RESULT"] = "1"
    sys.argv.append('rerun')
    cmd = ["py", "-3"] + sys.argv
    p = subprocess.run( cmd, stderr=subprocess.PIPE, universal_newlines=True )
else:
    # rs.log_to_file( rs.log_severity.debug, "rs.log" )

    c = rs.context()
    if not c.devices.size():  # if no device is connected we don't run the AC test
        print("No device found, aborting AC test")
        exit(0)
    dev = c.devices[0]
    depth_sensor = dev.first_depth_sensor()
    color_sensor = dev.first_color_sensor()

    status_list = []
    # call back funtion for testing status sequences
    def status_cb( status ):
        global status_list
        status_list.append(status)

    # Used to ignore irrelevant statuses in status_list
    def trim_list():
        global status_list
        for irrelevant_status in  [rs.calibration_status.retry, rs.calibration_status.scene_invalid, rs.calibration_status.bad_result]:
            while irrelevant_status in status_list:
                status_list.remove(irrelevant_status)

    def wait():
        global status_list
        while (status_list[-1] != rs.calibration_status.successful and status_list[-1] != rs.calibration_status.failed):
            sleep(10)

    d2r = rs.device_calibration(dev)
    d2r.register_calibration_change_callback( status_cb )

    cp = next(p for p in color_sensor.profiles if p.fps() == 30
                 and p.stream_type() == rs.stream.color
                 and p.format() == rs.format.yuyv
                 and p.as_video_stream_profile().width() == 1280
                 and p.as_video_stream_profile().height() == 720)

    dp = next(p for p in
                 depth_sensor.profiles if p.fps() == 30
                 and p.stream_type() == rs.stream.depth
                 and p.format() == rs.format.z16
                 and p.as_video_stream_profile().width() == 1024
                 and p.as_video_stream_profile().height() == 768)

    # Test-Case #1, require 1-3
    test.start_case("Depth sensor is off, should get an Error")
    status_list = []
    try:
        d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
        wait()  
    except Exception as e:
        test.require(type(e) == RuntimeError) 
        test.require(str(e) == "not streaming") # Corresponding message for this event
    else:
        test.require_no_reach(2) # no error accured, should have received a RuntimrError
    finally:
        test.require(not status_list) # No status changes are expected, list should remain empty
    test.finish_case()

    # Test-Case #2, requires 4-10
    test.start_case("Colour sensor is off, calibration should succeed")
    status_list = []
    depth_sensor.open( dp )
    depth_sensor.start( lambda f: None )
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    wait()
    trim_list()
    test.require(len(status_list) == 4)
    test.require(status_list[0] == rs.calibration_status.triggered)
    test.require(status_list[1] == rs.calibration_status.special_frame)
    test.require(status_list[2] == rs.calibration_status.started)
    test.require(status_list[3] == rs.calibration_status.successful)
    try:
    # Since the sensor was closed before calibration started, it should have been returned to a 
    # closed state
        color_sensor.stop() 
    except Exception as e:
        test.require(type(e) == RuntimeError)
        test.require(str(e) == "tried to stop sensor without starting it")
    else:
        test.require_no_reach(2)
    test.finish_case()

    # Test-Case #3, require 11-15
    test.start_case("Normal AC trigger")
    status_list = []
    color_sensor.open( cp )
    color_sensor.start( lambda f: None )
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    wait()
    trim_list()
    test.require(len(status_list) == 4)
    test.require(status_list[0] == rs.calibration_status.triggered)
    test.require(status_list[1] == rs.calibration_status.special_frame)
    test.require(status_list[2] == rs.calibration_status.started)
    test.require(status_list[3] == rs.calibration_status.successful)
    test.finish_case()
    
    # Test-Case #4, require 16-18
    test.start_case("2 AC triggers in a row")
    status_list = []
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    sleep(2)
    try:
        d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
        wait()
    except Exception as e: # Second trigger should throw exception
        test.require(type(e) == RuntimeError)
        test.require(str(e) == "Camera Accuracy Health is already active")
    else:
        test.require_no_reach(2)
    wait() # First trigger should continue and finish successfully
    print(status_list)
    trim_list()
    test.require(len(status_list) == 4)
    test.require(status_list[0] == rs.calibration_status.triggered)
    test.require(status_list[1] == rs.calibration_status.special_frame)
    test.require(status_list[2] == rs.calibration_status.started)
    test.require(status_list[3] == rs.calibration_status.successful)
    test.finish_case()

    test.print_results()