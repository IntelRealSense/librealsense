// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "hw_monitor_extended_buffers.h"

#include "ds/ds6/ds6-private.h"

namespace librealsense
{
    int hw_monitor_extended_buffers::get_msg_length(command cmd) const
    {
        int msg_length = HW_MONITOR_BUFFER_SIZE;
        if (cmd.cmd == ds::fw_cmd::GET_HKR_CONFIG_TABLE || cmd.cmd == ds::fw_cmd::SET_HKR_CONFIG_TABLE)
        {
            switch (cmd.param2)
            {
            case static_cast<int>(ds::ds6_calibration_table_id::coefficients_table_id):
            {
                msg_length = sizeof(ds::ds6_coefficients_table);
                break;
            }
            case static_cast<int>(ds::ds6_calibration_table_id::rgb_calibration_id):
            {
                msg_length = sizeof(ds::ds6_rgb_calibration_table);
                break;
            }
            default:
                throw std::runtime_error(rsutils::string::from() << "command GET_HKR_CONFIG_TABLE with wromg param2");
            }
        }
        return msg_length;
    }

    int hw_monitor_extended_buffers::get_number_of_chunks(int msg_length) const
    {
        return static_cast<int>(std::ceil(msg_length / 1024.0f));
    }


    std::vector<uint8_t> hw_monitor_extended_buffers::send(command cmd, hwmon_response* p_response, bool locked_transfer) const
    {
        //int send_msg_length = get_send_msg_length(cmd);
        //int send_chunks = send_msg_length / 1024;

        int recv_msg_length = get_msg_length(cmd);
        uint16_t recv_chunks = get_number_of_chunks(recv_msg_length);

        std::vector<byte> recv_msg;

        for (int i = 0; i < recv_chunks; ++i)
        {
            // updating the chunk number in case several chunks are expected
            if (recv_chunks > 1)
                cmd.param4 = ((recv_chunks - 1) << 16) | i;

            hwmon_cmd newCommand(cmd);
            auto opCodeXmit = static_cast<uint32_t>(newCommand.cmd);

            hwmon_cmd_details details;
            details.oneDirection = newCommand.oneDirection;
            details.timeOut = newCommand.timeOut;

            fill_usb_buffer(opCodeXmit,
                newCommand.param1,
                newCommand.param2,
                newCommand.param3,
                newCommand.param4,
                newCommand.data,
                newCommand.sizeOfSendCommandData,
                details.sendCommandData.data(),
                details.sizeOfSendCommandData);

            if (locked_transfer)
            {
                return _locked_transfer->send_receive({ details.sendCommandData.begin(),details.sendCommandData.end() });
            }

            send_hw_monitor_command(details);

            // Error/exit conditions
            if (p_response)
                *p_response = hwm_Success;
            if (newCommand.oneDirection)
                return std::vector<uint8_t>();

            librealsense::copy(newCommand.receivedOpcode, details.receivedOpcode.data(), 4);
            librealsense::copy(newCommand.receivedCommandData, details.receivedCommandData.data(), details.receivedCommandDataLength);
            newCommand.receivedCommandDataLength = details.receivedCommandDataLength;

            // endian?
            auto opCodeAsUint32 = pack(details.receivedOpcode[3], details.receivedOpcode[2],
                details.receivedOpcode[1], details.receivedOpcode[0]);
            if (opCodeAsUint32 != opCodeXmit)
            {
                auto err_type = static_cast<hwmon_response>(opCodeAsUint32);
                std::string err = hwmon_error_string(cmd, err_type);
                LOG_DEBUG(err);
                if (p_response)
                {
                    *p_response = err_type;
                    return std::vector<uint8_t>();
                }
                throw invalid_value_exception(err);
            }
            auto ans = std::vector<uint8_t>(newCommand.receivedCommandData,
                newCommand.receivedCommandData + newCommand.receivedCommandDataLength);

            recv_msg.insert(recv_msg.end(), ans.begin(), ans.end());
        }

        return recv_msg;
    }
    std::vector<uint8_t> hw_monitor_extended_buffers::send(std::vector<uint8_t> const& data) const
    {
        return hw_monitor::send(data);;
    }
}
