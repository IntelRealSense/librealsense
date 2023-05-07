
See also: [device](device.md)


# What is discovery

Discovery enables a client to get information about live devices, including how to access them.

It does not include connecting to those devices, talking to them, or anything else.


# Domains

DDS domains separate logical systems so they do not interfere with one another on the network. Domains are numbered 0-255. Discovery on domain 0 will not detect devices on domain 1.

All documentation assumes a single domain shared between all entities.


# realsense/device-info

Only one topic is used for discovery:

>`realsense/device-info`

This topic is not meant for consumption by ROS, and is not meant to be visible there.


# Protocol

The requirement is very simple: whenever a server detects a client, it should broadcast its device-info so the client can see it.

The DDS subsystem enables this by notifying a publisher (the server) that a subscriber (client) exists for the topic.


# Device-Info

The device-info is general information about the device, plus how to access it.

It should not change over time.

One device-info per device. At this time, only one device-info per message.

The message format is a [flexible message](../include/realdds/topics/flexible/) using a simple JSON structure, for example:

```JSON
{
  "name": "Intel RealSense D405",
  "serial": "123622270732",
  "product-line": "D400",
  "product-id": "0B5B",
  "topic-root": "realsense/D405_123622270732"
}
```

| Field        | Purpose                                                |
|--------------|--------------------------------------------------------|
| name         | The name of the device, as it would show in the Viewer |
| serial       | The device serial number                               |
| product-line | The type of device (D400, L500, etc.)                  |
| product-id   | The internal product code                              |
| topic-root   | The path to the root topic for the device              |

All are optional except `name` and `topic-root`. Any other fields will be ignored.


# Topic Root

This points to the topic used as the device root. This is how to access the device; without it, we're just guessing.

See [Device Initialization](initialization.md).


# librealsense

Librealsense manages a single point from which all other objects are derived: the `context`. To get access to a device, a `context` is created and then `query_devices()` called.

The `context` has been augmented to be able to see DDS devices. This is on by default if `BUILD_WITH_DDS` is on.

When a context is created, a JSON representation may be passed to it, e.g.: `{"dds-domain":123,"dds-participant-name":"librs"}`. This allows various customizations:

| Field                | Description                            |
|----------------------|------------------------------------------------------|
| dds-discovery        | Default to `true`; set to `false` to turn off DDS in this context
| dds-domain           | The domain number to use (0-255); `0` is the default
| dds-participant-name | The name given this context (how other participants will see it); defaults to the executable name
