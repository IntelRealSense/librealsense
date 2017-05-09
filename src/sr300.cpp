// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "sr300.h"
#include "hw-monitor.h"

namespace rsimpl2
{
    std::shared_ptr<rsimpl2::device> sr300_info::create(const uvc::backend& backend) const
    {
        return std::make_shared<sr300_camera>(backend, _color, _depth, _hwm);
    }

    std::vector<std::shared_ptr<device_info>> sr300_info::pick_sr300_devices(
        std::shared_ptr<uvc::backend> backend,
        std::vector<uvc::uvc_device_info>& uvc,
        std::vector<uvc::usb_device_info>& usb)
    {
        std::vector<uvc::uvc_device_info> chosen;
        std::vector<std::shared_ptr<device_info>> results;

        auto correct_pid = filter_by_product(uvc, { SR300_PID });
        auto group_devices = group_devices_by_unique_id(correct_pid);
        for (auto& group : group_devices)
        {
            if (group.size() == 2 &&
                mi_present(group, 0) &&
                mi_present(group, 2))
            {
                auto color = get_mi(group, 0);
                auto depth = get_mi(group, 2);
                uvc::usb_device_info hwm;

                if (ivcam::try_fetch_usb_device(usb, color, hwm))
                {
                    auto info = std::make_shared<sr300_info>(backend, color, depth, hwm);
                    chosen.push_back(color);
                    chosen.push_back(depth);
                    results.push_back(info);
                }
                else
                {
                    LOG_WARNING("try_fetch_usb_device(...) failed.");
                }
            }
            else
            {
                LOG_WARNING("SR300 group_devices is empty.");
            }
        }

        trim_device_list(uvc, chosen);

        return results;
    }

    rs2_intrinsics sr300_camera::make_depth_intrinsics(const ivcam::camera_calib_params & c, const int2 & dims)
    {
        return{ dims.x, dims.y, (c.Kc[0][2] * 0.5f + 0.5f) * dims.x,
            (c.Kc[1][2] * 0.5f + 0.5f) * dims.y,
            c.Kc[0][0] * 0.5f * dims.x,
            c.Kc[1][1] * 0.5f * dims.y,
            RS2_DISTORTION_INVERSE_BROWN_CONRADY,
            { c.Invdistc[0], c.Invdistc[1], c.Invdistc[2],
              c.Invdistc[3], c.Invdistc[4] } };
    }

    rs2_intrinsics sr300_camera::make_color_intrinsics(const ivcam::camera_calib_params & c, const int2 & dims)
    {
        rs2_intrinsics intrin = { dims.x, dims.y, c.Kt[0][2] * 0.5f + 0.5f,
            c.Kt[1][2] * 0.5f + 0.5f, c.Kt[0][0] * 0.5f,
            c.Kt[1][1] * 0.5f, RS2_DISTORTION_NONE,{} };

        if (dims.x * 3 == dims.y * 4) // If using a 4:3 aspect ratio, adjust intrinsics (defaults to 16:9)
        {
            intrin.fx *= 4.0f / 3;
            intrin.ppx *= 4.0f / 3;
            intrin.ppx -= 1.0f / 6;
        }

        intrin.fx *= dims.x;
        intrin.fy *= dims.y;
        intrin.ppx *= dims.x;
        intrin.ppy *= dims.y;

        return intrin;
    }

    float sr300_camera::read_mems_temp() const
    {
        command command(ivcam::fw_cmd::GetMEMSTemp);
        auto data = _hw_monitor->send(command);
        auto t = *reinterpret_cast<int32_t *>(data.data());
        return static_cast<float>(t) / 100;
    }

    int sr300_camera::read_ir_temp() const
    {
        command command(ivcam::fw_cmd::GetIRTemp);
        auto data = _hw_monitor->send(command);
        return static_cast<int8_t>(data[0]);
    }

    void sr300_camera::force_hardware_reset() const
    {
        command cmd(ivcam::fw_cmd::HWReset);
        cmd.require_response = false;
        _hw_monitor->send(cmd);
    }

    void sr300_camera::enable_timestamp(bool colorEnable, bool depthEnable) const
    {
        command cmd(ivcam::fw_cmd::TimeStampEnable);
        cmd.param1 = depthEnable ? 1 : 0;
        cmd.param2 = colorEnable ? 1 : 0;
        _hw_monitor->send(cmd);
    }

    void sr300_camera::set_auto_range(const ivcam::cam_auto_range_request& c) const
    {
        command cmd(ivcam::fw_cmd::SetAutoRange);
        cmd.param1 = c.enableMvR;
        cmd.param2 = c.enableLaser;

        std::vector<uint16_t> data;
        data.resize(6);
        data[0] = c.minMvR;
        data[1] = c.maxMvR;
        data[2] = c.startMvR;
        data[3] = c.minLaser;
        data[4] = c.maxLaser;
        data[5] = c.startLaser;

        if (c.ARUpperTh != -1)
        {
            data.push_back(c.ARUpperTh);
        }

        if (c.ARLowerTh != -1)
        {
            data.push_back(c.ARLowerTh);
        }

        cmd.data.resize(sizeof(uint16_t) * data.size());
        rsimpl2::copy(cmd.data.data(), data.data(), cmd.data.size());

        _hw_monitor->send(cmd);
    }

    struct sr300_raw_calibration
    {
        uint16_t tableVersion;
        uint16_t tableID;
        uint32_t dataSize;
        uint32_t reserved;
        int crc;
        ivcam::camera_calib_params CalibrationParameters;
        uint8_t reserved_1[176];
        uint8_t reserved21[148];
    };

    enum class cam_data_source : uint32_t
    {
        TakeFromRO = 0,
        TakeFromRW = 1,
        TakeFromRAM = 2
    };

    ivcam::camera_calib_params sr300_camera::get_calibration() const
    {
        command command(ivcam::fw_cmd::GetCalibrationTable);
        command.param1 = static_cast<uint32_t>(cam_data_source::TakeFromRAM);
        auto data = _hw_monitor->send(command);

        sr300_raw_calibration rawCalib;
        rsimpl2::copy(&rawCalib, data.data(), std::min(sizeof(rawCalib), data.size()));
        return rawCalib.CalibrationParameters;
    }
}
