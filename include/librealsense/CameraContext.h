#pragma once

#ifndef LIBREALSENSE_CAMERA_CONTEXT_H
#define LIBREALSENSE_CAMERA_CONTEXT_H

#include "libuvc/libuvc.h"

#include <librealsense/UVCCamera.h>

////////////////////
// Camera Context //
////////////////////

namespace rs
{

class CameraContext
{
    NO_MOVE(CameraContext);
    uvc_context_t * privateContext;
    void QueryDeviceList();
public:
    CameraContext();
    ~CameraContext();
    std::vector<std::shared_ptr<UVCCamera>> cameras;
};

} // end namespace rs

#endif
