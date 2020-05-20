// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "firmware_logger_device.h"
#include <string>

namespace librealsense
{
	firmware_logger_device::firmware_logger_device(std::shared_ptr<hw_monitor> hardware_monitor,
		std::string camera_op_code) :
		_hw_monitor(hardware_monitor),
        _last_timestamp(0),
        _timestamp_factor(0.00001)
	{
		auto op_code = static_cast<uint8_t>(std::stoi(camera_op_code.c_str()));
		_input_code = { 0x14, 0x00, 0xab, 0xcd, op_code, 0x00, 0x00, 0x00,
				 0xf4, 0x01, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00,
				 0x00, 0x00, 0x00, 0x00, 0x00,    0x00, 0x00, 0x00 };
	}


	std::vector<uint8_t> firmware_logger_device::get_fw_binary_logs() const
	{
		auto res = _hw_monitor->send(_input_code);
		if (res.empty())
		{
			throw std::runtime_error("Getting Firmware logs failed!");
		}
		return res;
	}

	std::vector<fw_logs::fw_log_data> firmware_logger_device::get_fw_logs()
	{
		auto res = _hw_monitor->send(_input_code);
		if (res.empty())
		{
			throw std::runtime_error("Getting Firmware logs failed!");
		}

        //TODO - REMI - check why this is required
        res.erase(res.begin(), res.begin() + 4);

        std::vector<fw_logs::fw_log_data> vec;
        auto beginOfLogIterator = res.begin();
        // convert bytes to fw_log_data
        for (int i = 0; i < res.size() / fw_logs::BINARY_DATA_SIZE; ++i)
        {
            auto endOfLogIterator = beginOfLogIterator + fw_logs::BINARY_DATA_SIZE;
            std::vector<uint8_t> resultsForOneLog;
            resultsForOneLog.insert(resultsForOneLog.begin(), beginOfLogIterator, endOfLogIterator);
            auto temp_pointer = reinterpret_cast<std::vector<uint8_t> const*>(resultsForOneLog.data());
            auto log = const_cast<char*>(reinterpret_cast<char const*>(temp_pointer));

            fw_logs::fw_log_data data = fill_log_data(log);
            beginOfLogIterator = endOfLogIterator;

            vec.push_back(data);
        }

		return vec;
	}

    std::vector<rs2_firmware_log_message> firmware_logger_device::get_fw_logs_msg()
    {
        auto res = _hw_monitor->send(_input_code);
        if (res.empty())
        {
            throw std::runtime_error("Getting Firmware logs failed!");
        }

        //TODO - REMI - check why this is required
        res.erase(res.begin(), res.begin() + 4);

        std::vector<rs2_firmware_log_message> vec;
        auto beginOfLogIterator = res.begin();
        // convert bytes to rs2_firmware_log_message
        for (int i = 0; i < res.size() / fw_logs::BINARY_DATA_SIZE; ++i)
        {
            auto endOfLogIterator = beginOfLogIterator + fw_logs::BINARY_DATA_SIZE;
            std::vector<uint8_t> resultsForOneLog;
            resultsForOneLog.insert(resultsForOneLog.begin(), beginOfLogIterator, endOfLogIterator);
            auto temp_pointer = reinterpret_cast<std::vector<uint8_t> const*>(resultsForOneLog.data());
            auto log = const_cast<char*>(reinterpret_cast<char const*>(temp_pointer));

            fw_logs::fw_log_data data = fill_log_data(log);
            rs2_firmware_log_message msg = data.to_rs2_firmware_log_message();
            beginOfLogIterator = endOfLogIterator;

            vec.push_back(msg);
        }
        return vec;
    }

    fw_logs::fw_log_data firmware_logger_device::fill_log_data(const char* fw_logs)
    {
        fw_logs::fw_log_data log_data;
        //fw_string_formatter reg_exp(_fw_logs_formating_options.get_enums());
        //fw_log_event log_event_data;

        auto* log_binary = reinterpret_cast<const fw_logs::fw_log_binary*>(fw_logs);

        //parse first DWORD
        log_data.magic_number = static_cast<uint32_t>(log_binary->dword1.bits.magic_number);
        log_data.severity = static_cast<uint32_t>(log_binary->dword1.bits.severity);

        log_data.file_id = static_cast<uint32_t>(log_binary->dword1.bits.file_id);
        log_data.group_id = static_cast<uint32_t>(log_binary->dword1.bits.group_id);

        //parse second DWORD
        log_data.event_id = static_cast<uint32_t>(log_binary->dword2.bits.event_id);
        log_data.line = static_cast<uint32_t>(log_binary->dword2.bits.line_id);
        log_data.sequence = static_cast<uint32_t>(log_binary->dword2.bits.seq_id);

        //parse third DWORD
        log_data.p1 = static_cast<uint32_t>(log_binary->dword3.p1);
        log_data.p2 = static_cast<uint32_t>(log_binary->dword3.p2);

        //parse forth DWORD
        log_data.p3 = static_cast<uint32_t>(log_binary->dword4.p3);

        //parse fifth DWORD
        log_data.timestamp = log_binary->dword5.timestamp;

        //-------------------------------------------------------------
        if (_last_timestamp == 0)
        {
            log_data.delta = 0;
        }
        else
        {
            log_data.delta = (log_data.timestamp - _last_timestamp) * _timestamp_factor;
        }

        _last_timestamp = log_data.timestamp;

        log_data.thread_id = static_cast<uint32_t>(log_binary->dword1.bits.thread_id);
        //std::string message;
        /*
        _fw_logs_formating_options.get_event_data(log_data->event_id, &log_event_data);
        uint32_t params[3] = { log_data->p1, log_data->p2, log_data->p3 };
        reg_exp.generate_message(log_event_data.line, log_event_data.num_of_params, params, &log_data->message);

        _fw_logs_formating_options.get_file_name(log_data->file_id, &log_data->file_name);
        _fw_logs_formating_options.get_thread_name(static_cast<uint32_t>(log_binary->dword1.bits.thread_id),
            &log_data->thread_name);*/

        return log_data;
    }
}