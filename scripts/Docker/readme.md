# Librealsense Docker

This tutorial aim is to provide instructions for installation and use of Librealsense Docker. 
Current version of the docker includes the following capabilities:
- use of librealsense devices
- use of librealsense API
- installation of the basic examples for use of librealsense

It does not include (may be enabled later on):
- graphic examples
- use of IMU devices

## Pre-Work: Docker Installation
Install docker in the environemnt using the  following tutorial (adjust for the OS):
https://www.digitalocean.com/community/tutorials/how-to-install-and-use-docker-on-ubuntu-16-04

## Dockerfile description
- Ubuntu base system (Ubuntu 20.04 by default)
- **librealsense-builder** stage - builds the binaries and has all the build dependencies 
- **librealsense** stage -  contains only the built binaries and the required runtime dependencies (~60MB)
- Support for Python bindings (python not included) and networking
- Binaries are stripped from debug symbols during build stage to minimize image size
- Support scripts for building and running the image are also included
- Next steps - TODO: python version, openGL, self info to be printed

# Getting librealsense docker - pre-built

Run the command:
```
docker pull librealsense/librealsense
```

## Running the Container
Now running the command: docker images, the docker librealsense/librealsense should appear.

### Default Command
Then run the container with default command:
```
docker run -it --rm \
    -v /dev:/dev \
    --device-cgroup-rule "c 81:* rmw" \
    --device-cgroup-rule "c 189:* rmw" \
    librealsense/librealsense
```

The default command that will run is: rs-enumerate-devices --compact

### Custom Command
In order to run some arbitrary command (run of the rs-depth demo in the following example), one can run for example:
```
docker run -it --rm \
    -v /dev:/dev \
    --device-cgroup-rule "c 81:* rmw" \
    --device-cgroup-rule "c 189:* rmw" \
    librealsense/librealsense rs-depth
```

### Running shell
Use the following command in order to interact with the Docker via shell interface:
```
docker run -it --rm \
    -v /dev:/dev \
    --device-cgroup-rule "c 81:* rmw" \
    --device-cgroup-rule "c 189:* rmw" \
    librealsense/librealsense /bin/bash
```

# Building librealsense docker image

The librealsense's docker image can be built locally using the [Dockerfile](Dockerfile). 
This  is done by running the [image building script](build_image.sh).

Then, running the container is done as described [above](#Running-the-Container) .






