// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_DS5D_H
#define LIBREALSENSE_DS5D_H

#include "ds5.h"

#define DS5_PSR_PRODUCT_ID 0x0ad1
#define DS5_ASR_PRODUCT_ID 0x0ad2

namespace rsimpl
{
    // This class represent DS5 variants that generate only depth data. There are 2 variants:
    // (1) DS5 PSR: Uses 2 RGB cameras (passive)
    // (2) DS5 ASR: Uses 2 IR cameras and an IR emitter (active)
    // The difference between the 2 variants is that DS4 ASR has extra controls for controlling the IR emitter
    class ds5d_camera final : public ds5_camera
    {

    public:
        ds5d_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, bool has_emitter);
        ~ds5d_camera() {};

        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;
        bool supports_option(rs_option option) const override;

    private:
        bool has_emitter;
    };

    std::shared_ptr<rs_device> make_ds5d_active_device(std::shared_ptr<uvc::device> device);
    std::shared_ptr<rs_device> make_ds5d_passive_device(std::shared_ptr<uvc::device> device);
}

#endif
