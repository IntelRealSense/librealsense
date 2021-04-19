// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "firmware_logger_device.h"
#include <string>

namespace librealsense
{
    firmware_logger_device::firmware_logger_device(std::shared_ptr<context> ctx,
        const platform::backend_device_group group,
        std::shared_ptr<hw_monitor> hardware_monitor,
        const command& fw_logs_command, const command& flash_logs_command) :
        device(ctx, group),
        _hw_monitor(hardware_monitor),
        _fw_logs(),
        _flash_logs(),
        _flash_logs_initialized(false),
        _parser(nullptr),
        _fw_logs_command(fw_logs_command),
        _flash_logs_command(flash_logs_command) { }

    bool firmware_logger_device::get_fw_log(fw_logs::fw_logs_binary_data& binary_data)
    {
        bool result = false;
        if (_fw_logs.empty())
        {
            get_fw_logs_from_hw_monitor();
        }

        if (!_fw_logs.empty())
        {
            fw_logs::fw_logs_binary_data data;
            data = _fw_logs.front();
            _fw_logs.pop();
            binary_data = data;
            result = true;
        }
        return result;
    }

    unsigned int firmware_logger_device::get_number_of_fw_logs() const
    {
        return (unsigned int)_fw_logs.size();
    }

    void firmware_logger_device::get_fw_logs_from_hw_monitor()
    {
        auto res = _hw_monitor->send(_fw_logs_command);
        if (res.empty())
        {
            return;
        }

        auto beginOfLogIterator = res.begin();
        // convert bytes to fw_logs_binary_data
        for (int i = 0; i < res.size() / fw_logs::BINARY_DATA_SIZE; ++i)
        {
            auto endOfLogIterator = beginOfLogIterator + fw_logs::BINARY_DATA_SIZE;
            std::vector<uint8_t> resultsForOneLog;
            resultsForOneLog.insert(resultsForOneLog.begin(), beginOfLogIterator, endOfLogIterator);
            fw_logs::fw_logs_binary_data binary_data{ resultsForOneLog };
            _fw_logs.push(binary_data);
            beginOfLogIterator = endOfLogIterator;
        }
    }

    void firmware_logger_device::get_flash_logs_from_hw_monitor()
    {
        auto res = _hw_monitor->send(_flash_logs_command);

        if (res.empty())
        {
            LOG_INFO("Getting Flash logs failed!");
            return;
        }

        //erasing header
        int size_of_flash_logs_header = 27;
        res.erase(res.begin(), res.begin() + size_of_flash_logs_header);

        auto beginOfLogIterator = res.begin();
        // convert bytes to flash_logs_binary_data
        for (int i = 0; i < res.size() / fw_logs::BINARY_DATA_SIZE && *beginOfLogIterator == 160; ++i)
        {
            auto endOfLogIterator = beginOfLogIterator + fw_logs::BINARY_DATA_SIZE;
            std::vector<uint8_t> resultsForOneLog;
            resultsForOneLog.insert(resultsForOneLog.begin(), beginOfLogIterator, endOfLogIterator);
            fw_logs::fw_logs_binary_data binary_data{ resultsForOneLog };
            _flash_logs.push(binary_data);
            beginOfLogIterator = endOfLogIterator;
        }

        _flash_logs_initialized = true;
    }

    bool firmware_logger_device::get_flash_log(fw_logs::fw_logs_binary_data& binary_data)
    {
        bool result = false;
        if (!_flash_logs_initialized)
        {
            get_flash_logs_from_hw_monitor();
        }

        if (!_flash_logs.empty())
        {
            fw_logs::fw_logs_binary_data data;
            data = _flash_logs.front();
            _flash_logs.pop();
            binary_data = data;
            result = true;
        }
        return result;
    }

    bool firmware_logger_device::init_parser(std::string xml_content)
    {
        _parser = new fw_logs::fw_logs_parser(xml_content);

        return (_parser != nullptr);
    }

    bool firmware_logger_device::parse_log(const fw_logs::fw_logs_binary_data* fw_log_msg,
        fw_logs::fw_log_data* parsed_msg)
    {
        bool result = false;
        if (_parser && parsed_msg && fw_log_msg)
        {
            *parsed_msg = _parser->parse_fw_log(fw_log_msg);
            result = true;
        }

        return result;
    }

}
