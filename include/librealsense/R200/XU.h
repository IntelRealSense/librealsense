#pragma once

#ifndef LIBREALSENSE_R200_XU_H
#define LIBREALSENSE_R200_XU_H

#include "libuvc/libuvc.h"

#include <stdint.h>
#include <string>
#include <stdio.h>
#include <stdint.h>
#include <iostream>
#include <cstring>

#include <librealsense/R200/SPI.h>

namespace r200
{
    
//@tofix - these can be safely hidden in the cpp with a little refactoring...
#define CAMERA_XU_UNIT_ID 2
    
#define STATUS_BIT_Z_STREAMING (1 << 0)
#define STATUS_BIT_LR_STREAMING (1 << 1)
#define STATUS_BIT_WEB_STREAMING (1 << 2)
#define STATUS_BIT_BOOT_DIAGNOSTIC_FAULT (1 << 3)
#define STATUS_BIT_IFFLEY_CONSTANTS_VALID (1 << 4)
#define STATUS_BIT_WATCHDOG_TIMER_RESET (1 << 5)
#define STATUS_BIT_REC_BUFFER_OVERRUN (1 << 6)
#define STATUS_BIT_CAM_DATA_FORMAT_ERROR (1 << 7)
#define STATUS_BIT_CAM_FIFO_OVERFLOW (1 << 8)
#define STATUS_BIT_REC_DIVIDED_BY_ZERO_ERROR (1 << 9)
#define STATUS_BIT_UVC_HEADER_ERROR (1 << 10)
#define STATUS_BIT_EMITTER_FAULT (1 << 11)
#define STATUS_BIT_THERMAL_FAULT (1 << 12)
#define STATUS_BIT_REC_RUN_ENABLED (1 << 13)
#define STATUS_BIT_VDF_DEPTH_POINTER_STREAMING (1 << 14)
#define STATUS_BIT_VDF_LR_POINTER_STREAMING (1 << 15)
#define STATUS_BIT_VDF_WEBCAM_POINTER_STREAMING (1 << 16)
#define STATUS_BIT_STREAMING_STATE (1 << 27) | (1 << 28) | (1 << 29) | (1 << 30)
#define STATUS_BIT_BUSY (1 << 31)
    
#define CONTROL_COMMAND_RESPONSE 1
#define CONTROL_IFFLEY 2
#define CONTROL_STREAM_INTENT 3
#define CONTROL_STATUS 20
    
#define COMMAND_DOWNLOAD_SPI_FLASH 0x1A
#define COMMAND_PROTECT_FLASH 0x1C
#define COMMAND_LED_ON 0x14
#define COMMAND_LED_OFF 0x15
#define COMMAND_GET_FWREVISION 0x21
#define COMMAND_GET_SPI_PROTECT 0x23
    
struct CommandPacket
{
    CommandPacket(uint32_t code_ = 0, uint32_t modifier_ = 0, uint32_t tag_ = 0, uint32_t address_ = 0, uint32_t value_ = 0)
    : code(code_), modifier(modifier_), tag(tag_), address(address_), value(value_)
    {
    }
    
    uint32_t code = 0;
    uint32_t modifier = 0;
    uint32_t tag = 0;
    uint32_t address = 0;
    uint32_t value = 0;
    uint32_t reserved[59]{};
};

struct ResponsePacket
{
    ResponsePacket(uint32_t code_ = 0, uint32_t modifier_ = 0, uint32_t tag_ = 0, uint32_t responseCode_ = 0, uint32_t value_ = 0)
    : code(code_), modifier(modifier_), tag(tag_), responseCode(responseCode_), value(value_)
    {
    }
    
    uint32_t code = 0;
    uint32_t modifier = 0;
    uint32_t tag = 0;
    uint32_t responseCode = 0;
    uint32_t value = 0;
    uint32_t revision[4]{};
    uint32_t reserved[55]{};
};

inline std::string ResponseCodeToString(uint32_t rc)
{
    switch (rc)
    {
        case 0x10: return std::string("RESPONSE_OK"); break;
        case 0x11: return std::string("RESPONSE_TIMEOUT"); break;
        case 0x12: return std::string("RESPONSE_ACQUIRING_IMAGE"); break;
        case 0x13: return std::string("RESPONSE_IMAGE_BUSY"); break;
        case 0x14: return std::string("RESPONSE_ACQUIRING_SPI"); break;
        case 0x15: return std::string("RESPONSE_SENDING_SPI"); break;
        case 0x16: return std::string("RESPONSE_SPI_BUSY"); break;
        case 0x17: return std::string("RESPSONSE_UNAUTHORIZED"); break;
        case 0x18: return std::string("RESPONSE_ERROR"); break;
        case 0x19: return std::string("RESPONSE_CODE_END"); break;
        default: return "RESPONSE_UNKNOWN";
    }
}

int GetStreamStatus(uvc_device_handle_t *devh);

bool SetStreamIntent(uvc_device_handle_t *devh, uint8_t intent);
    
std::string GetFirmwareVersion(uvc_device_handle_t *t);

} // end namespace r200

#endif
