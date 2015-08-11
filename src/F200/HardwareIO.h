#pragma once

#ifndef LIBREALSENSE_F200_HARDWARE_IO_H
#define LIBREALSENSE_F200_HARDWARE_IO_H

#include "F200Types.h"
#include <memory>

namespace f200
{
    
class IVCAMHardwareIOInternal;
class IVCAMHardwareIO
{

    std::unique_ptr<IVCAMHardwareIOInternal> internal;
public:
    
    IVCAMHardwareIO();
    ~IVCAMHardwareIO();
    
    bool StartTempCompensationLoop();
    void StopTempCompensationLoop();
    
    // SetDepthResolution(int width, int height)
    
    CameraCalibrationParameters & GetParameters();
};
    
} // end namespace f200

#endif
