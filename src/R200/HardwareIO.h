#pragma once

#ifndef LIBREALSENSE_R200_HARDWARE_IO_H
#define LIBREALSENSE_R200_HARDWARE_IO_H

#include <libuvc/libuvc.h>
#include <memory>

#include "XU.h"
#include "CameraHeader.h"
#include "R200Types.h"

namespace r200
{
    class DS4HardwareIOInternal;

    class DS4HardwareIO
    {
        std::unique_ptr<DS4HardwareIOInternal> internal;
    public:

        DS4HardwareIO(uvc_device_handle_t * deviceHandle);
        ~DS4HardwareIO();

        CameraCalibrationParameters & GetCalibration();

        CameraHeaderInfo & GetCameraHeader();
    };
} // end namespace r200

#endif


