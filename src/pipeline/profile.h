// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "resolver.h"

namespace librealsense
{
    namespace pipeline
    {
        class profile
        {
        public:
            profile(std::shared_ptr<device_interface> dev, util::config config, const std::string& file = "");
            std::shared_ptr<device_interface> get_device();
            stream_profiles get_active_streams() const;
            util::config::multistream _multistream;
        private:
            std::shared_ptr<device_interface> _dev;
            std::string _to_file;
        };
    }
}
