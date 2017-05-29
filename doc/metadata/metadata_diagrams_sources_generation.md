> **Note** - github is currently lacks embedded support for drawing diagrams
Therefore, in order to generate and embed diagrams into the readme we employ the following technique:

- Generate and Draw chart with https://stackedit.io/editor .The site's [main page](https://stackedit.io/) provides a comprehensive list of examples for formatting both text and diagrams.
Expo
- Export the diagram into svg with this [online tool](https://bramp.github.io/js-sequence-diagrams/)
- Use external tool ,such as [Inkscape](https://inkscape.org/en/download/windows/) to convert from svg to png, or other raster format.  
- Embed the resulted raster image into Markdown file


Source code for diagram generators
```sequence
title: RS4xx - Registration for metadata attributes
Participant C++ main
Participant rs2_context
Participant rs2_device
Participant rs2_metadata_handler
Participant OS
Participant Device
Device->OS: new device found
Note over OS: device is enumerated
Note over OS,C++ main: ..........      system is in idle state  ...............
Note over C++ main: Librealsense2 is started
C++ main->rs2_context: create context
rs2_context-->C++ main:
rs2_context->OS: Query connected devices
OS-->C++ main: List of device info
C++ main->rs2_device: create rs4xx object
rs2_device->rs2_device: ctor: register for metadata
rs2_device->rs2_metadata_handler: ctor: Instantiate handler
rs2_metadata_handler-->rs2_device:
rs2_device->rs2_device: ctor: map handler to an attribute
rs2_device-->C++ main:
Note over C++ main: device with metadata support is ready
```

```sequence
title: RS4xx - Metadata attributes propagation and query flow
Participant User Code
Participant rs2_device
Participant rs2_frame_callback
Participant rs2_frame
Participant rs2_backend
Participant Kernel/OS Driver
User Code->User Code: Initialize librealsense
Note over User Code,rs2_device:  Acquire Device handle
User Code->rs2_device: Start Stream (user-defined callback)
Note over rs2_device,rs2_frame_callback: Store callback handle
rs2_device->rs2_backend: Start Polling frames
Note over rs2_backend,Kernel/OS Driver: Polling frame
rs2_backend->rs2_frame: unpack and store frame pixels
rs2_backend->rs2_frame: check and store metadata payload
rs2_frame-->rs2_backend: frame is ready for dispatch
rs2_backend->rs2_frame_callback: invoke user-defined callback(rs2_frame)
rs2_frame_callback->rs2_frame_callback: User code is invoked
rs2_frame_callback->rs2_frame: Check metadata attribute support
rs2_frame-->rs2_frame_callback: result (true/false)
rs2_frame_callback->rs2_frame: query metadata attribute value
rs2_frame-->rs2_frame_callback: value(/exception)
```
