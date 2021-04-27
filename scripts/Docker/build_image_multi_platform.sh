#! /bin/sh

# This script builds librealsense for both the x86_64 and arm64 platforms. 
# See: https://docs.docker.com/buildx/working-with-buildx/

# Get the latest git TAG version
LIBRS_GIT_TAG=`git describe --abbrev=0 --tags`
LIBRS_VERSION=${LIBRS_GIT_TAG#"v"}

echo "Building images for librealsense version ${LIBRS_VERSION}"

# Build x86_64 image
docker buildx \
    build \
    --platform linux/amd64 \
    --target librealsense \
    --build-arg LIBRS_VERSION=$LIBRS_VERSION \
    --tag librealsense --tag librealsense:$LIBRS_GIT_TAG \
    -o type=docker,dest=- . > librealsense-amd64.tar

# Build arm64 image (slow!)
docker buildx \
    build \
    --platform linux/arm64 \
    --target librealsense \
    --tag librealsense --tag librealsense:$LIBRS_GIT_TAG \
    --build-arg LIBRS_VERSION=$LIBRS_VERSION \
    -o type=docker,dest=- . > librealsense-arm64.tar
