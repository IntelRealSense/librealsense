import pyrealsense2
from typing import overload, List, Tuple, Callable
import datetime

"""
Library for accessing Intel RealSenseTM cameras
"""


def adjust_2D_point_to_boundary():
    ...


def is_pixel_in_line(curr: List[float[2]]):
    ...


def log_to_console():
    ...


def log_to_file():
    ...


def next_pixel_in_line(curr: List[float[2]]):
    ...


def rs2_deproject_pixel_to_point():
    'Given pixel coordinates and depth in an image with no distortion or inverse distortion coefficients, compute the corresponding point in 3D space relative to the same camera'
    ...


def rs2_fov():
    'Calculate horizontal and vertical field of view, based on video intrinsics'
    ...


def rs2_project_color_pixel_to_depth_pixel():
    ...


def rs2_project_point_to_pixel():
    'Given a point in 3D space, compute the corresponding pixel coordinates in an image with no distortion or forward distortion coefficients produced by the same camera'
    ...


def rs2_transform_point_to_point():
    'Transform 3D coordinates relative to one sensor to 3D coordinates relative to another viewpoint'
    ...


class BufData:
    def __init__():
        'Initialize self. See help(type(self)) for accurate signature.'
        ...


class STAEControl:
    meanIntensitySetPoint = ...

    def __init__(self: pyrealsense2.STAEControl) -> None:
        ...


class STAFactor:
    a_factor = ...

    def __init__(self: pyrealsense2.STAFactor) -> None:
        ...


class STCensusRadius:
    uDiameter = ...
    vDiameter = ...

    def __init__(self: pyrealsense2.STCensusRadius) -> None:
        ...


class STColorControl:
    disableRAUColor = ...
    disableSADColor = ...
    disableSADNormalize = ...
    disableSLOLeftColor = ...
    disableSLORightColor = ...

    def __init__(self: pyrealsense2.STColorControl) -> None:
        ...


class STColorCorrection:
    colorCorrection1 = ...
    colorCorrection2 = ...
    colorCorrection3 = ...
    colorCorrection4 = ...
    colorCorrection5 = ...
    colorCorrection6 = ...
    colorCorrection7 = ...
    colorCorrection8 = ...
    colorCorrection9 = ...
    colorCorrection10 = ...
    colorCorrection11 = ...
    colorCorrection12 = ...

    def __init__(self: pyrealsense2.STColorCorrection) -> None:
        ...


class STDepthControlGroup:
    deepSeaMedianThreshold = ...
    deepSeaNeighborThreshold = ...
    deepSeaSecondPeakThreshold = ...
    lrAgreeThreshold = ...
    minusDecrement = ...
    plusIncrement = ...
    scoreThreshA = ...
    scoreThreshB = ...
    textureCountThreshold = ...
    textureDifferenceThreshold = ...

    def __init__(self: pyrealsense2.STDepthControlGroup) -> None:
        ...


class STDepthTableControl:
    depthClampMax = ...
    depthClampMin = ...
    depthUnits = ...
    disparityMode = ...
    disparityShift = ...

    def __init__(self: pyrealsense2.STDepthTableControl) -> None:
        ...


class STHdad:
    ignoreSAD = ...
    lambdaAD = ...
    lambdaCensus = ...

    def __init__(self: pyrealsense2.STHdad) -> None:
        ...


class STRauColorThresholdsControl:
    rauDiffThresholdBlue = ...
    rauDiffThresholdGreen = ...
    rauDiffThresholdRed = ...

    def __init__(self: pyrealsense2.STRauColorThresholdsControl) -> None:
        ...


class STRauSupportVectorControl:
    minEast = ...
    minNSsum = ...
    minNorth = ...
    minSouth = ...
    minWEsum = ...
    minWest = ...
    uShrink = ...
    vShrink = ...

    def __init__(self: pyrealsense2.STRauSupportVectorControl) -> None:
        ...


class STRsm:
    diffThresh = ...
    removeThresh = ...
    rsmBypass = ...
    sloRauDiffThresh = ...

    def __init__(self: pyrealsense2.STRsm) -> None:
        ...


class STSloColorThresholdsControl:
    diffThresholdBlue = ...
    diffThresholdGreen = ...
    diffThresholdRed = ...

    def __init__(self: pyrealsense2.STSloColorThresholdsControl) -> None:
        ...


class STSloPenaltyControl:
    sloK1Penalty = ...
    sloK1PenaltyMod1 = ...
    sloK1PenaltyMod2 = ...
    sloK2Penalty = ...
    sloK2PenaltyMod1 = ...
    sloK2PenaltyMod2 = ...

    def __init__(self: pyrealsense2.STSloPenaltyControl) -> None:
        ...


class align:
    """
    Performs alignment between depth image and another image.
    """

    def __init__(self: pyrealsense2.align, align_to: pyrealsense2.stream) -> None:
        'To perform alignment of a depth image to the other, set the align_to parameter with the other stream type. To perform alignment of a non depth image to a depth image, set the align_to parameter to RS2_STREAM_DEPTH. Camera calibration and frame\'s stream type are determined on the fly, according to the first valid frameset passed to process().'
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process(self: pyrealsense2.align) -> pyrealsense2.composite_frame:
        'Run the alignment process on the given frames to get an aligned set of frames'
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class auto_calibrated_device:
    sensors = ...   # List of adjacent devices, sharing the same physical parent composite device.

    def __init__(self: pyrealsense2.auto_calibrated_device, device: pyrealsense2.device) -> None:
        ...

    def as_auto_calibrated_device():
        ...

    def as_debug_protocol():
        ...

    def as_playback():
        ...

    def as_recorder():
        ...

    def as_tm2(self: pyrealsense2.device) -> pyrealsense2.tm2:
        ...

    def as_updatable():
        ...

    def as_update_device():
        ...

    def first_color_sensor():
        ...

    def first_depth_sensor():
        ...

    def first_fisheye_sensor():
        ...

    def first_motion_sensor():
        ...

    def first_pose_sensor():
        ...

    def first_roi_sensor():
        ...

    def get_calibration_table(self: pyrealsense2.auto_calibrated_device) -> list[int]:
        'Read current calibration table from flash.'
        ...

    def get_info(self: pyrealsense2.device):
        'Retrieve camera specific information, like versions of various internal components'
        ...

    def hardware_reset():
        'Send hardware reset request to the device'
        ...

    def is_auto_calibrated_device():
        ...

    def is_debug_protocol():
        ...

    def is_playback(self: pyrealsense2.device) -> bool:
        ...

    def is_recorder(self: pyrealsense2.device) -> bool:
        ...

    def is_tm2(self: pyrealsense2.device) -> bool:
        ...

    def is_updatable(self: pyrealsense2.device) -> bool:
        ...

    def is_update_device():
        ...

    def query_sensors():
        'Returns the list of adjacent devices, sharing the same physical parent composite device.'
        ...

    def reset_to_factory_calibration(self: pyrealsense2.auto_calibrated_device) -> None:
        'Reset device to factory calibration.'
        ...

    @overload
    def run_on_chip_calibration(self: pyrealsense2.auto_calibrated_device, json_content: str, timeout_ms: int) -> tuple:
        'This will improve the depth noise (plane fit RMS). This call is executed on the caller\'s thread.'
        ...

    @overload
    def run_on_chip_calibration(self: pyrealsense2.auto_calibrated_device, json_content: str, callback: Callable[[float], None], timeout_ms: int) -> tuple:
        'This will improve the depth noise (plane fit RMS). This call is executed on the caller\'s thread and it supports progress notifications via the callback.'
        ...

    @overload
    def run_tare_calibration(self: pyrealsense2.auto_calibrated_device, ground_truth_mm: float, json_content: str, timeout_ms: int) -> List[int]:
        'This will adjust camera absolute distance to flat target. This call is executed on the caller\'s thread and it supports progress notifications via the callback.'
        ...

    @overload
    def run_tare_calibration(self: pyrealsense2.auto_calibrated_device, ground_truth_mm: float, json_content: str, callback: Callable[[float], None], timeout_ms: int) -> List[int]:
        'This will adjust camera absolute distance to flat target. This call is executed on the caller\'s thread.'
        ...

    def set_calibration_table(self: pyrealsense2.auto_calibrated_device, arg0: List[int]) -> None:
        'Set current table to dynamic area.'
        ...

    def supports(self: pyrealsense2.device):
        'Check if specific camera info is supported.'
        ...

    def write_calibration(self: pyrealsense2.auto_calibrated_device) -> None:
        'Write calibration that was set by set_calibration_table to device\'s EEPROM.'
        ...


class camera_info:
    """
    This information is mainly available for camera debug and troubleshooting and should not be used in applications.
    """
    advanced_mode = camera_info.advanced_mode
    asic_serial_number = camera_info.asic_serial_number
    camera_locked = camera_info.camera_locked
    debug_op_code = camera_info.debug_op_code
    firmware_update_id = camera_info.firmware_update_id
    firmware_version = camera_info.firmware_version
    name = camera_info.name
    physical_port = camera_info.physical_port
    product_id = camera_info.product_id
    product_line = camera_info.product_line
    recommended_firmware_version = camera_info.recommended_firmware_version
    serial_number = camera_info.serial_number
    usb_type_descriptor = camera_info.usb_type_descriptor

    def __init__(self: pyrealsense2.camera_info) -> None:
        ...


class color_sensor:
    profiles = ...  # The list of stream profiles supported by the sensor.

    def __init__(self: pyrealsense2.color_sensor) -> None:
        ...

    def as_color_sensor():
        ...

    def as_depth_sensor():
        ...

    def as_fisheye_sensor():
        ...

    def as_motion_sensor():
        ...

    def as_pose_sensor():
        ...

    def as_roi_sensor():
        ...

    def as_wheel_odometer():
        ...

    def close(self: pyrealsense2.sensor) -> None:
        'Close sensor for exclusive access.'
        ...

    def get_info(self: pyrealsense2.sensor):
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning'
        ...

    def get_recommended_filters():
        'Return the recommended list of filters by the sensor.'
        ...

    def get_stream_profiles():
        'Retrieves the list of stream profiles supported by the sensor.'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def is_color_sensor():
        ...

    def is_depth_sensor():
        ...

    def is_fisheye_sensor():
        ...

    def is_motion_sensor():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_pose_sensor():
        ...

    def is_roi_sensor():
        ...

    def is_wheel_odometer():
        ...

    def open(*args, **kwargs):
        'Overloaded function.'
        ...

    def set_notifications_callback():
        'Register Notifications callback'
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(*args, **kwargs):
        'Overloaded function.'
        ...

    def stop(self: pyrealsense2.sensor) -> None:
        'Stop streaming.'
        ...

    def supports(*args, **kwargs):
        'Overloaded function.'
        ...


class colorizer:
    """
    Colorizer filter generates color images based on input depth frame
    """
    @overload
    def __init__(self: pyrealsense2.colorizer) -> None:
        ...

    @overload
    def __init__(self: pyrealsense2.colorizer, color_scheme: float) -> None:
        """
        Possible values for color_scheme:   \n
        0 - Jet                             \n
        1 - Classic                         \n
        2 - WhiteToBlack                    \n
        3 - BlackToWhite                    \n
        4 - Bio                             \n
        5 - Cold                            \n
        6 - Warm                            \n
        7 - Quantized                       \n
        8 - Pattern
        """
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def colorize(self: pyrealsense2.colorizer, depth: pyrealsense2.frame) -> pyrealsense2.video_frame:
        'Start to generate color image base on depth frame'
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class composite_frame:
    """
    Extends the frame class with additional frameset related attributes and functions
    """
    data = ...	                    # Data from the frame handle.
    frame_number = ...	            # The frame number.
    frame_timestamp_domain = ...    # The timestamp domain.
    profile = ...	                # Stream profile from frame handle.
    timestamp = ...	                # Time at which the frame was captured.

    def __init__(self: pyrealsense2.composite_frame, arg0: pyrealsense2.frame) -> None:
        ...

    def as_depth_frame():
        ...

    def as_frame():
        ...

    def as_frameset():
        ...

    def as_motion_frame():
        ...

    def as_points():
        ...

    def as_pose_frame():
        ...

    def as_video_frame():
        ...

    def first(self: pyrealsense2.composite_frame, s: pyrealsense2.stream, f: pyrealsense2.format = format.any) -> pyrealsense2.frame:
        'Retrieve the first frame of a specific stream type and optionally with a specific format.'
        ...

    def first_or_default(self: pyrealsense2.composite_frame, s: pyrealsense2.stream, f: pyrealsense2.format = format.any) -> pyrealsense2.frame:
        'Retrieve the first frame of a specific stream and optionally with a specific format.'
        ...

    def foreach(self: pyrealsense2.composite_frame, callable: Callable[[pyrealsense2.frame], None]):
        'Extract internal frame handles from the frameset and invoke the action function'
        ...

    def get_color_frame(self: pyrealsense2.composite_frame) -> pyrealsense2.video_frame:
        'Retrieve the first color frame, if no frame is found, search for the color frame from IR stream.'
        ...

    def get_data():
        'Retrieve data from the frame handle.'
        ...

    def get_data_size(self: pyrealsense2.frame) -> int:
        'Retrieve data size from frame handle.'
        ...

    def get_depth_frame(self: pyrealsense2.composite_frame) -> pyrealsense2.depth_frame:
        'Retrieve the first depth frame, if no frame is found, return an empty frame instance.'
        ...

    def get_fisheye_frame(self: pyrealsense2.composite_frame, index: int = 0) -> pyrealsense2.video_frame:
        'Retrieve the fisheye monochrome video frame'
        ...

    def get_frame_metadata():
        'Retrieve the current value of a single frame_metadata.'
        ...

    def get_frame_number():
        'Retrieve the frame number.'
        ...

    def get_frame_timestamp_domain():
        'Retrieve the timestamp domain.'
        ...

    def get_infrared_frame(self: pyrealsense2.composite_frame, index: int = 0) -> pyrealsense2.video_frame:
        'Retrieve the first infrared frame, if no frame is found, return an empty frame instance.'
        ...

    def get_pose_frame(self: pyrealsense2.composite_frame, index: int = 0) -> pyrealsense2.pose_frame:
        'Retrieve the pose frame'
        ...

    def get_profile():
        'Retrieve stream profile from frame handle.'
        ...

    def get_timestamp():
        'Retrieve the time at which the frame was captured'
        ...

    def is_depth_frame():
        ...

    # TODO: Why 'frame', not 'composite_frame'?
    def is_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_frameset(self: pyrealsense2.frame) -> bool:
        ...

    def is_motion_frame():
        ...

    def is_points(self: pyrealsense2.frame) -> bool:
        ...

    def is_pose_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_video_frame():
        ...

    def keep(self: pyrealsense2.frame) -> None:
        'Keep the frame, otherwise if no refernce to the frame, the frame will be released.'
        ...

    def size(self: pyrealsense2.composite_frame) -> int:
        'Return the size of the frameset'
        ...

    def supports_frame_metadata():
        'Determine if the device allows a specific metadata to be queried.'
        ...

    def swap(self: pyrealsense2.frame):
        'Swap the internal frame handle with the one in parameter'
        ...


