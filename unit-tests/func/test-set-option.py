import platform
import pyrealsense2 as rs, test
import time

#L515

dev = test.find_first_device_or_exit()
depth_sensor = dev.first_depth_sensor()
color_sensor = dev.first_color_sensor()

previous_depth_frame_number = -1
previous_color_frame_number = -1
after_set_option = 0


def get_allowed_drops(): 
    global after_set_option
    if platform.system() == 'Linux' and after_set_option == 1:
        return 4 #the number of allowed drops after set_option in linux is 4  
    return 0

def set_new_value(sensor, option, value): 
    global after_set_option
    after_set_option = 1
    sensor.set_option(option, value)
    time.sleep(0.5) #collect frames for 0.5 seconds
    after_set_option = 0

def check_depth_frame_drops(frame):
    global previous_depth_frame_number
    allowed_drops = get_allowed_drops()
    test.check_frame_drops(frame, previous_depth_frame_number, allowed_drops)
    previous_depth_frame_number = frame.get_frame_number()

def check_color_frame_drops(frame):
    global previous_color_frame_number
    allowed_drops = get_allowed_drops()
    test.check_frame_drops(frame, previous_color_frame_number, allowed_drops)
    previous_color_frame_number = frame.get_frame_number()

depth_profile = next(p for p in
                depth_sensor.profiles if p.fps() == 30
                and p.stream_type() == rs.stream.depth
                and p.format() == rs.format.z16
                and p.as_video_stream_profile().width() == 320
                and p.as_video_stream_profile().height() == 240)

color_profile = next(p for p in color_sensor.profiles if p.fps() == 30
                and p.stream_type() == rs.stream.color
                and p.format() == rs.format.yuyv
                and p.as_video_stream_profile().width() == 640
                and p.as_video_stream_profile().height() == 480)

depth_sensor.open( depth_profile )
depth_sensor.start( check_depth_frame_drops )
color_sensor.open( color_profile )
color_sensor.start( check_color_frame_drops )


#############################################################################################
# Test #1

laser_power = rs.option.laser_power
current_laser_control = 10

test.start("Checking for frame drops when setting laser power several times")

for i in range(1,5): 
    new_value = current_laser_control + 10*i
    set_new_value(depth_sensor, laser_power, new_value)


test.finish()
#############################################################################################
# Test #2

depth_options = depth_sensor.get_supported_options()
color_options = color_sensor.get_supported_options()

test.start("Checking for frame drops when setting any option")

for option in depth_options:
    if (depth_sensor.is_option_read_only(option) == 1): 
        continue
    new_value = depth_sensor.get_option_range(option).min
    set_new_value(depth_sensor, option, new_value)

for option in color_options:
    if (color_sensor.is_option_read_only(option) == 1): 
        continue
    new_value = color_sensor.get_option_range(option).min
    set_new_value(color_sensor, option, new_value)

test.finish()
#############################################################################################
depth_sensor.stop()
depth_sensor.close()

color_sensor.stop()
color_sensor.close()

test.print_results_and_exit()
