# License: Apache 2.0. See LICENSE file in root directory.
# Copyright(c) 2024 Intel Corporation. All Rights Reserved.

# test:device each(D400*)
# test:context nightly 

import pyrealsense2 as rs
import time
from rspy import test, log
from rspy.timer import Timer

# Test synchronization between color and depth streams across multiple configurations
# This test verifies that both streams can successfully stream frames simultaneously
# Uses different configuration sets for normal vs nightly runs

def test_stream_sync_configuration(device, config):
    """
    Test streaming synchronization for a specific configuration
    Returns tuple (success, color_frames, depth_frames, timestamp_data)
    """
    frame_collect = False
    tested_streams = [rs.stream.color, rs.stream.depth]
    arrived_frames = {rs.stream.depth: 0, rs.stream.color: 0}
    latest_frames = {rs.stream.depth: None, rs.stream.color: None}
    timestamp_deltas = []
    frameset_id = 0
    duration = 3  # test sleep duration.
    
    def callback(frame):
        nonlocal frame_collect, arrived_frames, latest_frames, timestamp_deltas, frameset_id
        if frame_collect:
            raw_profile = frame.get_profile()
            frame_profile = rs.video_stream_profile(raw_profile)
            stream_type = frame_profile.stream_type()
            
            if stream_type in tested_streams:
                arrived_frames[stream_type] += 1
                latest_frames[stream_type] = frame
                
                # Check for timestamp synchronization when we have both frames
                if latest_frames[rs.stream.color] is not None and latest_frames[rs.stream.depth] is not None:
                    try:
                        # Get timestamps of the current frames
                        current_depth_timestamp = latest_frames[rs.stream.depth].get_frame_metadata(rs.frame_metadata_value.frame_timestamp)
                        current_color_timestamp = latest_frames[rs.stream.color].get_frame_metadata(rs.frame_metadata_value.frame_timestamp)

                        # Calc the delta between timestamps in usec - hw timestamp is already in usec units so no need to convert
                        delta = abs(current_color_timestamp - current_depth_timestamp)
                        delta_ms = delta / 1000
                        timestamp_deltas.append(delta_ms)
                        log.d(f"Bundle #{frameset_id}\t Internal Delta is: {delta_ms} ms")
                        frameset_id += 1
                        
                        # Clear frames to wait for next pair
                        latest_frames[rs.stream.depth] = None
                        latest_frames[rs.stream.color] = None
                    except Exception as e:
                        log.d(f"Error getting timestamp metadata: {e}")
                        # Clear frames on error
                        latest_frames[rs.stream.depth] = None
                        latest_frames[rs.stream.color] = None
    
    w = config['w']
    h = config['h']
    fps = config['fps']
    
    try:
        sensors = device.query_sensors()
        color_sensor = next(s for s in sensors if s.get_info(rs.camera_info.name) == 'RGB Camera')
        depth_sensor = next(s for s in sensors if s.get_info(rs.camera_info.name) == 'Stereo Module')
        
        # Find profiles
        color_profiles = color_sensor.profiles
        try:
            color_profile = next(p for p in color_profiles 
                               if p.fps() == fps
                               and p.stream_type() == rs.stream.color
                               and p.format() == rs.format.rgb8
                               and p.as_video_stream_profile().width() == w
                               and p.as_video_stream_profile().height() == h)
        except StopIteration:
            log.d(f"Color profile not found for {w}x{h}@{fps}fps")
            return False, 0, 0
        
        depth_profiles = depth_sensor.profiles
        try:
            depth_profile = next(p for p in depth_profiles 
                               if p.fps() == fps
                               and p.stream_type() == rs.stream.depth
                               and p.format() == rs.format.z16
                               and p.as_video_stream_profile().width() == w
                               and p.as_video_stream_profile().height() == h)
        except StopIteration:
            log.d(f"Depth profile not found for {w}x{h}@{fps}fps")
            return False, 0, 0
        
        # Start streaming
        color_sensor.open(color_profile)
        color_sensor.start(callback)
        
        time.sleep(0.5)  # Brief delay between sensor starts

        depth_sensor.open(depth_profile)
        depth_sensor.start(callback)
        
        # Collect frames
        frame_collect = True
        time.sleep(duration)
        frame_collect = False
        
        # Stop sensors
        color_sensor.stop()
        color_sensor.close()
        depth_sensor.stop()
        depth_sensor.close()
        
        return True, arrived_frames[rs.stream.color], arrived_frames[rs.stream.depth], timestamp_deltas
        
    except Exception as e:
        log.d(f"Error testing {w}x{h}@{fps}fps: {e}")
        # Cleanup on error
        try:
            color_sensor.stop()
            color_sensor.close()
        except:
            pass
        try:
            depth_sensor.stop()
            depth_sensor.close()
        except:
            pass
        return False, 0, 0, []