class config:
    """
    The config allows pipeline users to request filters for the pipeline streams and device selection and configuration. This is an optional step in pipeline creation, as the pipeline resolves its streaming device internally. Config provides its users a way to set the filters and test if there is no conflict with the pipeline requirements from the device. It also allows the user to find a matching device for the config filters and the pipeline, in order to select a device explicitly, and modify its controls before streaming starts.
    """

    def __init__(self: pyrealsense2.config) -> None:
        ...

    def can_resolve(self: pyrealsense2.config, p: pyrealsense2.pipeline_wrapper) -> bool:
        'Check if the config can resolve the configuration filters, to find a matching device and streams profiles. The resolution conditions are as described in resolve().'
        ...

    def disable_all_streams(self: pyrealsense2.config) -> None:
        'Disable all device stream explicitly, to remove any requests on the streams profiles. The streams can still be enabled due to pipeline computer vision module request. This call removes any filter on the streams configuration.'
        ...

    def disable_stream(self: pyrealsense2.config, stream: pyrealsense2.stream, index: int = -1) -> None:
        'Disable a device stream explicitly, to remove any requests on this stream profile. The stream can still be enabled due to pipeline computer vision module request. This call removes any filter on the stream configuration.'
        ...

    def enable_all_streams(self: pyrealsense2.config) -> None:
        'Enable all device streams explicitly. The conditions and behavior of this method are similar to those of enable_stream(). This filter enables all raw streams of the selected device. The device is either selected explicitly by the application, or by the pipeline requirements or default. The list of streams is device dependent.'
        ...

    def enable_device(self: pyrealsense2.config, serial: str) -> None:
        'Select a specific device explicitly by its serial number, to be used by the pipeline. The conditions and behavior of this method are similar to those of enable_stream(). This method is required if the application needs to set device or sensor settings prior to pipeline streaming, to enforce the pipeline to use the configured device.'
        ...

    def enable_device_from_file(self: pyrealsense2.config, file_name: str, repeat_playback: bool = True) -> None:
        'Select a recorded device from a file, to be used by the pipeline through playback. The device available streams are as recorded to the file, and resolve() considers only this device and configuration as available. This request cannot be used if enable_record_to_file() is called for the current config, and vice versa.'
        ...

    def enable_record_to_file(self: pyrealsense2.config, file_name: str) -> None:
        'Requires that the resolved device would be recorded to file. This request cannot be used if enable_device_from_file() is called for the current config, and vice versa as available.'
        ...

    @overload
    def enable_stream(self: pyrealsense2.config, stream_type: pyrealsense2.stream, stream_index: int, width: int, height: int, format: pyrealsense2.format = format.any, framerate: int = 0) -> None:
        'Enable a device stream explicitly, with selected stream parameters. The method allows the application to request a stream with specific configuration. If no stream is explicitly enabled, the pipeline configures the device and its streams according to the attached computer vision modules and processing blocks requirements, or default configuration for the first available device. The application can configure any of the input stream parameters according to its requirement, or set to 0 for don\'t care value. The config accumulates the application calls for enable configuration methods, until the configuration is applied. Multiple enable stream calls for the same stream override each other, and the last call is maintained. Upon calling resolve(), the config checks for conflicts between the application configuration requests and the attached computer vision modules and processing blocks requirements, and fails if conflicts are found. Before resolve() is called, no conflict check is done.'
        ...

    @overload
    def enable_stream(self: pyrealsense2.config, stream_type: pyrealsense2.stream, stream_index: int = -1) -> None:
        'Stream type and possibly also stream index. Other parameters are resolved internally.'
        ...

    @overload
    def enable_stream(self: pyrealsense2.config, stream_type: pyrealsense2.stream, format: pyrealsense2.format, framerate: int = 0) -> None:
        'Stream type and format, and possibly frame rate. Other parameters are resolved internally.'
        ...

    @overload
    def enable_stream(self: pyrealsense2.config, stream_type: pyrealsense2.stream, width: int, height: int, format: pyrealsense2.format = format.any, framerate: int = 0) -> None:
        'Stream type and resolution, and possibly format and frame rate. Other parameters are resolved internally.'
        ...

    @overload
    def enable_stream(self: pyrealsense2.config, stream_type: pyrealsense2.stream, stream_index: int, format: pyrealsense2.format, framerate: int = 0) -> None:
        'Stream type, index, and format, and possibly framerate. Other parameters are resolved internally.'
        ...

    def resolve(self: pyrealsense2.config, p: pyrealsense2.pipeline_wrapper) -> pyrealsense2.pipeline_profile:
        'Resolve the configuration filters, to find a matching device and streams profiles. The method resolves the user configuration filters for the device and streams, and combines them with the requirements of the computer vision modules and processing blocks attached to the pipeline. If there are no conflicts of requests, it looks for an available device, which can satisfy all requests, and selects the first matching streams configuration. In the absence of any request, the config object selects the first available device and the first color and depth streams configuration.The pipeline profile selection during start() follows the same method. Thus, the selected profile is the same, if no change occurs to the available devices.Resolving the pipeline configuration provides the application access to the pipeline selected device for advanced control.The returned configuration is not applied to the device, so the application doesn\'t own the device sensors. However, the application can call enable_device(), to enforce the device returned by this method is selected by pipeline start(), and configure the device and sensors options or extensions before streaming starts.'
        ...


class context:
    """
    Librealsense context class. Includes realsense API version.
    """
    devices = ...   # A static snapshot of all connected devices at time of access. Identical to calling query_devices.
    sensors = ...   # A flat list of all available sensors from all RealSense devices. Identical to calling query_all_sensors.

    def __init__(self: pyrealsense2.context) -> None:
        ...

    def get_sensor_parent(self: pyrealsense2.context, s: pyrealsense2.sensor) -> pyrealsense2.device:
        ...

    def load_device(self: pyrealsense2.context, filename: str) -> pyrealsense2.playback:
        'Creates a devices from a RealSense file. On successful load, the device will be appended to the context and a devices_changed event triggered.'
        ...

    def query_all_sensors(self: pyrealsense2.context) -> List[pyrealsense2.sensor]:
        'Generate a flat list of all available sensors from all RealSense devices.'
        ...

    def query_devices(self: pyrealsense2.context) -> pyrealsense2.device_list:
        'Create a static snapshot of all connected devices at the time of the call.'
        ...

    def set_devices_changed_callback(self: pyrealsense2.context, callback: Callable[[pyrealsense2.event_information], None]) -> None:
        'Register devices changed callback.'
        ...

    def unload_device(self: pyrealsense2.context, filename: str) -> None:
        ...

    def unload_tracking_module(self: pyrealsense2.context) -> None:
        ...


class debug_protocol:
    def __init__(self: pyrealsense2.debug_protocol, arg0: pyrealsense2.device) -> None:
        ...

    def send_and_receive_raw_data(self: pyrealsense2.debug_protocol, input: List[int]) -> List[int]:
        ...


class decimation_filter:
    """
    Performs downsampling by using the median with specific kernel size.
    """
    @overload
    def __init__(self: pyrealsense2.decimation_filter) -> None:
        ...

    @overload
    def __init__(self: pyrealsense2.decimation_filter, magnitude: float) -> None:
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class depth_frame:
    """
    Extends the video_frame class with additional depth related attributes and functions.
    """
    bits_per_pixel = ...	        # Bits per pixel.
    bytes_per_pixel = ...	        # Bytes per pixel.
    data = ...	                    # Data from the frame handle.
    frame_number = ...	            # The frame number.
    frame_timestamp_domain = ...    # The timestamp domain.
    height = ...	                # Image height in pixels.
    profile = ...	                # Stream profile from frame handle.
    # Frame stride, meaning the actual line width in memory in bytes (not the logical image width).
    stride_in_bytes = ...
    timestamp = ...	                # Time at which the frame was captured.
    width = ...                     # Image width in pixels.

    def __init__(self: pyrealsense2.depth_frame, arg0: pyrealsense2.frame) -> None:
        ...

    def as_depth_frame():
        ...

    def as_frame():
        ...

    def as_frameset():
        ...

    def as_motion_frame():
        ...

    def as_points():
        ...

    def as_pose_frame():
        ...

    def as_video_frame():
        ...

    def get_bits_per_pixel():
        'Retrieve bits per pixel.'
        ...

    def get_bytes_per_pixel():
        'Retrieve bytes per pixel.'
        ...

    def get_data():
        'Retrieve data from the frame handle.'
        ...

    def get_data_size(self: pyrealsense2.frame) -> int:
        'Retrieve data size from frame handle.'
        ...

    def get_distance(self: pyrealsense2.depth_frame, x: int, y: int) -> float:
        'Provide the depth in meters at the given pixel'
        ...

    def get_frame_metadata():
        'Retrieve the current value of a single frame_metadata.'
        ...

    def get_frame_number():
        'Retrieve the frame number.'
        ...

    def get_frame_timestamp_domain():
        'Retrieve the timestamp domain.'
        ...

    def get_height():
        'Returns image height in pixels.'
        ...

    def get_profile():
        'Retrieve stream profile from frame handle.'
        ...

    def get_stride_in_bytes():
        'Retrieve frame stride, meaning the actual line width in memory in bytes (not the logical image width).'
        ...

    def get_timestamp():
        'Retrieve the time at which the frame was captured'
        ...

    def get_width():
        'Returns image width in pixels.'
        ...

    def is_depth_frame():
        ...

    def is_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_frameset(self: pyrealsense2.frame) -> bool:
        ...

    def is_motion_frame():
        ...

    def is_points(self: pyrealsense2.frame) -> bool:
        ...

    def is_pose_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_video_frame():
        ...

    def keep(self: pyrealsense2.frame) -> None:
        'Keep the frame, otherwise if no refernce to the frame, the frame will be released.'
        ...

    def supports_frame_metadata():
        'Determine if the device allows a specific metadata to be queried.'
        ...

    def swap(self: pyrealsense2.frame):
        'Swap the internal frame handle with the one in parameter'
        ...


class depth_huffman_decoder:
    """
    Decompresses Huffman-encoded Depth frame to standartized Z16 format
    """

    def __init__(self: pyrealsense2.depth_huffman_decoder) -> None:
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class depth_sensor:
    profiles = ...      # The list of stream profiles supported by the sensor.

    def __init__(self: pyrealsense2.depth_sensor, sensor: pyrealsense2.sensor) -> None:
        ...

    def as_color_sensor():
        ...

    def as_depth_sensor():
        ...

    def as_fisheye_sensor():
        ...

    def as_motion_sensor():
        ...

    def as_pose_sensor():
        ...

    def as_roi_sensor():
        ...

    def as_wheel_odometer():
        ...

    def close(self: pyrealsense2.sensor) -> None:
        'Close sensor for exclusive access.'
        ...

    def get_depth_scale(self: pyrealsense2.depth_sensor) -> float:
        'Retrieves mapping between the units of the depth image and meters.'
        ...

    def get_info(self: pyrealsense2.sensor):
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_recommended_filters():
        'Return the recommended list of filters by the sensor.'
        ...

    def get_stream_profiles():
        'Retrieves the list of stream profiles supported by the sensor.'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def is_color_sensor():
        ...

    def is_depth_sensor():
        ...

    def is_fisheye_sensor():
        ...

    def is_motion_sensor():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_pose_sensor():
        ...

    def is_roi_sensor():
        ...

    def is_wheel_odometer():
        ...

    def open(*args, **kwargs):
        'Overloaded function.'
        ...

    def set_notifications_callback():
        'Register Notifications callback'
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(*args, **kwargs):
        'Overloaded function.'
        ...

    def stop(self: pyrealsense2.sensor) -> None:
        'Stop streaming.'
        ...

    def supports(*args, **kwargs):
        'Overloaded function.'
        ...


class depth_stereo_sensor:
    profiles = ...      # The list of stream profiles supported by the sensor.

    def __init__(self: pyrealsense2.depth_stereo_sensor, arg0: pyrealsense2.sensor) -> None:
        ...

    def as_color_sensor():
        ...

    def as_depth_sensor():
        ...

    def as_fisheye_sensor():
        ...

    def as_motion_sensor():
        ...

    def as_pose_sensor():
        ...

    def as_roi_sensor():
        ...

    def as_wheel_odometer():
        ...

    def close(self: pyrealsense2.sensor) -> None:
        'Close sensor for exclusive access.'
        ...

    def get_depth_scale():
        'Retrieves mapping between the units of the depth image and meters.'
        ...

    def get_info(self: pyrealsense2.sensor):
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_recommended_filters():
        'Return the recommended list of filters by the sensor.'
        ...

    def get_stereo_baseline(self: pyrealsense2.depth_stereo_sensor) -> float:
        'Retrieve the stereoscopic baseline value from the sensor.'
        ...

    def get_stream_profiles():
        'Retrieves the list of stream profiles supported by the sensor.'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def is_color_sensor():
        ...

    def is_depth_sensor():
        ...

    def is_fisheye_sensor():
        ...

    def is_motion_sensor():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_pose_sensor():
        ...

    def is_roi_sensor():
        ...

    def is_wheel_odometer():
        ...

    def open(*args, **kwargs):
        'Overloaded function.'
        ...

    def set_notifications_callback():
        'Register Notifications callback'
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(*args, **kwargs):
        'Overloaded function.'
        ...

    def stop(self: pyrealsense2.sensor) -> None:
        'Stop streaming.'
        ...

    def supports(*args, **kwargs):
        'Overloaded function.'
        ...


class device:
    sensors = ...   # List of adjacent devices, sharing the same physical parent composite device. Identical to calling query_sensors.

    def __init__(self: pyrealsense2.device) -> None:
        ...

    def as_auto_calibrated_device(self: pyrealsense2.device) -> pyrealsense2.auto_calibrated_device:
        ...

    def as_debug_protocol(self: pyrealsense2.device) -> pyrealsense2.debug_protocol:
        ...

    def as_playback(self: pyrealsense2.device) -> pyrealsense2.playback:
        ...

    def as_recorder(self: pyrealsense2.device) -> pyrealsense2.recorder:
        ...

    def as_tm2(self: pyrealsense2.device) -> pyrealsense2.tm2:
        ...

    def as_updatable(self: pyrealsense2.device) -> pyrealsense2.updatable:
        ...

    def as_update_device(self: pyrealsense2.device) -> pyrealsense2.update_device:
        ...

    def first_color_sensor(self: pyrealsense2.device) -> pyrealsense2.color_sensor:
        ...

    def first_depth_sensor(self: pyrealsense2.device) -> pyrealsense2.depth_sensor:
        ...

    def first_fisheye_sensor(self: pyrealsense2.device) -> pyrealsense2.fisheye_sensor:
        ...

    def first_motion_sensor(self: pyrealsense2.device) -> pyrealsense2.motion_sensor:
        ...

    def first_pose_sensor(self: pyrealsense2.device) -> pyrealsense2.pose_sensor:
        ...

    def first_roi_sensor(self: pyrealsense2.device) -> pyrealsense2.roi_sensor:
        ...

    def get_info(self: pyrealsense2.device, info: pyrealsense2.camera_info) -> str:
        'Retrieve camera specific information, like versions of various internal components'
        ...

    def hardware_reset(self: pyrealsense2.device) -> None:
        'Send hardware reset request to the device'
        ...

    def is_auto_calibrated_device(self: pyrealsense2.device) -> bool:
        ...

    def is_debug_protocol(self: pyrealsense2.device) -> bool:
        ...

    def is_playback(self: pyrealsense2.device) -> bool:
        ...

    def is_recorder(self: pyrealsense2.device) -> bool:
        ...

    def is_tm2(self: pyrealsense2.device) -> bool:
        ...

    def is_updatable(self: pyrealsense2.device) -> bool:
        ...

    def is_update_device(self: pyrealsense2.device) -> bool:
        ...

    def query_sensors(self: pyrealsense2.device) -> List[pyrealsense2.sensor]:
        'Returns the list of adjacent devices, sharing the same physical parent composite device.'
        ...

    def supports(self: pyrealsense2.device, info: pyrealsense2.camera_info) -> bool:
        'Check if specific camera info is supported.'
        ...


