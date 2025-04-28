from time import time
from .pyrealsense_mock import context, create_mock_device

# # Global variables for frame generation
# fps = 30
# w = 640
# h = 480
# bpp = 2  # bytes
# pixels = bytearray(b'\x69' * (w * h * bpp))  # Dummy data
# domain = timestamp_domain.hardware_clock     # For either depth/color

# global_frame_number = 0


# class Sensor:
#     def __init__(self, sensor_name:str, dev:Device = None):
#         if dev is None:
#             dev = Device()
#         self._handle = dev._handle.add_sensor(sensor_name)
#         self._q = None

#     def __enter__(self):
#         return self

#     def __exit__(self, *args):
#         self.stop()

#     def video_stream(self, stream_name:str, type:str, format:str):
#         return Video_stream(self, stream_name, type, format)

#     def start(self, *streams):
#         """
#         Start streaming from the sensor with the given streams.
#         """
#         if self._q is not None:
#             raise RuntimeError('already started')
#         self._profiles = []
#         self._profiles_str = []
#         for stream in streams:
#             stream._profile = video_stream_profile(self._handle.add_video_stream(stream._handle))
#             self._profiles.append(stream._profile)
#             self._profiles_str.append(str(stream._profile))
#         self._q = frame_queue(100)
#         self._handle.open(self._profiles)
#         self._handle.start(self._q)

#     def stop(self):
#         """
#         Stop streaming from the sensor.
#         """
#         if self._q is not None:
#             self._handle.stop()
#             self._handle.close()
#             del self._q

#     def set(self, key, value):
#         self._handle.set_metadata(key, value)

#     def add_option(self, option, option_range, writable=True):
#         self._handle.add_option(option, option_range, writable)

#     def supports(self, option):
#         return self._handle.supports(option)

#     def get_option(self, option):
#         return self._handle.get_option(option)

#     def set_option(self, option, value):
#         self._handle.set_option(option, value)

#     def publish(self, frame):
#         if str(frame.profile) not in self._profiles_str:
#             raise RuntimeError(f'frame {frame.profile} is not part of sensor {self._profiles}')
#         self._handle.on_video_frame(frame)
#         received_frame = self.receive()
#         return received_frame

#     def receive(self):
#         """
#         Looks at the syncer queue and gets the next frame from it if available, checking its contents
#         against the expected frame numbers.
#         """
#         f = self._q.poll_for_frame()
#         # NOTE: f will never be None
#         if not f:
#             raise RuntimeError('expected a frame but got none')
#         return f


# class Video_stream:
#     def __init__(self, sensor:Sensor, stream_name:str, type:str, format:str):
#         self._sensor = sensor
#         self._handle = video_stream()
#         self._handle.type = type
#         self._handle.uid = 0
#         self._handle.width = w
#         self._handle.height = h
#         self._handle.bpp = bpp
#         self._handle.fmt = format
#         self._handle.fps = fps

#     def frame(self, frame_number=None, timestamp=None):
#         f = software_video_frame()
#         f.pixels = pixels
#         f.stride = w * bpp
#         f.bpp = bpp
#         if frame_number is not None:
#             f.frame_number = frame_number
#         else:
#             global global_frame_number
#             global_frame_number += 1
#             f.frame_number = global_frame_number
#         f.timestamp = timestamp or time()
#         f.domain = domain
#         f.profile = self._profile
#         return f


def setup_fake_devices():
    """
    Creates a set of fake RealSense devices for testing.
    Returns a dictionary containing all the mock objects.
    """
    # Create a mock context
    mock_context = context()

    # Create two fake devices with depth and color sensors
    fake_device1 = create_mock_device(
        "device1", "Test Device 1", with_depth=True, with_color=True
    )
    fake_device2 = create_mock_device(
        "device2", "Test Device 2", with_depth=True, with_color=True
    )

    # Add the devices to the context
    mock_context.add_device(fake_device1)
    mock_context.add_device(fake_device2)

    # Extract sensors for easy access in tests
    depth_sensors = []
    color_sensors = []

    # Collect sensors from device 1
    for i, sensor in enumerate(fake_device1.sensors):
        if sensor.is_depth_sensor():
            depth_sensors.append(sensor)
        elif sensor.is_color_sensor():
            color_sensors.append(sensor)

    # Collect sensors from device 2
    for i, sensor in enumerate(fake_device2.sensors):
        if sensor.is_depth_sensor():
            depth_sensors.append(sensor)
        elif sensor.is_color_sensor():
            color_sensors.append(sensor)

    # Return all mock objects in a dictionary
    return {
        "context": mock_context,
        "devices": [fake_device1, fake_device2],
        "depth_sensors": depth_sensors,
        "color_sensors": color_sensors,
    }