def reset_device(device):
    """Reset the device between configuration tests"""
    try:
        log.d("Resetting device...")
        device.hardware_reset()
        time.sleep(2)  # Wait for device to stabilize after reset
        log.d("Device reset completed")
    except Exception as e:
        log.d(f"Device reset failed (may not be supported): {e}")
        # If hardware reset fails, try a software approach by cycling sensors
        try:
            sensors = device.query_sensors()
            for sensor in sensors:
                if len(sensor.get_active_streams()) > 0:
                    sensor.stop()
                    sensor.close()
            time.sleep(1)
            log.d("Software reset completed")
        except Exception as e2:
            log.d(f"Software reset also failed: {e2}")

# Test configurations - different sets for normal vs nightly runs
gated_test_configurations = [
    # Quick subset for regular testing
    {'w': 640, 'h': 480, 'fps': 30},
    {'w': 848, 'h': 480, 'fps': 30},
    {'w': 1280, 'h': 720, 'fps': 30},
    {'w': 640, 'h': 480, 'fps': 15},
]

test_configurations = [
    # Full comprehensive set for nightly testing
    {'w': 480, 'h': 270, 'fps': 5},
    {'w': 480, 'h': 270, 'fps': 15},    
    {'w': 480, 'h': 270, 'fps': 30},
    {'w': 480, 'h': 270, 'fps': 60}, 
    {'w': 480, 'h': 270, 'fps': 90}, 
    {'w': 640, 'h': 360, 'fps': 5},
    {'w': 640, 'h': 360, 'fps': 15},
    {'w': 640, 'h': 360, 'fps': 30},
    {'w': 640, 'h': 360, 'fps': 60},
    {'w': 640, 'h': 480, 'fps': 5},
    {'w': 640, 'h': 480, 'fps': 15},
    {'w': 640, 'h': 480, 'fps': 30},
    {'w': 640, 'h': 480, 'fps': 60},
    {'w': 848, 'h': 480, 'fps': 5},
    {'w': 848, 'h': 480, 'fps': 15},
    {'w': 848, 'h': 480, 'fps': 30},
    {'w': 848, 'h': 480, 'fps': 60},
    {'w': 1280, 'h': 720, 'fps': 5},
    {'w': 1280, 'h': 720, 'fps': 15},
    {'w': 1280, 'h': 720, 'fps': 30},
]

device, _ = test.find_first_device_or_exit()
sn = device.get_info(rs.camera_info.serial_number)
fw = device.get_info(rs.camera_info.firmware_version)
log.d(f'Testing device {sn}, fw {fw}')

# Test each configuration
successful_configs = 0

# Choose configuration set based on context
if 'nightly' in test.context:
    # Use full comprehensive set for nightly testing
    configs_to_test = test_configurations
    log.d("Running in NIGHTLY context - using full configuration set (20 configs)")
else:
    # Use reduced set for normal/CI runs
    configs_to_test = gated_test_configurations  
    log.d("Running in NORMAL context - using reduced configuration set (4 configs)")

total_configs = len(configs_to_test)

for i, config in enumerate(configs_to_test):
    w = config['w']
    h = config['h'] 
    fps = config['fps']
    
    test.start(f"Stream sync test {i+1}/{total_configs}: {w}x{h}@{fps}fps")
    
    success, color_frames, depth_frames, timestamp_deltas = test_stream_sync_configuration(device, config)
    
    if success:
        # Check that both streams received frames
        min_expected_frames = fps * 2  # Expect at least 2 seconds worth of frames
        
        log.d(f"Received frames - Color: {color_frames}, Depth: {depth_frames}")
        
        # Test passes if both streams received reasonable number of frames
        color_ok = color_frames >= min_expected_frames
        depth_ok = depth_frames >= min_expected_frames
        
        test.check(color_ok, f"Color stream received {color_frames} frames (expected >= {min_expected_frames })")
        test.check(depth_ok, f"Depth stream received {depth_frames} frames (expected >= {min_expected_frames })")
        
        # Check timestamp synchronization
        if timestamp_deltas:
            avg_delta = sum(timestamp_deltas) / len(timestamp_deltas)
            max_delta = max(timestamp_deltas)
            
            # Maximum allowed timestamp difference is defined by MAX_ALLOWED_TIMESTAMP_DELTA_MS
            max_allowed_delta = MAX_ALLOWED_TIMESTAMP_DELTA_MS
            
            timestamp_sync_ok = max_delta <= max_allowed_delta
            test.check(timestamp_sync_ok, f"Timestamp sync acceptable: max delta {max_delta:.2f}ms <= {max_allowed_delta:.2f}ms")
            log.d(f"Timestamp stats - Avg: {avg_delta:.2f}ms, Max: {max_delta:.2f}ms, Samples: {len(timestamp_deltas)}")
        else:
            test.check(False, "No timestamp synchronization data collected")
        
        if color_ok and depth_ok:
            successful_configs += 1
    else:
        test.check(False, f"Failed to test configuration {w}x{h}@{fps}fps")
    
    test.finish()
    
    # Reset device between configurations (except after the last test)
    if i < total_configs - 1:
        reset_device(device)

# Overall test summary
test.start("Overall synchronization test summary")
success_rate = successful_configs / total_configs
test.check(success_rate == 1, f"100% of configurations successful ({successful_configs}/{total_configs})")
log.d(f"Successfully tested {successful_configs} out of {total_configs} configurations")
test.finish()

test.print_results_and_exit()