class device_list:
    def __init__(self: pyrealsense2.device_list) -> None:
        ...

    def back(self: pyrealsense2.device_list) -> pyrealsense2.device:
        ...

    def contains(self: pyrealsense2.device_list, arg0: pyrealsense2.device) -> bool:
        ...

    def front(self: pyrealsense2.device_list) -> pyrealsense2.device:
        ...

    def size(self: pyrealsense2.device_list) -> int:
        ...


class disparity_frame:
    """
    Extends the depth_frame class with additional disparity related attributes and functions.
    """
    bits_per_pixel = ...	        # Bits per pixel.
    bytes_per_pixel = ...	        # Bytes per pixel.
    data = ...	                    # Data from the frame handle.
    frame_number = ...	            # The frame number.
    frame_timestamp_domain = ...    # The timestamp domain.
    height = ...	                # Image height in pixels.
    profile = ...                   # Stream profile from frame handle.
    # Frame stride, meaning the actual line width in memory in bytes (not the logical image width).
    stride_in_bytes = ...
    timestamp = ...	                # Time at which the frame was captured.
    width = ...	                    # Image width in pixels.

    def __init__(self: pyrealsense2.disparity_frame, arg0: pyrealsense2.frame) -> None:
        ...

    def as_depth_frame():
        ...

    def as_frame():
        ...

    def as_frameset():
        ...

    def as_motion_frame():
        ...

    def as_points():
        ...

    def as_pose_frame():
        ...

    def as_video_frame():
        ...

    def get_baseline(self: pyrealsense2.disparity_frame) -> float:
        'Retrieve the distance between the two IR sensors.'
        ...

    def get_bits_per_pixel():
        'Retrieve bits per pixel.'
        ...

    def get_bytes_per_pixel():
        'Retrieve bytes per pixel.'
        ...

    def get_data():
        'Retrieve data from the frame handle.'
        ...

    def get_data_size(self: pyrealsense2.frame) -> int:
        'Retrieve data size from frame handle.'
        ...

    def get_distance():
        'Provide the depth in meters at the given pixel'
        ...

    def get_frame_metadata():
        'Retrieve the current value of a single frame_metadata.'
        ...

    def get_frame_number():
        'Retrieve the frame number.'
        ...

    def get_frame_timestamp_domain():
        'Retrieve the timestamp domain.'
        ...

    def get_height():
        'Returns image height in pixels.'
        ...

    def get_profile():
        'Retrieve stream profile from frame handle.'
        ...

    def get_stride_in_bytes():
        'Retrieve frame stride, meaning the actual line width in memory in bytes (not the logical image width).'
        ...

    def get_timestamp():
        'Retrieve the time at which the frame was captured'
        ...

    def get_width():
        'Returns image width in pixels.'
        ...

    def is_depth_frame():
        ...

    def is_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_frameset(self: pyrealsense2.frame) -> bool:
        ...

    def is_motion_frame():
        ...

    def is_points(self: pyrealsense2.frame) -> bool:
        ...

    def is_pose_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_video_frame():
        ...

    def keep(self: pyrealsense2.frame) -> None:
        'Keep the frame, otherwise if no refernce to the frame, the frame will be released.'
        ...

    def supports_frame_metadata():
        'Determine if the device allows a specific metadata to be queried.'
        ...

    def swap(self: pyrealsense2.frame):
        'Swap the internal frame handle with the one in parameter'
        ...


class disparity_transform:
    """
    Converts from depth representation to disparity representation and vice - versa in depth frames
    """

    def __init__(self: pyrealsense2.disparity_transform, transform_to_disparity: bool = True) -> None:
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class distortion:
    """
    Distortion model: defines how pixel coordinates should be mapped to sensor coordinates.
    """
    brown_conrady = distortion.brown_conrady
    ftheta = distortion.ftheta
    inverse_brown_conrady = distortion.inverse_brown_conrady
    kannala_brandt4 = distortion.kannala_brandt4
    modified_brown_conrady = distortion.modified_brown_conrady
    none = distortion.none

    def __init__(self: pyrealsense2.distortion, arg0: int) -> None:
        ...


class event_information:
    def __init__(self) -> None:
        'Initialize self. See help(type(self)) for accurate signature.'
        ...

    def get_new_devices(self: pyrealsense2.event_information) -> pyrealsense2.device_list:
        'Returns a list of all newly connected devices'
        ...

    def was_added(self: pyrealsense2.event_information, dev: pyrealsense2.device) -> bool:
        'Check if a specific device was added.'
        ...

    def was_removed(self: pyrealsense2.event_information, dev: pyrealsense2.device) -> bool:
        'Check if a specific device was disconnected.'
        ...


class extension:
    """
    Specifies advanced interfaces (capabilities) objects may implement.
    """
    advanced_mode = extension.advanced_mode
    auto_calibrated_device = extension.auto_calibrated_device
    color_sensor = extension.color_sensor
    composite_frame = extension.composite_frame
    debug = extension.debug
    decimation_filter = extension.decimation_filter
    depth_frame = extension.depth_frame
    depth_huffman_decoder = extension.depth_huffman_decoder
    depth_sensor = extension.depth_sensor
    depth_stereo_sensor = extension.depth_stereo_sensor
    disparity_filter = extension.disparity_filter
    disparity_frame = extension.disparity_frame
    fisheye_sensor = extension.fisheye_sensor
    global_timer = extension.global_timer
    hole_filling_filter = extension.hole_filling_filter
    info = extension.info
    l500_depth_sensor = extension.l500_depth_sensor
    motion = extension.motion
    motion_frame = extension.motion_frame
    motion_profile = extension.motion_profile
    motion_sensor = extension.motion_sensor
    options = extension.options
    playback = extension.playback
    points = extension.points
    pose = extension.pose
    pose_frame = extension.pose_frame
    pose_profile = extension.pose_profile
    pose_sensor = extension.pose_sensor
    recommended_filters = extension.recommended_filters
    record = extension.record
    roi = extension.roi
    serializable = extension.serializable
    software_device = extension.software_device
    software_sensor = extension.software_sensor
    spatial_filter = extension.spatial_filter
    temporal_filter = extension.temporal_filter
    threshold_filter = extension.threshold_filter
    tm2 = extension.tm2
    tm2_sensor = extension.tm2_sensor
    unknown = extension.unknown
    updatable = extension.updatable
    update_device = extension.update_device
    video = extension.video
    video_frame = extension.video_frame
    video_profile = extension.video_profile
    wheel_odometer = extension.wheel_odometer
    zero_order_filter = extension.zero_order_filter

    def __init__(self: pyrealsense2.extension, arg0: int) -> None:
        ...


class extrinsics:
    """
    Cross-stream extrinsics: encodes the topology describing how the different devices are oriented.
    """
    rotation = ...      # Column - major 3x3 rotation matrix
    translation = ...   # Three-element translation vector, in meters

    def __init__(self: pyrealsense2.extrinsics) -> None:
        ...


class filter:
    """
    Define the filter workflow, inherit this class to generate your own filter.
    """

    def __init__(self: pyrealsense2.filter, filter_function: Callable[[pyrealsense2.frame, pyrealsense2.frame_source], None], queue_size: int = 1) -> None:
        ...

    def as_decimation_filter(self: pyrealsense2.filter) -> pyrealsense2.decimation_filter:
        ...

    def as_depth_huffman_decoder(self: pyrealsense2.filter) -> pyrealsense2.depth_huffman_decoder:
        ...

    def as_disparity_transform(self: pyrealsense2.filter) -> pyrealsense2.disparity_transform:
        ...

    def as_hole_filling_filter(self: pyrealsense2.filter) -> pyrealsense2.hole_filling_filter:
        ...

    def as_spatial_filter(self: pyrealsense2.filter) -> pyrealsense2.spatial_filter:
        ...

    def as_temporal_filter(self: pyrealsense2.filter) -> pyrealsense2.temporal_filter:
        ...

    def as_threshold_filter(self: pyrealsense2.filter) -> pyrealsense2.threshold_filter:
        ...

    def as_zero_order_invalidation(self: pyrealsense2.filter) -> pyrealsense2.zero_order_invalidation:
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter(self: pyrealsense2.filter) -> bool:
        ...

    def is_depth_huffman_decoder(self: pyrealsense2.filter) -> bool:
        ...

    def is_disparity_transform(self: pyrealsense2.filter) -> bool:
        ...

    def is_hole_filling_filter(self: pyrealsense2.filter) -> bool:
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter(self: pyrealsense2.filter) -> bool:
        ...

    def is_temporal_filter(self: pyrealsense2.filter) -> bool:
        ...

    def is_threshold_filter(self: pyrealsense2.filter) -> bool:
        ...

    def is_zero_order_invalidation(self: pyrealsense2.filter) -> bool:
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class filter_interface:
    """
    Interface for frame filtering functionality
    """

    def __init__(self) -> None:
        'Initialize self. See help(type(self)) for accurate signature.'
        ...

    def process(self: pyrealsense2.filter_interface, frame: pyrealsense2.frame) -> pyrealsense2.frame:
        ...


class fisheye_sensor:
    profiles = ...  # The list of stream profiles supported by the sensor.

    def __init__(self: pyrealsense2.fisheye_sensor, sensor: pyrealsense2.sensor) -> None:
        ...

    def as_color_sensor():
        ...

    def as_depth_sensor():
        ...

    def as_fisheye_sensor():
        ...

    def as_motion_sensor():
        ...

    def as_pose_sensor():
        ...

    def as_roi_sensor():
        ...

    def as_wheel_odometer():
        ...

    def close(self: pyrealsense2.sensor) -> None:
        'Close sensor for exclusive access.'
        ...

    def get_info(self: pyrealsense2.sensor):
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_recommended_filters():
        'Return the recommended list of filters by the sensor.'
        ...

    def get_stream_profiles():
        'Retrieves the list of stream profiles supported by the sensor.'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def is_color_sensor():
        ...

    def is_depth_sensor():
        ...

    def is_fisheye_sensor():
        ...

    def is_motion_sensor():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_pose_sensor():
        ...

    def is_roi_sensor():
        ...

    def is_wheel_odometer():
        ...

    def open(*args, **kwargs):
        'Overloaded function.'
        ...

    def set_notifications_callback():
        'Register Notifications callback'
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(*args, **kwargs):
        'Overloaded function.'
        ...

    def stop(self: pyrealsense2.sensor) -> None:
        'Stop streaming.'
        ...

    def supports(*args, **kwargs):
        'Overloaded function.'
        ...


class format:
    """
    A stream's format identifies how binary data is encoded within a frame.
    """
    any = format.any
    bgr8 = format.bgr8
    bgra8 = format.bgra8
    disparity16 = format.disparity16
    disparity32 = format.disparity32
    distance = format.distance
    gpio_raw = format.gpio_raw
    invi = format.invi
    inzi = format.inzi
    mjpeg = format.mjpeg
    motion_raw = format.motion_raw
    motion_xyz32f = format.motion_xyz32f
    raw10 = format.raw10
    raw16 = format.raw16
    raw8 = format.raw8
    rgb8 = format.rgb8
    rgba8 = format.rgba8
    six_dof = format.six_dof
    uyvy = format.uyvy
    w10 = format.w10
    xyz32f = format.xyz32f
    y10bpack = format.y10bpack
    y12i = format.y12i
    y16 = format.y16
    y8 = format.y8
    y8i = format.y8i
    yuyv = format.yuyv
    z16 = format.z16
    z16h = format.z16h

    def __init__(self: pyrealsense2.format, arg0: int) -> None:
        ...


class frame:
    """
    Base class for multiple frame extensions
    """
    data = ...      	            # Data from the frame handle. Identical to calling get_data.
    frame_number = ...	            # The frame number. Identical to calling get_frame_number.
    # The timestamp domain. Identical to calling get_frame_timestamp_domain.
    frame_timestamp_domain = ...
    # Stream profile from frame handle. Identical to calling get_profile.
    profile = ...
    # Time at which the frame was captured. Identical to calling get_timestamp.
    timestamp = ...

    @overload
    def __init__(self: pyrealsense2.frame) -> None:
        ...

    @overload
    def __init__(self: pyrealsense2.frame, arg0: pyrealsense2.frame) -> None:
        ...

    def as_depth_frame(self: pyrealsense2.frame) -> pyrealsense2.depth_frame:
        ...

    def as_frame(self: pyrealsense2.frame) -> pyrealsense2.frame:
        ...

    def as_frameset(self: pyrealsense2.frame) -> pyrealsense2.frameset:
        ...

    def as_motion_frame(self: pyrealsense2.frame) -> pyrealsense2.motion_frame:
        ...

    def as_points(self: pyrealsense2.frame) -> pyrealsense2.points:
        ...

    def as_pose_frame(self: pyrealsense2.frame) -> pyrealsense2.pose_frame:
        ...

    def as_video_frame(self: pyrealsense2.frame) -> pyrealsense2.video_frame:
        ...

    def get_data(self: pyrealsense2.frame) -> pyrealsense2.BufData:
        'Retrieve data from the frame handle.'
        ...

    def get_data_size(self: pyrealsense2.frame) -> int:
        'Retrieve data size from frame handle.'
        ...

    def get_frame_metadata(self: pyrealsense2.frame, frame_metadata: pyrealsense2.frame_metadata_value) -> int:
        'Retrieve the current value of a single frame_metadata.'
        ...

    def get_frame_number(self: pyrealsense2.frame) -> int:
        'Retrieve the frame number.'
        ...

    def get_frame_timestamp_domain(self: pyrealsense2.frame) -> pyrealsense2.timestamp_domain:
        'Retrieve the timestamp domain.'
        ...

    def get_profile(self: pyrealsense2.frame) -> pyrealsense2.stream_profile:
        'Retrieve stream profile from frame handle.'
        ...

    def get_timestamp(self: pyrealsense2.frame) -> float:
        'Retrieve the time at which the frame was captured'
        ...

    def is_depth_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_frameset(self: pyrealsense2.frame) -> bool:
        ...

    def is_motion_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_points(self: pyrealsense2.frame) -> bool:
        ...

    def is_pose_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_video_frame(self: pyrealsense2.frame) -> bool:
        ...

    def keep(self: pyrealsense2.frame) -> None:
        'Keep the frame, otherwise if no refernce to the frame, the frame will be released.'
        ...

    def supports_frame_metadata(self: pyrealsense2.frame, frame_metadata: pyrealsense2.frame_metadata_value) -> bool:
        'Determine if the device allows a specific metadata to be queried.'
        ...

    def swap(self: pyrealsense2.frame, other: pyrealsense2.frame) -> None:
        'Swap the internal frame handle with the one in parameter'
        ...


