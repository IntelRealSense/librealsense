import pyrealsense2
from typing import overload, List, Tuple, Callable

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


class pipeline:
    def __init__(self: pyrealsense2.pipeline, ctx: pyrealsense2.context = ...) -> None:
        'The caller can provide a context created by the application, usually for playback or testing purposes.'
        ...

    def get_active_profile(self: pyrealsense2.pipeline) -> pyrealsense2.pipeline_profile:
        ...

    def poll_for_frames(self: pyrealsense2.pipeline) -> pyrealsense2.composite_frame:
        'Check if a new set of frames is available and retrieve the latest undelivered set. The frames set includes time-synchronized frames of each enabled stream in the pipeline. The method returns without blocking the calling thread, with status of new frames available or not. If available, it fetches the latest frames set. Device frames, which were produced while the function wasn\'t called, are dropped. To avoid frame drops, this method should be called as fast as the device frame rate. The application can maintain the frames handles to defer processing. However, if the application maintains too long history, the device may lack memory resources to produce new frames, and the following calls to this method shall return no new frames, until resources become available.'
        ...

    # TODO: Find a way to declare variable 'callback' so that following requirement is met: 'callback: Callable[[pyrealsense2.frame], None]'
    @overload
    def start(self: pyrealsense2.pipeline) -> pyrealsense2.pipeline_profile:
        'Start the pipeline streaming with its default configuration. The pipeline streaming loop captures samples from the device, and delivers them to the attached computer vision modules and processing blocks, according to each module requirements and threading model. During the loop execution, the application can access the camera streams by calling wait_for_frames() or poll_for_frames(). The streaming loop runs until the pipeline is stopped. Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised.'
        ...

    @overload
    def start(self, config: pyrealsense2.config) -> pyrealsense2.pipeline_profile:
        'Start the pipeline streaming according to the configuraion. The pipeline streaming loop captures samples from the device, and delivers them to the attached computer vision modules and processing blocks, according to each module requirements and threading model. During the loop execution, the application can access the camera streams by calling wait_for_frames() or poll_for_frames(). The streaming loop runs until the pipeline is stopped. Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised. The pipeline selects and activates the device upon start, according to configuration or a default configuration. When the rs2::config is provided to the method, the pipeline tries to activate the config resolve() result. If the application requests are conflicting with pipeline computer vision modules or no matching device is available on the platform, the method fails. Available configurations and devices may change between config resolve() call and pipeline start, in case devices are connected or disconnected, or another application acquires ownership of a device.'
        ...

    @overload
    def start(self: pyrealsense2.pipeline, callback: Callable[[pyrealsense2.frame], None]) -> pyrealsense2.pipeline_profile:
        'Start the pipeline streaming with its default configuration. The pipeline captures samples from the device, and delivers them to the provided frame callback. Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised. When starting the pipeline with a callback both wait_for_frames() and poll_for_frames() will throw exception.'
        ...

    @overload
    def start(self: pyrealsense2.pipeline, config: pyrealsense2.config, callback: Callable[[pyrealsense2.frame], None]) -> pyrealsense2.pipeline_profile:
        'Start the pipeline streaming according to the configuraion. The pipeline captures samples from the device, and delivers them to the provided frame callback. Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised. When starting the pipeline with a callback both wait_for_frames() and poll_for_frames() will throw exception. The pipeline selects and activates the device upon start, according to configuration or a default configuration. When the rs2::config is provided to the method, the pipeline tries to activate the config resolve() result. If the application requests are conflicting with pipeline computer vision modules or no matching device is available on the platform, the method fails. Available configurations and devices may change between config resolve() call and pipeline start, in case devices are connected or disconnected, or another application acquires ownership of a device.'
        ...

    @overload
    def start(self: pyrealsense2.pipeline, queue: pyrealsense2.frame_queue) -> pyrealsense2.pipeline_profile:
        'Start the pipeline streaming with its default configuration. The pipeline captures samples from the device, and delivers them to the provided frame queue. Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised. When starting the pipeline with a callback both wait_for_frames() and poll_for_frames() will throw exception.'
        ...

    @overload
    def start(self: pyrealsense2.pipeline, config: pyrealsense2.config, queue: pyrealsense2.frame_queue) -> pyrealsense2.pipeline_profile:
        'Start the pipeline streaming according to the configuraion. The pipeline captures samples from the device, and delivers them to the provided frame queue. Starting the pipeline is possible only when it is not started. If the pipeline was started, an exception is raised. When starting the pipeline with a callback both wait_for_frames() and poll_for_frames() will throw exception. The pipeline selects and activates the device upon start, according to configuration or a default configuration. When the rs2::config is provided to the method, the pipeline tries to activate the config resolve() result. If the application requests are conflicting with pipeline computer vision modules or no matching device is available on the platform, the method fails. Available configurations and devices may change between config resolve() call and pipeline start, in case devices are connected or disconnected, or another application acquires ownership of a device.'
        ...

    def stop(self: pyrealsense2.pipeline) -> None:
        'Stop the pipeline streaming. The pipeline stops delivering samples to the attached computer vision modules and processing blocks, stops the device streaming and releases the device resources used by the pipeline. It is the application\'s responsibility to release any frame reference it owns. The method takes effect only after start() was called, otherwise an exception is raised.'
        ...

    def try_wait_for_frames(self: pyrealsense2.pipeline, timeout_ms: int = 5000) -> Tuple[bool, pyrealsense2.composite_frame]:
        ...

    def wait_for_frames(self: pyrealsense2.pipeline, timeout_ms: int = 5000) -> pyrealsense2.composite_frame:
        ...


class pipeline_profile:
    def __init__(self) -> None:
        ...

    def get_device(self) -> pyrealsense2.device:
        'Retrieve the device used by the pipeline. The device class provides the application access to control camera additional settings - get device information, sensor options information, options value query and set, sensor specific extensions. Since the pipeline controls the device streams configuration, activation state and frames reading, calling the device API functions, which execute those operations, results in unexpected behavior. The pipeline streaming device is selected during pipeline start(). Devices of profiles, which are not returned by pipeline start() or get_active_profile(), are not guaranteed to be used by the pipeline.'
        ...

    def get_device(self, stream_type: pyrealsense2.stream, stream_index: int = -1) -> pyrealsense2.stream_profile:
        'Return the stream profile that is enabled for the specified stream in this profile.'
        ...

    def get_device(self) -> List[pyrealsense2.stream_profile]:
        'Return the selected streams profiles, which are enabled in this profile.'
        ...


class Dummy:
    def __init__(self) -> None:
        ''
        ...
