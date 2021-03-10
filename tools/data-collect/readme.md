# rs-data-collect Tool

## Goal

This tool is designed to help collect statistics about various streams. For full data capture, you can use [recorder](../recorder/) or [realsense-viewer](../realsense-viewer).

## Description
Given a configuration file the tool will configure the first Realsense device recognized to the desired streams and wait for either set number of frames or a timeout.  
The tool uses `best-effort` to satisfy the maximum of the user-requested profiles. Configuration that are either not supported or ill-formatted will be reported.
The tools is designed as a low-latency profiler with zero frames drops, and therefore utilizes the Librealsense low-level sensor API with callbacks.
For each frame, timestamp, frame-number, time of arrival, stream profile, index and timestamp domain will be recorded into the output log. For the tracking-supported devices (D435i/T260) the motion data is recorded as well  
**The tool does not record the content of video frames**


## Command Line Parameters

|Flag   |Description   |Default|
|---|---|---|
|`-c <filename>`|Load stream configuration from <filename>||
|`-m X`|Stop the test after receiving at least X frames|100|
|`-t X`|Stop the test after X seconds|10|
|`-f <filename>`|Save results into <filename>||

For example:  
`rs-data-collect -c ./data_collect.cfg -f ./log.csv -t 60 -m 1000`  
will apply streaming configuration from `./data_collect.cfg`to, then stream and collect the data for 60 seconds or 1000 frames (whatever comes first).
The resulted data will be saved into `./log.csv` file.

### Config File Format
```
STREAM1,WIDTH1,HEIGHT1,FPS1,FORMAT1,STREAM_INDEX1
STREAM2,WIDTH2,HEIGHT2,FPS2,FORMAT2,STREAM_INDEX1
...
```
Practical examples:
D415/D435  
```
DEPTH,640,480,30,Z16,0
INFRARED,640,480,30,Y8,1
INFRARED,640,480,30,Y8,2
COLOR,640,480,30,RGB8,0
```  

D435i  
```
#Video streams
DEPTH,640,480,30,Z16,0
INFRARED,640,480,30,Y8,1
INFRARED,640,480,30,Y8,2
COLOR,640,480,30,RGB8,0
#Lines starting with non-alpha character (#@!...) will be skipped
# IMU streams will produce data in m/sec^2 & rad/sec
ACCEL,1,1,63,MOTION_XYZ32F
GYRO,1,1,200,MOTION_XYZ32F
```

T265
```
ACCEL,1,1,62,MOTION_XYZ32F
GYRO,1,1,200,MOTION_XYZ32F
FISHEYE,848,800,30,Y8,1
FISHEYE,848,800,30,Y8,2
POSE,0,0,200,6DOF,0
```

SR300  
```
DEPTH,640,480,30,Z16,0
INFRARED,640,480,30,Y8,1
COLOR,1280,720,30,RGB8,0
```
