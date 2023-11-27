
See also: [device](device.md), [notifications](notifications.md)


# Initialization

In order to stream, a client must know the stream names, available formats, etc.

The only way for a client to get these is by subscribing to the `notification` topic. When the server detects a subscriber on this topic, it will broadcast a set of initialization messages in the following order:

- `device-header`
- `device-options` - optional
- For each stream:
    - `stream-header`
    - `stream-options`

These initialization messages should only have effect on devices that are not already initialized. They are expected in the above order.

Once all streams have been received, the device is initialized and ready to use. See [Streaming](streaming.md).


## "Atomic" Initialization


Initialization may happen interspersed with regular notifications: for example, a server that's already serving data and notifications to an existing client may see another client and broadcast initialization messages.

Only one set of initialization messages can be sent at a time. I.e., if a new client is detected while initialization messages for a previous client are still outgoing, a new set of messages must go out but only once the previous set is finished.

While a set of initialization messages are outgoing, all other notifications must take a back seat and wait until the set is written out.


## Messages


#### `device-header`

This is the very first message, for example:

```JSON
{
    "id": "device-header",
    "n-streams": 5,
    "extrinsics": [
        ["Depth","Gyro",[1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0,-0.005520000122487545,0.005100000184029341,0.011739999987185001]]
        ["Depth","Infrared 2",[1.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,1.0,-0.04986396059393883,0.0,0.0]]
    ],
    "presets": ["Default", "Max Range", "Max Quality"]
}
```

Mainly the number of streams to expect. The device will wait for this many stream headers to arrive to finish initialization.

The `extrinsics` describe world coordinate transformations between any two streams in the device. This is required for proper alignment on the client.

Optionally, the device may supply a `presets` array of preset names. The presets may then be applied (currently not implemented).


#### `device-options`

This is optional: not all devices have options. See [device](device.md).

```JSON
{
    "id": "device-options",
    "options": [
        ["Domain",0,0,232,1,0,"The DDS domain (0-232) in which this device will be discovered"],
        ["IP Address","1.2.3.4",[],"","Which IP address to assign to the device; if empty, DHCP will be used"]
    ]
}
```

* `"options"` is an array of options
    * Each option is an array of `[name, value, range..., default-value, description, options...]`:
        * The `name` is what will be displayed to the user
        * The current `value`
        * An optional `range` of valid values
            * Numeric options (`float`, `int`), defined by a `minimum`, `maximum`, and `stepping`, e.g. "Domain" in the example above
                * I.e., is-valid = one-of( `minimum`, `minimum+1*stepping`, `minimum+2*stepping`, ..., `maximum` )
            * Booleans can remove the range, e.g. `["Enabled", true, true, "Description"]`
            * Free string options would likewise have no range, e.g. `["Name", "Bob", "", "The customer's name"]`
            * Choice options are strings with an array of choices, e.g. `["Preset", "Maximum Quality", ["Maximum Range", "Maximum Quality", "Maximum Speed"], "Maximum Speed", "Standard preset combination of options"]`
            * **NOTE**: Only numeric options are implemented at this time
                * Booleans can be expressed as a range with `minimum=0`, `maximum=1`, `stepping=1`
        * A `default-value` which also adheres to the range
        * A user-friendly description that describes the option, to be shown in any tooltip
        * Additional `options` describing behavior
            * E.g., `"read-only"`, `"volatile"`, etc.
            * **NOTE**: none are implemented at this time
    * The device server has final say whether an option value is valid or not, and return an error if `set-option` specifies an unsupported or invalid value based on context

Device options will not be shown in the Viewer.


#### `stream-header`

Information about a specific stream:
- `name` is the stream name, e.g. `Color`
    - This will be shown in the Viewer
    - This is also the name of the topic containing the stream data
        - E.g., `rt/<topic-root>_Color`
        - If it contains a space, the space will be replaced by `_` - e.g., `rt/<topic-root>_Infrared_1`
- `profiles` array
    - Each profile is itself an array of `[frequency, format, width, height]`
    - Format is a string representation, similar to the image encoding in ROS
- `default-profile-index` is the index into the `profiles` for the default profile
- `sensor-name` is the name of the sensor
    - Sometimes, a single sensor may produce multiple streams, e.g. `Stereo Module` produces a `Depth` stream and also `Infrared`
    - Streaming one stream requires starting the sensor, and so may have effect on the other streams, depending on the server implementation
    - This allows streams to be grouped by the client and may affect its logic
- `type` is one of `ir`, `depth`, `color`, `confidence`, `motion` - similar to the librealsense `rs2_stream` enum
- `metadata-enabled` is `true` if a `metadata` topic for the device will be written to


```JSON
{
    "default-profile-index": 29,
    "id": "stream-header",
    "metadata-enabled": true,
    "name": "Color",
    "profiles": [
        [30,"rgb8",1280,800],
        [30,"BYR2",1280,800],
        [30,"Y16",1280,800],
        ...
    ],
    "sensor-name": "RGB Camera",
    "type": "color"
}
```

#### `stream-options`

- `stream-name` is the name of the stream, same as in `stream-header`
- `intrinsics` is:
    - For video streams, an array of (width,height)-specific intrinsic values
        - Each is itself an array of `[width, height, principal_point_x, principal_point_y, focal_lenght_x, focal_lenght_y, distortion_model,  distortion_coeffs[0], distortion_coeffs[1], distortion_coeffs[2], distortion_coeffs[3], distortion_coeffs[4]]`
    - For motion streams, a mapping from either `accel` or `gyro` to an array of float values conforming to:
      ```C++
      struct rs2_motion_device_intrinsic
      {
          // Scale X       cross axis  cross axis  Bias X
          // cross axis    Scale Y     cross axis  Bias Y
          // cross axis    cross axis  Scale Z     Bias Z
          float data[3][4];

          // Variance of noise for X, Y, and Z axis
          float noise_variances[3];  

          // Variance of bias for X, Y, and Z axis
          float bias_variances[3];   
      }
      ```
    e.g.:
      ```JSON
      "intrinsics": {
          "accel": [1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0],
          "gyro": [1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,1.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0]
      }
      ```
- `options` is an array of option objects, same as `device-options` above

```JSON
{
    "id": "stream-options",
    "intrinsics": [
        [640,480,320.14276123046875,238.4058837890625,378.80572509765625,378.80572509765625,4,0.0,0.0,0.0,0.0,0.0],
        [1280,720,640.2379150390625,357.3431396484375,631.3428955078125,631.3428955078125,4,0.0,0.0,0.0,0.0,0.0]
    ],
    "options": [
        ["Backlight Compensation",0.0,0.0,1.0,1.0,0.0,"Enable / disable backlight compensation"],
        ["Brightness",0.0,-64.0,64.0,1.0,0.0,"UVC image brightness"],
    ],
    "stream-name":"Infrared 1"
}
```

Stream options are shown in the Viewer.
