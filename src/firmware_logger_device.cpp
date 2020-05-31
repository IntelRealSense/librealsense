// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "firmware_logger_device.h"
#include <string>

namespace librealsense
{
	firmware_logger_device::firmware_logger_device(std::shared_ptr<hw_monitor> hardware_monitor,
		std::string camera_op_code) :
		_hw_monitor(hardware_monitor),
        _fw_logs_queue(),
        _flash_logs_queue()
	{
		auto op_code = static_cast<uint8_t>(std::stoi(camera_op_code.c_str()));
        _input_code_for_fw_logs = { 0x14, 0x00, 0xab, 0xcd, op_code, 0x00, 0x00, 0x00,
				 0xf4, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

        //TODO - get the right code for flah logs
        auto flash_logs_op_code = static_cast<uint8_t>(0x09);// static_cast<uint8_t>(std::stoi(camera_op_code.c_str()));
        //auto flash_logs_offset = { 0x7a, 0x01, 0x00, 0x00 };
        //auto flash_logs_length = { 0xf8, 0x03, 0x00, 0x00 };
        _input_code_for_flash_logs =  { 0x14, 0x00, 0xab, 0xcd, flash_logs_op_code, 0x00, 0x00, 0x00,
                 0x00, 0xa0, 0x17, 0x00, 0xf8, 0x03, 0x00, 0x00,
                 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	}
    
    bool firmware_logger_device::get_fw_log(fw_logs::fw_logs_binary_data& binary_data)
    {
        bool result = false;
        if (_fw_logs_queue.empty())
        {
            get_logs_from_hw_monitor(LOG_TYPE_FW_LOG);
        }
        
        if (!_fw_logs_queue.empty())
        {
            fw_logs::fw_logs_binary_data data;
            data = _fw_logs_queue.front();
            _fw_logs_queue.pop();
            binary_data = data;
            result = true;
        }
        return result;
    }

    bool firmware_logger_device::get_flash_log(fw_logs::fw_logs_binary_data& binary_data)
    {
        bool result = false;
        if (_flash_logs_queue.empty())
        {
            get_logs_from_hw_monitor(LOG_TYPE_FLASH_LOG);
        }

        if (!_flash_logs_queue.empty())
        {
            fw_logs::fw_logs_binary_data data;
            data = _flash_logs_queue.front();
            _flash_logs_queue.pop();
            binary_data = data;
            result = true;
        }
        return result;
    }

    bool firmware_logger_device::get_log(fw_logs::fw_logs_binary_data& binary_data, log_type type)
    {
        std::queue<fw_logs::fw_logs_binary_data>* logs_queue;
        logs_queue = (type == LOG_TYPE_FW_LOG)  ? &_fw_logs_queue : &_flash_logs_queue;

        bool result = false;
        if ((*logs_queue).empty())
        {
            get_logs_from_hw_monitor(type);
        }

        // if log type is fw log --> pop should be done
        // if log type is flash log --> no pop should be done (queue pulled only once)
        if (!(*logs_queue).empty())
        {
            fw_logs::fw_logs_binary_data data;
            data = (*logs_queue).front();
            (*logs_queue).pop();
            binary_data = data;
            result = true;
        }
        return result;
    }

    bool firmware_logger_device::get_flash_log(fw_logs::fw_logs_binary_data& binary_data)
    {
        std::queue<fw_logs::fw_logs_binary_data>* logs_queue;
        logs_queue = &_flash_logs_queue;

        bool result = false;
        if (_flash_logs_queue.empty())
        {
            get_logs_from_hw_monitor(LOG_TYPE_FLASH_LOG);
        }

        // if log type is fw log --> pop should be done
        // if log type is flash log --> no pop should be done (queue pulled only once)
        if (!(*logs_queue).empty())
        {
            fw_logs::fw_logs_binary_data data;
            data = (*logs_queue).front();
            (*logs_queue).pop();
            binary_data = data;
            result = true;
        }
        return result;
    }

    // get number N of flash logs - save in object
    // user to call get_flash_log N times
    // index of vector (not queue) to be saved in fw logger object
    // if user calls get_flash_log N+1 times, index to be reset to 0 (cyclic read)

    void firmware_logger_device::get_logs_from_hw_monitor(log_type type)
    {
        std::queue<fw_logs::fw_logs_binary_data>& logs_queue = (type == LOG_TYPE_FW_LOG) ? _fw_logs_queue : _flash_logs_queue;
        const std::vector <uint8_t>& input_code = (type == LOG_TYPE_FW_LOG) ? _input_code_for_fw_logs : _input_code_for_flash_logs;
        int size_of_header = (type == LOG_TYPE_FW_LOG) ? 4 : 31;
        auto res = _hw_monitor->send(input_code);
        if (res.empty())
        {
            throw std::runtime_error("Getting Firmware logs failed!");
        }

        int limit = (type == LOG_TYPE_FW_LOG) ? (res.size() - 4) / fw_logs::BINARY_DATA_SIZE :
            static_cast<uint32_t>(res[0] + (res[1] << 8) + (res[2] << 16) + (res[3] << 24));
        
        //erasing header
        res.erase(res.begin(), res.begin() + size_of_header);

        auto beginOfLogIterator = res.begin();
        // convert bytes to fw_logs_binary_data
        for (int i = 0; i < limit; ++i)
        {
            if (*beginOfLogIterator == 0)
                break;
            auto endOfLogIterator = beginOfLogIterator + fw_logs::BINARY_DATA_SIZE;
            std::vector<uint8_t> resultsForOneLog;
            resultsForOneLog.insert(resultsForOneLog.begin(), beginOfLogIterator, endOfLogIterator);
            fw_logs::fw_logs_binary_data binary_data{ resultsForOneLog };
            logs_queue.push(binary_data);
            beginOfLogIterator = endOfLogIterator;
        }
    }

    int firmware_logger_device::get_number_of_flash_logs()
    {

    }
}