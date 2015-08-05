#pragma once

#ifndef LIBREALSENSE_CAMERA_CONTEXT_H
#define LIBREALSENSE_CAMERA_CONTEXT_H

#ifndef WIN32
#include "libuvc/libuvc.h"
#endif

#include <librealsense/Camera.h>

////////////////////
// Camera Context //
////////////////////

namespace rs
{

class CameraContext
{
    NO_MOVE(CameraContext);
#ifndef WIN32
    uvc_context_t * privateContext;
#endif
    void QueryDeviceList();
public:
    CameraContext();
    ~CameraContext();
    std::vector<std::shared_ptr<Camera>> cameras;
};

} // end namespace rs

#endif
