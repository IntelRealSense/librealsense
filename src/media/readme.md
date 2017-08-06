Realsense Record and Playback
===================


Overview
-------------
In addition to streaming video and other data from <mark>TODO</mark>[devices and sensors](TODO:-link-to_device-readme), the realsense SDK provides the ability to record a live session of streaming to a file. Recorded files can later be loaded by the SDK and to create a device with "read-only" abilities of the recorded device ( we will explain what "read-only" abilities mean later on).
A well written application should be able to end up with a single switch-case at the start of the program, while the remaining code is the same, for a live, record or playback device.

----------
#### Example: `rs2::recorder`

> If you are not familiar with how the basic streaming examples, please follow them before moving on

To enable recording on any device, simply create a **recorder** from it and provide a path to the desired output file:
```cpp
//Create a context and get the first device
context ctx;
auto devices = ctx.query_devices();
if (devices.size() > 0)
{
	//Create a recorder from the device
	device record_device = recorder(file_path, devices[0]);
	//recorder "is a" device, so just use it like any other device now
}
```
A **recorder** has the same functionality as a "real" device, with additional control for recording, such as Pause and Resume.
Pausing a recorder will cause it to stop writing new data to the file, in particular, frames and changes to <mark>TODO</mark>[extensions](TODO:_add_link) (such as sensor options) will not appear in the recorded file. 
After calling Resume on a paused recorder, it will resume writing data and changes to the file. 

----------
#### Example: `rs2::playback`

> If you are not familiar with how the basic streaming examples, please follow them before moving on

Recorded files can be loaded and used to create a playback device by simply create a **playback** device:
```cpp
//Create a playback device from file
device playback_device = playback(file_path)
//playback "is a" device, so just use it like any other device now
```
The above code creates a playback device, which can be used is any device, but has the obvious limitation of only playing the recorded streams. Playback devices can be used to query information on the device and it sensors, and can be extended to which ever extension the "real" device could.
A **playback** provides additional functionality such as Seek, Pause, Resume and RealTime.


------


### Terminology & Basics

A **Device** is a container of Sensors with some correlation between them (e.g - all sensors are on a single board, sensors are mounted on a robot and share calibration information, etc.). A **Sensor** is a data streaming object, that provides one or more Streams.
**Stream** is a representation of a single type of data, meaning that a stream contains one type of data that it provides. Streams are exposed to the user via frames that the Sensor raises These frames are made available sequentially over time. A Stream can be thought of as data items on a conveyor belt being packed into a frame one at a time and raised by the sensor.

 A **Stream** represents a single data type that is sequentially made available over time and is exposed via frames that the Sensor raises. A stream can be thought of as data items on a conveyor belt being packed into a frame one at a time, where each frame is delivered to the user via the sensor.
We call the device's sensors and stream, the **topology** of the device. 

Devices and Sensors can have **Extensions** that provide additional functionality. Each extension's functionality is either constant  or non constant (get or set usually). A **Snapshot** of an Extension is a snapshot of the data that is available by the extension at some point of time, it is a sort of "read-only" version of the extension. For example, say we have a `DigitalClockExtension`, that can set the time and show the time. If we take a snapshot of that extension at noon, whenever we ask it to show the time it will show "12:00", and trying to set its time will fail.

Finally, we will refer to a an actual implementation of devices and sensors as "live" or "real" devices and sensors.

Recording
----------
Recording is performed at the Device level, meaning that the device, its sensors, their streams' data (frames) and all extensions are saved to file.
To allow for a seamless experience when switching between a live device and a record or playback device we save the device's topology and all of the extensions' snapshots to the file, in addition to the streaming frames. 

A record device is like a wrapper around a real device, that delegates actions to the device and sensors while recording new data in between actions. 
When a record device is created, a record sensor is created per real sensor of the real device. 
A record sensor will record newly arriving frames for each of its streams, and changes to extensions data (snapshots). 

