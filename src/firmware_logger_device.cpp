// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "firmware_logger_device.h"
#include <string>

namespace librealsense
{
	firmware_logger_device::firmware_logger_device(std::shared_ptr<hw_monitor> hardware_monitor,
		std::string camera_op_code) :
		_hw_monitor(hardware_monitor),
        _message_queue()
	{
		auto op_code = static_cast<uint8_t>(std::stoi(camera_op_code.c_str()));
		_input_code = { 0x14, 0x00, 0xab, 0xcd, op_code, 0x00, 0x00, 0x00,
				 0xf4, 0x01, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00,
				 0x00, 0x00, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00 };
	}

    bool firmware_logger_device::get_fw_log(fw_logs::fw_logs_binary_data& binary_data)
    {
        bool result = false;
        if (_message_queue.empty())
        {
            get_fw_logs_from_hw_monitor();
        }
        
        if (!_message_queue.empty())
        {
            fw_logs::fw_logs_binary_data data;
            data = _message_queue.front();
            _message_queue.pop();
            binary_data = data;
            result = true;
        }
        return result;
    }

    void firmware_logger_device::get_fw_logs_from_hw_monitor()
    {
        auto res = _hw_monitor->send(_input_code);
        if (res.empty())
        {
            throw std::runtime_error("Getting Firmware logs failed!");
        }

        //erasing header
        res.erase(res.begin(), res.begin() + 4);

        auto beginOfLogIterator = res.begin();
        // convert bytes to fw_logs_binary_data
        for (int i = 0; i < res.size() / fw_logs::BINARY_DATA_SIZE; ++i)
        {
            auto endOfLogIterator = beginOfLogIterator + fw_logs::BINARY_DATA_SIZE;
            std::vector<uint8_t> resultsForOneLog;
            resultsForOneLog.insert(resultsForOneLog.begin(), beginOfLogIterator, endOfLogIterator);
            fw_logs::fw_logs_binary_data binary_data{ resultsForOneLog };
            _message_queue.push(binary_data);
            beginOfLogIterator = endOfLogIterator;
        }
    }


    
}