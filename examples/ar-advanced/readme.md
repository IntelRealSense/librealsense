# rs-ar-advanced Sample

> In order to run this example, a device supporting pose stream (T265) is required.

## Overview
This sample demonstrates how to extend the [`rs-ar-basic` Sample](../ar-basic) with map import/export API.
It also shows how to receive a relocalization event callback, and how to use the get and set static node API to add landmarks to the localization map.

## Command Line Inputs

Users should specify the path(s) to the localization map data file(s) by using command line arguments. For example, to set input map file `src_map.raw` at current directory:

```cpp
rs-ar-advanced --load_map src_map.raw
```

To set both input map file `src_map.raw` and output map file `dst_map.raw` at current directory:

```cpp
rs-ar-advanced --load_map src_map.raw --save_map dst_map.raw`
```

Then, the application will import localization map data from `src_map.raw` at the beginning of tracking and will export the modified map including a virtual object as a static node to `dst_map.raw`.

>Note: if neither input nor output path is given, this application will behave exactly the same as the `rs-ar-basic` example.

## Expected Output
Same as the [`rs-ar-basic` Sample](../ar-basic), the application should open a window in which it shows one of the fisheye streams with a virtual object in the scene. The virtual object is 3 red, green and blue segments.

![](../ar-basic/example.gif "Example")

## Code Overview

Please refer to code overview of the [rs-ar-basic Sample](../ar-basic/readme.md). 

## Additions to the `rs-ar-basic`

Before start running the pipe, we import the localization map using the file path (if available) from command line input and a `tm_sensor` object from the pipeline:

```cpp
// Get pose sensor
auto tm_sensor = cfg.resolve(pipe).get_device().first<rs2::pose_sensor>();

tm_sensor.import_localization_map(bytes_from_raw_file(in_map_filepath));
std::cout << "Map loaded from " << in_map_filepath << std::endl;
```

Then, we set a relocalization notification callback using the same `tm_sensor` object:

```cpp

// Add relocalization callback
tm_sensor.set_notifications_callback([&](const rs2::notification& n) {
       if (n.get_category() == RS2_NOTIFICATION_CATEGORY_POSE_RELOCALIZATION) {
           std::cout << "Relocalization Event Detected." << std::endl;
```

Continue within the callback is that we load the virtual object saved as static node in the map file in previous run:

```cpp
           // Get static node if available
           if (tm_sensor.get_static_node(virtual_object_guid, object_pose_in_world.translation, object_pose_in_world.rotation)) {
               std::cout << "Virtual object loaded:  " << object_pose_in_world.translation << std::endl;
               object_pose_in_world_initialized = true;
           }
```

The rest of the main body is similar to the [`rs-ar-basic` Sample](../ar-basic), except, at the end of the application we will save the modified virtual object as static node:

```cpp
// Exit if user presses escape
if (tm_sensor.set_static_node(virtual_object_guid, object_pose_in_world.translation, object_pose_in_world.rotation)) {
    std::cout << "Saved virtual object as static node. " << std::endl;
}
```

We also save the modified localization map to file path given by the command line inputs (if available):

```cpp
// Export map to a raw file
if (!out_map_filepath.empty()) {
    pipe.stop();
    raw_file_from_bytes(out_map_filepath, tm_sensor.export_localization_map());
    std::cout << "Saved map to " << out_map_filepath << std::endl;
}
```
