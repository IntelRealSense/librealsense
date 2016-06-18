// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_ZR300_PRIVATE_H
#define LIBREALSENSE_ZR300_PRIVATE_H

#include <cstring>

#include "uvc.h"
#include "ds-common.h"

namespace rsimpl 
{
    namespace zr300
    {       
        const uvc::extension_unit fisheye_xu = {3, 3, 2, {0xf6c3c3d1, 0x5cde, 0x4477, {0xad, 0xf0, 0x41, 0x33, 0xf5, 0x8d, 0xa6, 0xf4}}};

        void get_register_value(uvc::device & device, uint32_t reg, uint32_t & value);
        void set_register_value(uvc::device & device, uint32_t reg, uint32_t value);

        // Claim USB interface used for motion module device
        void claim_motion_module_interface(uvc::device & device);

        uint8_t get_strobe(const uvc::device & device);
        void set_strobe(uvc::device & device, uint8_t strobe);
        uint8_t get_ext_trig(const uvc::device & device);
        void set_ext_trig(uvc::device & device, uint8_t ext_trig);

    }
}

#endif // ZR300PRIVATE_H
