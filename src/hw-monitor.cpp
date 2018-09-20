// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#include "hw-monitor.h"
#include "types.h"
#include <iomanip>

namespace librealsense
{

    void hw_monitor::fill_usb_buffer(int opCodeNumber, int p1, int p2, int p3, int p4,
        uint8_t* data, int dataLength, uint8_t* bufferToSend, int& length)
    {
        auto preHeaderData = IVCAM_MONITOR_MAGIC_NUMBER;

        uint8_t* writePtr = bufferToSend;
        auto header_size = 4;

        auto cur_index = 2;
        memcpy(writePtr + cur_index, &preHeaderData, sizeof(uint16_t));
        cur_index += sizeof(uint16_t);
        memcpy(writePtr + cur_index, &opCodeNumber, sizeof(uint32_t));
        cur_index += sizeof(uint32_t);
        memcpy(writePtr + cur_index, &p1, sizeof(uint32_t));
        cur_index += sizeof(uint32_t);
        memcpy(writePtr + cur_index, &p2, sizeof(uint32_t));
        cur_index += sizeof(uint32_t);
        memcpy(writePtr + cur_index, &p3, sizeof(uint32_t));
        cur_index += sizeof(uint32_t);
        memcpy(writePtr + cur_index, &p4, sizeof(uint32_t));
        cur_index += sizeof(uint32_t);

        if (dataLength)
        {
            librealsense::copy(writePtr + cur_index, data, dataLength);
            cur_index += dataLength;
        }

        length = cur_index;
        uint16_t tmp_size = length - header_size;
        memcpy(bufferToSend, &tmp_size, sizeof(uint16_t)); // Length doesn't include header
    }


    void hw_monitor::execute_usb_command(uint8_t *out, size_t outSize, uint32_t & op, uint8_t * in, size_t & inSize) const
    {
        std::vector<uint8_t> out_vec(out, out + outSize);
        auto res = _locked_transfer->send_receive(out_vec);

        // read
        if (in && inSize)
        {
            if (res.size() < static_cast<int>(sizeof(uint32_t)))
                throw invalid_value_exception("Incomplete bulk usb transfer!");

            if (res.size() > IVCAM_MONITOR_MAX_BUFFER_SIZE)
                throw invalid_value_exception("Out buffer is greater than max buffer size!");

            op = *reinterpret_cast<uint32_t *>(res.data());
            if (res.size() > static_cast<int>(inSize))
                throw invalid_value_exception("bulk transfer failed - user buffer too small");

            inSize = res.size();
            librealsense::copy(in, res.data(), inSize);
        }
    }

    void hw_monitor::update_cmd_details(hwmon_cmd_details& details, size_t receivedCmdLen, unsigned char* outputBuffer)
    {
        details.receivedCommandDataLength = receivedCmdLen;

        if (details.oneDirection) return;

        if (details.receivedCommandDataLength < 4)
            throw invalid_value_exception("received incomplete response to usb command");

        details.receivedCommandDataLength -= 4;
        librealsense::copy(details.receivedOpcode.data(), outputBuffer, 4);

        if (details.receivedCommandDataLength > 0)
            librealsense::copy(details.receivedCommandData.data(), outputBuffer + 4, details.receivedCommandDataLength);
    }

    void hw_monitor::send_hw_monitor_command(hwmon_cmd_details& details) const
    {
        unsigned char outputBuffer[HW_MONITOR_BUFFER_SIZE];

        uint32_t op{};
        size_t receivedCmdLen = HW_MONITOR_BUFFER_SIZE;

        execute_usb_command(details.sendCommandData.data(), details.sizeOfSendCommandData, op, outputBuffer, receivedCmdLen);
        update_cmd_details(details, receivedCmdLen, outputBuffer);
    }

    std::vector<uint8_t> hw_monitor::send(std::vector<uint8_t> data) const
    {
        return _locked_transfer->send_receive(data);
    }

    std::vector<uint8_t> hw_monitor::send(command cmd) const
    {
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

        send_hw_monitor_command(details);

        // Error/exit conditions
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
            throw invalid_value_exception(to_string() << "OpCodes do not match! Sent "
                << opCodeXmit << " but received " << static_cast<int>(opCodeAsUint32) << "!");
        }

        return std::vector<uint8_t>(newCommand.receivedCommandData,
            newCommand.receivedCommandData + newCommand.receivedCommandDataLength);
    }

    void hw_monitor::get_gvd(size_t sz, unsigned char* gvd, uint8_t gvd_cmd) const
    {
        command command(gvd_cmd);
        auto data = send(command);
        auto minSize = std::min(sz, data.size());
        librealsense::copy(gvd, data.data(), minSize);
    }

    std::string hw_monitor::get_firmware_version_string(int gvd_cmd, uint32_t offset) const
    {
        std::vector<unsigned char> gvd(HW_MONITOR_BUFFER_SIZE);
        get_gvd(gvd.size(), gvd.data(), gvd_cmd);
        uint8_t fws[8];
        librealsense::copy(fws, gvd.data() + offset, 8);
        return to_string() << static_cast<int>(fws[3]) << "." << static_cast<int>(fws[2])
            << "." << static_cast<int>(fws[1]) << "." << static_cast<int>(fws[0]);
    }

    std::string hw_monitor::get_module_serial_string(uint8_t gvd_cmd, uint32_t offset) const
    {
        std::vector<unsigned char> gvd(HW_MONITOR_BUFFER_SIZE);
        get_gvd(gvd.size(), gvd.data(), gvd_cmd);
        unsigned char ss[8];
        librealsense::copy(ss, gvd.data() + offset, 8);
        std::stringstream formattedBuffer;
        formattedBuffer << std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(ss[0]) <<
            std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(ss[1]) <<
            std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(ss[2]) <<
            std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(ss[3]) <<
            std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(ss[4]) <<
            std::setfill('0') << std::setw(2) << std::hex << static_cast<int>(ss[5]);

        return formattedBuffer.str();
    }

    bool hw_monitor::is_camera_locked(uint8_t gvd_cmd, uint32_t offset) const
    {
        std::vector<unsigned char> gvd(HW_MONITOR_BUFFER_SIZE);
        get_gvd(gvd.size(), gvd.data(), gvd_cmd);
        bool value;
        librealsense::copy(&value, gvd.data() + offset, 1);
        return value;
    }
}
