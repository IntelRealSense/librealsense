
The files in this directory are, for the most part, automatically generated from files in
    `third-party/realdds/include/topics/*/*.idl`


## Topic list

The various topics are arranged hierarchically:

* `realsense/`
    * `device-info` — device name, serial-number, product-line, lock status, and topic root enumeration
    * `<model>_<serial-number>/` — topic root per device pointed-to in device-info
        * `control` — client -> camera messaging
        * `notification` — camera -> client notifications, responses, etc.
* `rt/realsense/` — [ROS2](#ros2)-compatible streams
    * `<model>_<serial-number>_<stream-name>` — Image/Imu stream supported by the device (e.g., Depth, Infrared, Color, Gyro, etc.)

For a complete overview of the topics structure please refer to [DDS ICD](https://github.com/IntelRealSense/librealsense/blob/dds/third-party/realdds/doc/DDS%20ICD.md)

## Generating

A utility from FastDDS called [FastDDSGen](https://fast-dds.docs.eprosima.com/en/latest/fastddsgen/introduction/introduction.html#fastddsgen-intro) is required to convert the [IDL](https://fast-dds.docs.eprosima.com/en/latest/fastddsgen/dataTypes/dataTypes.html) files to the headers that're needed.
Unfortunately, this utility requires a java installation and sometimes not even the latest.

Instead, we found it easiest to download a Docker image for FastDDS (last used is version 2.7.1) and generate the files within.

1. Download the Docker image from [here](https://www.eprosima.com/index.php?option=com_ars&view=browses&layout=normal) (link was valid at time of writing; if no longer, register with eProsima and update it)

    ```zsh
    docker load -i /mnt/c/work/ubuntu-fastdds_v2.9.1.tar
    ```

2. In a `zsh` shell:

    ```zsh
    # from third-party/realdds/
    #
    for topic in flexible
    do
    cd include/realdds/topics/${topic}
    cid=`docker run -itd --privileged ubuntu-fastdds:v2.9.1`
    docker exec $cid mkdir /idl /idl/out
    docker cp *.idl $cid:idl/
    docker exec -w /idl/out $cid fastddsgen -typeobject /idl/`ls -1 *.idl`
    docker cp $cid:/idl/out .
    docker kill $cid
    cd out
    for cxx in *.cxx; do mv -- "$cxx" "../../../../../src/topics/${cxx%.cxx}.cpp"; done
    mv -- *TypeObject.h "../../../../../src/topics/"
    mv * ..
    cd ..
    rmdir out
    cd ../../../..
    done
    ```

    This automatically renames .cxx to .cpp, places them in the right directory, etc.

3. Once files are generated, they still need some manipulation:
    * Certain `#include` statements (mainly in the .cpp files moved into src/) need to be updated (e.g., `#include "flexible.h"`  changed to `#include <realdds/topics/flexible/flexible.h>`)
    * Copyright notices in all the files needs updating to LibRealSense's

## ROS2

All the ROS2 topics are under `ros2/`. The IDLs are not part of this repo and the source code was pre-generated then modified appropriately.

The per-distro (including rolling) source message (`.msg`) files can be found in:
>https://github.com/ros2/common_interfaces

The `.msg` files can be converted to `.idl` with [rosidl](https://docs.ros.org/en/rolling/Concepts/About-Internal-Interfaces.html#the-rosidl-repository) and then `fastddsgen` can be used to generate source code, similar to what was detailed above. This was all done offline and is not part of the build.

Because there is an interest in keeping our topics ROS-compatible (i.e., so that ROS applications can pick up on and read them), this requires specific topic naming conventions and formats. We try to use ROS-compatible formats, encodings, and names where possible. Right now this is only done one-way: ROS2 can read certain topics, but cannot otherwise control the camera.