class frame_metadata_value:
    """
    Per-Frame-Metadata is the set of read-only properties that might be exposed for each individual frame.
    """
    actual_exposure = frame_metadata_value.actual_exposure
    actual_fps = frame_metadata_value.actual_fps
    auto_exposure = frame_metadata_value.auto_exposure
    auto_white_balance_temperature = frame_metadata_value.auto_white_balance_temperature
    backend_timestamp = frame_metadata_value.backend_timestamp
    backlight_compensation = frame_metadata_value.backlight_compensation
    brightness = frame_metadata_value.brightness
    contrast = frame_metadata_value.contrast
    exposure_priority = frame_metadata_value.exposure_priority
    exposure_roi_bottom = frame_metadata_value.exposure_roi_bottom
    exposure_roi_left = frame_metadata_value.exposure_roi_left
    exposure_roi_right = frame_metadata_value.exposure_roi_right
    exposure_roi_top = frame_metadata_value.exposure_roi_top
    frame_counter = frame_metadata_value.frame_counter
    frame_emitter_mode = frame_metadata_value.frame_emitter_mode
    frame_laser_power = frame_metadata_value.frame_laser_power
    frame_laser_power_mode = frame_metadata_value.frame_laser_power_mode
    frame_led_power = frame_metadata_value.frame_led_power
    frame_timestamp = frame_metadata_value.frame_timestamp
    gain_level = frame_metadata_value.gain_level
    gamma = frame_metadata_value.gamma
    hue = frame_metadata_value.hue
    low_light_compensation = frame_metadata_value.low_light_compensation
    manual_white_balance = frame_metadata_value.manual_white_balance
    power_line_frequency = frame_metadata_value.power_line_frequency
    raw_frame_size = frame_metadata_value.raw_frame_size
    saturation = frame_metadata_value.saturation
    sensor_timestamp = frame_metadata_value.sensor_timestamp
    sharpness = frame_metadata_value.sharpness
    temperature = frame_metadata_value.temperature
    time_of_arrival = frame_metadata_value.time_of_arrival
    white_balance = frame_metadata_value.white_balance

    def __init__(self: pyrealsense2.frame_metadata_value, arg0: int) -> None:
        ...


class frame_queue:
    """
    Frame queues are the simplest cross-platform synchronization primitive provided by librealsense to help developers who are not using async APIs.
    """
    @overload
    def __init__(self: pyrealsense2.frame_queue) -> None:
        ...

    @overload
    def __init__(self: pyrealsense2.frame_queue, capacity: int, keep_frames: bool = False) -> None:
        ...

    def capacity(self: pyrealsense2.frame_queue) -> int:
        'Return the capacity of the queue.'
        ...

    def enqueue(self: pyrealsense2.frame_queue, f: pyrealsense2.frame) -> None:
        'Enqueue a new frame into the queue.'
        ...

    def keep_frames(self: pyrealsense2.frame_queue) -> bool:
        'Return whether or not the queue calls keep on enqueued frames.'
        ...

    def poll_for_frame(self: pyrealsense2.frame_queue) -> pyrealsense2.frame:
        'Poll if a new frame is available and dequeue it if it is'
        ...

    def try_wait_for_frame(self: pyrealsense2.frame_queue, timeout_ms: int = 5000) -> Tuple[bool, pyrealsense2.frame]:
        ...

    def wait_for_frame(self: pyrealsense2.frame_queue, timeout_ms: int = 5000) -> pyrealsense2.frame:
        'Wait until a new frame becomes available in the queue and dequeue it.'
        ...


class frame_source:
    """
    The source used to generate frames, which is usually done by the low level driver for each sensor. frame_source is one of the parameters of processing_block's callback function, which can be used to re-generate the frame and via frame_ready invoke another callback function to notify application frame is ready.
    """

    def __init__(self) -> None:
        'Initialize self. See help(type(self)) for accurate signature.'
        ...

    def allocate_composite_frame(self: pyrealsense2.frame_source, frames: List[pyrealsense2.frame]) -> pyrealsense2.frame:
        'Allocate composite frame with given params'
        ...

    def allocate_points(self: pyrealsense2.frame_source, profile: pyrealsense2.stream_profile, original: pyrealsense2.frame) -> pyrealsense2.frame:
        ...

    def allocate_video_frame(self: pyrealsense2.frame_source, profile: pyrealsense2.stream_profile, original: pyrealsense2.frame, new_bpp: int = 0, new_width: int = 0, new_height: int = 0, new_stride: int = 0, frame_type: pyrealsense2.extension = extension.video_frame) -> pyrealsense2.frame:
        'Allocate a new video frame with given params'
        ...

    def frame_ready(self: pyrealsense2.frame_source, result: pyrealsense2.frame) -> None:
        'Invoke the callback funtion informing the frame is ready.'
        ...


class hole_filling_filter:
    """
    The processing performed depends on the selected hole filling mode.
    """
    @overload
    def __init__(self: pyrealsense2.hole_filling_filter) -> None:
        ...

    @overload
    def __init__(self: pyrealsense2.hole_filling_filter, mode: int) -> None:
        """
        Possible values for mode: \n
        0 - fill_from_left      - Use the value from the left neighbor pixel to fill the hole \n
        1 - farest_from_around  - Use the value from the neighboring pixel which is furthest away from the sensor \n
        2 - nearest_from_around - Use the value from the neighboring pixel closest to the sensor
        """
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options, ):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class intrinsics:
    """
    Video stream intrinsics.
    """
    coeffs = ...  # Distortion coefficients
    fx = ...	    # Focal length of the image plane, as a multiple of pixel width
    fy = ...	    # Focal length of the image plane, as a multiple of pixel height
    height = ...    # Height of the image in pixels
    model = ...     # Distortion model of the image
    ppx = ...	    # Horizontal coordinate of the principal point of the image, as a pixel offset from the left edge
    ppy = ...	    # Vertical coordinate of the principal point of the image, as a pixel offset from the top edge
    width = ...	    # Width of the image in pixels

    def __init__(self: pyrealsense2.intrinsics) -> None:
        ...


class log_severity:
    """
    Severity of the librealsense logger.
    """
    debug = log_severity.debug
    error = log_severity.error
    fatal = log_severity.fatal
    info = log_severity.info
    none = log_severity.none
    warn = log_severity.warn

    def __init__(self: pyrealsense2.log_severity, arg0: int) -> None:
        ...


class motion_device_intrinsic:
    """
    Motion device intrinsics: scale, bias, and variances.
    """
    bias_variances = ...  # Variance of bias for X, Y, and Z axis
    data = ...	            # 3x4 matrix with 3x3 scale and cross axis and 3x1 biases
    noise_variances = ...   # Variance of noise for X, Y, and Z axis

    def __init__(self: pyrealsense2.motion_device_intrinsic) -> None:
        ...


class motion_frame:
    """
    Extends the frame class with additional motion related attributes and functions
    """
    data = ...	                    # Data from the frame handle.
    frame_number = ...	            # The frame number.
    frame_timestamp_domain = ...    # The timestamp domain.
    # Motion data from IMU sensor. Identical to calling get_motion_data.
    motion_data = ...
    profile = ...	                # Stream profile from frame handle.
    timestamp = ...	                # Time at which the frame was captured.

    def __init__(self: pyrealsense2.motion_frame, arg0: pyrealsense2.frame) -> None:
        ...

    def as_depth_frame():
        ...

    def as_frame():
        ...

    def as_frameset():
        ...

    def as_motion_frame():
        ...

    def as_points():
        ...

    def as_pose_frame():
        ...

    def as_video_frame():
        ...

    def get_data():
        'Retrieve data from the frame handle.'
        ...

    def get_data_size(self: pyrealsense2.frame) -> int:
        'Retrieve data size from frame handle.'
        ...

    def get_frame_metadata():
        'Retrieve the current value of a single frame_metadata.'
        ...

    def get_frame_number():
        'Retrieve the frame number.'
        ...

    def get_frame_timestamp_domain():
        'Retrieve the timestamp domain.'
        ...

    def get_motion_data(self: pyrealsense2.motion_frame) -> pyrealsense2.vector:
        'Retrieve the motion data from IMU sensor.'
        ...

    def get_profile():
        'Retrieve stream profile from frame handle.'
        ...

    def get_timestamp():
        'Retrieve the time at which the frame was captured'
        ...

    def is_depth_frame():
        ...

    def is_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_frameset(self: pyrealsense2.frame) -> bool:
        ...

    def is_motion_frame():
        ...

    def is_points(self: pyrealsense2.frame) -> bool:
        ...

    def is_pose_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_video_frame():
        ...

    def keep(self: pyrealsense2.frame) -> None:
        'Keep the frame, otherwise if no refernce to the frame, the frame will be released.'
        ...

    def supports_frame_metadata():
        'Determine if the device allows a specific metadata to be queried.'
        ...

    def swap(self: pyrealsense2.frame):
        'Swap the internal frame handle with the one in parameter'
        ...


class motion_sensor:
    profiles = ...  # The list of stream profiles supported by the sensor.

    def __init__(self: pyrealsense2.motion_sensor, sensor: pyrealsense2.sensor) -> None:
        ...

    def as_color_sensor():
        ...

    def as_depth_sensor():
        ...

    def as_fisheye_sensor():
        ...

    def as_motion_sensor():
        ...

    def as_pose_sensor():
        ...

    def as_roi_sensor():
        ...

    def as_wheel_odometer():
        ...

    def close(self: pyrealsense2.sensor) -> None:
        'Close sensor for exclusive access.'
        ...

    def get_info(self: pyrealsense2.sensor):
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_recommended_filters():
        'Return the recommended list of filters by the sensor.'
        ...

    def get_stream_profiles():
        'Retrieves the list of stream profiles supported by the sensor.'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def is_color_sensor():
        ...

    def is_depth_sensor():
        ...

    def is_fisheye_sensor():
        ...

    def is_motion_sensor():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_pose_sensor():
        ...

    def is_roi_sensor():
        ...

    def is_wheel_odometer():
        ...

    def open(*args, **kwargs):
        'Overloaded function.'
        ...

    def set_notifications_callback():
        'Register Notifications callback'
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(*args, **kwargs):
        'Overloaded function.'
        ...

    def stop(self: pyrealsense2.sensor) -> None:
        'Stop streaming.'
        ...

    def supports(*args, **kwargs):
        'Overloaded function.'
        ...


class motion_stream_profile:
    """
    Stream profile instance which contains IMU-specific intrinsics.
    """

    def __init__(self: pyrealsense2.motion_stream_profile, sp: pyrealsense2.stream_profile) -> None:
        ...

    def as_motion_stream_profile():
        ...

    def as_stream_profile():
        ...

    def as_video_stream_profile():
        ...

    def clone(self: pyrealsense2.stream_profile):
        'Clone the current profile and change the type, index and format to input parameters'
        ...

    def format():
        'The stream\'s format'
        ...

    def fps(self: pyrealsense2.stream_profile) -> int:
        'The streams framerate'
        ...

    def get_extrinsics_to():
        'Get the extrinsic transformation between two profiles (representing physical sensors)'
        ...

    def get_motion_intrinsics(self: pyrealsense2.motion_stream_profile) -> pyrealsense2.motion_device_intrinsic:
        'Returns scale and bias of a motion stream.'
        ...

    def is_default():
        'Checks if the stream profile is marked/assigned as default, meaning that the profile will be selected when the user requests stream configuration using wildcards.'
        ...

    def is_motion_stream_profile():
        ...

    def is_stream_profile():
        ...

    def is_video_stream_profile():
        ...

    def register_extrinsics_to():
        'Assign extrinsic transformation parameters to a specific profile (sensor).'
        ...

    def stream_index():
        'The stream\'s index'
        ...

    def stream_name():
        'The stream\'s human-readable name.'
        ...

    def stream_type():
        'The stream\'s type'
        ...

    def unique_id():
        'Unique index assigned whent the stream was created'
        ...


class notification:
    category = ...          # The notification's category. Identical to calling get_category.
    # The notification's description. Identical to calling get_description.
    description = ...
    # The notification's serialized data. Identical to calling get_serialized_data.
    serialized_data = ...
    severity = ...          # The notification's severity. Identical to calling get_severity.
    # The notification's arrival timestamp. Identical to calling get_timestamp.
    timestamp = ...

    def __init__(self: pyrealsense2.notification) -> None:
        ...

    def get_category(self: pyrealsense2.notification) -> pyrealsense2.notification_category:
        'Retrieve the notification\'s category.'
        ...

    def get_description(self: pyrealsense2.notification) -> str:
        'Retrieve the notification\'s description.'
        ...

    def get_serialized_data(self: pyrealsense2.notification) -> pyrealsense2.log_severity:
        'Retrieve the notification\'s serialized data.'
        ...

    def get_severity(self: pyrealsense2.notification) -> pyrealsense2.log_severity:
        'Retrieve the notification\'s severity.'
        ...

    def get_timestamp(self: pyrealsense2.notification) -> float:
        'Retrieve the notification\'s arrival timestamp.'
        ...


class notification_category:
    """
    Category of the librealsense notification.
    """
    firmware_update_recommended = notification_category.firmware_update_recommended
    frame_corrupted = notification_category.frame_corrupted
    frames_timeout = notification_category.frames_timeout
    hardware_error = notification_category.hardware_error
    hardware_event = notification_category.hardware_event
    pose_relocalization = notification_category.pose_relocalization
    unknown_error = notification_category.unknown_error

    def __init__(self: pyrealsense2.notification_category, arg0: int) -> None:
        ...


class option:
    """
    Defines general configuration controls. These can generally be mapped to camera UVC controls, and can be set/queried at any time unless stated otherwise.
    """
    accuracy = option.accuracy
    ambient_light = option.ambient_light
    apd_temperature = option.apd_temperature
    asic_temperature = option.asic_temperature
    auto_exposure_converge_step = option.auto_exposure_converge_step
    auto_exposure_mode = option.auto_exposure_mode
    auto_exposure_priority = option.auto_exposure_priority
    avalanche_photo_diode = option.avalanche_photo_diode
    backlight_compensation = option.backlight_compensation
    brightness = option.brightness
    color_scheme = option.color_scheme
    confidence_threshold = option.confidence_threshold
    contrast = option.contrast
    depth_offset = option.depth_offset
    depth_units = option.depth_units
    emitter_enabled = option.emitter_enabled
    emitter_on_off = option.emitter_on_off
    enable_auto_exposure = option.enable_auto_exposure
    enable_auto_white_balance = option.enable_auto_white_balance
    enable_dynamic_calibration = option.enable_dynamic_calibration
    enable_map_preservation = option.enable_map_preservation
    enable_mapping = option.enable_mapping
    enable_motion_correction = option.enable_motion_correction
    enable_pose_jumping = option.enable_pose_jumping
    enable_relocalization = option.enable_relocalization
    error_polling_enabled = option.error_polling_enabled
    exposure = option.exposure
    filter_magnitude = option.filter_magnitude
    filter_option = option.filter_option
    filter_smooth_alpha = option.filter_smooth_alpha
    filter_smooth_delta = option.filter_smooth_delta
    frames_queue_size = option.frames_queue_size
    freefall_detection_enabled = option.freefall_detection_enabled
    gain = option.gain
    gamma = option.gamma
    global_time_enabled = option.global_time_enabled
    hardware_preset = option.hardware_preset
    histogram_equalization_enabled = option.histogram_equalization_enabled
    holes_fill = option.holes_fill
    hue = option.hue
    inter_cam_sync_mode = option.inter_cam_sync_mode
    invalidation_bypass = option.invalidation_bypass
    laser_power = option.laser_power
    led_power = option.led_power
    lld_temperature = option.lld_temperature
    ma_temperature = option.ma_temperature
    max_distance = option.max_distance
    mc_temperature = option.mc_temperature
    min_distance = option.min_distance
    motion_module_temperature = option.motion_module_temperature
    motion_range = option.motion_range
    noise_filtering = option.noise_filtering
    output_trigger_enabled = option.output_trigger_enabled
    post_processing_sharpening = option.post_processing_sharpening
    power_line_frequency = option.power_line_frequency
    pre_processing_sharpening = option.pre_processing_sharpening
    projector_temperature = option.projector_temperature
    saturation = option.saturation
    sensor_mode = option.sensor_mode
    sharpness = option.sharpness
    stereo_baseline = option.stereo_baseline
    stream_filter = option.stream_filter
    stream_format_filter = option.stream_format_filter
    stream_index_filter = option.stream_index_filter
    texture_source = option.texture_source
    total_frame_drops = option.total_frame_drops
    visual_preset = option.visual_preset
    white_balance = option.white_balance
    zero_order_enabled = option.zero_order_enabled
    zero_order_point_x = option.zero_order_point_x
    zero_order_point_y = option.zero_order_point_y

    def __init__(self: pyrealsense2.option, arg0: int) -> None:
        ...


