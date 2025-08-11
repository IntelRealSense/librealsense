# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 RealSense, Inc. All Rights Reserved.
                                                          ##
import sys
import time
import pyrealsense2 as rs
from rspy import test, log
from test_calibrations_common import calibration_main, is_mipi_device

#disabled until we stabilize lab
#test:donotrun

def tare_calibration_json(tare_json_file, host_assistance):
    tare_json = None
    if tare_json_file is not None:
        try:
            tare_json = open(tare_json_file).read()
        except:
            tare_json = None
            log.e('Error reading tare_json_file: ', tare_json_file)
    if tare_json is None:
        log.i('Using default parameters for Tare calibration.')
        tare_json = '{\n  '+\
                    '"host assistance": ' + str(int(host_assistance)) + ',\n'+\
                    '"speed": 3,\n'+\
                    '"scan parameter": 0,\n'+\
                    '"step count": 20,\n'+\
                    '"apply preset": 1,\n'+\
                    '"accuracy": 2,\n'+\
                    '"depth": 0,\n'+\
                    '"resize factor": 1\n'+\
                    '}'
    return tare_json


def calculate_target_z():
    number_of_images = 50  # The required number of frames is 10+
    timeout_s = 30
    target_size = [175, 100]

    cfg = rs.config()
    cfg.enable_stream(rs.stream.infrared, 1, 1280, 720, rs.format.y8, 30)

    q = rs.frame_queue(capacity=number_of_images, keep_frames=True)
    q2 = rs.frame_queue(capacity=number_of_images, keep_frames=True)
    q3 = rs.frame_queue(capacity=number_of_images, keep_frames=True)

    counter = 0

    def cb(frame):
        nonlocal counter
        if counter > number_of_images:
            return
        for f in frame.as_frameset():
            q.enqueue(f)
            counter += 1

    ctx = rs.context()
    pipe = rs.pipeline(ctx)
    pp = pipe.start(cfg, cb)
    dev = pp.get_device()

    try:
        stime = time.time()
        while counter < number_of_images:
            time.sleep(0.5)
            if timeout_s < (time.time() - stime):
                raise RuntimeError(f"Failed to capture {number_of_images} frames in {timeout_s} seconds, got only {counter} frames")

        adev = dev.as_auto_calibrated_device()
        log.i('Calculating distance to target...')
        log.i(f'\tTarget Size:\t{target_size}')
        target_z = adev.calculate_target_z(q, q2, q3, target_size[0], target_size[1])
        log.i(f'Calculated distance to target is {target_z}')
    finally:
        pipe.stop()

    return target_z


# Constants for validation
HEALTH_FACTOR_THRESHOLD = 0.25
TARGET_Z_MIN = 600
TARGET_Z_MAX = 1500
_target_z = None

with test.closure("Tare calibration test with host assistance"):
    try:
        host_assistance = True
        if (_target_z is None):
            _target_z = calculate_target_z()
            test.check(_target_z > TARGET_Z_MIN and _target_z < TARGET_Z_MAX)
        
        tare_json = tare_calibration_json(None, host_assistance)
        health_factor = calibration_main(host_assistance, False, tare_json, _target_z)

        test.check(abs(health_factor) < HEALTH_FACTOR_THRESHOLD)
    except Exception as e:
        log.e("Tare calibration test with host assistance failed: ", str(e))
        test.fail()

with test.closure("Tare calibration test"):
    try:
        host_assistance = False
        if is_mipi_device():
            log.i("MIPI device - skip the test w/o host assistance")
            test.skip()

        if _target_z is None:
            _target_z = calculate_target_z()
            test.check(_target_z > TARGET_Z_MIN and _target_z < TARGET_Z_MAX)

        tare_json = tare_calibration_json(None, host_assistance)
        health_factor = calibration_main(host_assistance, False, tare_json, _target_z)
        
        test.check(abs(health_factor) < HEALTH_FACTOR_THRESHOLD)
    except Exception as e:
        log.e("Tare calibration test failed: ", str(e))
        test.fail()

# for step 2 -  not in use for now
"""
# This test performs Tare calibration with calibration table backup and modification.
# It demonstrates backing up the calibration table, running the calibration, and restoring the table if needed.
# The test checks that the health factor after calibration is within the allowed threshold.
with test.closure("Tare calibration with table backup and modification"):
    try:
        host_assistance = False
        target_z = calculate_target_z()
        test.check(target_z > TARGET_Z_MIN and target_z < TARGET_Z_MAX)
        tare_json = tare_calibration_json(None, host_assistance)
        
        log.i("Starting Tare calibration with calibration table backup/restore demonstration")
        health_factor = perform_calibration_with_table_backup(host_assistance, False, tare_json, target_z)
        
        if health_factor is not None:
            test.check(abs(health_factor) < HEALTH_FACTOR_THRESHOLD)
            log.i("Tare calibration with table manipulation completed successfully")
        else:
            log.e("Tare calibration with table backup failed")
            test.fail()
            
    except Exception as e:
        log.e("Tare calibration with table backup failed: ", str(e))
        test.fail()
    log.i("Done\n")

test.print_results_and_exit()
"""