# API Architecture

RealSense APIs is clustered into three categories, aimed at different usages: 
1. [High-Level Computer-Vision Pipeline API](#high-level-pipeline-api) - If you are getting started with RealSense and want do build something that would just work, Pipeline API will configure the device with the best recommended settings and will manage hardware resources as well as threading on your behalf. Recommended for **Researchers** and **App Developers**. 
2. [Processig Building-Blocks](#processing-blocks) - If you need to take control over threading, spatial and temporal syncronization and memory management, we provide a set of building blocks you can easily arrange into custom processing flows. Recommended for **Game Developers**.
3. [Low-Level Sensor Access](#low-level-sensor-api) - If you know what you are doing you can take direct control over the individual sensors composing the device to get the maximum out of the hardware. Recommended for **Framework and Tools Developers**, as well as Developers in emerging feilds like **VR/AR**. 

## Low-Level Sensor API
RealSense devices are composed from sensors. Some are commonplace, like a regular RGB camera, some are more exotic, like RS400 Stereo module or SR300 Structured-Light sensor:

Each sensor has its own power management and control. Standard sensors pass UVC / HID compliance and can be used without custom drivers. 

Different sensors can be safely used from different applications and can only influence each other indirectly. 

Each sensor can offer one or more streams. Streams must be configured together and are usually dependent on each other. For example, RS400 depth stream depends on infrared data, so they must be configured together with a single resolution.

All sensors provide streaming (not nesseserily of images) but each individual sensor can be extended to offer additional functionality. For example, most video devices can let the user configure custom region of interest for the auto-exposure mechanism.

RS400 stereo module offers **Advanced Mode** functionality, letting you control the various ASIC registers responsible for depth generation.  

The user of sensor API provides a callback to be invoked whenever new data frame becomes avaialable. This callback runs immediately on the OS thread providing best possible latency. 

* TODO: Sensor enumeration code snippet
* TODO: Sensor streaming code snippet
* TODO: Link to sensor class
* TODO: Advanced Mode code snippet

## Processing Blocks

* If you are not interested in the individual sensors of the device, you can **configure** the device for streaming using `config` class. 
* To **marshal** frames from OS callback thread to the main application we provide safe `queue` implementation. 
* If you wish to **syncronize** any set of different asynchronous streams with respect to hardware timestamps, we offer `syncer` class. 
* If you need to **align** streams to a single viewport, we offer `TODO` class for that. You can also use your own calibration data to align otherwise uncalibrated devices. 
* If you need to perfom **processing** on the frame while benefiting from librealsense memory pooling, we offer `processing` class for that. 

Putting all these together lets you define your own custom processing pipeline, best suited for your needs. 

## High-Level Pipeline API

To further simplify working with the camera, we introduce the concept of middlewares. 

Each middleware knows the best configuration for its use-case and what processing needs to be done. 

You can use the `pipeline` class to run one or more middlewares. The pipeline will make sure all syncronization and alignment requirements are met and will take care of threading and resource management. 

* TODO: Link to background removal
* TODO: Link to measurement tool?
* TODO: Code snippet with pipeline
