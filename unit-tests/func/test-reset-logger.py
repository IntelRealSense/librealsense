import platform
import pyrealsense2 as rs
import time


rs.log_to_file( rs.log_severity.debug, "rs7.log" )

c = rs.context()
dev = c.devices[0]

depth_sensor = dev.first_depth_sensor()
color_sensor = dev.first_color_sensor()

previous_depth_frame_number = -1
previous_color_frame_number = -1
after_set_option = 0



def set_new_value(sensor, option, value): 
    sensor.set_option(option, value)

def check_depth_frame_drops(frame):
    global previous_depth_frame_number
    previous_depth_frame_number = frame.get_frame_number()

def check_color_frame_drops(frame):
    global previous_color_frame_number
    previous_color_frame_number = frame.get_frame_number()

# Use a profile that's common to both L500 and D400
depth_profile = next(p for p in
                depth_sensor.profiles if p.fps() == 30
                and p.stream_type() == rs.stream.depth
                and p.format() == rs.format.z16
                and p.as_video_stream_profile().width() == 640
                and p.as_video_stream_profile().height() == 480)

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

for i in range(1,5): 
    new_value = current_laser_control + 10*i
    set_new_value(depth_sensor, laser_power, new_value)


depth_sensor.stop()
depth_sensor.close()

color_sensor.stop()
color_sensor.close()

print("Done Test 1")


depth_options = depth_sensor.get_supported_options()
color_options = color_sensor.get_supported_options()

depth_sensor.open( depth_profile )
depth_sensor.start( check_depth_frame_drops )
color_sensor.open( color_profile )
color_sensor.start( check_color_frame_drops )

for option in depth_options:
    if depth_sensor.is_option_read_only(option): 
        continue
    new_value = depth_sensor.get_option_range(option).min
    print(option)
    set_new_value(depth_sensor, option, new_value)

for option in color_options:
    if color_sensor.is_option_read_only(option): 
        continue
    new_value = color_sensor.get_option_range(option).min
    set_new_value(color_sensor, option, new_value)



print("Done Test 2")

depth_sensor.stop()
depth_sensor.close()

color_sensor.stop()
color_sensor.close()

exit()

