// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "hw-monitor.h"

namespace librealsense
{
    class hw_monitor_extended_buffers : public hw_monitor
    {
    public:
        explicit hw_monitor_extended_buffers(std::shared_ptr<locked_transfer> locked_transfer)
            : hw_monitor(locked_transfer)
        {}

        virtual std::vector<uint8_t> send(std::vector<uint8_t> const& data) const override;
        virtual std::vector<uint8_t> send(command cmd, hwmon_response* = nullptr, bool locked_transfer = false) const override;

    private:
        int get_msg_length(command cmd) const;
        int get_number_of_chunks(int msg_length) const;
    };
}
