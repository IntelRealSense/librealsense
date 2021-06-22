# Librealsense Docker

## Pre-Work: Docker Installation
Install docker in the environemnt using the  following tutorial (adjust the OS):
https://www.digitalocean.com/community/tutorials/how-to-install-and-use-docker-on-ubuntu-16-04

## Dockerfile description
- Ubuntu base system (Ubuntu 20.04 by default)
- **librealsense-builder** stage - builds the binaries and has all the build dependencies 
- **librealsense** stage -  contain only the built binaries and the required runtime dependencies (~60MB)
- Support for Python bindings (python not included) and networking
- Binaries are stripped during build stage to minimize image size
- Support scripts for building and running the image are also included

## Getting librealsense docker

As long as the repo is private, login is needed:
1. add the password in some password.txt file
2. run: cat password.txt | docker login –username=“…” –password-stdin

Then run the command:
docker pull librealsense/librealsense

## Running the Container
Now running the command: docker images, the docker librealsense/librealsense should appear.

### Default Command
Then run the container with default command:
docker run -it --rm \
    -v /dev:/dev \
    --device-cgroup-rule "c 81:* rmw" \
    --device-cgroup-rule "c 189:* rmw" \
    librealsense/librealsense

The default command that will run is: rs-enumerate-devices --compact

### Custom Command
In order to run another command, one can run for example:
docker run -it --rm \
    -v /dev:/dev \
    --device-cgroup-rule "c 81:* rmw" \
    --device-cgroup-rule "c 189:* rmw" \
    librealsense/librealsense rs-depth

### Running shell
Or, in order to open bash inside the container:
docker run -it --rm \
    -v /dev:/dev \
    --device-cgroup-rule "c 81:* rmw" \
    --device-cgroup-rule "c 189:* rmw" \
    librealsense/librealsense /bin/bash







