# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2023 RealSense, Inc. All Rights Reserved.

import sys
import time
import copy
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
    log.d( f"Calibration at {progress}%" )


def calibration_main(host_assistance, occ_calib, json_config, ground_truth):
    """
    Main calibration function for both OCC and Tare calibrations.
    
    Args:
        host_assistance (bool): Whether to use host assistance mode
        occ_calib (bool): True for OCC calibration, False for Tare calibration
        json_config (str): JSON configuration string
        ground_truth (float): Ground truth value for Tare calibration (None for OCC)
    
    Returns:
        float: Health factor from calibration
    """
    config = rs.config()
    pipeline = rs.pipeline()
    pipeline_wrapper = rs.pipeline_wrapper(pipeline)
    if host_assistance:
        config.enable_stream(rs.stream.depth, 1280, 720, rs.format.z16, 30)            
    else:    
        config.enable_stream(rs.stream.depth, 256, 144, rs.format.z16, 90)
    pipeline_profile = config.resolve(pipeline_wrapper)
    auto_calibrated_device = rs.auto_calibrated_device(pipeline_profile.get_device())
    if not auto_calibrated_device:
        raise RuntimeError("Failed to open auto_calibrated_device for calibration")

    conf = pipeline.start(config)
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
        pipeline.stop()

    return health[0]


def is_mipi_device():
    ctx = rs.context()
    device = ctx.query_devices()[0]
    return device.supports(rs.camera_info.connection_type) and device.get_info(rs.camera_info.connection_type) == "GMSL"

# for step 2 -  not in use for now
"""
def read_and_modify_calibration_table(device):
    Demonstrates how to read, modify, and write calibration table.
    Returns original and modified calibration tables.
    try:
        # Get the auto calibrated device interface
        auto_calib_device = rs.auto_calibrated_device(device)
        if not auto_calib_device:
            log.e("Device does not support auto calibration")
            return None, None
        
        # Read current calibration table
        log.i("Reading current calibration table...")
        original_calib_table = auto_calib_device.get_calibration_table()
        log.i(f"Original calibration table size: {len(original_calib_table)} bytes")
        
        # Make a copy for modification
        modified_calib_table = copy.deepcopy(original_calib_table)
        
        # Example modification: demonstrate table manipulation
        # Note: This is just for demonstration - in real scenarios you would
        # modify specific calibration parameters based on your needs
        if len(modified_calib_table) > 10:
            # Create a backup flag to indicate this table was modified
            # This doesn't change actual calibration parameters, just marks the table
            log.i("Marking calibration table as modified (demo purposes)")
            # In a real scenario, you would modify actual calibration parameters
            # based on the specific calibration table structure
        
        # Set the modified calibration table (temporarily)
        log.i("Setting modified calibration table...")
        auto_calib_device.set_calibration_table(modified_calib_table)
        
        # Verify the table was set by reading it back
        readback_table = auto_calib_device.get_calibration_table()
        log.i(f"Readback calibration table size: {len(readback_table)} bytes")
        
        # Restore original calibration table
        log.i("Restoring original calibration table...")
        auto_calib_device.set_calibration_table(original_calib_table)
        
        return original_calib_table, modified_calib_table
        
    except Exception as e:
        log.e(f"Error in calibration table manipulation: {e}")
        return None, None
"""
# for step 2 -  not in use for now
"""
def perform_calibration_with_table_backup(host_assistance, occ_calib, json_config, ground_truth):    
    Perform calibration while backing up and restoring calibration table.
    Can be used for both OCC and Tare calibrations.
    
    Args:
        host_assistance (bool): Whether to use host assistance mode
        occ_calib (bool): True for OCC calibration, False for Tare calibration
        json_config (str): JSON configuration string
        ground_truth (float): Ground truth value for Tare calibration (None for OCC)
    
    Returns:
        float: Health factor from calibration, or None if failed

    try:
        # Get device
        ctx = rs.context()
        devices = ctx.query_devices()
        if len(devices) == 0:
            log.e("No devices found")
            return None
        
        device = devices[0]
        auto_calib_device = rs.auto_calibrated_device(device)
        if not auto_calib_device:
            log.e("Device does not support auto calibration")
            return None
        
        # Step 1: Read and backup original calibration table
        log.i("=== Step 1: Backup original calibration ===")
        original_table = auto_calib_device.get_calibration_table()
        log.i(f"Backed up calibration table ({len(original_table)} bytes)")
        
        # Step 2: Perform calibration using calibration_main
        calib_type = "OCC" if occ_calib else "Tare"
        log.i(f"=== Step 2: Perform {calib_type} calibration ===")
        status, health_factor = calibration_main(host_assistance, occ_calib, json_config, ground_truth)
        
        if not status:
            log.w("Calibration was skipped")
            return None
            
        log.i(f"Calibration health factor: {health_factor}")
        
        # Step 3: Read new calibration table after calibration
        log.i("=== Step 3: Read new calibration after calibration ===")
        new_table = auto_calib_device.get_calibration_table()
        log.i(f"New calibration table size: {len(new_table)} bytes")
        
        # Step 4: Compare tables
        tables_different = (original_table != new_table)
        log.i(f"Calibration table changed: {tables_different}")
        
        # Step 5: Demonstrate table manipulation
        log.i("=== Step 4: Demonstrate table read/write ===")
        orig_table, mod_table = read_and_modify_calibration_table(device)
        
        # Step 6: Write new calibration to flash (if calibration was successful)
        if abs(health_factor) < 0.25:  # Good calibration
            log.i("=== Step 5: Writing good calibration to flash ===")
            auto_calib_device.write_calibration()
            log.i("New calibration written to flash")
        else:
            log.w("Calibration health not good enough, restoring original")
            auto_calib_device.set_calibration_table(original_table)
            auto_calib_device.write_calibration()
        
        return health_factor
        
    except Exception as e:
        log.e(f"Error in calibration with table backup: {e}")
        return None
"""
