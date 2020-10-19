import sys
from time import sleep

path = "C:/Users/mmirbach/git/librealsense/build/Debug"
sys.path.append(path)

import pyrealsense2 as rs

status_list = []
# Call back funtion for testing status sequences
def list_status_cb( status ):
    global status_list
    status_list.append(status)

def reset_status_list():
    global status_list
    status_list = []

def status_list_empty():
    global status_list
    return not status_list

# Used to ignore irrelevant statuses in status_list
def trim_irrelevant_statuses(irrelevant_statuses):
    global status_list
    for irrelevant_status in  irrelevant_statuses:
        while irrelevant_status in status_list:
            status_list.remove(irrelevant_status)

# Waits for calibration to end
def wait_for_calibration():
    global status_list
    while (status_list[-1] != rs.calibration_status.successful and status_list[-1] != rs.calibration_status.failed):
        sleep(1)
