# API Architecture

RealSense APIs are clustered into three groups. Choose the API best suited to your needs:

1. [High-Level Computer-Vision Pipeline API](#high-level-pipeline-api)  
The Pipeline API configures the Intel® RealSense™ device with the best recommended settings and manages hardware resources and threading.
Use this API when you require the Camera to work but do not need to fine tune functionality. Recommended for **Researchers** and **App Developers**.

2. [Processing Blocks API](#processing-blocks-api)  
The Processing Blocks API enables you to take control of threading, spatial & temporal synchronization and memory management. This API provides a set of building blocks that you can easily arrange into custom processing flows. Recommended for **Game Developers**.

3. [Low-Level Sensor Access API](#low-level-sensor-api)  
 The Low Level Sensor Access API enables you to take direct control of the individual device sensors. Recommended for **Framework and Tool Developers** as well as Developers in emerging fields like **VR/AR**.

## High-Level Pipeline API

The High-Level Pipeline API provides a **middleware** toolkit to interface with the camera.

Each **middleware** is set up with the best configuration for its use-case and the processing that needs to be done.

You can use the `pipeline` class to run one or more middleware program(s).  
The `pipeline` makes sure all synchronization and alignment requirements are met and takes care of threading and resource management.  

## Processing Blocks API

The Processing Block API provides the following tools to control threading, spatial & temporal synchronization and memory management:

* **Configure** the device for streaming using the `config` class
* **Marshal** frames from the OS callback thread to the main application using the safe `queue` implementation
* **Synchronize** any set of different asynchronous streams with respect to hardware timestamps using the `syncer` class
* **Align** streams to a common viewport, using the `align` class. You can also use your own calibration data to align devices that were not otherwise calibrated.
* **Project** depth data into the 3D space using the `pointcloud` class. 

 Assembling these components lets you define a custom processing pipeline best suited to your needs.

## Low-Level Sensor API
Intel RealSense™ devices use sensors, some commonplace like a regular RGB camera and some more exotic like the RS400 Stereo module or SR300 Structured-Light sensor:
<p align="center"><img src="img/sensors_within_device.png" width="400" height="200" /></p>

The Low-Level Sensor API provides you with the means to take direct control of the individual device sensors.  
* Each sensor has its own power management and control.  
* Standard sensors pass UVC / HID compliance and can be used without custom drivers.
* Different sensors can be safely used from different applications and can only influence each other indirectly.
* Each sensor can offer one or more streams. Streams must be configured together and are usually dependent on each other. For example, RS400 depth stream depends on infrared data, so the streams must be configured with a single resolution.
* All sensors provide streaming (not necessarily of images) but each individual sensor can be extended to offer additional functionality. For example, most video devices let the user configure a custom region of interest for the auto-exposure mechanism.

Intel RealSense™ RS400 stereo module offers **Advanced Mode** functionality, letting you control the various ASIC registers responsible for depth generation.  

The user of the sensor API provides a callback to be invoked whenever new data frames become available. This callback runs immediately on the OS thread providing the best possible latency.