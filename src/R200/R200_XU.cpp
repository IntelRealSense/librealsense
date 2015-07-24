#include <librealsense/R200/R200_XU.h>

namespace r200
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

std::string GetFirmwareVersion(uvc_device_handle_t * devh)
{
    CommandPacket command;
    command.code = COMMAND_GET_FWREVISION;
    command.modifier = COMMAND_MODIFIER_DIRECT;
    command.tag = 12;
    
    unsigned int cmdLength = sizeof(CommandPacket);
    auto XUWriteCmdStatus = uvc_set_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, &command, cmdLength);
    
    ResponsePacket response;
    unsigned int resLength = sizeof(ResponsePacket);
    auto XUReadCmdStatus = uvc_get_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, &response, resLength, UVC_GET_CUR);
    
    char fw[16];
    
    memcpy(fw, &response.revision, 16);
    
    return std::string(fw);
}
    
//@tofix - implement generic xu read / write + response
    
} // end namespace r200
