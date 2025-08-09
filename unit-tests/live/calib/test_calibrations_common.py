# Copyright (2017-2025), RealSense, Inc.
# Certain Intel® RealSense™ products are sold by RealSense, Inc. under license from Intel Corporation),
# This "Software" is furnished under license and may only be used or copied in accordance with the terms of that license.
# No license, express or implied, by estoppel or otherwise, to any intellectual property rights is granted by this document.
# The Software is subject to change without notice and should not be construed as a commitment by RealSense, Inc. or Intel Corporation to market, license, sell or support any product or technology.
# Unless otherwise provided for in the license under which this Software is provided, the Software is provided AS IS, with no warranties of any kind, express or implied.
# Except as expressly permitted by the Software license, neither RealSense, Inc. nor Intel Corporation, or any of their suppliers, assumes any responsibility or liability for any errors or inaccuracies that may appear herein.
# Except as expressly permitted by the Software license, no part of the Software may be reproduced, stored in a retrieval system, transmitted in any form, or distributed by any means without the express written consent of RealSense, Inc.

#########################################################################################################################################
##                   tests for OCC and Tare calib flows                                                                ##
#########################################################################################################################################
import sys
import time
import pyrealsense2 as rs
from rspy import test, log

# Constants for calibration
CALIBRATION_TIMEOUT_SECONDS = 30
OCC_TIMEOUT_MS = 9000
TARE_TIMEOUT_MS = 10000
FRAME_PROCESSING_TIMEOUT_MS = 5000
HARDWARE_RESET_DELAY_SECONDS = 3

def on_calib_cb(progress):
    """Callback function for calibration progress reporting."""
    pp = int(progress)
    log.i('\r' + '*' * pp + ' ' * (99 - pp) + '*')
    if pp == 100:
        log.i("progress - ", pp)

def calibration_main(host_assistance, occ_calib, json_config, ground_truth):
    """
    Main calibration function for both OCC and Tare calibrations.
    
    Args:
        host_assistance (bool): Whether to use host assistance mode
        occ_calib (bool): True for OCC calibration, False for Tare calibration
        json_config (str): JSON configuration string
        ground_truth (float): Ground truth value for Tare calibration (None for OCC)
    
    Returns:
        bool: True if calibration was not skipped, False if was skipped
        float: Health factor from calibration
    """
    config = rs.config()
    pipeline = rs.pipeline()
    pipeline_wrapper = rs.pipeline_wrapper(pipeline)
    pipeline_profile = config.resolve(pipeline_wrapper)
    auto_calibrated_device = rs.auto_calibrated_device(pipeline_profile.get_device())
    if not auto_calibrated_device:
        log.e("Failed to open device for calibration")
        test.fail()
        return True, 0
    
    ctx = rs.context()
    try:
        device = ctx.query_devices()[0]
    except IndexError:
        log.e("Failed to connect device for calibration")
        test.fail()
        return True, 0
    
    # Skip test for MIPI devices and are not in host assistance mode
    if not device.supports(rs.camera_info.usb_type_descriptor) and not host_assistance:
        log.i("MIPI device - skip the test")
        return False, 0
    
    # Bring device in start phase
    device.hardware_reset()
    time.sleep(HARDWARE_RESET_DELAY_SECONDS)

    stream_config = rs.config()
    if host_assistance:
        stream_config.enable_stream(rs.stream.depth, 1280, 720, rs.format.z16, 30)            
    else:    
        stream_config.enable_stream(rs.stream.depth, 256, 144, rs.format.z16, 90)

    conf = pipeline.start(stream_config)
    pipeline.wait_for_frames()  # Verify streaming started before calling calibration methods
    calib_dev = rs.auto_calibrated_device(conf.get_device())

    depth_sensor = conf.get_device().first_depth_sensor()
    if depth_sensor.supports(rs.option.emitter_enabled):
        depth_sensor.set_option(rs.option.emitter_enabled, 1)
    if depth_sensor.supports(rs.option.thermal_compensation):
        depth_sensor.set_option(rs.option.thermal_compensation, 0)

    # Execute calibration based on type
    try:
        if occ_calib:    
            log.i("Starting on-chip calibration")
            new_calib, health = calib_dev.run_on_chip_calibration(json_config, on_calib_cb, OCC_TIMEOUT_MS)
        else:    
            log.i("Starting tare calibration")
            new_calib, health = calib_dev.run_tare_calibration(ground_truth, json_config, on_calib_cb, TARE_TIMEOUT_MS)

        calib_done = len(new_calib) > 0
        timeout_end = time.time() + CALIBRATION_TIMEOUT_SECONDS
        
        while not calib_done:
            if time.time() > timeout_end:
                raise RuntimeError("Calibration timed out after {} seconds".format(CALIBRATION_TIMEOUT_SECONDS))
            frame_set = pipeline.wait_for_frames()
            depth_frame = frame_set.get_depth_frame()
            new_calib, health = calib_dev.process_calibration_frame(depth_frame, on_calib_cb, FRAME_PROCESSING_TIMEOUT_MS)
            calib_done = len(new_calib) > 0
            
        log.i("Calibration completed successfully")
        log.i("Health factor = ", health[0])
        
    except Exception as e:
        log.e("Calibration failed: ", str(e))
        raise
    finally:
        # Stop pipeline
        try:
            pipeline.stop()
        except Exception as stop_error:
            log.e("Failed to stop pipeline: ", str(stop_error))

    return True, health[0]

