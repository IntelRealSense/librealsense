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


        void execute_usb_command(uvc::device & device, std::timed_mutex & mutex, uint8_t *out, size_t outSize, uint32_t & op, uint8_t * in, size_t & inSize)
        {
            // write
            errno = 0;

            int outXfer;

            if (!mutex.try_lock_for(std::chrono::milliseconds(IVCAM_MONITOR_MUTEX_TIMEOUT))) throw std::runtime_error("timed_mutex::try_lock_for(...) timed out");
            std::lock_guard<std::timed_mutex> guard(mutex, std::adopt_lock);

            bulk_transfer(device, IVCAM_MONITOR_ENDPOINT_OUT, out, (int)outSize, &outXfer, 1000); // timeout in ms

            std::this_thread::sleep_for(std::chrono::milliseconds(20));
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

        void send_hw_monitor_command(uvc::device & device, std::timed_mutex & mutex, hwmon_cmd_details & details)
        {
            unsigned char outputBuffer[HW_MONITOR_BUFFER_SIZE];

            uint32_t op;
            size_t receivedCmdLen = HW_MONITOR_BUFFER_SIZE;

            execute_usb_command(device, mutex, (uint8_t*)details.sendCommandData, (size_t)details.sizeOfSendCommandData, op, outputBuffer, receivedCmdLen);
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

            send_hw_monitor_command(device, mutex, details);

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

        void perform_and_send_monitor_command(uvc::device & device, std::timed_mutex & mutex, unsigned char /*handle_id*/, hwmon_cmd & newCommand)
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

            send_hw_monitor_command(device, mutex, details);

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
        void i2c_write_reg(int command, uvc::device & device, uint16_t slave_address, uint16_t reg, uint32_t value)
        {
            hw_monitor::hwmon_cmd cmd(command);

            cmd.Param1 = slave_address;
            cmd.Param2 = reg;
            cmd.Param3 = sizeof(value);

            memcpy(cmd.data, &value, sizeof(value));
            cmd.sizeOfSendCommandData = sizeof(value);

            std::timed_mutex mutex;
            perform_and_send_monitor_command(device, mutex, cmd);

            return;
        }

        // Read a 32 bit value from the i2c register.
        void i2c_read_reg(int command, uvc::device & device, uint16_t slave_address, uint16_t reg, uint32_t size, byte* data)
        {
            hw_monitor::hwmon_cmd cmd(command);

            cmd.Param1 = slave_address;
            cmd.Param2 = reg;
            cmd.Param3 = size;
            const int num_retries = 10;
            std::timed_mutex mutex;
            int retries = 0;
            do {
                try {
                    hw_monitor::perform_and_send_monitor_command(device, mutex, cmd);

                    // validate that the size is of 32 bit (value size).
                    if (cmd.receivedCommandDataLength == size)
                    {
                        memcpy(data, cmd.receivedCommandData, cmd.receivedCommandDataLength);
                        break;
                    }
                }
                catch (...)
                {
                    retries++;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    if (retries == num_retries)
                    {
                        throw;
                    }
                }

            } while (retries < num_retries);
            return;
        }

        void check_eeprom_read_write_status(int IRB_opcode, uvc::device & device)
        {
            uint32_t value = 0;
            i2c_read_reg(IRB_opcode, device, 0x42, 0x70, sizeof(uint32_t), (byte*)&value);
            if (value & 0x100)
            {
                throw std::runtime_error(to_string() << "EEPRom Error" << value);
            }
        }


        void read_from_eeprom(int IRB_opcode, int IWB_opcode, uvc::device & device, unsigned int offset, int size, byte* data)
        {
            unsigned int  command = offset;

            //bits[0:12] - Offset In EEprom
            command &= 0x00001FFF;

            //bit[13] - Direction 0 = read, 1 = write
            //Doesn't do anything since it is already 0. Just for redability/consistency.
            command &= 0xFFFFDFFF;

            //bit[14:15] - Reserved
            //Nothing to do

            //bits[16:23] - Size to read
            unsigned int  lengthR = size;
            lengthR = lengthR << 16;

            command |= lengthR;

            //bit[14:15] - Reserved
            //Nothing to do

            //expected = 0x100005

            uint32_t value = 0;
            i2c_read_reg(IRB_opcode, device, 0x42, 0x70, sizeof(uint32_t), (byte*)&value); //clean the register
            i2c_write_reg(IWB_opcode, device, 0x42, 0x0C, command);
            check_eeprom_read_write_status(IRB_opcode, device);
            i2c_read_reg(IRB_opcode, device, 0x42, 0xD0, size, data);
            
        }
        void get_raw_data(uint8_t opcode, uvc::device & device, std::timed_mutex & mutex, uint8_t * data, size_t & bytesReturned)
        {
            hw_monitor::hwmon_cmd command(opcode);

            perform_and_send_monitor_command(device, mutex, command);
            memcpy(data, command.receivedCommandData, HW_MONITOR_BUFFER_SIZE);
            bytesReturned = command.receivedCommandDataLength;
        }
    }
}
