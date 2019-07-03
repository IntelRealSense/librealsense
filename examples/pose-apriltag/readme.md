# rs-pose-apriltag Sample

In order to run this example, a T265 is required.

## Overview

This sample demonstrates streaming image data at 30Hz using an synchronous pipeline and running Apriltag detection on the image data on a separate thread. 
The Apriltag detection is done on a lower rate than the image streaming rate.

## Expected Output

The application will calculate and print the detected apriltag pose relative to the camera. 

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

For every 4th fisheye frame arrived, we run Apriltag detection on that fisheye frame and compute the relative pose of the detected tag.
The detection will be done on the separate thread so we need to keep the fisheye frame from the main loop until the detection is completed.
```cpp
    if(frame_number % 4 == 0)
    {
        fisheye_frame.keep();
        std::async(std::launch::async, [img=fisheye_frame,fn=frame_number,pose=camera_pose,&tag_manager](){
            auto tags = tag_manager.detect((unsigned char*)img.get_data(), &pose);
```

For each detected Apriltag, print out the Apriltag pose relative to the fisheye lens as well as the Apriltag pose in the same coordinate of the regular T265 output pose coordinate.
```
            for(int t=0; t<tags.pose_in_camera.size(); ++t){
                std::stringstream ss; ss << "frame " << fn << "|tag id: " << tags.get_id(t) << "|";
                std::cout << ss.str() << "camera " << print(tags.pose_in_camera[t]) << std::endl;
                std::cout << std::setw(ss.str().size()) << " " << "world  "<< print(tags.pose_in_world[t]) << std::endl << std::endl;
            }
        });
    }
}
```
