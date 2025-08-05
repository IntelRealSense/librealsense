#!/usr/bin/env python3
"""
Intel RealSense FPS Performance Test
Tests different stream combinations: Color+Depth, Color+IR, or IR+Depth
"""

import pyrealsense2 as rs
import numpy as np
import time
import sys
from collections import deque
from typing import List, Tuple, Dict


class FPSMonitor:
    """Monitor and calculate FPS statistics"""
    
    def __init__(self, window_size: int = 30):
        self.window_size = window_size
        self.frame_times = deque(maxlen=window_size)
        self.start_time = None
        self.total_frames = 0
        
    def reset(self):
        """Reset the FPS monitor"""
        self.frame_times.clear()
        self.start_time = None
        self.total_frames = 0
        
    def update(self, frame_time: float):
        """Update with new frame timestamp"""
        if self.start_time is None:
            self.start_time = frame_time
            
        self.frame_times.append(frame_time)
        self.total_frames += 1
        
    def get_current_fps(self) -> float:
        """Calculate current FPS based on recent frames"""
        if len(self.frame_times) < 2:
            return 0.0
            
        time_diff = self.frame_times[-1] - self.frame_times[0]
        if time_diff <= 0:
            return 0.0
            
        return (len(self.frame_times) - 1) / time_diff
        
    def get_average_fps(self) -> float:
        """Calculate average FPS since start"""
        if self.start_time is None or self.total_frames < 2:
            return 0.0
            
        elapsed_time = self.frame_times[-1] - self.start_time
        if elapsed_time <= 0:
            return 0.0
            
        return self.total_frames / elapsed_time


