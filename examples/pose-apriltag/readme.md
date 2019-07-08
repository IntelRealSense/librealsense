# rs-pose-apriltag Sample

In order to run this example, a T265 is required.

## Overview

[Apriltags](https://april.eecs.umich.edu/software/apriltag) are fiducial markers that have been designed to be easily detected in images (other fiducial markers include QR codes and ARTags). Once detected, these markers can be used for a variety of applications.

This sample demonstrates how to detect the pose of a Apriltag of a known size that is visible in a T265 fisheye frame relative to the camera. Apriltags are detected on every 6th frame (at 5Hz). The sample uses `36h11` tags, though it can be easily modified to work with any tag supported by the Apriltag library.

## Dependency

This sample requires the [Apriltag library 3.1.1](https://github.com/AprilRobotics/apriltag/tree/3.1.1)  installed in the default location suggested by the library or a path specified in cmake variable `CMAKE_PREFIX_PATH`.

### Installing Apriltag library in a local directory

First, download [Apriltag library 3.1.1](https://github.com/AprilRobotics/apriltag/tree/3.1.1) and unpack it in a local directory, here we use `~/apriltag-3.1.1` and install it in `~/apriltag`:

```
tar -xzf apriltag-3.1.1.tar.gz
mkdir apriltag-build
cd apriltag-build
cmake -DCMAKE_INSTALL_PREFIX=~/apriltag/ ../apriltag-3.1.1
make install
```

Next, switch to your librealsense build directory and add `-DCMAKE_PREFIX_PATH` to your cmake like to tell librealsense where to find the Apriltag library:

```
cmake <librealsense_root_path> -DCMAKE_PREFIX_PATH=~/apriltag
```

Build as you normally would, and pose-apriltag should find the
Apriltag library and build.

## Expected Output

The application will calculate and print the detected Apriltag pose relative to the camera.

## Code Overview

We start by configuring the pipeline to pose stream, similar to `rs-pose-predict`:
```cpp
// Declare RealSense pipeline, encapsulating the actual device and sensors
rs2::pipeline pipe;
// Create a configuration for configuring the pipeline with a non default profile
rs2::config cfg;
// Add pose stream
cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);
// Enable both image streams
// Note: It is not currently possible to enable only one
cfg.enable_stream(RS2_STREAM_FISHEYE, 1, RS2_FORMAT_Y8);
cfg.enable_stream(RS2_STREAM_FISHEYE, 2, RS2_FORMAT_Y8);
```

In each iteration, while the application is alive, we wait for new frames from the camera. From the frameset that arrives we get the fisheye frame.
```cpp
// Main loop
while (true)
{
    // Wait for the next set of frames from the camera
    auto frames        = pipe.wait_for_frames();
    auto fisheye_frame = frames.get_fisheye_frame(fisheye_sensor_idx);
```

For every 6th fisheye frame, we run Apriltag detection on that frame and compute the relative pose of the detected tag.
The detection will be done on the separate thread so we need to keep the fisheye frame from the main loop until the detection is completed.
```cpp
    if(frame_number % 6 == 0)
    {
        fisheye_frame.keep();
        std::async(std::launch::async, [img=fisheye_frame,fn=frame_number,pose=camera_pose,&tag_manager](){
            auto tags = tag_manager.detect((unsigned char*)img.get_data(), &pose);
```

For each detected Apriltag, we print out the Apriltag pose relative to the fisheye lens as well as the Apriltag pose in the same coordinate of the regular T265 output pose coordinate.
```cpp
            for(int t=0; t<tags.pose_in_camera.size(); ++t){
                std::stringstream ss; ss << "frame " << fn << "|tag id: " << tags.get_id(t) << "|";
                std::cout << ss.str() << "camera " << print(tags.pose_in_camera[t]) << std::endl;
                std::cout << std::setw(ss.str().size()) << " " << "world  "<< print(tags.pose_in_world[t]) << std::endl << std::endl;
            }
        });
    }
}
```
