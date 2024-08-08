#! /bin/sh

# This script builds docker image of the latest librealsense github tag 
# Get the latest git TAG version
LIBRS_GIT_TAG=`git describe --abbrev=0 --tags`
LIBRS_VERSION=${LIBRS_GIT_TAG#"v"}

echo "Building images for librealsense version ${LIBRS_VERSION}"
docker build \
        --target librealsense \
        --build-arg LIBRS_VERSION=$LIBRS_VERSION \
        --tag librealsense \
        --tag librealsense --tag librealsense:$LIBRS_GIT_TAG \
        .