#### Recording Frames
Frame are usually merely a container of raw data. 
Frames are stored with the time stamp of arrival to the sensor, and a sensor ID. 
By storing this data to the file, when reading a frame, we will know which sensor should dispatch it and when.

#### Recording Snapshots 
Upon creation, the record device goes over all of the extensions of the real device and its sensors, and save a snapshot of those extensions to the file. These snapshots will allow the playback device to recreate the topology of the recorded device, and will serve as the initial data of their extensions.
To allow record sensors to save changes to extensions' data over the life time of the program, when a user asks a record sensor for an extension, a record-able extension would be provide. It's worth mentioning that we expect extensions to provide this recordable version of themselves <mark>TODO</mark>[(see more details in the class diagram ahead)](TODO-link-to-diagram).
A record-able version of an extension holds an action to perform whenever the extension's data changes. This action is provided by the record sensor, and will usually be the action of writing the extension's snapshot to file.

### Rosbag

#### Overview on ROSBAG file structure

The Rosbag file consists of data records, each data that is written to the file is represented by a topic that represents the data content, message that is the data definition, and capture time.
Any topic can be coupled with any message, in our file each topic will be represented by a single message. However, we will have different topics with the same message such as property.  
A bag file consists of a list of records, each record can be one of the following types:

- Bag header. Stores information about the entire bag, such as the offset to the first index data record, and the number of chunks and connections.
- Chunk. Stores (possibly compressed) connection and message records.
- Connection. Stores the header of a ROS connection, including topic name and full text of the message definition (message structure).
- Message data. Stores the serialized message data (which can be zero-length) with the ID of the connection.
- Index data. Stores an index of messages in a single connection of the preceding chunk.
- Chunk info. Stores information about messages in a chunk.

Each message that we write is stored in a chunk, each chunk consists of a number of messages based on the chunk size. In addition to the message data, the chunk stores also the connections of the messages, the connection describes the topic name and message definition to be able to serialize the message data. 
The flow to find a message on topic “A”: 

- Check in all chunk info’s what kind of connections can be found in the chunk.
- Access the chunk with the wanted connection, chunk info contains an offset to the paired chunk.
- Access the message from the relevant index data.

#### Dependencies
The SDK depends on ROS packages that are used for recording and playing the bag file.
The dependencies are:
<mark>TODO verfiy list is complete</mark>
- rosbag_storage
- roscpp_serialization
- cpp_common
- rostime
- boost_system
- lz4

> **Note:** The above dependencies are embedded into the source files, there is no need for additional installations.

#### Topics

The following section is devoted for topics that will be supported by RealSense file format.
The next section will elaborate on message types.

##### General Topics
<table>
  <tr>
    <th>Topic</th>
    <th>Name</th>
    <th>Message Type</th>
    <th>Description</th>
  </tr>
  <tr>
    <td>Version </td>
    <td>/file_version</td>
    <td><a href="http://docs.ros.org/api/std_msgs/html/msg/UInt32.html">std_msgs::UInt32</a></td>
    <td>The RealSense file version</td>
  </tr>
  <tr>
    <td>IHV specific data</td>
    <td>/info/vendor_info/&lt;device_id&gt;</td>
    <td><a href="http://docs.ros.org/api/diagnostic_msgs/html/msg/KeyValue.html">diagnostic_msgs/KeyValue.msg</a></td>
    <td>Custom vendor information</td>
  </tr>
  <tr>
    <td>Properties</td>
    <td>/camera/property/&lt;IHV_specific_property&gt;/&lt;device_id&gt;</td>
    <td><a href="http://docs.ros.org/api/std_msgs/html/msg/Float64.html">std_msgs::Float64</a></td>
    <td>Each topic represents a property</td>
  </tr>
    <tr>
    <td>Time Stream</td>
    <td>/time/&lt;device_id&gt;</td>
    <td> 
    <a href="http://docs.ros.org/jade/api/sensor_msgs/html/msg/TimeReference.html">
    sensor_msgs::TimeReference
    </a> 
    </td>
    <td><mark>TODO</mark> </td>
  </tr>
