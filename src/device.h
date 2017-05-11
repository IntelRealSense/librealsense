// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "archive.h"
#include <chrono>
#include <memory>
#include <vector>
#include "hw-monitor.h"
#include "option.h"
#include "subdevice.h"
#include "sync.h"

namespace rsimpl2
{
    class device
    {
    public:
        device();

        virtual ~device() = default;

        unsigned int get_endpoints_count() const { return static_cast<unsigned int>(_endpoints.size()); }
        endpoint& get_endpoint(unsigned subdevice)
        {
            try
            {
                return *(_endpoints.at(subdevice));
            }
            catch (std::out_of_range)
            {
                throw invalid_value_exception("invalid subdevice value");
            }
        }

        virtual rs2_extrinsics get_extrinsics(int from, rs2_stream from_stream, int to, rs2_stream to_stream);

        virtual rs2_intrinsics get_intrinsics(unsigned int subdevice, const stream_profile& profile) const = 0;
        virtual rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream) const
        {
            throw not_implemented_exception(to_string() << __FUNCTION__ << " is not implemented for this device!");
        }

        virtual void hardware_reset() = 0;

        virtual std::vector<uint8_t> send_receive_raw_data(const std::vector<uint8_t>& input)
        {
            throw not_implemented_exception(to_string() << __FUNCTION__ << " is not implemented for this device!");
        }

        virtual std::shared_ptr<sync_interface> create_syncer()
        {
            return std::make_shared<syncer>();
        }

    protected:
        int add_endpoint(std::shared_ptr<endpoint> endpoint);

        uvc_endpoint& get_uvc_endpoint(int subdevice);

        void register_endpoint_info(int sub, std::map<rs2_camera_info, std::string> camera_info);

        void declare_capability(supported_capability cap);
    private:
        std::vector<std::shared_ptr<endpoint>> _endpoints;
        static_device_info _static_info;
    };


}
