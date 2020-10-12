import os
# add these 3 lines to run independently and not through run-unit-tests.py
import sys
path = "C:/Users/mmirbach/git/librealsense/build/RelWithDebInfo"
sys.path.append(path)
import pyrealsense2 as rs
from time import sleep

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
    # print( "-----> " + str(status))
    # print("list is")
    status_list.append(status)
    # print(status_list)

n_assertions = 0
n_failed_assertions = 0
n_test_cases = 0
n_failed_test_cases = 0
test_case_failed = False

# Functions for asserting test cases
def require(exp):
    global n_assertions, n_failed_assertions, test_case_failed
    n_assertions += 1
    if not exp:
        n_failed_assertions += 1
        test_case_failed = True
        print("require failed")

# Functions for formating test cases
def start_test_case(msg):
    global n_test_cases, test_case_failed
    n_test_cases += 1
    test_case_failed = False
    status_list = []
    print("\n\n" + msg)

def finish_test_case():
    global test_case_failed, n_failed_test_cases
    if test_case_failed:
        n_failed_test_cases += 1
        print("Test-Case failed")
    else:
        print("Test passed")

def wait():
    global status_list
    while (status_list[-1] != rs.calibration_status.successful and status_list[-1] != rs.calibration_status.failed):
        sleep(1)


# Saving original values of enviroment variables to be reset at the end of the test
OG_env_vars = {
    "RS2_AC_LOG_TO_STDOUT" : os.getenv('RS2_AC_LOG_TO_STDOUT'),
    "RS2_AC_DISABLE_RETRIES" : os.getenv('RS2_AC_DISABLE_RETRIES'),
    "RS2_AC_INVALID_SCENE_FAIL" : os.getenv('RS2_AC_INVALID_SCENE_FAIL'),
    "RS2_AC_FORCE_BAD_RESULT" : os.getenv('RS2_AC_FORCE_BAD_RESULT'),
    "RS2_AC_INVALID_RETRY_SECONDS_MANUAL" : os.getenv('RS2_AC_INVALID_RETRY_SECONDS_MANUAL'),
    "RS2_AC_INVALID_RETRY_SECONDS_AUTO" : os.getenv('RS2_AC_INVALID_RETRY_SECONDS_AUTO'),
    "RS2_AC_AUTO_TRIGGER" : os.getenv('RS2_AC_AUTO_TRIGGER'),
    "RS2_AC_DISABLE_CONDITIONS" : os.getenv('RS2_AC_DISABLE_CONDITIONS'),
    "RS2_AC_SF_RETRY_SECONDS" : os.getenv('RS2_AC_SF_RETRY_SECONDS'),
    "RS2_AC_TRIGGER_SECONDS" : os.getenv('RS2_AC_TRIGGER_SECONDS'),
    "RS2_AC_TEMP_DIFF" : os.getenv('RS2_AC_TEMP_DIFF')
    }

# This is a copy of the enviroment variables that we will change during the testing
env_vars = dict(OG_env_vars)

# Sets all relevant enviroment varables according to there values in the env_vars dictionary
def set_env_vars():
    global env_vars
    for env_var, value in env_vars.items():
        print("env: ", end='')
        print(env_var)
        print("val: ", end='')
        print(value)
        os.environ[env_var] = value
set_env_vars()
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

# Test-Case #1
start_test_case("Depth sensor is off, should get RuntimeError with message 'not streaming'")
try:
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    wait()
except RuntimeError as rte:
    require(str(rte) == "not streaming")
except Exception:
    require(False) # unexpected error 
else:
    require(False) # no error accured, should have received a RuntimrError
finally:
    require(not status_list) # No status changes are expected, list should remain empty
finish_test_case()

# Test-Case #2
start_test_case("Colour sensor is off, calibration should succeed")
depth_sensor.open( dp )
depth_sensor.start( lambda f: None )
env_vars["RS2_AC_SF_RETRY_SECONDS"] = "2"
env_vars["RS2_AC_TRIGGER_SECONDS"] = "600"
env_vars["RS2_AC_TEMP_DIFF"] = "5"
set_env_vars()
try:
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    wait()
except Exception as e:
    require(False) # unexpected error
finally:
    require(status_list[0] == rs.calibration_status.triggered)
    require(status_list[1] == rs.calibration_status.special_frame)
    require(status_list[2] == rs.calibration_status.started)
    require(status_list[3] == rs.calibration_status.successful)
finish_test_case()

color_sensor.open( cp )
color_sensor.start( lambda f: None )

#depth_sensor.set_option( rs.option.trigger_camera_accuracy_health, 2 )
#d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )

set_env_vars(OG_env_vars)
if n_failed_assertions:
    passed = str(n_assertions - n_failed_assertions)
    print("test cases: " + str(n_failed_test_cases) + " | " + str(n_test_cases) + " failed")
    print("assertions: " + str(n_assertions) + " | " + passed + " passed | " + str(n_failed_assertions) + " failed")
    exit(1)
else:
    print("All tests passed (" + str(n_assertions) + " assertions in " + str(n_test_cases) + " test cases)")
    exit(0)