</table>


##### Image Stream topics
<table>
  <tr>
    <th>Topic</th>
    <th>Name</th>
    <th>Message Type</th>
    <th>Description</th>
  </tr>
  <tr>
    <td>Stream Information</td>
    <td>/camera/rs_stream_info/&lt;device_id&gt;</td>
    <td>realsense_msgs::stream_info</td>
    <td>This topic contains all the necessary data for each stream type.</td>
  </tr>
  <tr>
    <td>Image Data</td>
    <td>/camera/&lt;stream_type&gt;/image_raw/&lt;device_id&gt;</td>
    <td><a href="http://docs.ros.org/api/sensor_msgs/html/msg/Image.html>sensor_msgs::Image</a></td>
    <td>These are multiple topics, each topic represents image<br>data</td>
  </tr>
  <tr>
    <td>Compressed Image Data</td>
    <td>/camera/&lt;stream_type&gt;/compressed_image /&lt;device_id&gt;</td>
    <td><a href="http://docs.ros.org/jade/api/sensor_msgs/html/msg/CompressedImage.html>sensor_msgs::CompressedImage</a></td>
    <td>These are multiple topics, each topic represents compressed<br>image data</td>
  </tr>
  <tr>
    <td>Additional Frame Information</td>
    <td>/camera/&lt;stream_type&gt;/rs_frame_info_ext/&lt;device_id&gt;</td>
    <td>realsense_msgs::frame_info<br></td>
    <td>The following topic contains data that is saved by the SDK but is<br>missing from the sensor_msgs::Image message.</td>
  </tr>
  <tr>
    <td>Additional Compressed Frame Information</td>
    <td>/camera/&lt;stream_type&gt;/rs_compressed_frame_info_ext/&lt;device_id&gt;</td>
    <td>realsense_msgs::compressed_frame_info</td>
    <td>The following topic contains data that is saved by the SDK but is<br>missing from the sensor_msgs::CompressedImage,message.</td>
  </tr>
  <tr>
    <td>Motion Data</td>
    <td>/imu//imu_raw/&lt;device_id&gt;</td>
    <td><a href="http://docs.ros.org/api/sensor_msgs/html/msg/Imu.html">sensor_msgs::Imu</td>
    <td>These are multiple topics, each topic represents motion<br>data</td>
  </tr>
  <tr>
    <td>Motion Stream Information</td>
    <td>/imu/rs_motion_stream_info/&lt;device_id&gt;</td>
    <td>realsense_msgs::motion_stream_info</td>
    <td>This topic contains all the necessary data for each motion type.</td>
  </tr>
  <tr>
    <td>6DOF stream</td>
    <td>/camera/rs_6DoF/&lt;device_id&gt;</td>
    <td>realsense_msgs::6DoF</td>
    <td>The following topic represent the latest pose of the camera relative to<br>its initial position.</td>
  </tr>
  <tr>
    <td>Occupancy Map</td>
    <td>/camera/rs_occupancy_map/&lt;device_id&gt;</td>
    <td>realsense_msgs::occupancy_map</td>
    <td>The following topic represent new real world tiles detected by the SLAM<br>middleware. Tiles are represented in location relative to the device’s initial.</td>
  </tr>
</table>

#### Messages
ROS uses a simplified messages description language for describing the data values (aka messages) that ROS nodes publish. This description makes it easy for ROS tools to automatically generate source code for the message type in several target languages. Message descriptions are stored in .msg files.
There are two parts to a .msg file: fields and constants. Fields are the data that is sent inside of the message. Constants define useful values that can be used to interpret those fields (e.g. enum-like constants for an integer value).

--------------

#### realsense_msgs::stream_info