class option_range:
    default = ...
    max = ...
    min = ...
    step = ...

    def __init__(self) -> None:
        'Initialize self. See help(type(self)) for accurate signature.'
        ...


class options:
    """
    Base class for options interface. Should be used via sensor or processing_block.
    """

    def __init__(self) -> None:
        'Initialize self. See help(type(self)) for accurate signature.'
        ...

    def get_option(self: pyrealsense2.options, option: pyrealsense2.option) -> float:
        'Read option value from the device.'
        ...

    def get_option_description(self: pyrealsense2.options, option: pyrealsense2.option) -> str:
        'Get option description.'
        ...

    def get_option_range(self: pyrealsense2.options, option: pyrealsense2.option) -> pyrealsense2.option_range:
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description(self: pyrealsense2.options, option: pyrealsense2.option, value: float) -> str:
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options(self: pyrealsense2.options) -> List[pyrealsense2.option]:
        'Retrieve list of supported options'
        ...

    def is_option_read_only(self: pyrealsense2.options, option: pyrealsense2.option) -> bool:
        'Check if particular option is read only.'
        ...

    def set_option(self: pyrealsense2.options, option: pyrealsense2.option, value: float) -> None:
        'Write new value to device option'
        ...

    def supports(self: pyrealsense2.options, option: pyrealsense2.option) -> bool:
        'Check if particular option is supported by a subdevice'
        ...


class pipeline:
    """
    The pipeline simplifies the user interaction with the device and computer vision processing modules. The class abstracts the camera configuration and streaming, and the vision modules triggering and threading. It lets the application focus on the computer vision output of the modules, or the device output data. The pipeline can manage computer vision modules, which are implemented as a processing blocks. The pipeline is the consumer of the processing block interface, while the application consumes the computer vision interface.
    """

    def __init__(self: pyrealsense2.pipeline, ctx: pyrealsense2.context = ...) -> None:
        'The caller can provide a context created by the application, usually for playback or testing purposes.'
        ...

    def get_active_profile(self: pyrealsense2.pipeline) -> pyrealsense2.pipeline_profile:
        ...

    def poll_for_frames(self: pyrealsense2.pipeline) -> pyrealsense2.composite_frame:
        'Check if a new set of frames is available and retrieve the latest undelivered set. The frames set includes time-synchronized frames of each enabled stream in the pipeline. The method returns without blocking the calling thread, with status of new frames available or not. If available, it fetches the latest frames set. Device frames, which were produced while the function wasn\'t called, are dropped. To avoid frame drops, this method should be called as fast as the device frame rate. The application can maintain the frames handles to defer processing. However, if the application maintains too long history, the device may lack memory resources to produce new frames, and the following calls to this method shall return no new frames, until resources become available.'
        ...

    @overload
    def start(self: pyrealsense2.pipeline) -> pyrealsense2.pipeline_profile:
        'Start the pipeline streaming with its default configuration. The pipeline streaming loop captures samples from the device, and delivers them to the attached computer vision modules and processing blocks, according to each module requirements and threading model. During the loop execution, the application can access the camera streams by calling wait_for_frames() or poll_for_frames(). The streaming loop runs until the pipeline is stopped. Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised.'
        ...

    @overload
    def start(self: pyrealsense2.pipeline, config: pyrealsense2.config) -> pyrealsense2.pipeline_profile:
        'Start the pipeline streaming according to the configuraion. The pipeline streaming loop captures samples from the device, and delivers them to the attached computer vision modules and processing blocks, according to each module requirements and threading model. During the loop execution, the application can access the camera streams by calling wait_for_frames() or poll_for_frames(). The streaming loop runs until the pipeline is stopped. Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised. The pipeline selects and activates the device upon start, according to configuration or a default configuration. When the pyrealsense2.config is provided to the method, the pipeline tries to activate the config resolve() result. If the application requests are conflicting with pipeline computer vision modules or no matching device is available on the platform, the method fails. Available configurations and devices may change between config resolve() call and pipeline start, in case devices are connected or disconnected, or another application acquires ownership of a device.'
        ...

    @overload
    def start(self: pyrealsense2.pipeline, callback: Callable[[pyrealsense2.frame], None]) -> pyrealsense2.pipeline_profile:
        'Start the pipeline streaming with its default configuration. The pipeline captures samples from the device, and delivers them to the provided frame callback. Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised. When starting the pipeline with a callback both wait_for_frames() and poll_for_frames() will throw exception.'
        ...

    @overload
    def start(self: pyrealsense2.pipeline, config: pyrealsense2.config, callback: Callable[[pyrealsense2.frame], None]) -> pyrealsense2.pipeline_profile:
        'Start the pipeline streaming according to the configuraion. The pipeline captures samples from the device, and delivers them to the provided frame callback. Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised. When starting the pipeline with a callback both wait_for_frames() and poll_for_frames() will throw exception. The pipeline selects and activates the device upon start, according to configuration or a default configuration. When the pyrealsense2.config is provided to the method, the pipeline tries to activate the config resolve() result. If the application requests are conflicting with pipeline computer vision modules or no matching device is available on the platform, the method fails. Available configurations and devices may change between config resolve() call and pipeline start, in case devices are connected or disconnected, or another application acquires ownership of a device.'
        ...

    @overload
    def start(self: pyrealsense2.pipeline, queue: pyrealsense2.frame_queue) -> pyrealsense2.pipeline_profile:
        'Start the pipeline streaming with its default configuration. The pipeline captures samples from the device, and delivers them to the provided frame queue. Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised. When starting the pipeline with a callback both wait_for_frames() and poll_for_frames() will throw exception.'
        ...

    @overload
    def start(self: pyrealsense2.pipeline, config: pyrealsense2.config, queue: pyrealsense2.frame_queue) -> pyrealsense2.pipeline_profile:
        'Start the pipeline streaming according to the configuraion. The pipeline captures samples from the device, and delivers them to the provided frame queue. Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised. When starting the pipeline with a callback both wait_for_frames() and poll_for_frames() will throw exception. The pipeline selects and activates the device upon start, according to configuration or a default configuration. When the pyrealsense2.config is provided to the method, the pipeline tries to activate the config resolve() result. If the application requests are conflicting with pipeline computer vision modules or no matching device is available on the platform, the method fails. Available configurations and devices may change between config resolve() call and pipeline start, in case devices are connected or disconnected, or another application acquires ownership of a device.'
        ...

    def stop(self: pyrealsense2.pipeline) -> None:
        'Stop the pipeline streaming. The pipeline stops delivering samples to the attached computer vision modules and processing blocks, stops the device streaming and releases the device resources used by the pipeline. It is the application\'s responsibility to release any frame reference it owns. The method takes effect only after start() was called, otherwise an exception is raised.'
        ...

    def try_wait_for_frames(self: pyrealsense2.pipeline, timeout_ms: int = 5000) -> Tuple[bool, pyrealsense2.composite_frame]:
        ...

    def wait_for_frames(self: pyrealsense2.pipeline, timeout_ms: int = 5000) -> pyrealsense2.composite_frame:
        'Wait until a new set of frames becomes available. The frames set includes time-synchronized frames of each enabled stream in the pipeline. In case of different frame rates of the streams, the frames set include a matching frame of the slow stream, which may have been included in previous frames set. The method blocks the calling thread, and fetches the latest unread frames set. Device frames, which were produced while the function wasn\'t called, are dropped. To avoid frame drops, this method should be called as fast as the device frame rate. The application can maintain the frames handles to defer processing. However, if the application maintains too long history, the device may lack memory resources to produce new frames, and the following call to this method shall fail to retrieve new frames, until resources become available.'
        ...


class pipeline_profile:
    """
    The pipeline profile includes a device and a selection of active streams, with specific profiles. The profile is a selection of the above under filters and conditions defined by the pipeline. Streams may belong to more than one sensor of the device.
    """

    def __init__(self: pyrealsense2.pipeline_profile) -> None:
        ...

    def get_device(self: pyrealsense2.pipeline_profile) -> pyrealsense2.device:
        'Retrieve the device used by the pipeline. The device class provides the application access to control camera additional settings - get device information, sensor options information, options value query and set, sensor specific extensions. Since the pipeline controls the device streams configuration, activation state and frames reading, calling the device API functions, which execute those operations, results in unexpected behavior. The pipeline streaming device is selected during pipeline start(). Devices of profiles, which are not returned by pipeline start() or get_active_profile(), are not guaranteed to be used by the pipeline.'
        ...

    def get_device(self: pyrealsense2.pipeline_profile, stream_type: pyrealsense2.stream, stream_index: int = -1) -> pyrealsense2.stream_profile:
        'Return the stream profile that is enabled for the specified stream in this profile.'
        ...

    def get_device(self: pyrealsense2.pipeline_profile) -> List[pyrealsense2.stream_profile]:
        'Return the selected streams profiles, which are enabled in this profile.'
        ...


class pipeline_wrapper:
    def __init__(self: pyrealsense2.pipeline_wrapper, arg0: pyrealsense2.pipeline) -> None:
        ...


class playback:
    sensors = ...   # List of adjacent devices, sharing the same physical parent composite device.

    def __init__(self: pyrealsense2.playback, device: pyrealsense2.device) -> None:
        ...

    def as_auto_calibrated_device():
        ...

    def as_debug_protocol():
        ...

    def as_playback():
        ...

    def as_recorder():
        ...

    def as_tm2(self: pyrealsense2.device) -> pyrealsense2.tm2:
        ...

    def as_updatable():
        ...

    def as_update_device():
        ...

    def current_status(self: pyrealsense2.playback) -> pyrealsense2.playback_status:
        'Returns the current state of the playback device'
        ...

    def file_name(self: pyrealsense2.playback) -> str:
        'The name of the playback file.'
        ...

    def first_color_sensor():
        ...

    def first_depth_sensor():
        ...

    def first_fisheye_sensor():
        ...

    def first_motion_sensor():
        ...

    def first_pose_sensor():
        ...

    def first_roi_sensor():
        ...

    def get_duration(self: pyrealsense2.playback) -> datetime.timedelta:
        'Retrieves the total duration of the file.'
        ...

    def get_info(self: pyrealsense2.device):
        'Retrieve camera specific information, like versions of various internal components'
        ...

    def get_position(self: pyrealsense2.playback) -> int:
        'Retrieves the current position of the playback in the file in terms of time. Units are expressed in nanoseconds.'
        ...

    def hardware_reset():
        'Send hardware reset request to the device'
        ...

    def is_auto_calibrated_device():
        ...

    def is_debug_protocol():
        ...

    def is_playback(self: pyrealsense2.device) -> bool:
        ...

    def is_real_time(self: pyrealsense2.playback) -> bool:
        'Indicates if playback is in real time mode or non real time.'
        ...

    def is_recorder(self: pyrealsense2.device) -> bool:
        ...

    def is_tm2(self: pyrealsense2.device) -> bool:
        ...

    def is_updatable(self: pyrealsense2.device) -> bool:
        ...

    def is_update_device():
        ...

    def pause(self: pyrealsense2.playback) -> None:
        'Pauses the playback. Calling pause() in “Paused” status does nothing. If pause() is called while playback status is “Playing” or “Stopped”, the playback will not play until resume() is called.'
        ...

    def query_sensors():
        'Returns the list of adjacent devices, sharing the same physical parent composite device.'
        ...

    def resume(self: pyrealsense2.playback) -> None:
        'Un-pauses the playback.'
        ...

    def seek(self: pyrealsense2.playback, time: datetime.timedelta) -> None:
        'Sets the playback to a specified time point of the played data.'
        ...

    def set_real_time(self: pyrealsense2.playback, real_time: bool) -> None:
        'Set the playback to work in real time or non real time. In real time mode, playback will play the same way the file was recorded. If the application takes too long to handle the callback, frames may be dropped. In non real time mode, playback will wait for each callback to finish handling the data before reading the next frame. In this mode no frames will be dropped, and the application controls the framerate of playback via callback duration.'
        ...

    def set_status_changed_callback(self: pyrealsense2.playback, callback: Callable[[pyrealsense2.playback_status], None]) -> None:
        'Register to receive callback from playback device upon its status changes.'
        ...

    def supports(self: pyrealsense2.device):
        'Register to receive callback from playback device upon its status changes. Callbacks are invoked from the reading thread, and as such any heavy processing in the callback handler will affect the reading thread and may cause frame drops/high latency.'
        ...


class playback_status:
    paused = playback_status.paused
    playing = playback_status.playing
    stopped = playback_status.stopped
    unknown = playback_status.unknown

    def __init__(self: pyrealsense2.playback_status, arg0: int) -> None:
        ...


class pointcloud:
    """
    Generates 3D point clouds based on a depth frame. Can also map textures from a color frame.
    """
    @overload
    def __init__(self: pyrealsense2.pointcloud) -> None:
        ...

    @overload
    def __init__(self: pyrealsense2.pointcloud, stream: pyrealsense2.stream, index: int = 0) -> None:
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def calculate(self: pyrealsense2.pointcloud, depth: pyrealsense2.frame) -> pyrealsense2.points:
        'Generate the pointcloud and texture mappings of depth map.'
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def map_to(self: pyrealsense2.pointcloud, mapped: pyrealsense2.frame) -> None:
        'Map the point cloud to the given color frame.'
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        ...


