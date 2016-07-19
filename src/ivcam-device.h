// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_IV_CAMERA_H
#define LIBREALSENSE_IV_CAMERA_H

#include <iostream>
#include "ivcam-private.h"
#include "device.h"

namespace rsimpl
{
    struct cam_mode { int2 dims; std::vector<int> fps; };

    rs_intrinsics MakeDepthIntrinsics(const ivcam::camera_calib_params & c, const int2 & dims);
    rs_intrinsics MakeColorIntrinsics(const ivcam::camera_calib_params & c, const int2 & dims);
    void update_supported_options(uvc::device& dev,
        const uvc::extension_unit depth_xu,
        const std::vector <std::pair<rs_option, char>> options,
        std::vector<supported_option>& supported_options);

    class iv_camera : public rs_device_base
    {
    protected:
        std::timed_mutex usbMutex;

        ivcam::camera_calib_params base_calibration;
        ivcam::cam_auto_range_request arr;

    public:
        iv_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, const ivcam::camera_calib_params & calib);
        ~iv_camera() {};

        void on_before_start(const std::vector<subdevice_mode_selection> & selected_modes) override;
        rs_stream select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) override;
        
        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;

        std::shared_ptr<frame_timestamp_reader> create_frame_timestamp_reader() const override;
    };

    // TODO: This may need to be modified for thread safety
    class rolling_timestamp_reader : public frame_timestamp_reader
    {
        bool started;
        int64_t total;
        int last_timestamp;
    public:
        rolling_timestamp_reader() : started(), total(), last_timestamp(0) {}

        bool validate_frame(const subdevice_mode & mode, const void * frame) const override
        {
            // Validate that at least one byte of the image is nonzero
            for (const uint8_t * it = (const uint8_t *)frame, *end = it + mode.pf.get_image_size(mode.native_dims.x, mode.native_dims.y); it != end; ++it)
            {
                if (*it)
                {
                    return true;
                }
            }

            // F200 and SR300 can sometimes produce empty frames shortly after starting, ignore them
            LOG_INFO("Subdevice " << mode.subdevice << " produced empty frame");
            return false;
        }

        double get_frame_timestamp(const subdevice_mode & mode, const void * frame) override
        {
            // Timestamps are encoded within the first 32 bits of the image
            int rolling_timestamp = *reinterpret_cast<const int32_t *>(frame);

            if (!started)
            {
                last_timestamp = rolling_timestamp;
                started = true;
            }

            const int delta = rolling_timestamp - last_timestamp; // NOTE: Relies on undefined behavior: signed int wraparound
            last_timestamp = rolling_timestamp;
            total += delta;
            const int timestamp = static_cast<int>(total / 100000);
            return timestamp;
        }
        int get_frame_counter(const subdevice_mode & mode, const void * frame) override
        {
            return 0;
        }
    };
}

#endif  // IV_CAMERA_H
