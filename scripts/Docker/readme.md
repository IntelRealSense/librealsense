### Dockerfile for librealsense containers
- Ubuntu base system (Ubuntu 20.04 by default)
- **librealsense-builder** stage - builds the binaries and has all the build dependencies 
- **librealsense** stage -  contain only the built binaries and the required runtime dependencies (~60MB)
- Support for Python bindings (python not included) and networking
- Binaries are stripped during build stage to minimize image size
- Support scripts for building and running the image are also included