class points:
    """
    Extends the frame class with additional point cloud related attributes and functions.
    """
    data = ...                      # Data from the frame handle.
    frame_number = ...              # The frame number.
    frame_timestamp_domain = ...    # The timestamp domain.
    profile = ...                   # Stream profile from frame handle.
    timestamp = ...                 # Time at which the frame was captured.

    @overload
    def __init__(self: pyrealsense2.points) -> None:
        ...

    @overload
    def __init__(self: pyrealsense2.points, arg0: pyrealsense2.frame) -> None:
        ...

    def as_depth_frame():
        ...

    def as_frame():
        ...

    def as_frameset():
        ...

    def as_motion_frame():
        ...

    def as_points():
        ...

    def as_pose_frame():
        ...

    def as_video_frame():
        ...

    def export_to_ply(self: pyrealsense2.points, arg0: str, arg1: pyrealsense2.video_frame) -> None:
        'Export the point cloud to a PLY file'
        ...

    def get_data():
        'Retrieve data from the frame handle.'
        ...

    def get_data_size(self: pyrealsense2.frame) -> int:
        'Retrieve data size from frame handle.'
        ...

    def get_frame_metadata():
        'Retrieve the current value of a single frame_metadata.'
        ...

    def get_frame_number():
        'Retrieve the frame number.'
        ...

    def get_frame_timestamp_domain():
        'Retrieve the timestamp domain.'
        ...

    def get_profile():
        'Retrieve stream profile from frame handle.'
        ...

    def get_texture_coordinates(self: pyrealsense2.points, dims: int = 1) -> pyrealsense2.BufData:
        'Retrieve the texture coordinates (uv map) for the point cloud'
        ...

    def get_timestamp():
        'Retrieve the time at which the frame was captured'
        ...

    def get_vertices(self: pyrealsense2.points, dims: int = 1) -> pyrealsense2.BufData:
        'Retrieve the vertices of the point cloud'
        ...

    def is_depth_frame():
        ...

    def is_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_frameset(self: pyrealsense2.frame) -> bool:
        ...

    def is_motion_frame():
        ...

    def is_points(self: pyrealsense2.frame) -> bool:
        ...

    def is_pose_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_video_frame():
        ...

    def keep(self: pyrealsense2.frame) -> None:
        'Keep the frame, otherwise if no reference to the frame, the frame will be released.'
        ...

    def size(self: pyrealsense2.points) -> int:
        ...

    def supports_frame_metadata():
        'Determine if the device allows a specific metadata to be queried.'
        ...

    def swap(self: pyrealsense2.frame):
        'Swap the internal frame handle with the one in parameter'
        ...


class pose:
    acceleration = ...          # X, Y, Z values of acceleration, in meters/sec^2
    angular_acceleration = ...  # X, Y, Z values of angular acceleration, in radians/sec^2
    angular_velocity = ...      # X, Y, Z values of angular velocity, in radians/sec
    mapper_confidence = ...     # Pose map confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High
    # Qi, Qj, Qk, Qr components of rotation as represented in quaternion rotation (relative to initial position)
    rotation = ...
    tracker_confidence = ...    # Pose confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High
    # X, Y, Z values of translation, in meters (relative to initial position)
    translation = ...
    velocity = ...              # X, Y, Z values of velocity, in meters/sec

    def __init__(self: pyrealsense2.pose) -> None:
        ...


class pose_frame:
    """
    Extends the frame class with additional pose related attributes and functions.
    """
    data = ...                      # Data from the frame handle.
    frame_number = ...              # The frame number.
    frame_timestamp_domain = ...    # The timestamp domain.
    # Pose data from T2xx position tracking sensor. Identical to calling get_pose_data.
    pose_data = ...
    profile = ...                   # Stream profile from frame handle.
    timestamp = ...                 # Time at which the frame was captured.

    def __init__(self: pyrealsense2.pose_frame, arg0: pyrealsense2.frame) -> None:
        ...

    def as_depth_frame():
        ...

    def as_frame():
        ...

    def as_frameset():
        ...

    def as_motion_frame():
        ...

    def as_points():
        ...

    def as_pose_frame():
        ...

    def as_video_frame():
        ...

    def get_data():
        'Retrieve data from the frame handle.'
        ...

    def get_data_size(self: pyrealsense2.frame) -> int:
        'Retrieve data size from frame handle.'
        ...

    def get_frame_metadata():
        'Retrieve the current value of a single frame_metadata.'
        ...

    def get_frame_number():
        'Retrieve the frame number.'
        ...

    def get_frame_timestamp_domain():
        'Retrieve the timestamp domain.'
        ...

    def get_pose_data(self: pyrealsense2.pose_frame) -> pyrealsense2.pose:
        'Retrieve the pose data from T2xx position tracking sensor.'
        ...

    def get_profile():
        'Retrieve stream profile from frame handle.'
        ...

    def get_timestamp():
        'Retrieve the time at which the frame was captured'
        ...

    def is_depth_frame():
        ...

    def is_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_frameset(self: pyrealsense2.frame) -> bool:
        ...

    def is_motion_frame():
        ...

    def is_points(self: pyrealsense2.frame) -> bool:
        ...

    def is_pose_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_video_frame():
        ...

    def keep(self: pyrealsense2.frame) -> None:
        'Keep the frame, otherwise if no refernce to the frame, the frame will be released.'
        ...

    def supports_frame_metadata():
        'Determine if the device allows a specific metadata to be queried.'
        ...

    def swap(self: pyrealsense2.frame):
        'Swap the internal frame handle with the one in parameter'
        ...


class pose_sensor:
    profiles = ...  # The list of stream profiles supported by the sensor.

    def __init__(self: pyrealsense2.pose_sensor, sensor: pyrealsense2.sensor) -> None:
        ...

    def as_color_sensor():
        ...

    def as_depth_sensor():
        ...

    def as_fisheye_sensor():
        ...

    def as_motion_sensor():
        ...

    def as_pose_sensor():
        ...

    def as_roi_sensor():
        ...

    def as_wheel_odometer():
        ...

    def close(self: pyrealsense2.sensor) -> None:
        'Close sensor for exclusive access.'
        ...

    def export_localization_map(self: pyrealsense2.pose_sensor) -> List[int]:
        'Get relocalization map that is currently on device, created and updated during most recent tracking session. Can be called before or after stop().'
        ...

    def get_info(self: pyrealsense2.sensor):
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_recommended_filters():
        'Return the recommended list of filters by the sensor.'
        ...

    def get_static_node(self: pyrealsense2.pose_sensor, guid: str) -> Tuple[bool, pyrealsense2.vector, pyrealsense2.quaternion]:
        'Gets the current pose of a static node that was created in the current map or in an imported map. Static nodes of imported maps are available after relocalizing the imported map. The static node\'s pose is returned relative to the current origin of coordinates of device poses. Thus, poses of static nodes of an imported map are consistent with current device poses after relocalization. This function fails if the current tracker confidence is below 3 (high confidence).'
        ...

    def get_stream_profiles():
        'Retrieves the list of stream profiles supported by the sensor.'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def import_localization_map(self: pyrealsense2.pose_sensor, lmap_buf: List[int]) -> bool:
        'Load relocalization map onto device. Only one relocalization map can be imported at a time; any previously existing map will be overwritten. The imported map exists simultaneously with the map created during the most recent tracking session after start(),and they are merged after the imported map is relocalized. This operation must be done before start().'
        ...

    def is_color_sensor():
        ...

    def is_depth_sensor():
        ...

    def is_fisheye_sensor():
        ...

    def is_motion_sensor():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_pose_sensor():
        ...

    def is_roi_sensor():
        ...

    def is_wheel_odometer():
        ...

    def open(*args, **kwargs):
        'Overloaded function.'
        ...

    def remove_static_node(self: pyrealsense2.pose_sensor, guid: str) -> bool:
        'Removes a named virtual landmark in the current map, known as static node.'
        ...

    def set_notifications_callback():
        'Register Notifications callback'
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def set_static_node(self: pyrealsense2.pose_sensor, guid: str, pos: pyrealsense2.vector, orient: pyrealsense2.quaternion) -> bool:
        'Creates a named virtual landmark in the current map, known as static node. The static node\'s pose is provided relative to the origin of current coordinate system of device poses. This function fails if the current tracker confidence is below 3 (high confidence).'
        ...

    def start(*args, **kwargs):
        'Overloaded function.'
        ...

    def stop(self: pyrealsense2.sensor) -> None:
        'Stop streaming.'
        ...

    def supports(*args, **kwargs):
        'Overloaded function.'
        ...


class pose_stream_profile:
    """
    Stream profile instance with an explicit pose extension type.
    """

    def __init__(self: pyrealsense2.pose_stream_profile, sp: pyrealsense2.stream_profile) -> None:
        ...

    def as_motion_stream_profile():
        ...

    def as_stream_profile():
        ...

    def as_video_stream_profile():
        ...

    def clone(self: pyrealsense2.stream_profile):
        'Clone the current profile and change the type, index and format to input parameters'
        ...

    def format():
        'The stream\'s format'
        ...

    def fps(self: pyrealsense2.stream_profile) -> int:
        'The streams framerate'
        ...

    def get_extrinsics_to():
        'Get the extrinsic transformation between two profiles (representing physical sensors)'
        ...

    def is_default():
        'Checks if the stream profile is marked/assigned as default, meaning that the profile will be selected when the user requests stream configuration using wildcards.'
        ...

    def is_motion_stream_profile():
        ...

    def is_stream_profile():
        ...

    def is_video_stream_profile():
        ...

    def register_extrinsics_to():
        'Assign extrinsic transformation parameters to a specific profile (sensor).'
        ...

    def stream_index():
        'The stream\'s index'
        ...

    def stream_name():
        'The stream\'s human-readable name.'
        ...

    def stream_type():
        'The stream\'s type'
        ...

    def unique_id():
        'Unique index assigned whent the stream was created'
        ...


class processing_block:
    """
    Define the processing block workflow, inherit this class to generate your own processing_block.
    """

    def __init__(self: pyrealsense2.processing_block, processing_function: Callable[[pyrealsense2.frame, pyrealsense2.frame_source], None]) -> None:
        ...

    def get_info(self: pyrealsense2.processing_block, arg0: pyrealsense2.camera_info) -> str:
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block, f: pyrealsense2.frame) -> None:
        'Ask processing block to process the frame'
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block, callback: Callable[[pyrealsense2.frame], None]) -> None:
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports(self: pyrealsense2.processing_block, arg0: pyrealsense2.camera_info) -> bool:
        'Check if a specific camera info field is supported.'
        ...


class quaternion:
    """
    Quaternion used to represent rotation.
    """
    w = ...
    x = ...
    y = ...
    z = ...

    def __init__(self: pyrealsense2.quaternion) -> None:
        ...


class recorder:
    """
    Records the given device and saves it to the given file as rosbag format.
    """
    sensors = ...   # List of adjacent devices, sharing the same physical parent composite device.

    @overload
    def __init__(self: pyrealsense2.recorder, arg0: str, arg1: pyrealsense2.device) -> None:
        ...

    @overload
    def __init__(self: pyrealsense2.recorder, arg0: str, arg1: pyrealsense2.device, arg2: bool) -> None:
        ...

    def as_auto_calibrated_device():
        ...

    def as_debug_protocol():
        ...

    def as_playback():
        ...

    def as_recorder():
        ...

    def as_tm2(self: pyrealsense2.device) -> pyrealsense2.tm2:
        ...

    def as_updatable():
        ...

    def as_update_device():
        ...

    def first_color_sensor():
        ...

    def first_depth_sensor():
        ...

    def first_fisheye_sensor():
        ...

    def first_motion_sensor():
        ...

    def first_pose_sensor():
        ...

    def first_roi_sensor():
        ...

    def get_info(self: pyrealsense2.device):
        'Retrieve camera specific information, like versions of various internal components'
        ...

    def hardware_reset():
        'Send hardware reset request to the device'
        ...

    def is_auto_calibrated_device():
        ...

    def is_debug_protocol():
        ...

    def is_playback(self: pyrealsense2.device) -> bool:
        ...

    def is_recorder(self: pyrealsense2.device) -> bool:
        ...

    def is_tm2(self: pyrealsense2.device) -> bool:
        ...

    def is_updatable(self: pyrealsense2.device) -> bool:
        ...

    def is_update_device():
        ...

    def pause(self: pyrealsense2.recorder) -> None:
        'Pause the recording device without stopping the actual device from streaming.'
        ...

    def query_sensors():
        'Returns the list of adjacent devices, sharing the same physical parent composite device.'
        ...

    def resume(self: pyrealsense2.recorder) -> None:
        'Unpauses the recording device, making it resume recording.'
        ...

    def supports(self: pyrealsense2.device):
        'Check if specific camera info is supported.'
        ...


class region_of_interest:
    max_x = ...
    max_y = ...
    min_x = ...
    min_y = ...

    def __init__(self: pyrealsense2.region_of_interest) -> None:
        ...


class roi_sensor:
    profiles = ...  # The list of stream profiles supported by the sensor.

    def __init__(self: pyrealsense2.roi_sensor, sensor: pyrealsense2.sensor) -> None:
        ...

    def as_color_sensor():
        ...

    def as_depth_sensor():
        ...

    def as_fisheye_sensor():
        ...

    def as_motion_sensor():
        ...

    def as_pose_sensor():
        ...

    def as_roi_sensor():
        ...

    def as_wheel_odometer():
        ...

    def close(self: pyrealsense2.sensor) -> None:
        'Close sensor for exclusive access.'
        ...

    def get_info(self: pyrealsense2.sensor):
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_recommended_filters():
        'Return the recommended list of filters by the sensor.'
        ...

    def get_region_of_interest(self: pyrealsense2.roi_sensor) -> pyrealsense2.region_of_interest:
        ...

    def get_stream_profiles():
        'Retrieves the list of stream profiles supported by the sensor.'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def is_color_sensor():
        ...

    def is_depth_sensor():
        ...

    def is_fisheye_sensor():
        ...

    def is_motion_sensor():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_pose_sensor():
        ...

    def is_roi_sensor():
        ...

    def is_wheel_odometer():
        ...

    def open(*args, **kwargs):
        'Overloaded function.'
        ...

    def set_notifications_callback():
        'Register Notifications callback'
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def set_region_of_interest(self: pyrealsense2.roi_sensor, roi: pyrealsense2.region_of_interest) -> None:
        ...

    def start(*args, **kwargs):
        'Overloaded function.'
        ...

    def stop(self: pyrealsense2.sensor) -> None:	'Stop streaming.'
    ...

    def supports(*args, **kwargs):
        'Overloaded function.'
        ...


