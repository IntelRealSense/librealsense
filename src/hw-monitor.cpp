// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#include "hw-monitor.h"

namespace rsimpl
{

    void hw_monitor::fill_usb_buffer(int opCodeNumber, int p1, int p2, int p3, int p4,
        uint8_t * data, int dataLength, uint8_t * bufferToSend, int & length)
    {
        auto preHeaderData = IVCAM_MONITOR_MAGIC_NUMBER;

        auto writePtr = bufferToSend;
        auto header_size = 4;

        auto cur_index = 2;
        *reinterpret_cast<uint16_t *>(writePtr + cur_index) = preHeaderData;
        cur_index += sizeof(uint16_t);
        *reinterpret_cast<int *>(writePtr + cur_index) = opCodeNumber;
        cur_index += sizeof(uint32_t);
        *reinterpret_cast<int *>(writePtr + cur_index) = p1;
        cur_index += sizeof(uint32_t);
        *reinterpret_cast<int *>(writePtr + cur_index) = p2;
        cur_index += sizeof(uint32_t);
        *reinterpret_cast<int *>(writePtr + cur_index) = p3;
        cur_index += sizeof(uint32_t);
        *reinterpret_cast<int *>(writePtr + cur_index) = p4;
        cur_index += sizeof(uint32_t);

        if (dataLength)
        {
            memcpy(writePtr + cur_index, data, dataLength);
            cur_index += dataLength;
        }

        length = cur_index;
        *reinterpret_cast<uint16_t *>(bufferToSend) = static_cast<uint16_t>(length - header_size); // Length doesn't include header
    }


    void hw_monitor::execute_usb_command(uint8_t *out, size_t outSize, uint32_t & op, uint8_t * in, size_t & inSize) const
    {
        std::vector<uint8_t> out_vec(out, out + outSize);
        auto res = _command_transfer->send_receive(out_vec);

        // read
        if (in && inSize)
        {
            if (res.size() < static_cast<int>(sizeof(uint32_t))) 
                throw std::runtime_error("Incomplete bulk usb transfer!");

            if (res.size() > IVCAM_MONITOR_MAX_BUFFER_SIZE) 
                throw std::runtime_error("Out buffer is greater than max buffer size!");

            op = *reinterpret_cast<uint32_t *>(res.data());
            if (res.size() > static_cast<int>(inSize)) throw std::runtime_error("bulk transfer failed - user buffer too small");
            inSize = res.size();
            memcpy(in, res.data(), inSize);
        }
    }

    void hw_monitor::update_cmd_details(hwmon_cmd_details& details, size_t receivedCmdLen, unsigned char* outputBuffer)
    {
        details.receivedCommandDataLength = receivedCmdLen;

        if (details.oneDirection) return;

        if (details.receivedCommandDataLength < 4) throw std::runtime_error("received incomplete response to usb command");

        details.receivedCommandDataLength -= 4;
        memcpy(details.receivedOpcode, outputBuffer, 4);

        if (details.receivedCommandDataLength > 0)
            memcpy(details.receivedCommandData, outputBuffer + 4, details.receivedCommandDataLength);
    }

    void hw_monitor::send_hw_monitor_command(hwmon_cmd_details& details) const
    {
        unsigned char outputBuffer[HW_MONITOR_BUFFER_SIZE];

        uint32_t op{};
        size_t receivedCmdLen = HW_MONITOR_BUFFER_SIZE;

        execute_usb_command(details.sendCommandData, details.sizeOfSendCommandData, op, outputBuffer, receivedCmdLen);
        update_cmd_details(details, receivedCmdLen, outputBuffer);
    }

    std::vector<uint8_t> hw_monitor::send(command cmd) const
    {
        hwmon_cmd newCommand(cmd);
        auto opCodeXmit = static_cast<uint32_t>(newCommand.cmd);

        hwmon_cmd_details details;
        details.oneDirection = newCommand.oneDirection;
        details.TimeOut = newCommand.TimeOut;

        fill_usb_buffer(opCodeXmit,
            newCommand.Param1,
            newCommand.Param2,
            newCommand.Param3,
            newCommand.Param4,
            newCommand.data,
            newCommand.sizeOfSendCommandData,
            details.sendCommandData,
            details.sizeOfSendCommandData);

        send_hw_monitor_command(details);

        // Error/exit conditions
        if (newCommand.oneDirection)
            return std::vector<uint8_t>();

        memcpy(newCommand.receivedOpcode, details.receivedOpcode, 4);
        memcpy(newCommand.receivedCommandData, details.receivedCommandData, details.receivedCommandDataLength);
        newCommand.receivedCommandDataLength = details.receivedCommandDataLength;

        // endian?
        auto opCodeAsUint32 = pack(details.receivedOpcode[3], details.receivedOpcode[2], 
                                   details.receivedOpcode[1], details.receivedOpcode[0]);
        if (opCodeAsUint32 != opCodeXmit)
        {
            throw std::runtime_error(to_string() << "OpCodes do not match! Sent " 
                << opCodeXmit << " but received " << opCodeAsUint32 << "!");
        }

        return std::vector<uint8_t>(newCommand.receivedCommandData,
            newCommand.receivedCommandData + newCommand.receivedCommandDataLength);
    }
}