<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Format</th>
  </tr>
  <tr>
    <td>fps</td>
    <td>Frame per second value</td>
    <td>uint32</td>
  </tr>
  <tr>
    <td>stream_type</td>
    <td>Stream type enum value</td>
    <td>string</td>
  </tr>
  <tr>
    <td>camera_info</td>
    <td>Camera info including stream intrinsics and distortion data<br>  More information on camera_info in <a href="http://docs.ros.org/api/sensor_msgs/html/msg/CameraInfo.html">this link</a><br>  Supported distortion_model types are in <a href="http://docs.ros.org/jade/api/sensor_msgs/html/distortion__models_8h.html">this link</a><br>  The used fields are:<br>  - string distortion_model<br>  - float64[] D<br>  - float64[9] K<br>  </td>
    <td>sensor_msgs::camera_info</td>
  </tr>
  <tr>
    <td>extrinsics</td>
    <td>Camera extrinsics</td>
    <td>realsense_msgs::stream_extrinsics</td>
  </tr>
  <tr>
    <td>width</td>
    <td>Stream resolution width</td>
    <td>uint32</td>
  </tr>
  <tr>
    <td>height</td>
    <td>Stream resolution height</td>
    <td>uint32</td>
  </tr>
  <tr>
    <td>encoding</td>
    <td>Stream's pixel format<br>Supported encoding types are in <a href="http://docs.ros.org/jade/api/sensor_msgs/html/namespacesensor__msgs_1_1image__encodings.html">this link</a></td>
    <td>string</td>
  </tr>
</table>


#### realsense_msgs::stream_extrinsics

<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Format </th>
  </tr>
  <tr>
    <td>extrinsics</td>
    <td>Stream exstisics</td>
    <td>realsense_msgs::extrinsics</td>
  </tr>
  <tr>
    <td>reference_point_id</td>
    <td>Identifier for the extrinsics reference point, as sensor index<br>  </td>
    <td><br>  Uint64<br>  </td>
  </tr>
</table>


#### realsense_msgs::extrinsics
<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Format</th>
  </tr>
  <tr>
    <td>rotation</td>
    <td>column-major 3x3 rotation matrix</td>
    <td>float32[9]</td>
  </tr>
  <tr>
    <td>translation</td>
    <td>3 element translation vector, in meters</td>
    <td>float32[3]</td>
  </tr>
</table>


----------


#### realsense_msgs::frame_info
<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Format</th>
  </tr>
  <tr>
    <td>system_time</td>
    <td>System time value</td>
    <td>uint64</td>
  </tr>
  <tr>
    <td>time_stamp_domain</td>
    <td>Time stamp domain value</td>
    <td>uint32</td>
  </tr>
  <tr>
    <td>frame_metadata</td>
    <td>Array of additionalframe data </td>
    <td>realsense_msgs:metadata[]</td>
  </tr>
</table>

#### realsense_msgs::metadata

<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Format</th>
  </tr>
  <tr>
    <td>type</td>
    <td>Metadata type</td>
    <td>uint64</td>
  </tr>
  <tr>
    <td>Data</td>
    <td>Metadata content</td>
    <td>uint8[]</td>
  </tr>
</table>


----------


#### realsense_msgs::compressed_frame_info
<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Format</th>
  </tr>
  <tr>
    <td>system_time</td>
    <td>System time value</td>
    <td>uint64</td>
  </tr>
  <tr>
    <td>time_stamp_domain</td>
    <td>Time stamp domain value</td>
    <td>uint32</td>
  </tr>
  <tr>
    <td>frame_metadata</td>
    <td>Array of additionalframe data </td>
    <td>realsense_msgs:metadata[]</td>
  </tr>
  <tr>
    <td>width</td>
    <td>Stream resolution width</td>
    <td>uint32</td>
  </tr>
  <tr>
    <td>height</td>
    <td>Streamresolution height</td>
    <td>uint32</td>
  </tr>
  <tr>
    <td>encoding</td>
    <td>Stream pixel format</td>
    <td>String</td>
  </tr>
  <tr>
    <td>is_bigendian</td>
    <td>is this data bigendian</td>
    <td>uint8</td>
  </tr>
  <tr>
    <td>step </td>
    <td>Full row length in bytes</td>
    <td>uint32</td>
  </tr>