class rs400_advanced_mode:
    def __init__(self: pyrealsense2.rs400_advanced_mode, device: pyrealsense2.device) -> None:
        ...

    def get_ae_control(self: pyrealsense2.rs400_advanced_mode, mode: int = 0) -> pyrealsense2.STAEControl:
        ...

    def get_amp_factor(self: pyrealsense2.rs400_advanced_mode, mode: int = 0) -> pyrealsense2.STAFactor:
        ...

    def get_census(self: pyrealsense2.rs400_advanced_mode, mode: int = 0) -> pyrealsense2.STCensusRadius:
        ...

    def get_color_control(self: pyrealsense2.rs400_advanced_mode, mode: int = 0) -> pyrealsense2.STColorControl:
        ...

    def get_color_correction(self: pyrealsense2.rs400_advanced_mode, mode: int = 0) -> pyrealsense2.STColorCorrection:
        ...

    def get_depth_control(self: pyrealsense2.rs400_advanced_mode, mode: int = 0) -> pyrealsense2.STDepthControlGroup:
        ...

    def get_depth_table(self: pyrealsense2.rs400_advanced_mode, mode: int = 0) -> pyrealsense2.STDepthTableControl:
        ...

    def get_hdad(self: pyrealsense2.rs400_advanced_mode, mode: int = 0) -> pyrealsense2.STHdad:
        ...

    def get_rau_support_vector_control(self: pyrealsense2.rs400_advanced_mode, mode: int = 0) -> pyrealsense2.STRauSupportVectorControl:
        ...

    def get_rau_thresholds_control(self: pyrealsense2.rs400_advanced_mode, mode: int = 0) -> pyrealsense2.STRauColorThresholdsControl:
        ...

    def get_rsm(self: pyrealsense2.rs400_advanced_mode, mode: int = 0) -> pyrealsense2.STRsm:
        ...

    def get_slo_color_thresholds_control(self: pyrealsense2.rs400_advanced_mode, mode: int = 0) -> pyrealsense2.STSloColorThresholdsControl:
        ...

    def get_slo_penalty_control(self: pyrealsense2.rs400_advanced_mode, mode: int = 0) -> pyrealsense2.STSloPenaltyControl:
        ...

    def is_enabled(self: pyrealsense2.rs400_advanced_mode) -> bool:
        ...

    def load_json(self: pyrealsense2.rs400_advanced_mode, json_content: str) -> None:
        ...

    def serialize_json(self: pyrealsense2.rs400_advanced_mode) -> str:
        ...

    def set_ae_control(self: pyrealsense2.rs400_advanced_mode, group: pyrealsense2.STAEControl) -> None:
        ...

    def set_amp_factor(self: pyrealsense2.rs400_advanced_mode, group: pyrealsense2.STAFactor) -> None:
        ...

    def set_census(self: pyrealsense2.rs400_advanced_mode, group: pyrealsense2.STCensusRadius) -> None:
        ...

    def set_color_control(self: pyrealsense2.rs400_advanced_mode, group: pyrealsense2.STColorControl) -> None:
        ...

    def set_color_correction(self: pyrealsense2.rs400_advanced_mode, group: pyrealsense2.STColorCorrection) -> None:
        ...

    def set_depth_control(self: pyrealsense2.rs400_advanced_mode, group: pyrealsense2.STDepthControlGroup) -> None:
        ...

    def set_depth_table(self: pyrealsense2.rs400_advanced_mode, group: pyrealsense2.STDepthTableControl) -> None:
        ...

    def set_hdad(self: pyrealsense2.rs400_advanced_mode, group: pyrealsense2.STHdad) -> None:
        ...

    def set_rau_support_vector_control(self: pyrealsense2.rs400_advanced_mode, group: pyrealsense2.STRauSupportVectorControl) -> None:
        ...

    def set_rau_thresholds_control(self: pyrealsense2.rs400_advanced_mode, group: pyrealsense2.STRauColorThresholdsControl) -> None:
        ...

    def set_rsm(self: pyrealsense2.rs400_advanced_mode, arg0: pyrealsense2.STRsm) -> None:
        'group'
        ...

    def set_slo_color_thresholds_control(self: pyrealsense2.rs400_advanced_mode, group: pyrealsense2.STSloColorThresholdsControl) -> None:
        ...

    def set_slo_penalty_control(self: pyrealsense2.rs400_advanced_mode, group: pyrealsense2.STSloPenaltyControl) -> None:
        ...

    def toggle_advanced_mode(self: pyrealsense2.rs400_advanced_mode, enable: bool) -> None:
        ...


class save_single_frameset:

    def __init__(self: pyrealsense2.save_single_frameset, filename: str = 'RealSense Frameset ') -> None:
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class save_to_ply:

    def __init__(self: pyrealsense2.save_to_ply, filename: str = 'RealSense Pointcloud ', pc: pyrealsense2.pointcloud=...) -> None:
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class sensor:
    profiles = ...  # The list of stream profiles supported by the sensor. Identical to calling get_stream_profiles

    def __init__(self: pyrealsense2.sensor) -> None:
        ...

    def as_color_sensor(self: pyrealsense2.sensor) -> pyrealsense2.color_sensor:
        ...

    def as_depth_sensor(self: pyrealsense2.sensor) -> pyrealsense2.depth_sensor:
        ...

    def as_fisheye_sensor(self: pyrealsense2.sensor) -> pyrealsense2.fisheye_sensor:
        ...

    def as_motion_sensor(self: pyrealsense2.sensor) -> pyrealsense2.motion_sensor:
        ...

    def as_pose_sensor(self: pyrealsense2.sensor) -> pyrealsense2.pose_sensor:
        ...

    def as_roi_sensor(self: pyrealsense2.sensor) -> pyrealsense2.roi_sensor:
        ...

    def as_wheel_odometer(self: pyrealsense2.sensor) -> pyrealsense2.wheel_odometer:
        ...

    def close(self: pyrealsense2.sensor) -> None:
        'Close sensor for exclusive access.'
        ...

    def get_info(self: pyrealsense2.sensor, info: pyrealsense2.camera_info) -> str:
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_recommended_filters(self: pyrealsense2.sensor) -> List[pyrealsense2.filter]:
        'Return the recommended list of filters by the sensor.'
        ...

    def get_stream_profiles(self: pyrealsense2.sensor) -> List[pyrealsense2.stream_profile]:
        'Retrieves the list of stream profiles supported by the sensor.'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def is_color_sensor(self: pyrealsense2.sensor) -> bool:
        ...

    def is_depth_sensor(self: pyrealsense2.sensor) -> bool:
        ...

    def is_fisheye_sensor(self: pyrealsense2.sensor) -> bool:
        ...

    def is_motion_sensor(self: pyrealsense2.sensor) -> bool:
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_pose_sensor(self: pyrealsense2.sensor) -> bool:
        ...

    def is_roi_sensor(self: pyrealsense2.sensor) -> bool:
        ...

    def is_wheel_odometer(self: pyrealsense2.sensor) -> bool:
        ...

    @overload
    def open(self: pyrealsense2.sensor, profile: pyrealsense2.stream_profile) -> None:
        'Open sensor for exclusive access, by commiting to a configuration'
        ...

    @overload
    def open(self: pyrealsense2.sensor, profiles: List[pyrealsense2.stream_profile]) -> None:
        'Open sensor for exclusive access, by committing to a composite configuration, specifying one or more stream profiles.'
        ...

    def set_notifications_callback(self: pyrealsense2.sensor, callback: Callable[[pyrealsense2.notification], None]) -> None:
        'Register Notifications callback'
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    @overload
    def start(self: pyrealsense2.sensor, callback: Callable[[pyrealsense2.frame], None]) -> None:
        'Start passing frames into user provided callback.'
        ...

    @overload
    def start(self: pyrealsense2.sensor, syncer: pyrealsense2.syncer) -> None:
        'Start passing frames into user provided syncer.'
        ...

    @overload
    def start(self: pyrealsense2.sensor, queue: pyrealsense2.frame_queue) -> None:
        'start passing frames into specified frame_queue'
        ...

    def stop(self: pyrealsense2.sensor) -> None:
        'Stop streaming.'
        ...

    @overload
    def supports(self: pyrealsense2.sensor, arg0: pyrealsense2.camera_info) -> bool:
        'info'
        ...

    @overload
    def supports(self: pyrealsense2.sensor, arg0: pyrealsense2.option) -> bool:
        'info'
        ...


class spatial_filter:
    """
    Spatial filter smooths the image by calculating frame with alpha and delta settings. Alpha defines the weight of the current pixel for smoothing, and is bounded within [25..100]%. Delta defines the depth gradient below which the smoothing will occur as number of depth levels.
    """
    @overload
    def __init__(self: pyrealsense2.spatial_filter) -> None:
        ...

    @overload
    def __init__(self: pyrealsense2.spatial_filter, smooth_alpha: float, smooth_delta: float, magnitude: float, hole_fill: float) -> None:
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class stream:
    """
    Streams are different types of data provided by RealSense devices.
    """
    accel = stream.accel
    any = stream.any
    color = stream.color
    confidence = stream.confidence
    depth = stream.depth
    fisheye = stream.fisheye
    gpio = stream.gpio
    gyro = stream.gyro
    infrared = stream.infrared
    pose = stream.pose

    def __init__(self: pyrealsense2.stream) -> None:
        ...


class stream_profile:
    """
    Stores details about the profile of a stream.
    """

    def __init__(self: pyrealsense2.stream_profile) -> None:
        ...

    def as_motion_stream_profile(self: pyrealsense2.stream_profile) -> pyrealsense2.motion_stream_profile:
        ...

    def as_stream_profile(self: pyrealsense2.stream_profile) -> pyrealsense2.stream_profile:
        ...

    def as_video_stream_profile(self: pyrealsense2.stream_profile) -> pyrealsense2.video_stream_profile:
        ...

    def clone(self: pyrealsense2.stream_profile, type: pyrealsense2.stream, index: int, format: pyrealsense2.format) -> pyrealsense2.stream_profile:
        'Clone the current profile and change the type, index and format to input parameters'
        ...

    def format(self: pyrealsense2.stream_profile) -> pyrealsense2.format:
        'The stream\'s format'
        ...

    def fps(self: pyrealsense2.stream_profile) -> int:
        'The streams framerate'
        ...

    def get_extrinsics_to(self: pyrealsense2.stream_profile, to: pyrealsense2.stream_profile) -> pyrealsense2.extrinsics:
        'Get the extrinsic transformation between two profiles (representing physical sensors)'
        ...

    def is_default(self: pyrealsense2.stream_profile) -> bool:
        'Checks if the stream profile is marked/assigned as default, meaning that the profile will be selected when the user requests stream configuration using wildcards.'
        ...

    def is_motion_stream_profile(self: pyrealsense2.stream_profile) -> bool:
        ...

    def is_stream_profile(self: pyrealsense2.stream_profile) -> bool:
        ...

    def is_video_stream_profile(self: pyrealsense2.stream_profile) -> bool:
        ...

    def register_extrinsics_to(self: pyrealsense2.stream_profile, to: pyrealsense2.stream_profile, extrinsics: pyrealsense2.extrinsics) -> None:
        'Assign extrinsic transformation parameters to a specific profile (sensor). The extrinsic information is generally available as part of the camera calibration, and librealsense is responsible for retrieving and assigning these parameters where appropriate. This specific function is intended for synthetic/mock-up (software) devices for which the parameters are produced and injected by the user.'
        ...

    def stream_index(self: pyrealsense2.stream_profile) -> int:
        'The stream\'s index'
        ...

    def stream_name(self: pyrealsense2.stream_profile) -> int:
        'The stream\'s human-readable name.'
        ...

    def stream_type(self: pyrealsense2.stream_profile) -> pyrealsense2.stream:
        'The stream\'s type'
        ...

    def unique_id(self: pyrealsense2.stream_profile) -> int:
        'Unique index assigned whent the stream was created'
        ...


class syncer:
    """
    Sync instance to align frames from different streams
    """

    def __init__(self: pyrealsense2.syncer, queue_size: int = 1) -> None:
        ...

    def poll_for_frames(self: pyrealsense2.syncer) -> pyrealsense2.composite_frame:
        'Check if a coherent set of frames is available'
        ...

    def try_wait_for_frames(self: pyrealsense2.syncer, timeout_ms: int = 5000) -> Tuple[bool, pyrealsense2.composite_frame]:
        ...

    def wait_for_frames(self: pyrealsense2.syncer, timeout_ms: int = 5000) -> pyrealsense2.composite_frame:
        'Wait until a coherent set of frames becomes available'
        ...


class temporal_filter:
    """
    Temporal filter smooths the image by calculating multiple frames with alpha and delta settings. Alpha defines the weight of current frame, and delta defines thethreshold for edge classification and preserving.
    """

    @overload
    def __init__(self: pyrealsense2.temporal_filter) -> None:
        ...

    @overload
    def __init__(self: pyrealsense2.temporal_filter, smooth_alpha: float, smooth_delta: float, persistence_control: int) -> None:
        """
        Possible values for persistence_control: \n 
        1 - Valid in 8/8         - Persistency activated if the pixel was valid in 8 out of the last 8 frames \n
        2 - Valid in 2 / last 3  - Activated if the pixel was valid in two out of the last 3 frames \n 
        3 - Valid in 2 / last 4  - Activated if the pixel was valid in two out of the last 4 frames \n 
        4 - Valid in 2 / 8       - Activated if the pixel was valid in two out of the last 8 frames \n 
        5 - Valid in 1 / last 2  - Activated if the pixel was valid in one of the last two frames \n 
        6 - Valid in 1 / last 5  - Activated if the pixel was valid in one out of the last 5 frames \n 
        7 - Valid in 1 / last 8  - Activated if the pixel was valid in one out of the last 8 frames \n 
        8 - Persist Indefinitely - Persistency will be imposed regardless of the stored history(most aggressive filtering) \n
        """
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class texture_coordinate:
    u = ...
    v = ...

    @overload
    def __init__(self: pyrealsense2.texture_coordinate) -> None:
        ...

    @overload
    def __init__(self: pyrealsense2.texture_coordinate, arg0: float, arg1: float) -> None:
        ...


class threshold_filter:
    """
    Depth thresholding filter. By controlling min and max options on the block, one could filter out depth values that are either too large or too small, as a software post-processing step
    """

    def __init__(self: pyrealsense2.threshold_filter, min_dist: float = 0.15000000596046448, max_dist: float = 4.0) -> None:
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class timestamp_domain:
    """
    Specifies the clock in relation to which the frame timestamp was measured.
    """
    global_time = timestamp_domain.global_time
    hardware_clock = timestamp_domain.hardware_clock
    system_time = timestamp_domain.system_time

    def __init__(self: pyrealsense2.timestamp_domain, arg0: int) -> None:
        ...


class tm2:
    """
    The tm2 class is an interface for T2XX devices, such as T265.
    For T265, it provides RS2_STREAM_FISHEYE(2), RS2_STREAM_GYRO, RS2_STREAM_ACCEL, and RS2_STREAM_POSE streams, and contains the following sensors: 
        - pose_sensor: map and relocalization functions. 
        - wheel_odometer: input for odometry data.
    """
    sensors = ...   # List of adjacent devices, sharing the same physical parent composite device.

    def __init__(self: pyrealsense2.tm2, device: pyrealsense2.device) -> None:
        ...

    def as_auto_calibrated_device():
        ...

    def as_debug_protocol():
        ...

    def as_playback():
        ...

    def as_recorder():
        ...

    def as_tm2(self: pyrealsense2.device) -> pyrealsense2.tm2:
        ...

    def as_updatable():
        ...

    def as_update_device():
        ...

    def disable_loopback(self: pyrealsense2.tm2) -> None:
        'Restores the given device into normal operation mode'
        ...

    def enable_loopback(self: pyrealsense2.tm2, filename: str) -> None:
        'Enter the given device into loopback operation mode that uses the given file as input for raw data'
        ...

    def first_color_sensor():
        ...

    def first_depth_sensor():
        ...

    def first_fisheye_sensor():
        ...

    def first_motion_sensor():
        ...

    def first_pose_sensor():
        ...

    def first_roi_sensor():
        ...

    def get_info(self: pyrealsense2.device):
        'Retrieve camera specific information, like versions of various internal components'
        ...

    def hardware_reset():
        'Send hardware reset request to the device'
        ...

    def is_auto_calibrated_device():
        ...

    def is_debug_protocol():
        ...

    def is_loopback_enabled(self: pyrealsense2.tm2) -> bool:
        'Checks if the device is in loopback mode or not'
        ...

    def is_playback(self: pyrealsense2.device) -> bool:
        ...

    def is_recorder(self: pyrealsense2.device) -> bool:
        ...

    def is_tm2(self: pyrealsense2.device) -> bool:
        ...

    def is_updatable(self: pyrealsense2.device) -> bool:
        ...

    def is_update_device():
        ...

    def query_sensors():
        'Returns the list of adjacent devices, sharing the same physical parent composite device.'
        ...

    def reset_to_factory_calibration(self: pyrealsense2.tm2) -> None:
        'Reset to factory calibration'
        ...

    def set_extrinsics(self: pyrealsense2.tm2, from_stream: pyrealsense2.stream, from_id: int, to_stream: pyrealsense2.stream, to_id: int, extrinsics: pyrealsense2.extrinsics) -> None:
        'Set camera extrinsics'
        ...

    def set_intrinsics(self: pyrealsense2.tm2, sensor_id: int, intrinsics: pyrealsense2.intrinsics) -> None:
        'Set camera intrinsics'
        ...

    def set_motion_device_intrinsics(self: pyrealsense2.tm2, stream_type: pyrealsense2.stream, motion_intrinsics: pyrealsense2.motion_device_intrinsic) -> None:
        'Set motion device intrinsics'
        ...

    def supports(self: pyrealsense2.device):
        'Check if specific camera info is supported.'
        ...

    def write_calibration(self: pyrealsense2.tm2) -> None:
        'Write calibration to device\'s EEPROM'
        ...


