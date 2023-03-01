// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "hw_monitor_extended_buffers.h"
#include "ds/d500/d500-private.h"


namespace librealsense
{
    int hw_monitor_extended_buffers::get_msg_length(command cmd) const
    {
        int msg_length = HW_MONITOR_BUFFER_SIZE;
        if (cmd.cmd == ds::fw_cmd::GET_HKR_CONFIG_TABLE || cmd.cmd == ds::fw_cmd::SET_HKR_CONFIG_TABLE)
        {
            auto calib_table_id = static_cast<ds::d500_calibration_table_id>(cmd.param2);
            auto it = ds::d500_calibration_tables_size.find(calib_table_id);
            if (it == ds::d500_calibration_tables_size.end())
                throw std::runtime_error(rsutils::string::from() << " hwm command with wrong param2");

            msg_length = it->second;
        }
        return msg_length;
    }

    int hw_monitor_extended_buffers::get_number_of_chunks(int msg_length) const
    {
        return static_cast<int>(std::ceil(msg_length / 1024.0f));
    }

    hw_monitor_extended_buffers::hwm_buffer_type hw_monitor_extended_buffers::get_buffer_type(command cmd) const
    {
        if (cmd.cmd == ds::fw_cmd::GET_HKR_CONFIG_TABLE || cmd.cmd == ds::fw_cmd::SET_HKR_CONFIG_TABLE)
        {
            auto calibration_table_size = get_msg_length(cmd);
            if (calibration_table_size <= HW_MONITOR_COMMAND_SIZE)
                return hwm_buffer_type::standard;
            if (cmd.cmd == ds::fw_cmd::GET_HKR_CONFIG_TABLE)
                return hwm_buffer_type::big_buffer_to_receive;
            return hwm_buffer_type::big_buffer_to_send;
        }
        return hwm_buffer_type::standard;
    }

    // 3 cases are foreseen in this method:
    // - no buffer bigger than 1 KB is expected with the current command => work as normal hw_monitor::send method
    // - buffer bigger than 1 KB expected to be received => iterate the hw_monitor::send method and append the results
    // - buffer bigger than 1 KB expected to be sent => iterate the hw_monitor, while iterating over the input
    std::vector<uint8_t> hw_monitor_extended_buffers::send(command cmd, hwmon_response* p_response, bool locked_transfer) const
    {
        hwm_buffer_type buffer_type = get_buffer_type(cmd);
        if (buffer_type == hwm_buffer_type::standard)
            return hw_monitor::send(cmd, p_response, locked_transfer);

        if (buffer_type == hwm_buffer_type::big_buffer_to_receive)
            return send_big_buffer_to_receive(cmd, p_response, locked_transfer);

        // hwm_buffer_type::big_buffer_to_send is the last remaining option
        send_big_buffer_to_send(cmd, p_response, locked_transfer);
        return std::vector<uint8_t>();
    }

    std::vector<uint8_t> hw_monitor_extended_buffers::send_big_buffer_to_receive(command cmd, hwmon_response* p_response, bool locked_transfer) const
    {
        int recv_msg_length = get_msg_length(cmd);
        uint16_t overall_chunks = get_number_of_chunks(recv_msg_length);

        std::vector<byte> recv_msg;
        for (int i = 0; i < overall_chunks; ++i)
        {
            // chunk number is in param4
            cmd.param4 = compute_chunks_param(overall_chunks, i);
        
            auto ans = hw_monitor::send(cmd, p_response, locked_transfer);
            recv_msg.insert(recv_msg.end(), ans.begin(), ans.end());
        }
        return recv_msg;
    }

    void hw_monitor_extended_buffers::send_big_buffer_to_send(command cmd, hwmon_response* p_response, bool locked_transfer) const
    {
        int send_msg_length = get_msg_length(cmd);
        uint16_t overall_chunks = get_number_of_chunks(send_msg_length);

        // copying the data, so that this param can be overrien for the sending via hwm
        auto table_data = cmd.data;
        if (table_data.size() != send_msg_length)
            throw std::runtime_error(rsutils::string::from() << "Table data has size = " << table_data.size() << ", it should be: " << send_msg_length);

        std::vector<uint8_t> answer;
        for (int i = 0; i < overall_chunks; ++i)
        {
            // preparing data to be sent in current chunk
            cmd.data = std::move(get_data_for_current_iteration(table_data, i));
            // chunk number is in param4
            cmd.param4 = compute_chunks_param(overall_chunks, i);

            hw_monitor::send(cmd, p_response, locked_transfer);
        }
    }

    std::vector<uint8_t> hw_monitor_extended_buffers::get_data_for_current_iteration(const std::vector<uint8_t>& table_data, int iteration) const
    {
        auto remaining_data_size_to_be_sent = table_data.size() - iteration * HW_MONITOR_COMMAND_SIZE;

        auto start_iterator_for_current_chunk = table_data.begin() + iteration * HW_MONITOR_COMMAND_SIZE;
        auto end_iterator_for_current_chunk = (remaining_data_size_to_be_sent >= HW_MONITOR_COMMAND_SIZE) ?
            start_iterator_for_current_chunk + HW_MONITOR_COMMAND_SIZE :
            start_iterator_for_current_chunk + remaining_data_size_to_be_sent;

        return std::vector<uint8_t>(start_iterator_for_current_chunk, end_iterator_for_current_chunk);
    }
        
    std::vector<uint8_t> hw_monitor_extended_buffers::send(std::vector<uint8_t> const& data) const
    {
        command cmd = build_command_from_data(data);

        hwm_buffer_type buffer_type = get_buffer_type(cmd);
        if (buffer_type == hwm_buffer_type::standard)
            return hw_monitor::send(data);
        return send(cmd);
    }
}
