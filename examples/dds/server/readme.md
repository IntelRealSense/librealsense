# rs-dds-server Sample

## Overview

RS DDS server example with a matching running  `librealsense` application on the client side demonstrates how `librealsense` SDK can detect a RS device being connected / disconnected on the server side while Client <> Server is connected throw Ethernet.

## Expected Output
Assuming a running `librealsense` found on the client side and network connection is stable.

We expect to see prints like:

 `Participant discovered`  --> The server found a valid client

`Device x - added` --> The device was added and a matching DDS data writer was created and publish the device information.

`Device x - removed`  --> The device was removes and a matching DDS data writer was destructed.