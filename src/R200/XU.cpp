#include "XU.h"

namespace r200
{
    
int GetStreamStatus(uvc_device_handle_t *devh)
{
    uint32_t status = 0xffffffff;
    
    size_t length = sizeof(uint32_t);
    
    // XU Read
    auto s = uvc_get_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_STATUS, &status, int(length), UVC_GET_CUR);

    if (s > 0)
    {
        return s;
    }
    
    if (s < 0) uvc_perror((uvc_error_t)s, "uvc_get_ctrl - ");

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

    if (s < 0) uvc_perror((uvc_error_t)s, "uvc_set_ctrl -");

    return false;
}

std::string GetFirmwareVersion(uvc_device_handle_t * devh)
{
    CommandPacket command;
    command.code = COMMAND_GET_FWREVISION;
    command.modifier = COMMAND_MODIFIER_DIRECT;
    command.tag = 12;
    
    unsigned int cmdLength = sizeof(CommandPacket);
    auto s = uvc_set_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, &command, cmdLength);
    if (s < 0) uvc_perror((uvc_error_t)s, "uvc_set_ctrl - ");
    
    ResponsePacket response;
    unsigned int resLength = sizeof(ResponsePacket);
    s = uvc_get_ctrl(devh, CAMERA_XU_UNIT_ID, CONTROL_COMMAND_RESPONSE, &response, resLength, UVC_GET_CUR);
    if (s < 0) uvc_perror((uvc_error_t)s, "uvc_get_ctrl - ");

    char fw[16];
    
    memcpy(fw, &response.revision, 16);
    
    return std::string(fw);
}
    
//@tofix - implement generic xu read / write + response
    
} // end namespace r200
