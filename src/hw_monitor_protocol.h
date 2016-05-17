// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef HW_MONITOR_PROTOCOL_H
#define HW_MONITOR_PROTOCOL_H

#include "uvc.h"

#include <cstring>

namespace rsimpl
{
void execute_usb_command(uvc::device & device, std::timed_mutex & mutex, uint8_t *out, size_t outSize, uint32_t & op, uint8_t * in, size_t & inSize)
{
    // write
    errno = 0;

    int outXfer;

    if (!mutex.try_lock_for(std::chrono::milliseconds(IVCAM_MONITOR_MUTEX_TIMEOUT))) throw std::runtime_error("timed_mutex::try_lock_for(...) timed out");
    std::lock_guard<std::timed_mutex> guard(mutex, std::adopt_lock);

    bulk_transfer(device, IVCAM_MONITOR_ENDPOINT_OUT, out, (int) outSize, &outXfer, 1000); // timeout in ms

    // read
    if (in && inSize)
    {
        uint8_t buf[IVCAM_MONITOR_MAX_BUFFER_SIZE];

        errno = 0;

        bulk_transfer(device, IVCAM_MONITOR_ENDPOINT_IN, buf, sizeof(buf), &outXfer, 1000);
        if (outXfer < (int)sizeof(uint32_t)) throw std::runtime_error("incomplete bulk usb transfer");

        op = *(uint32_t *)buf;
        if (outXfer > (int)inSize) throw std::runtime_error("bulk transfer failed - user buffer too small");
        inSize = outXfer;
        memcpy(in, buf, inSize);
    }
}

void send_hw_monitor_command(uvc::device & device, std::timed_mutex & mutex, IVCAMCommandDetails & details)
{
    unsigned char outputBuffer[HW_MONITOR_BUFFER_SIZE];

    uint32_t op;
    size_t receivedCmdLen = HW_MONITOR_BUFFER_SIZE;

    execute_usb_command(device, mutex, (uint8_t*) details.sendCommandData, (size_t) details.sizeOfSendCommandData, op, outputBuffer, receivedCmdLen);
    details.receivedCommandDataLength = receivedCmdLen;

    if (details.oneDirection) return;

    if (details.receivedCommandDataLength < 4) throw std::runtime_error("received incomplete response to usb command");

    details.receivedCommandDataLength -= 4;
    memcpy(details.receivedOpcode, outputBuffer, 4);

    if (details.receivedCommandDataLength > 0 )
        memcpy(details.receivedCommandData, outputBuffer + 4, details.receivedCommandDataLength);
}

void perform_and_send_monitor_command(uvc::device & device, std::timed_mutex & mutex, IVCAMCommand & newCommand)
{
    uint32_t opCodeXmit = (uint32_t) newCommand.cmd;

    IVCAMCommandDetails details;
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

    send_hw_monitor_command(device, mutex, details);

    // Error/exit conditions
    if (newCommand.oneDirection)
        return;

    memcpy(newCommand.receivedOpcode, details.receivedOpcode, 4);
    memcpy(newCommand.receivedCommandData, details.receivedCommandData,details.receivedCommandDataLength);
    newCommand.receivedCommandDataLength = details.receivedCommandDataLength;

    // endian?
    uint32_t opCodeAsUint32 = pack(details.receivedOpcode[3], details.receivedOpcode[2], details.receivedOpcode[1], details.receivedOpcode[0]);
    if (opCodeAsUint32 != opCodeXmit)
    {
        throw std::runtime_error("opcodes do not match");
    }
}
}

#endif // HW_MONITOR_PROTOCOL_H
