#pragma once

#ifndef LIBREALSENSE_CAMERA_CONTEXT_H
#define LIBREALSENSE_CAMERA_CONTEXT_H

#include <stdint.h>
#include <string>
#include <vector>
#include <iostream>
#include <exception>
#include <mutex>
#include <map>

#include "libuvc/libuvc.h"

#include "Common.h"

////////////////////
// Camera Context //
////////////////////

class UVCCamera;

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

#endif
