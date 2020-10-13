import os, sys, subprocess,  test_tools.common as test
# add these 2 lines to run independently and not through run-unit-tests.py
path = "C:/Users/mmirbach/git/librealsense/build/RelWithDebInfo"
sys.path.append(path)

import pyrealsense2 as rs
from time import sleep

# If this is the first run we need to set the correct enviroment variables and rerun the script
if os.getenv("RS2_AC_DISABLE_CONDITIONS") == "1":
    os.environ["RS2_AC_DISABLE_CONDITIONS"] = "0"
    cmd = ["py", "-3"] + sys.argv
    p = subprocess.run( cmd, stderr=subprocess.PIPE, universal_newlines=True )
    print("done")
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

    def wait():
        global status_list
        while (status_list[-1] != rs.calibration_status.successful and status_list[-1] != rs.calibration_status.failed):
            print("waiting")
            sleep(1)

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

    
    depth_sensor.open( dp )
    depth_sensor.start( lambda f: None )
    color_sensor.open( cp )
    color_sensor.start( lambda f: None )
    
    # Test-Case #1
    test.start_case("Failing check_conditions function")
    status_list = []
    depth_sensor.set_option(rs.option.ambient_light, 2)
    depth_sensor.set_option(rs.option.avalanche_photo_diode, 0)
    try:
        d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
        wait()
    except Exception as e:
        test.require(type(e) == RuntimeError)
    else:
        test.require_no_reach(1)
    finally:
        test.require(len(status_list) == 1)
        test.require(status_list[0] == rs.calibration_status.bad_conditions)
    test.finish_case()

    test.print_results()