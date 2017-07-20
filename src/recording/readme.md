Realsense Record and Playback
===================


Overview
-------------
In addition to streaming video and other data from [devices and sensors](TODO:-link-to_device-readme), the realsense SDK provides the ability to record a live session of streaming to a file. Recorded files can later be loaded by the SDK and to create a device with "read-only" abilities of the recorded device ( we will explain what "read-only" abilities mean later on).
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
Pausing a recorder will cause it to stop writing new data to the file, in particular, frames and changes to [extensions](TODO:_add_link) (such as sensor options) will not appear in the recorded file. 
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

#### Saving Frames
Frame are usually merely a container of raw data. 
Frames are stored with the time stamp of arrival to the sensor, and a sensor ID. 
By storing this data to the file, when reading a frame, we will know which sensor should dispatch it and when.

#### Saving Snapshots 
Upon creation, the record device goes over all of the extensions of the real device and its sensors, and save a snapshot of those extensions to the file. These snapshots will allow the playback device to recreate the topology of the recorded device, and will serve as the initial data of their extensions.
To allow record sensors to save changes to extensions' data over the life time of the program, when a user asks a record sensor for an extension, a record-able extension would be provide. It's worth mentioning that we expect extensions to provide this recordable version of themselves [(see more details in the class diagram ahead)](TODO-link-to-diagram).
A record-able version of an extension holds an action to perform whenever the extension's data changes. This action is provided by the record sensor, and will usually be the action of writing the extension's snapshot to file.



