# rs-record Tool

## Overview

This tool is designed to collect raw sensor data to rosbag.

## Description
The tool records for a certain amount of time to a file as specified by the user.
The goal is to offer a command line recorder with low latency and zero frame drops.


## Command Line Parameters

|Flag   |Description   |Default|
|---|---|---|
|`-t X`|Stop recording after X seconds|10|
|`-f <filename>`|Save recording to <filename>|"test.bag"|

For example:
`rs-record -f ./test1.bag -t 60`
will collect the data for 60 seconds.
The data will be saved to `./test1.bag`.

# Recording file
The recorded rosbag can be replayed within librealsense as well as within ROS and inspected by common ROS tools such as `rosbag info` or `rqt_bag`.
One sample of the included data can be seen below:
```
path:        test.bag
version:     2.0
duration:    10.0s
start:       Dec 31 1969 16:00:00.00 (0.00)
end:         Dec 31 1969 16:00:09.00 (10.00)
size:        392.2 MB
messages:    33420
compression: none [301/301 chunks]
types:       diagnostic_msgs/KeyValue    [cf57fdc6617a881a88c16e768132149c]
             geometry_msgs/Accel         [9f195f881246fdfa2798d1d3eebca84a]
             geometry_msgs/Transform     [ac9eff44abf714214112b05d54a3cf9b]
             geometry_msgs/Twist         [9f195f881246fdfa2798d1d3eebca84a]
             realsense_msgs/ImuIntrinsic [aebdc2f8f9726f1c3ca823ab56e47429]
             realsense_msgs/StreamInfo   [311d7e24eac31bb87271d041bf70ff7d]
             sensor_msgs/CameraInfo      [c9a58c1b0b154e0e6da7578cb991d214]
             sensor_msgs/Image           [060021388200f6f0f447d0fcd9c64743]
             sensor_msgs/Imu             [6a62c6daae103f4ff57a132d6f95cec2]
             std_msgs/Float32            [73fcbf46b49191e672908e50842a83d4]
             std_msgs/String             [992ce8a1687cec8c8bd883ec73ca41d1]
             std_msgs/UInt32             [304a39449588c7f8ce2df6e8001c5fce]
topics:      /device_0/info                                                        5 msgs    : diagnostic_msgs/KeyValue   
             /device_0/sensor_0/Accel_0/imu/data                                 640 msgs    : sensor_msgs/Imu            
             /device_0/sensor_0/Accel_0/imu/metadata                            2560 msgs    : diagnostic_msgs/KeyValue   
             /device_0/sensor_0/Accel_0/imu_intrinsic                              1 msg     : realsense_msgs/ImuIntrinsic
             /device_0/sensor_0/Accel_0/info                                       1 msg     : realsense_msgs/StreamInfo  
             /device_0/sensor_0/Accel_0/tf/0                                       1 msg     : geometry_msgs/Transform    
             /device_0/sensor_0/Fisheye_1/image/data                             300 msgs    : sensor_msgs/Image          
             /device_0/sensor_0/Fisheye_1/image/metadata                        1200 msgs    : diagnostic_msgs/KeyValue   
             /device_0/sensor_0/Fisheye_1/info                                     1 msg     : realsense_msgs/StreamInfo  
             /device_0/sensor_0/Fisheye_1/info/camera_info                         1 msg     : sensor_msgs/CameraInfo     
             /device_0/sensor_0/Fisheye_1/tf/0                                     1 msg     : geometry_msgs/Transform    
             /device_0/sensor_0/Fisheye_2/image/data                             300 msgs    : sensor_msgs/Image          
             /device_0/sensor_0/Fisheye_2/image/metadata                        1200 msgs    : diagnostic_msgs/KeyValue   
             /device_0/sensor_0/Fisheye_2/info                                     1 msg     : realsense_msgs/StreamInfo  
             /device_0/sensor_0/Fisheye_2/info/camera_info                         1 msg     : sensor_msgs/CameraInfo     
             /device_0/sensor_0/Fisheye_2/tf/0                                     1 msg     : geometry_msgs/Transform    
             /device_0/sensor_0/Gyro_0/imu/data                                 2006 msgs    : sensor_msgs/Imu            
             /device_0/sensor_0/Gyro_0/imu/metadata                             8024 msgs    : diagnostic_msgs/KeyValue   
             /device_0/sensor_0/Gyro_0/imu_intrinsic                               1 msg     : realsense_msgs/ImuIntrinsic
             /device_0/sensor_0/Gyro_0/info                                        1 msg     : realsense_msgs/StreamInfo  
             /device_0/sensor_0/Gyro_0/tf/0                                        1 msg     : geometry_msgs/Transform    
             /device_0/sensor_0/Pose_0/info                                        1 msg     : realsense_msgs/StreamInfo  
             /device_0/sensor_0/Pose_0/pose/accel/data                          1907 msgs    : geometry_msgs/Accel        
             /device_0/sensor_0/Pose_0/pose/metadata                           11442 msgs    : diagnostic_msgs/KeyValue   
             /device_0/sensor_0/Pose_0/pose/transform/data                      1907 msgs    : geometry_msgs/Transform    
             /device_0/sensor_0/Pose_0/pose/twist/data                          1907 msgs    : geometry_msgs/Twist        
             /device_0/sensor_0/Pose_0/tf/0                                        1 msg     : geometry_msgs/Transform    
             /device_0/sensor_0/info                                               1 msg     : diagnostic_msgs/KeyValue   
             /device_0/sensor_0/option/Asic Temperature/description                1 msg     : std_msgs/String            
             /device_0/sensor_0/option/Asic Temperature/value                      1 msg     : std_msgs/Float32           
             /device_0/sensor_0/option/Frames Queue Size/description               1 msg     : std_msgs/String            
             /device_0/sensor_0/option/Frames Queue Size/value                     1 msg     : std_msgs/Float32           
             /device_0/sensor_0/option/Motion Module Temperature/description       1 msg     : std_msgs/String            
             /device_0/sensor_0/option/Motion Module Temperature/value             1 msg     : std_msgs/Float32           
             /file_version                                                         1 msg     : std_msgs/UInt32
```
