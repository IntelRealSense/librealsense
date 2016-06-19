// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.


#include "hw-monitor.h"

namespace rsimpl
{
    namespace hw_monitor
    {

    void fill_usb_buffer(int opCodeNumber, int p1, int p2, int p3, int p4, uint8_t * data, int dataLength, uint8_t * bufferToSend, int & length)
    {
        uint16_t preHeaderData = IVCAM_MONITOR_MAGIC_NUMBER;

        uint8_t * writePtr = bufferToSend;
        int header_size = 4;

        int cur_index = 2;
        *(uint16_t *)(writePtr + cur_index) = preHeaderData;
        cur_index += sizeof(uint16_t);
        *(int *)(writePtr + cur_index) = opCodeNumber;
        cur_index += sizeof(uint32_t);
        *(int *)(writePtr + cur_index) = p1;
        cur_index += sizeof(uint32_t);
        *(int *)(writePtr + cur_index) = p2;
        cur_index += sizeof(uint32_t);
        *(int *)(writePtr + cur_index) = p3;
        cur_index += sizeof(uint32_t);
        *(int *)(writePtr + cur_index) = p4;
        cur_index += sizeof(uint32_t);

        if (dataLength)
        {
            memcpy(writePtr + cur_index, data, dataLength);
            cur_index += dataLength;
        }

        length = cur_index;
        *(uint16_t *)bufferToSend = (uint16_t)(length - header_size); // Length doesn't include header
    }


    void execute_usb_command(uvc::device & device, std::timed_mutex & mutex, unsigned char handle_id, uint8_t *out, size_t outSize, uint32_t & op, uint8_t * in, size_t & inSize)
    {
        // write
        errno = 0;

        int outXfer;

        if (!mutex.try_lock_for(std::chrono::milliseconds(IVCAM_MONITOR_MUTEX_TIMEOUT))) throw std::runtime_error("timed_mutex::try_lock_for(...) timed out");
        std::lock_guard<std::timed_mutex> guard(mutex, std::adopt_lock);

        bulk_transfer(device, handle_id, IVCAM_MONITOR_ENDPOINT_OUT, out, (int)outSize, &outXfer, 1000); // timeout in ms

        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        // read
        if (in && inSize)
        {
            uint8_t buf[IVCAM_MONITOR_MAX_BUFFER_SIZE];

            errno = 0;

            bulk_transfer(device, handle_id, IVCAM_MONITOR_ENDPOINT_IN, buf, sizeof(buf), &outXfer, 1000);
            if (outXfer < (int)sizeof(uint32_t)) throw std::runtime_error("incomplete bulk usb transfer");

            op = *(uint32_t *)buf;
            if (outXfer > (int)inSize) throw std::runtime_error("bulk transfer failed - user buffer too small");
            inSize = outXfer;
            memcpy(in, buf, inSize);
        }
    }

    void send_hw_monitor_command(uvc::device & device, std::timed_mutex & mutex, unsigned char handle_id, hwmon_cmd_details & details)
    {
        unsigned char outputBuffer[HW_MONITOR_BUFFER_SIZE];

        uint32_t op;
        size_t receivedCmdLen = HW_MONITOR_BUFFER_SIZE;

        execute_usb_command(device, mutex, handle_id, (uint8_t*)details.sendCommandData, (size_t)details.sizeOfSendCommandData, op, outputBuffer, receivedCmdLen);
        details.receivedCommandDataLength = receivedCmdLen;

        if (details.oneDirection) return;

        if (details.receivedCommandDataLength < 4) throw std::runtime_error("received incomplete response to usb command");

        details.receivedCommandDataLength -= 4;
        memcpy(details.receivedOpcode, outputBuffer, 4);

        if (details.receivedCommandDataLength > 0)
            memcpy(details.receivedCommandData, outputBuffer + 4, details.receivedCommandDataLength);
    }

    void perform_and_send_monitor_command(uvc::device & device, std::timed_mutex & mutex, hwmon_cmd & newCommand)
    {
        uint32_t opCodeXmit = (uint32_t)newCommand.cmd;

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

        send_hw_monitor_command(device, mutex, 0, details);

        // Error/exit conditions
        if (newCommand.oneDirection)
            return;

        memcpy(newCommand.receivedOpcode, details.receivedOpcode, 4);
        memcpy(newCommand.receivedCommandData, details.receivedCommandData, details.receivedCommandDataLength);
        newCommand.receivedCommandDataLength = details.receivedCommandDataLength;

        // endian?
        uint32_t opCodeAsUint32 = pack(details.receivedOpcode[3], details.receivedOpcode[2], details.receivedOpcode[1], details.receivedOpcode[0]);
        if (opCodeAsUint32 != opCodeXmit)
        {
            throw std::runtime_error("opcodes do not match");
        }
    }

    void perform_and_send_monitor_command(uvc::device & device, std::timed_mutex & mutex, unsigned char handle_id, hwmon_cmd & newCommand)
    {
        uint32_t opCodeXmit = (uint32_t)newCommand.cmd;

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

        send_hw_monitor_command(device, mutex, handle_id, details);

        // Error/exit conditions
        if (newCommand.oneDirection)
            return;

        memcpy(newCommand.receivedOpcode, details.receivedOpcode, 4);
        memcpy(newCommand.receivedCommandData, details.receivedCommandData, details.receivedCommandDataLength);
        newCommand.receivedCommandDataLength = details.receivedCommandDataLength;

        // endian?
        uint32_t opCodeAsUint32 = pack(details.receivedOpcode[3], details.receivedOpcode[2], details.receivedOpcode[1], details.receivedOpcode[0]);
        if (opCodeAsUint32 != opCodeXmit)
        {
            throw std::runtime_error("opcodes do not match");
        }
    }
    }
}
