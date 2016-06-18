// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "hw-monitor.h"
#include "zr300-private.h"

#include <cstring>
#include <cmath>
#include <ctime>
#include <thread>
#include <chrono>
#include <iomanip>
#include <mutex>
#include <algorithm>

#pragma pack(push, 1) // All structs in this file are byte-aligend

namespace rsimpl {
    namespace zr300
    {
        //const uvc::extension_unit lr_xu = {0, 2, 1, {0x18682d34, 0xdd2c, 0x4073, {0xad, 0x23, 0x72, 0x14, 0x73, 0x9a, 0x07, 0x4c}}};

        const uvc::guid MOTION_MODULE_USB_DEVICE_GUID = { 0xC0B55A29, 0xD7B6, 0x436E, { 0xA6, 0xEF, 0x2E, 0x76, 0xED, 0x0A, 0xBC, 0xA5 } };
        const unsigned short motion_module_interrupt_interface = 0x2; // endpint to pull sensors data continuously (interrupt transmit)

        uint8_t get_ext_trig(const uvc::device & device)
        {
            return ds::xu_read<uint8_t>(device, fisheye_xu, ds::control::fisheye_xu_ext_trig);
        }

        void set_ext_trig(uvc::device & device, uint8_t ext_trig)
        {
            ds::xu_write(device, fisheye_xu, ds::control::fisheye_xu_ext_trig, &ext_trig, sizeof(ext_trig));
        }

        uint8_t get_strobe(const uvc::device & device)
        {
            return ds::xu_read<uint8_t>(device, fisheye_xu, ds::control::fisheye_xu_strobe);
        }

        void set_strobe(uvc::device & device, uint8_t strobe)
        {
            ds::xu_write(device, fisheye_xu, ds::control::fisheye_xu_strobe, &strobe, sizeof(strobe));
        }

        void claim_motion_module_interface(uvc::device & device)
        {
            claim_aux_interface(device, MOTION_MODULE_USB_DEVICE_GUID, motion_module_interrupt_interface);
        }

    }
} // namespace rsimpl::r200

#pragma pack(pop)