class RealSenseFPSTest:
    """RealSense FPS test class"""
    
    def __init__(self):
        self.pipeline = None
        self.config = None
        self.device = None
        self.stream_combinations = {
            'color_depth': ['color', 'depth'],
            'color_ir': ['color', 'infrared'],
            'ir_depth': ['infrared', 'depth'],
            'color_only': ['color']
        }
        
    def initialize_camera(self) -> bool:
        """Initialize RealSense camera"""
        try:
            # Create a context object
            ctx = rs.context()
            devices = ctx.query_devices()
            
            if len(devices) == 0:
                print("ERROR: No RealSense devices found!")
                return False
                
            self.device = devices[0]
            print(f"Found device: {self.device.get_info(rs.camera_info.name)}")
            print(f"Serial number: {self.device.get_info(rs.camera_info.serial_number)}")
            print(f"Firmware version: {self.device.get_info(rs.camera_info.firmware_version)}")
            
            # Get ISP firmware version using GVD command
            try:
                # Send GVD command
                debug_protocol = rs.debug_protocol(self.device)
                gvd_command = debug_protocol.build_command(0x10)  # 0x10 is GVD opcode
                gvd_response = debug_protocol.send_and_receive_raw_data(gvd_command)
                
                if gvd_response and len(gvd_response) >= 150:
                    
                    rgb_isp_fw_version_offset = 152
                    
                    if len(gvd_response) >= rgb_isp_fw_version_offset + 2:
                        # Extract 2-byte rgbIspFWVersion
                        fw_bytes = gvd_response[rgb_isp_fw_version_offset:rgb_isp_fw_version_offset + 2]
                        # Convert to uint16_t (big-endian)
                        isp_fw_version_raw = fw_bytes[1] | (fw_bytes[0] << 8)
                        # Format as version (the raw value represents the ISP firmware version)
                        print(f"ISP Firmware version: (0x{isp_fw_version_raw:04X})")
                    else:
                        print("ISP Firmware version: Insufficient GVD response data for rgbIspFWVersion")
                else:
                    print(f"ISP Firmware version: GVD response too short (got {len(gvd_response) if gvd_response else 0} bytes, need at least 150)")
                    
            except Exception as e:
                print(f"ISP Firmware version: Unable to retrieve ({e})")
            
            # Create pipeline and config
            self.pipeline = rs.pipeline()
            self.config = rs.config()
            
            return True
            
        except Exception as e:
            print(f"ERROR: Failed to initialize camera: {e}")
            return False
            
    def get_supported_resolutions(self) -> Dict[str, List[Tuple[int, int, int]]]:
        """Get all supported resolutions for Color, Depth, and IR1 streams"""
        supported_resolutions = {'color': [], 'depth': [], 'infrared': []}
        
        try:
            # Get device sensors
            sensors = self.device.query_sensors()
            
            for sensor in sensors:
                # Get stream profiles
                profiles = sensor.get_stream_profiles()
                
                for profile in profiles:
                    if profile.stream_type() == rs.stream.color:
                        vp = profile.as_video_stream_profile()
                        res_fps = (vp.width(), vp.height(), vp.fps())
                        if res_fps not in supported_resolutions['color']:
                            supported_resolutions['color'].append(res_fps)
                    elif profile.stream_type() == rs.stream.depth:
                        vp = profile.as_video_stream_profile()
                        res_fps = (vp.width(), vp.height(), vp.fps())
                        if res_fps not in supported_resolutions['depth']:
                            supported_resolutions['depth'].append(res_fps)
                    elif profile.stream_type() == rs.stream.infrared and profile.stream_index() == 1:
                        vp = profile.as_video_stream_profile()
                        res_fps = (vp.width(), vp.height(), vp.fps())
                        if res_fps not in supported_resolutions['infrared']:
                            supported_resolutions['infrared'].append(res_fps)
            
            # Sort by resolution (width * height) and then by fps
            for stream_type in supported_resolutions:
                supported_resolutions[stream_type].sort(key=lambda x: (x[0] * x[1], x[2]))
            
        except Exception as e:
            print(f"ERROR: Failed to get supported resolutions: {e}")
            
        return supported_resolutions
    
    def find_common_configurations(self, streams: List[str]) -> List[Tuple[int, int, int]]:
        """Find resolutions and FPS that are supported by the specified streams"""
        supported_res = self.get_supported_resolutions()
        
        # Start with the first stream's configurations
        common_configs = set(supported_res[streams[0]])
        
        # Find intersection with each additional stream
        for stream in streams[1:]:
            stream_configs = set(supported_res[stream])
            common_configs = common_configs.intersection(stream_configs)
        
        # Convert back to sorted list
        common_list = sorted(list(common_configs), key=lambda x: (x[0] * x[1], x[2]))
        
        return common_list
    
    def print_supported_resolutions(self, streams: List[str] = None):
        """Print all supported resolutions"""
        print("\n" + "="*60)
        print("SUPPORTED RESOLUTIONS AND FRAME RATES")
        print("="*60)
        
        supported_res = self.get_supported_resolutions()
        
        if streams is None:
            streams = ['color', 'depth', 'infrared']
            
        for stream in streams:
            stream_name = stream.upper() if stream != 'infrared' else 'IR1'
            print(f"\n{stream_name} Stream supported configurations:")
            for width, height, fps in supported_res[stream]:
                print(f"  - {width}x{height} @ {fps} FPS")
        
        common_configs = self.find_common_configurations(streams)
        stream_names = [s.upper() if s != 'infrared' else 'IR1' for s in streams]
        stream_combo = ' + '.join(stream_names)
        
        print(f"\nCommon configurations ({stream_combo}): {len(common_configs)}")
        for width, height, fps in common_configs:
            print(f"  - {width}x{height} @ {fps} FPS")

    def configure_streams(self, streams: List[str], width: int, height: int, fps: int) -> bool:
        """Configure specified streams with given resolution and FPS"""
        try:
            # Clear previous configuration
            self.config = rs.config()
            
            # Enable specified streams
            for stream in streams:
                if stream == 'color':
                    self.config.enable_stream(rs.stream.color, width, height, rs.format.bgr8, fps)
                elif stream == 'depth':
                    self.config.enable_stream(rs.stream.depth, width, height, rs.format.z16, fps)
                elif stream == 'infrared':
                    self.config.enable_stream(rs.stream.infrared, 1, width, height, rs.format.y8, fps)

            return True
            
        except Exception as e:
            print(f"ERROR: Failed to configure streams: {e}")
            return False
            
    def test_fps_performance(self, streams: List[str], width: int, height: int, target_fps: int, test_duration: int = 10) -> Dict[str, float]:
        """Test FPS performance for specified streams, resolution and duration"""
        
        stream_names = [s.upper() if s != 'infrared' else 'IR1' for s in streams]
        stream_combo = ' + '.join(stream_names)
        
        print(f"\n{'='*60}")
        print(f"Testing {stream_combo} at {width}x{height} @ {target_fps} FPS for {test_duration} seconds...")
        print(f"{'='*60}")
        
        # Configure streams
        if not self.configure_streams(streams, width, height, target_fps):
            return {
                'status': 'FAILED',
                'error': 'Failed to configure streams',
                'total_frames': 0,
                'test_duration': test_duration,
                'width': width,
                'height': height,
                'target_fps': target_fps,
                'streams': streams
            }
            
        # Initialize FPS monitors for each stream
        fps_monitors = {}
        for stream in streams:
            fps_monitors[stream] = FPSMonitor()
        
        start_time = None
        frame_count = 0
        error_message = None
        
        try:
            # Start streaming
            profile = self.pipeline.start(self.config)
            
            # Get stream info and print active streams
            print("Active streams:")
            for stream in streams:
                if stream == 'color':
                    stream_profile = profile.get_stream(rs.stream.color)
                elif stream == 'depth':
                    stream_profile = profile.get_stream(rs.stream.depth)
                elif stream == 'infrared':
                    stream_profile = profile.get_stream(rs.stream.infrared, 1)
                
                stream_name = stream.upper() if stream != 'infrared' else 'IR1'
                vsp = stream_profile.as_video_stream_profile()
                print(f"  - {stream_name}: {vsp.width()}x{vsp.height()} @ {vsp.fps()} FPS")
            
            # Allow camera to settle
            print("Warming up camera...")
            warmup_frames = 0
            warmup_timeout = 10  # 10 seconds for warmup
            warmup_start = time.time()
            
            while warmup_frames < 30 and (time.time() - warmup_start) < warmup_timeout:
                try:
                    frames = self.pipeline.wait_for_frames(timeout_ms=1000)
                    if frames:
                        warmup_frames += 1
                except RuntimeError:
                    print(f"WARNING: Timeout during warmup (frame {warmup_frames}/30)")
                    
            if warmup_frames < 30:
                error_message = f"Failed to get sufficient warmup frames ({warmup_frames}/30)"
                print(f"WARNING: {error_message}")
            
            print(f"Starting {test_duration}-second test...")
            start_time = time.time()
            last_print_time = start_time
            no_frame_count = 0
            max_no_frame_count = 10  # Allow up to 10 consecutive failed frame retrievals
            
            while time.time() - start_time < test_duration:
                try:
                    # Wait for frames with timeout
                    frames = self.pipeline.wait_for_frames(timeout_ms=1000)
                    current_time = time.time()
                    
                    # Get frames for each stream
                    stream_frames = {}
                    all_frames_valid = True
                    
                    for stream in streams:
                        if stream == 'color':
                            stream_frames[stream] = frames.get_color_frame()
                        elif stream == 'depth':
                            stream_frames[stream] = frames.get_depth_frame()
                        elif stream == 'infrared':
                            stream_frames[stream] = frames.get_infrared_frame(1)
                        
                        if not stream_frames[stream]:
                            all_frames_valid = False
                            break
                    
                    if all_frames_valid:
                        # Reset no frame counter
                        no_frame_count = 0
                        
                        # Update FPS monitors
                        for stream in streams:
                            fps_monitors[stream].update(current_time)
                        frame_count += 1
                        
                        # Print current FPS every second
                        if current_time - last_print_time >= 1.0:
                            elapsed = current_time - start_time
                            fps_display = []
                            for stream in streams:
                                current_fps = fps_monitors[stream].get_current_fps()
                                stream_name = stream.upper() if stream != 'infrared' else 'IR1'
                                fps_display.append(f"{stream_name}: {current_fps:5.1f}")
                            
                            print(f"[{elapsed:5.1f}s] Current FPS - {', '.join(fps_display)}")
                            last_print_time = current_time
                    else:
                        no_frame_count += 1
                        print(f"WARNING: Invalid frames received ({no_frame_count}/{max_no_frame_count})")
                        
                        if no_frame_count >= max_no_frame_count:
                            error_message = f"Too many consecutive invalid frames ({no_frame_count})"
                            break
                            
                except RuntimeError as e:
                    no_frame_count += 1
                    print(f"WARNING: Frame timeout ({no_frame_count}/{max_no_frame_count}): {e}")
                    
                    if no_frame_count >= max_no_frame_count:
                        error_message = f"Too many consecutive frame timeouts ({no_frame_count})"
                        break
                        
        except Exception as e:
            error_message = f"ERROR during streaming: {e}"
            print(error_message)
            
        finally:
            # Stop streaming
            try:
                self.pipeline.stop()
            except Exception:
                pass
        
        # Determine test status
        actual_duration = time.time() - start_time if start_time else 0
        min_expected_frames = int(target_fps * test_duration * 0.5)  # At least 50% of expected frames
        test_status = 'PASSED'
        
        if error_message:
            test_status = 'FAILED'
        elif frame_count < min_expected_frames:
            test_status = 'FAILED'
            if not error_message:
                error_message = f"Insufficient frames captured ({frame_count} < {min_expected_frames})"
        elif actual_duration < test_duration * 0.9:  # Test ended early
            test_status = 'FAILED'
            if not error_message:
                error_message = f"Test ended early ({actual_duration:.1f}s < {test_duration}s)"
            
        # Calculate final statistics
        results = {
            'status': test_status,
            'total_frames': frame_count,
            'test_duration': test_duration,
            'actual_duration': actual_duration,
            'width': width,
            'height': height,
            'target_fps': target_fps,
            'streams': streams,
            'min_expected_frames': min_expected_frames
        }
        
        if error_message:
            results['error'] = error_message
        
        # Add FPS results for each stream
        for stream in streams:
            if stream in fps_monitors:
                results[f'{stream}_avg_fps'] = fps_monitors[stream].get_average_fps()
                results[f'{stream}_current_fps'] = fps_monitors[stream].get_current_fps()
            else:
                results[f'{stream}_avg_fps'] = 0.0
                results[f'{stream}_current_fps'] = 0.0
        
        # Print results
        print(f"\n{'='*40}")
        print(f"TEST RESULTS FOR {stream_combo} at {width}x{height} @ {target_fps} FPS:")
        print(f"{'='*40}")
        print(f"Status: {test_status}")
        if error_message:
            print(f"Error: {error_message}")
        print(f"Test Duration: {test_duration} seconds (actual: {actual_duration:.1f}s)")
        print(f"Total Frames Captured: {frame_count} (min expected: {min_expected_frames})")
        
        if test_status == 'PASSED':
            total_avg_fps = 0
            for stream in streams:
                stream_name = stream.upper() if stream != 'infrared' else 'IR1'
                avg_fps = results[f'{stream}_avg_fps']
                current_fps = results[f'{stream}_current_fps']
                print(f"{stream_name} Stream:")
                print(f"  - Average FPS: {avg_fps:.2f}")
                print(f"  - Current FPS: {current_fps:.2f}")
                total_avg_fps += avg_fps
            
            avg_combined_fps = total_avg_fps / len(streams)
            print(f"Overall Performance: {avg_combined_fps:.2f} FPS avg")
        
        return results
        
    def hardware_reset(self):
        """Perform hardware reset of the device"""
        try:
            print("Performing hardware reset...")
            if self.device:
                # Stop pipeline if running
                if self.pipeline:
                    try:
                        self.pipeline.stop()
                    except Exception:
                        pass
                
                # Reset device
                self.device.hardware_reset()
                print("Hardware reset completed")
                
                # Wait for device to restart
                time.sleep(3)
                
                # Re-initialize device connection
                ctx = rs.context()
                devices = ctx.query_devices()
                if len(devices) > 0:
                    self.device = devices[0]
                    self.pipeline = rs.pipeline()
                    print("Device reconnected after reset")
                    return True
                else:
                    print("WARNING: Device not found after reset")
                    return False
            else:
                print("WARNING: No device available for reset")
                return False
                
        except Exception as e:
            print(f"WARNING: Hardware reset failed: {e}")
            return False
            
    def cleanup(self):
        """Clean up resources"""
        if self.pipeline:
            try:
                self.pipeline.stop()
            except Exception:
                pass
                
    def _select_test_configurations(self, common_configs: List[Tuple[int, int, int]], max_tests: int) -> List[Tuple[int, int, int]]:
        """Select a subset of configurations for testing"""
        if len(common_configs) <= max_tests:
            return common_configs
            
        configs_to_test = []
        
        # Always include smallest resolution
        if len(common_configs) >= 1:
            configs_to_test.append(common_configs[0])
            
        # Always include largest resolution
        if len(common_configs) >= 2:
            configs_to_test.append(common_configs[-1])
            
        # Add some middle configurations
        if len(common_configs) >= 3:
            remaining = max_tests - len(configs_to_test)
            step = len(common_configs) // (remaining + 1)
            for i in range(1, remaining + 1):
                idx = i * step
                if idx < len(common_configs) - 1:
                    configs_to_test.append(common_configs[idx])
                    
        return configs_to_test

    def run_all_tests(self, stream_combo: str, test_duration: int = 10, max_tests: int = None) -> Dict[str, Dict[str, float]]:
        """Run FPS tests for all supported resolutions with specified stream combination"""
        
        if stream_combo not in self.stream_combinations:
            print(f"ERROR: Invalid stream combination '{stream_combo}'. Valid options: {list(self.stream_combinations.keys())}")
            return {}
            
        streams = self.stream_combinations[stream_combo]
        stream_names = [s.upper() if s != 'infrared' else 'IR1' for s in streams]
        stream_display = ' + '.join(stream_names)
        
        print("Intel RealSense FPS Performance Test")
        print(f"Testing {stream_display} streams at all supported resolutions")
        print(f"Test duration: {test_duration} seconds per configuration")
        
        # Get supported resolutions
        self.print_supported_resolutions(streams)
        common_configs = self.find_common_configurations(streams)
        
        if not common_configs:
            print(f"ERROR: No common configurations found for {stream_display} streams")
            return {}
        
        # Limit number of tests if specified
        if max_tests and len(common_configs) > max_tests:
            print(f"\nLimiting to {max_tests} test configurations...")
            common_configs = self._select_test_configurations(common_configs, max_tests)
        
        all_results = {}
        
        for i, (width, height, fps) in enumerate(common_configs):
            config_key = f"{width}x{height}@{fps}fps"
            print(f"\n[{i+1}/{len(common_configs)}] Testing {config_key}")
            
            try:
                # Perform hardware reset before each test
                if i > 0:  # Skip reset for first test
                    print(f"Performing hardware reset before test {i+1}...")
                    if not self.hardware_reset():
                        print(f"WARNING: Hardware reset failed before {config_key}")
                
                results = self.test_fps_performance(streams, width, height, fps, test_duration)
                all_results[config_key] = results  # Always store results, even if failed
                    
                # Brief pause between tests
                #time.sleep(2)
                
            except KeyboardInterrupt:
                print("\nTest interrupted by user")
                break
            except Exception as e:
                print(f"ERROR in test for {config_key}: {e}")
                # Store failed test result
                all_results[config_key] = {
                    'status': 'FAILED',
                    'error': f"Exception during test: {e}",
                    'total_frames': 0,
                    'test_duration': test_duration,
                    'width': width,
                    'height': height,
                    'target_fps': fps,
                    'streams': streams
                }
                # Add zero FPS for each stream
                for stream in streams:
                    all_results[config_key][f'{stream}_avg_fps'] = 0.0
                    all_results[config_key][f'{stream}_current_fps'] = 0.0
                continue
                
        return all_results
        
    def print_summary(self, all_results: Dict[str, Dict[str, float]], stream_combo: str):
        """Print test summary for the specified stream combination"""
        
        if not all_results:
            return
            
        streams = self.stream_combinations[stream_combo]
        stream_names = [s.upper() if s != 'infrared' else 'IR1' for s in streams]
        
        print(f"\n{'='*120}")
        print(f"SUMMARY OF ALL TESTS - {' + '.join(stream_names)}")
        print(f"{'='*120}")
        
        # Create header based on streams
        header = f"{'Configuration':<20} "
        for stream_name in stream_names:
            header += f"{stream_name + ' Avg':<10} "
        header += f"{'Combined Avg':<14} {'Performance':<12} {'Resolution':<12} {'Status':<8}"
        print(header)
        print("-" * 130)
        
        # Sort by resolution (pixels) and then by fps
        sorted_results = sorted(all_results.items(), 
                              key=lambda x: (x[1]['width'] * x[1]['height'], x[1]['target_fps']))
        
        passed_tests = 0
        failed_tests = 0
        
        for config_key, results in sorted_results:
            line = f"{config_key:<20} "
            
            status = results.get('status', 'UNKNOWN')
            
            if status == 'PASSED':
                passed_tests += 1
                # Add FPS for each stream
                total_fps = 0
                for stream in streams:
                    avg_fps = results[f'{stream}_avg_fps']
                    line += f"{avg_fps:<10.2f} "
                    total_fps += avg_fps
                
                combined_avg = total_fps / len(streams)
                performance = (combined_avg / results['target_fps']) * 100
                resolution = f"{results['width']}x{results['height']}"
                
                line += f"{combined_avg:<14.2f} {performance:<12.1f}% {resolution:<12} {status:<8}"
            else:
                failed_tests += 1
                # Add dashes for failed tests
                for stream in streams:
                    line += f"{'N/A':<10} "
                
                combined_avg = 0.0
                performance = 0.0
                resolution = f"{results['width']}x{results['height']}"
                
                line += f"{'N/A':<14} {'N/A':<12} {resolution:<12} {status:<8}"
                
                # Add error message if available
                if 'error' in results:
                    line += f" ({results['error']})"
            
            print(line)
            
        print("-" * 130)
        print("Performance = (Achieved FPS / Target FPS) * 100%")
        print(f"SUMMARY: {passed_tests} passed, {failed_tests} failed, {len(all_results)} total tests")
        
        # Print resolution statistics for passed tests only
        if passed_tests > 0:
            print("\nResolution Statistics (Passed Tests Only):")
            resolutions = {}
            for results in all_results.values():
                if results.get('status') == 'PASSED':
                    res_key = f"{results['width']}x{results['height']}"
                    if res_key not in resolutions:
                        resolutions[res_key] = []
                    
                    total_fps = sum(results[f'{stream}_avg_fps'] for stream in streams)
                    combined_avg = total_fps / len(streams)
                    resolutions[res_key].append(combined_avg)
            
            for res, fps_list in sorted(resolutions.items()):
                avg_fps = sum(fps_list) / len(fps_list)
                max_fps = max(fps_list)
                min_fps = min(fps_list)
                print(f"  {res}: Avg {avg_fps:.1f} FPS, Max {max_fps:.1f} FPS, Min {min_fps:.1f} FPS")
        
        # Print failed test details
        if failed_tests > 0:
            print("\nFailed Test Details:")
            for config_key, results in sorted_results:
                if results.get('status') != 'PASSED':
                    error_msg = results.get('error', 'Unknown error')
                    print(f"  {config_key}: {error_msg}")

    def run_quick_test(self, stream_combo: str, test_duration: int = 5) -> Dict[str, Dict[str, float]]:
        """Run a quick test with common resolutions only"""
        
        if stream_combo not in self.stream_combinations:
            print(f"ERROR: Invalid stream combination '{stream_combo}'. Valid options: {list(self.stream_combinations.keys())}")
            return {}
            
        streams = self.stream_combinations[stream_combo]
        stream_names = [s.upper() if s != 'infrared' else 'IR1' for s in streams]
        stream_display = ' + '.join(stream_names)
        
        common_resolutions = [
            (640, 480, 30),   # VGA
            (640, 480, 60),   # VGA High FPS
            (1280, 720, 15),  # HD 15FPS
            (1280, 720, 30),  # HD 30FPS
        ]
        
        print("Intel RealSense Quick FPS Test")
        print(f"Testing {stream_display} streams with common resolutions only")
        print(f"Test duration: {test_duration} seconds per configuration")
        
        # Get supported configurations
        supported_configs = self.find_common_configurations(streams)
        
        # Filter to only test common resolutions that are supported
        test_configs = []
        for width, height, fps in common_resolutions:
            if (width, height, fps) in supported_configs:
                test_configs.append((width, height, fps))
            else:
                print(f"Skipping {width}x{height}@{fps} - not supported")
        
        if not test_configs:
            print("ERROR: No common test configurations are supported")
            return {}
        
        all_results = {}
        
        for i, (width, height, fps) in enumerate(test_configs):
            config_key = f"{width}x{height}@{fps}fps"
            print(f"\n[{i+1}/{len(test_configs)}] Testing {config_key}")
            
            try:
                # Perform hardware reset before each test
                if i > 0:  # Skip reset for first test
                    print(f"Performing hardware reset before test {i+1}...")
                    if not self.hardware_reset():
                        print(f"WARNING: Hardware reset failed before {config_key}")
                
                results = self.test_fps_performance(streams, width, height, fps, test_duration)
                all_results[config_key] = results  # Always store results, even if failed
                    
                # Brief pause between tests
                time.sleep(1)
                
            except KeyboardInterrupt:
                print("\nTest interrupted by user")
                break
            except Exception as e:
                print(f"ERROR in test for {config_key}: {e}")
                # Store failed test result
                all_results[config_key] = {
                    'status': 'FAILED',
                    'error': f"Exception during test: {e}",
                    'total_frames': 0,
                    'test_duration': test_duration,
                    'width': width,
                    'height': height,
                    'target_fps': fps,
                    'streams': streams
                }
                # Add zero FPS for each stream
                for stream in streams:
                    all_results[config_key][f'{stream}_avg_fps'] = 0.0
                    all_results[config_key][f'{stream}_current_fps'] = 0.0
                continue
                
        return all_results