</table>

#### realsense_msgs::motion_stream_info
<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Format</th>
  </tr>
  <tr>
    <td>motion_type</td>
    <td>Motion type</td>
    <td>string</td>
  </tr>
  <tr>
    <td>fps</td>
    <td>Frame per second</td>
    <td>uint32</td>
  </tr>
  <tr>
    <td>motion_device_intrinsic</td>
    <td>Sensor intrinsics</td>
    <td>realsense_msgs::motion_intrinsic</td>
  </tr>
  <tr>
    <td>motion_device_extrinsics</td>
    <td>Sensor extrinsics values, world coordinates are the depth cameracoordinates</td>
    <td>realsense_msgs::stream_extrinsics</td>
  </tr>
</table>

#### realsense_msgs::6DoF
<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Format</th>
  </tr>
  <tr>
    <td>translation</td>
    <td>Translation, in meters(relative to initial position)</td>
    <td>float32[3]</td>
  </tr>
  <tr>
    <td>rotation</td>
    <td>Rotation as represented in quaternion rotation(relative to initial position)</td>
    <td>float32[4]</td>
  </tr>
  <tr>
    <td>velocity</td>
    <td>Device velocity. x,y,z represented in m/s</td>
    <td>geometry_msgs/Vector3</td>
  </tr>
  <tr>
    <td>angular_velocity</td>
    <td>Device angular velocityyaw, pitch, roll represented in RAD/s </td>
    <td>geometry_msgs/Vector3</td>
  </tr>
  <tr>
    <td>acceleration</td>
    <td>Devic eacceleration. x,y,z represented in m/s^2</td>
    <td>geometry_msgs/Vector3</td>
  </tr>
  <tr>
    <td>angular_ acceleration</td>
    <td>Device angular acceleration. yaw, pitch, roll represented in RAD/ </td>
    <td>geometry_msgs/Vector3</td>
  </tr>
  <tr>
    <td>timestamp</td>
    <td>Timestamp of pose, measured in nanoseconds since device system initialization</td>
    <td>Uint64</td>
  </tr>
</table>

#### realsense_msgs::occupancy_map
<table>
  <tr>
    <th>Name</th>
    <th>Description</th>
    <th>Format</th>
  </tr>
  <tr>
    <td>accuracy</td>
    <td>Accuracy of occupancy map calculation:<br>0x0 – low accuracy<br>0x1 – medium accuracy<br>0x2 – high accuracy</td>
    <td>Uint8</td>
  </tr>
  <tr>
    <td>reserved</td>
    <td>reserved, must be 0</td>
    <td>byte</td>
  </tr>
  <tr>
    <td>tile_count</td>
    <td>Number of tiles in thefollowing array (= N</td>
    <td>Uint16</td>
  </tr>
  <tr>
    <td>tiles</td>
    <td>Array of tiles, in meters (relative to initial position)</td>
    <td>Float32MultiArray </td>
  </tr>
</table>


## Playback

Playback device is an implementation of device interface which reads from a 
file to simulate a real device.                                              
                                                                          
Playback device holds playback sensors which simulate real sensors.         
                                                                          
When creating the playback device, it will read the initial device snapshot from the file in order to map itself and 
its sensors in matters of functionality and data provided.                              
When creating each sensor, the device will create a sensor from the         
sensor's initial snapshot.                            
Each sensor will hold a single thread for each of the sensor's streams which is used to raise frames to the user.
The playback device holds a single reading thread that reads the next frame in a loop and dispatches the frame to the relevant sensor.
                 
TODO: Describe snapshot changes mechanism                                                            



## Screenshots

>TODO:











