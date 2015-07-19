#include "R200_SPI.h"

#include <stdio.h>
#include <stdint.h>
#include <iostream>

namespace XUControl
{
    
int GetStreamStatus(uvc_device_handle_t *devh)
{
    uint32_t status = 0xffffffff;
    
    size_t length = sizeof(uint32_t);
    
    // XU Read
    auto s = uvc_get_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_STATUS, &status, int(length), UVC_GET_CUR);
    
    std::cout << "Actual Status " << s << std::endl;
    
    if (s > 0)
    {
        return s;
    }
    
    std::cerr << "XURead failed for DSDevice::GetStreamStatus" << std::endl;
    return -1;
}

bool SetStreamIntent(uvc_device_handle_t *devh, uint8_t intent)
{
    size_t length = sizeof(uint8_t);
    
    // XU Write
    auto s = uvc_set_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_STREAM_INTENT, &intent, int(length));
    
    if (s > 0)
    {
        return true;
    }
    
    std::cerr << "XUWrite failed for DSDevice::SetStreamIntent" << std::endl;
    return false;
}
    
}

//@todo implement generic xu read / write + response