// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "hw-monitor.h"

namespace librealsense
{
    // The aim of this class is to permit to send and receive buffers that are bigger than 1KB via the hw monitor mechanism
    // A protocol has been defined using an additional parameter in each message, 
    // that will indicate which chunk of the whole message is currently sent/received 
    // (see following private method compute_chunks_param)
    class hw_monitor_extended_buffers : public hw_monitor
    {
    public:
        enum class hwm_buffer_type
        {
            standard,
            extended_receive,
            extended_send
        };

        explicit hw_monitor_extended_buffers(std::shared_ptr<locked_transfer> locked_transfer)
            : hw_monitor(locked_transfer)
        {}

        virtual std::vector<uint8_t> send(std::vector<uint8_t> const& data) const override;
        virtual std::vector<uint8_t> send(command const & cmd, hwmon_response* = nullptr, bool locked_transfer = false) const override;

    private:
        int get_number_of_chunks(size_t msg_length) const;
        hwm_buffer_type get_buffer_type(command cmd) const;
        std::vector<uint8_t> extended_receive(command cmd, hwmon_response* p_response, bool locked_transfer) const;
        void extended_send(command cmd, hwmon_response* p_response, bool locked_transfer) const;

        // The following method prepares the param4 of the command for extended buffers
        // The aim of this param is to send the chunk number out of max of expected chunks for the current command
        // It results to a 32 bit integer, which is built from two 16 bits ints:
        // Chunk Id / Max Chunk id
        // uint16(LSB / Little Endian) - current chunk id(0...N - 1)
        // uint16(MSB / Little Endian) - max chunk id(N - 1)
        // Each table is split into N chunks of 1000 bytes as per XU limit
        // The last chunk is within 1..1000 range by design
        // examples:
        // Sending chunk 1 out of 1 chunk => current chunk = 0, expected chunks = 0 (means 1), so param = 0
        // Sending chunk 2 out of 3 chunks => current chunk = 1, expected chunks = 2 (means 3), so param = 0x00020001
        inline int compute_chunks_param(uint16_t overall_chunks, int iteration) const
        { return ((overall_chunks - 1) << 16) | iteration; }

        std::vector<uint8_t> get_data_for_current_iteration(const std::vector<uint8_t>& table_data, int iteration) const;
    };
}
