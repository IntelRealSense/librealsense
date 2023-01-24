// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "hw-monitor.h"

namespace librealsense
{
    class hwm_buffer_type;
    class hw_monitor_extended_buffers : public hw_monitor
    {
    public:
        enum class hwm_buffer_type
        {
            standard,
            big_buffer_to_receive,
            big_buffer_to_send
        };

        explicit hw_monitor_extended_buffers(std::shared_ptr<locked_transfer> locked_transfer)
            : hw_monitor(locked_transfer)
        {}

        virtual std::vector<uint8_t> send(std::vector<uint8_t> const& data) const override;
        virtual std::vector<uint8_t> send(command cmd, hwmon_response* = nullptr, bool locked_transfer = false) const override;

    private:
        int get_msg_length(command cmd) const;
        int get_number_of_chunks(int msg_length) const;
        hwm_buffer_type get_buffer_type(command cmd) const;
        std::vector<uint8_t> send_big_buffer_to_receive(command cmd, hwmon_response* p_response, bool locked_transfer) const;
        std::vector<uint8_t> send_big_buffer_to_send(command cmd, hwmon_response* p_response, bool locked_transfer) const;

        inline int compute_chunks_param(uint16_t overall_chunks, int iteration) const
        { return ((overall_chunks - 1) << 16) | iteration; }

        std::vector<uint8_t> get_data_for_current_iteration(const std::vector<uint8_t>& table_data, int iteration) const;
    };
}