class units_transform:

    def __init__(self: pyrealsense2.units_transform) -> None:
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class updatable:
    sensors = ...   # List of adjacent devices, sharing the same physical parent composite device.

    def __init__(self: pyrealsense2.updatable, arg0: pyrealsense2.device) -> None:
        ...

    def as_auto_calibrated_device():
        ...

    def as_debug_protocol():
        ...

    def as_playback():
        ...

    def as_recorder():
        ...

    def as_tm2(self: pyrealsense2.device) -> pyrealsense2.tm2:
        ...

    def as_updatable():
        ...

    def as_update_device():
        ...

    @overload
    def create_flash_backup(self: pyrealsense2.updatable) -> List[int]:
        'Create backup of camera flash memory. Such backup does not constitute valid firmware image, and cannot be loaded back to the device, but it does contain all calibration and device information.'
        ...

    @overload
    def create_flash_backup(self: pyrealsense2.updatable, callback: Callable[[float], None]) -> List[int]:
        'Create backup of camera flash memory. Such backup does not constitute valid firmware image, and cannot be loaded back to the device, but it does contain all calibration and device information.'
        ...

    def enter_update_state(self: pyrealsense2.updatable) -> None:
        'Move the device to update state, this will cause the updatable device to disconnect and reconnect as an update device.'
        ...

    def first_color_sensor():
        ...

    def first_depth_sensor():
        ...

    def first_fisheye_sensor():
        ...

    def first_motion_sensor():
        ...

    def first_pose_sensor():
        ...

    def first_roi_sensor():
        ...

    def get_info(self: pyrealsense2.device):
        'Retrieve camera specific information, like versions of various internal components'
        ...

    def hardware_reset():
        'Send hardware reset request to the device'
        ...

    def is_auto_calibrated_device():
        ...

    def is_debug_protocol():
        ...

    def is_playback(self: pyrealsense2.device) -> bool:
        ...

    def is_recorder(self: pyrealsense2.device) -> bool:
        ...

    def is_tm2(self: pyrealsense2.device) -> bool:
        ...

    def is_updatable(self: pyrealsense2.device) -> bool:
        ...

    def is_update_device():
        ...

    def query_sensors():
        'Returns the list of adjacent devices, sharing the same physical parent composite device.'
        ...

    def supports(self: pyrealsense2.device):
        'Check if specific camera info is supported.'
        ...

    @overload
    def update_unsigned(self: pyrealsense2.updatable, fw_image: List[int], update_mode: int = 0) -> None:
        'Update an updatable device to the provided unsigned firmware. This call is executed on the caller\'s thread.'
        ...

    @overload
    def update_unsigned(self: pyrealsense2.updatable, fw_image: List[int], callback: Callable[[float], None], update_mode: int = 0) -> None:
        'Update an updatable device to the provided unsigned firmware. This call is executed on the caller\'s thread and it supports progress notifications via the callback.'
        ...


class update_device:
    sensors = ...   # List of adjacent devices, sharing the same physical parent composite device.

    def __init__(self: pyrealsense2.update_device, arg0: pyrealsense2.device) -> None:
        ...

    def as_auto_calibrated_device():
        ...

    def as_debug_protocol():
        ...

    def as_playback():
        ...

    def as_recorder():
        ...

    def as_tm2(self: pyrealsense2.device) -> pyrealsense2.tm2:
        ...

    def as_updatable():
        ...

    def as_update_device():
        ...

    def first_color_sensor():
        ...

    def first_depth_sensor():
        ...

    def first_fisheye_sensor():
        ...

    def first_motion_sensor():
        ...

    def first_pose_sensor():
        ...

    def first_roi_sensor():
        ...

    def get_info(self: pyrealsense2.device):
        'Retrieve camera specific information, like versions of various internal components'
        ...

    def hardware_reset():
        'Send hardware reset request to the device'
        ...

    def is_auto_calibrated_device():
        ...

    def is_debug_protocol():
        ...

    def is_playback(self: pyrealsense2.device) -> bool:
        ...

    def is_recorder(self: pyrealsense2.device) -> bool:
        ...

    def is_tm2(self: pyrealsense2.device) -> bool:
        ...

    def is_updatable(self: pyrealsense2.device) -> bool:
        ...

    def is_update_device():
        ...

    def query_sensors():
        'Returns the list of adjacent devices, sharing the same physical parent composite device.'
        ...

    def supports(self: pyrealsense2.device):
        'Check if specific camera info is supported.'
        ...

    @overload
    def update(self: pyrealsense2.update_device, fw_image: List[int]) -> None:
        'Update an updatable device to the provided firmware. This call is executed on the caller\'s thread.'
        ...

    @overload
    def update(self: pyrealsense2.update_device, fw_image: List[int], callback: Callable[[float], None]) -> None:
        'Update an updatable device to the provided firmware. This call is executed on the caller\'s thread and it supports progress notifications via the callback.'
        ...


class vector:
    """
    3D vector in Euclidean coordinate space.
    """
    x = ...
    y = ...
    z = ...

    def __init__(self: pyrealsense2.vector) -> None:
        ...


class vertex:
    x = ...
    y = ...
    z = ...

    @overload
    def __init__(self: pyrealsense2.vertex) -> None:
        ...

    @overload
    def __init__(self: pyrealsense2.vertex, arg0: float, arg1: float, arg2: float) -> None:
        ...


class video_frame:
    """
    Extends the frame class with additional video related attributes and functions.
    """

    # Bits per pixel. Identical to calling get_bits_per_pixel.
    bits_per_pixel = ...
    # Bytes per pixel. Identical to calling get_bytes_per_pixel.
    bytes_per_pixel = ...
    data = ...                      # Data from the frame handle.
    frame_number = ...              # The frame number.
    frame_timestamp_domain = ...    # The timestamp domain.
    height = ...                    # Image height in pixels. Identical to calling get_height.
    profile = ...                   # Stream profile from frame handle.
    # Frame stride, meaning the actual line width in memory in bytes (not the logical image width). Identical to calling get_stride_in_bytes.
    stride_in_bytes = ...
    timestamp = ...                 # Time at which the frame was captured.
    width = ...                     # Image width in pixels. Identical to calling get_width.

    def __init__(self: pyrealsense2.video_frame, arg0: pyrealsense2.frame) -> None:
        ...

    def as_depth_frame():
        ...

    def as_frame():
        ...

    def as_frameset():
        ...

    def as_motion_frame():
        ...

    def as_points():
        ...

    def as_pose_frame():
        ...

    def as_video_frame():
        ...

    def get_bits_per_pixel(self: pyrealsense2.video_frame) -> int:
        'Retrieve bits per pixel.'
        ...

    def get_bytes_per_pixel(self: pyrealsense2.video_frame) -> int:
        'Retrieve bytes per pixel.'
        ...

    def get_data():
        'Retrieve data from the frame handle.'
        ...

    def get_data_size(self: pyrealsense2.frame) -> int:
        'Retrieve data size from frame handle.'
        ...

    def get_frame_metadata():
        'Retrieve the current value of a single frame_metadata.'
        ...

    def get_frame_number():
        'Retrieve the frame number.'
        ...

    def get_frame_timestamp_domain():
        'Retrieve the timestamp domain.'
        ...

    def get_height(self: pyrealsense2.video_frame) -> int:
        'Returns image height in pixels.'
        ...

    def get_profile():
        'Retrieve stream profile from frame handle.'
        ...

    def get_stride_in_bytes(self: pyrealsense2.video_frame) -> int:
        'Retrieve frame stride, meaning the actual line width in memory in bytes (not the logical image width).'
        ...

    def get_timestamp():
        'Retrieve the time at which the frame was captured'
        ...

    def get_width(self: pyrealsense2.video_frame) -> int:
        'Returns image width in pixels.'
        ...

    def is_depth_frame():
        ...

    def is_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_frameset(self: pyrealsense2.frame) -> bool:
        ...

    def is_motion_frame():
        ...

    def is_points(self: pyrealsense2.frame) -> bool:
        ...

    def is_pose_frame(self: pyrealsense2.frame) -> bool:
        ...

    def is_video_frame():
        ...

    def keep(self: pyrealsense2.frame) -> None:
        'Keep the frame, otherwise if no refernce to the frame, the frame will be released.'
        ...

    def supports_frame_metadata():
        'Determine if the device allows a specific metadata to be queried.'
        ...

    def swap(self: pyrealsense2.frame):
        'Swap the internal frame handle with the one in parameter'
        ...


class video_stream_profile:
    """
    Stream profile instance which contains additional video attributes.
    """
    intrinsics = ...    # Stream profile instrinsics attributes. Identical to calling get_intrinsics.

    def __init__(self: pyrealsense2.video_stream_profile, sp: pyrealsense2.stream_profile) -> None:
        ...

    def as_motion_stream_profile():
        ...

    def as_stream_profile():
        ...

    def as_video_stream_profile():
        ...

    def clone(self: pyrealsense2.stream_profile):
        'Clone the current profile and change the type, index and format to input parameters'
        ...

    def format():
        'The stream\'s format'
        ...

    def fps(self: pyrealsense2.stream_profile) -> int:
        'The streams framerate'
        ...

    def get_extrinsics_to():
        'Get the extrinsic transformation between two profiles (representing physical sensors)'
        ...

    def get_intrinsics(self: pyrealsense2.video_stream_profile) -> pyrealsense2.intrinsics:
        'Get stream profile instrinsics attributes.'
        ...

    def height(self: pyrealsense2.video_stream_profile) -> int:
        ...

    def is_default():
        'Checks if the stream profile is marked/assigned as default, meaning that the profile will be selected when the user requests stream configuration using wildcards.'
        ...

    def is_motion_stream_profile():
        ...

    def is_stream_profile():
        ...

    def is_video_stream_profile():
        ...

    def register_extrinsics_to():
        'Assign extrinsic transformation parameters to a specific profile (sensor).'
        ...

    def stream_index():
        'The stream\'s index'
        ...

    def stream_name():
        'The stream\'s human-readable name.'
        ...

    def stream_type():
        'The stream\'s type'
        ...

    def unique_id():
        'Unique index assigned whent the stream was created'
        ...

    def width(self: pyrealsense2.video_stream_profile) -> int:
        ...


class wheel_odometer:

    profiles = ...  # The list of stream profiles supported by the sensor.

    def __init__(self: pyrealsense2.wheel_odometer, sensor: pyrealsense2.sensor) -> None:
        ...

    def as_color_sensor():
        ...

    def as_depth_sensor():
        ...

    def as_fisheye_sensor():
        ...

    def as_motion_sensor():
        ...

    def as_pose_sensor():
        ...

    def as_roi_sensor():
        ...

    def as_wheel_odometer():
        ...

    def close(self: pyrealsense2.sensor) -> None:
        'Close sensor for exclusive access.'
        ...

    def get_info(self: pyrealsense2.sensor):
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_recommended_filters():
        'Return the recommended list of filters by the sensor.'
        ...

    def get_stream_profiles():
        'Retrieves the list of stream profiles supported by the sensor.'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def is_color_sensor():
        ...

    def is_depth_sensor():
        ...

    def is_fisheye_sensor():
        ...

    def is_motion_sensor():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_pose_sensor():
        ...

    def is_roi_sensor():
        ...

    def is_wheel_odometer():
        ...

    def load_wheel_odometery_config(self: pyrealsense2.wheel_odometer, odometry_config_buf: List[int]) -> bool:
        'Load Wheel odometer settings from host to device.'
        ...

    def open(*args, **kwargs):
        'Overloaded function.'
        ...

    def send_wheel_odometry(self: pyrealsense2.wheel_odometer, wo_sensor_id: int, frame_num: int, translational_velocity: pyrealsense2.vector) -> bool:
        'Send wheel odometry data for each individual sensor (wheel)'
        ...

    def set_notifications_callback():
        'Register Notifications callback'
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(*args, **kwargs):
        'Overloaded function.'
        ...

    def stop(self: pyrealsense2.sensor) -> None:
        'Stop streaming.'
        ...

    def supports(*args, **kwargs):
        'Overloaded function.'
        ...


class yuy_decoder:
    """
    Converts frames in raw YUY format to RGB. This conversion is somewhat costly, but the SDK will automatically try to use SSE2, AVX, or CUDA instructions where available to get better performance. Othere implementations (GLSL, OpenCL, Neon, NCS) should follow.
    """

    def __init__(self: pyrealsense2.yuy_decoder) -> None:
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...


class zero_order_invalidation:
    """
    Fixes the zero order artifact
    """

    def __init__(self: pyrealsense2.zero_order_invalidation) -> None:
        ...

    def as_decimation_filter():
        ...

    def as_depth_huffman_decoder():
        ...

    def as_disparity_transform():
        ...

    def as_hole_filling_filter():
        ...

    def as_spatial_filter():
        ...

    def as_temporal_filter():
        ...

    def as_threshold_filter():
        ...

    def as_zero_order_invalidation():
        ...

    def get_info():
        'Retrieve camera specific information, like versions of various internal components.'
        ...

    def get_option(self: pyrealsense2.options):
        'Read option value from the device.'
        ...

    def get_option_description():
        'Get option description.'
        ...

    def get_option_range():
        'Retrieve the available range of values of a supported option'
        ...

    def get_option_value_description():
        'Get option value description (In case a specific option value holds special meaning)'
        ...

    def get_supported_options():
        'Retrieve list of supported options'
        ...

    def invoke(self: pyrealsense2.processing_block):
        'Ask processing block to process the frame'
        ...

    def is_decimation_filter():
        ...

    def is_depth_huffman_decoder():
        ...

    def is_disparity_transform():
        ...

    def is_hole_filling_filter():
        ...

    def is_option_read_only():
        'Check if particular option is read only.'
        ...

    def is_spatial_filter():
        ...

    def is_temporal_filter():
        ...

    def is_threshold_filter():
        ...

    def is_zero_order_invalidation():
        ...

    def process():
        ...

    def set_option(self: pyrealsense2.options):
        'Write new value to device option'
        ...

    def start(self: pyrealsense2.processing_block):
        'Start the processing block with callback function to inform the application the frame is processed.'
        ...

    def supports():
        'Check if a specific camera info field is supported.'
        ...