def main():
    """Main function"""
    
    import argparse
    
    parser = argparse.ArgumentParser(description='Intel RealSense FPS Performance Test')
    parser.add_argument('--duration', type=int, default=10, help='Test duration in seconds (default: 10)')
    parser.add_argument('--max-tests', type=int, help='Maximum number of configurations to test')
    parser.add_argument('--quick', action='store_true', help='Run quick test with common resolutions only')
    parser.add_argument('--list-only', action='store_true', help='List supported resolutions only')
    parser.add_argument('--streams', choices=['color_depth', 'color_ir', 'ir_depth', 'color_only'], 
                        default='color_depth', help='Stream combination to test (default: color_depth)')
    
    args = parser.parse_args()
    
    # Create test instance
    fps_test = RealSenseFPSTest()
    
    try:
        # Initialize camera
        if not fps_test.initialize_camera():
            print("Failed to initialize camera. Exiting...")
            return 1
            
        # List supported resolutions only
        if args.list_only:
            streams = fps_test.stream_combinations[args.streams]
            fps_test.print_supported_resolutions(streams)
            return 0
            
        # Run tests
        if args.quick:
            results = fps_test.run_quick_test(args.streams, test_duration=args.duration)
        else:
            results = fps_test.run_all_tests(args.streams, test_duration=args.duration, max_tests=args.max_tests)
        
        if results:
            fps_test.print_summary(results, args.streams)
            
            # Save results to file
            import json
            timestamp = time.strftime("%Y%m%d_%H%M%S")
            filename = f"fps_test_results_{args.streams}_{timestamp}.json"
            with open(filename, 'w') as f:
                json.dump(results, f, indent=2)
            print(f"\nResults saved to: {filename}")
        else:
            print("No test results available")
            return 1
            
    except KeyboardInterrupt:
        print("\nTest interrupted by user")
        return 1
    except Exception as e:
        print(f"Unexpected error: {e}")
        return 1
    finally:
        fps_test.cleanup()
        
    return 0


if __name__ == "__main__":
    sys.exit(main())
