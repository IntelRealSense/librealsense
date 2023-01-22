
# Library goal

realdds goal is help control an Intel RealSense camera through a network (not locally using USB).

# DDS

[Data Distribution Service](https://en.wikipedia.org/wiki/Data_Distribution_Service) (DDS) is a [standard](https://www.omg.org/spec/DDS) managed by the Object Management Group (OMG) defining a publish-subscribe pattern for connecting "nodes" over a network.

DDS communication is conducted over a *domain* where various *readers* and *writers* register for *topics* they want to publish or subscribe to.
A *Topic* defines the underlying structure and meaning of the data being shared, while each message transferred is called a *sample*.

There are many vendors that offer an implementation that meet the DDS standard. Currently we use [FastDDS](https://github.com/eProsima/Fast-DDS) as the underlying middleware.
For more information about DDS entities and the FastDDS API you can check out the [documentation](https://fast-dds.docs.eprosima.com/en/latest/).

# realdds

This stand-alone library aims to help control all Intel RealSense camera functionality.

The objects defined can help to create two types of applications camera's server and user.

### Camera's server

Connected directly to the camera, the server represents it on the network publishes its availability and shares it's data.

The main objects that can be used in this application type
* **dds_device_broadcaster** publishes the *device-info* topic, informing subscribers on cameras availability.
* **dds_device_server** represents a camera at DDS level, manages streams, publishes images and receives controls from the user.
* **dds_stream_server** distributes stream images into a dedicated topic

### Camera's user

Consumes images and data produced by the camera.

The main objects that can be used in this application type
* **dds_device_watcher** subscribes to the *device-info* topic and can inform the application on additions/removals of cameras.
* **dds_device** represents a device via the DDS system, manages streams and can query or set the value of different options.
* **dds_stream** represents a stream of information (images, motion data, etc..) from a single source received via the DDS system.

