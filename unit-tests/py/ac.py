import sys
# add these 2 lines to run independently and not through run-unit-tests.py
path = "C:/Users/mmirbach/git/librealsense/build/RelWithDebInfo"
sys.path.append(path)

import pyrealsense2 as rs
from time import sleep

status_list = []

# Functions for using status list
def status_list_empty():
    global status_list
    return not status_list

def reset_status_list():
    global status_list
    status_list = []

# Call back funtion for testing status sequences
def status_list_cb( status ):
    global status_list
    status_list.append(status)

# Removes irrelevant statuses from status list
def trim_irrelevant_statuses(irrelevant_statuses):
    global status_list
    for irrelevant_status in irrelevant_statuses:
        while irrelevant_status in status_list:
            status_list.remove(irrelevant_status)

# Waits for calibration to end by checking last status call-back
def wait_for_calibration():
    global status_list
    while (status_list[-1] != rs.calibration_status.successful and status_list[-1] != rs.calibration_status.failed):
        sleep(1)
