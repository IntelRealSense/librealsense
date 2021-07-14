# Playback

`librealsense2` supports playback of frames from rosbag files.
Here you can find the flow of the function calls from the moment the user asked to start the playback, until the user get the frames.

*Note: at this flow we choose sensor API, a mostly similar flow will `librealsense2` supports playback with pipeline API as well.*

## Software Design and Implementation
The library's approach on playback is that a playback device is in charge of reading the frames from the file and distribute them to each of the recorded sensors.
The frame handling from that point will be done at the relevant sensor thread.

## Sequence Diagram
![playback](./img/playback/playback-flow.png)

*Created using  [DrawIO](https://app.diagrams.net/)

