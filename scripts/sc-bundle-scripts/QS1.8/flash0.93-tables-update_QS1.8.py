import sys
import json
import pyrealsense2 as rs
import time
print(f"pyrealsense2 location: {rs.__file__}")
#############################################################################################

ctx = rs.context()
dev = ctx.query_devices()[0]
safety_sensor = dev.first_safety_sensor()

with open('d585s_tables.json') as f:
    d585s_tables = f.read()

#############################################################################################
print("***************************************************")
print("Setting default tables according to spec flash 0.93")
print("***************************************************")
#############################################################################################

print("Switching to Service Mode")  # See SRS ID 3.3.1.7.a
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.service)

#############################################################################################

print("Setting Safety Interface Config table")
safety_sensor.set_safety_interface_config(d585s_tables)


print( "Sending HW-reset command" )
dev.hardware_reset()

print( "Sleep to give some time for the device to reconnect (10 sec)" )
time.sleep( 10 )

print( "Fetching new device" )
dev = ctx.query_devices()[0]
safety_sensor = dev.first_safety_sensor()

print("Switching to Service Mode again")  # See SRS ID 3.3.1.7.a
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.service)

#############################################################################################
print("Setting all Safety Zone tables")
for x in range(64):
    sys.stdout.write("\rInit preset ID = " + repr(x))
    safety_sensor.set_safety_preset(x, d585s_tables)
sys.stdout.write("\r")

#############################################################################################

print("Setting App Config table")
safety_sensor.set_application_config(d585s_tables)

#############################################################################################

print("Setting Calib Config table")
ac_dev = dev.as_auto_calibrated_device()
ac_dev.set_calibration_config(d585s_tables)

#############################################################################################

# switch back to Run safety mode
safety_sensor.set_option(rs.option.safety_mode, rs.safety_mode.run)

#############################################################################################

print("***************************************************")
print("CONGRATS!")
print("All set to flash 0.93 tables for:")
print("- Safety Presets")
print("- Safety Interface Configuration")
print("- App Config")
print("- Calib Config")
print("***************************************************")