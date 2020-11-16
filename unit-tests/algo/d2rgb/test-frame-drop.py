import pyrealsense2 as rs, test, ac


# We set the environment variables to suit this test
test.set_env_vars({"RS2_AC_DISABLE_CONDITIONS":"1",
                   "RS2_AC_DISABLE_RETRIES":"1",
                   "RS2_AC_FORCE_BAD_RESULT":"1",
                   "LOG_TO_STDOUT":"1"
                   })

# rs.log_to_file( rs.log_severity.debug, "rs.log" )

dev = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()
color_sensor = dev.first_color_sensor()

# Resetting sensors to factory calibration
dcs = rs.calibrated_sensor(depth_sensor)
dcs.reset_calibration()

ccs = rs.calibrated_sensor(color_sensor)
ccs.reset_calibration()

d2r = rs.device_calibration(dev)
d2r.register_calibration_change_callback( ac.status_list_callback )

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

# This variable controls how many calibrations we do while testing for frame drops
n_cal = 5
# Variables for saving the previous color frame number and previous depth frame number
pcf_num = -1
pdf_num = -1

# Functions that assert that each frame we receive has the frame number following the previous frame number
def check_next_cf(frame):
    global pcf_num
    cf_num = frame.get_frame_number()
    if pcf_num != -1:
        # print("pre: " + str(pcf_num) + " ,cur: " + str(cf_num))
        test.check_equal(pcf_num + 1, cf_num)
    pcf_num = cf_num

def check_next_df(frame):
    global pdf_num
    cf_num = frame.get_frame_number()
    if pdf_num != -1:
        # print("pre: " + str(pdf_num) + " ,cur: " + str(cf_num))
        test.check_equal(pdf_num + 1, cf_num)
    pdf_num = cf_num

depth_sensor.open( dp )
depth_sensor.start( check_next_df )
color_sensor.open( cp )
color_sensor.start( check_next_cf )

#############################################################################################
# Test #1
test.start("Checking for frame drops in " + str(n_cal) + " calibrations")
for i in range(n_cal):
    try:
        d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
        ac.wait_for_calibration()
    except:
        continue
test.finish()

#############################################################################################
# Test #2
test.start("Checking for frame drops in a failed calibration")
ac.reset_status_list()
d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
try:
    d2r.trigger_device_calibration( rs.calibration_type.manual_depth_to_rgb )
    ac.wait_for_calibration()
except Exception as e: # Second trigger should throw exception
    test.check_exception(e, RuntimeError, "Camera Accuracy Health is already active")
else:
    test.check_no_reach()
ac.wait_for_calibration() # First trigger should continue and finish successfully
test.finish()

#############################################################################################
test.print_results_and_exit()
