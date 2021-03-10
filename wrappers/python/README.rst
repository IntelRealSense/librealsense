Python Wrapper for Intel RealSense SDK 2.0
==========================================

The python wrapper for Intel RealSense SDK 2.0 provides the C++ to Python binding required to access the SDK.

Quick start
-----------

::

  import pyrealsense2 as rs
  pipe = rs.pipeline()
  profile = pipe.start()
  try:
    for i in range(0, 100):
      frames = pipe.wait_for_frames()
      for f in frames:
        print(f.profile)
  finally:
      pipe.stop()
