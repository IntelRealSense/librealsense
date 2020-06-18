// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "firmware_logger_device.h"
#include "ds5/ds5-private.h"
#include "l500/l500-private.h"
#include "ivcam/sr300.h"
#include <string>

namespace librealsense
{
    std::map<firmware_logger_device::logs_commands_group, firmware_logger_device::logs_commands>
        firmware_logger_device::_logs_commands_per_group =
    {
        {LOGS_COMMANDS_GROUP_DS, {command{ds::GLD, 0x1f4}, command{ds::FRB, 0x17a000, 0x3f8}}},
        {LOGS_COMMANDS_GROUP_L5, {command{ivcam2::GLD, 0x1f4}, command{ivcam2::FRB, 0x0011E000, 0x3f8}}},
        {LOGS_COMMANDS_GROUP_SR, {command{ivcam::GLD, 0x1f4}, command{ivcam::FlashRead, 0x000B6000, 0x3f8}}}
    };


    firmware_logger_device::logs_commands_group firmware_logger_device::get_logs_commands_group(uint16_t device_pid)
    {
        logs_commands_group group = LOGS_COMMANDS_GROUP_NONE;
        switch (device_pid)
        {
        case ds::RS400_PID:
        case ds::RS410_PID:
        case ds::RS415_PID:
        case ds::RS430_PID:
        case ds::RS430_MM_PID:
        case ds::RS_USB2_PID:
        case ds::RS_RECOVERY_PID:
        case ds::RS_USB2_RECOVERY_PID:
        case ds::RS400_IMU_PID:
        case ds::RS420_PID:
        case ds::RS420_MM_PID:
        case ds::RS410_MM_PID:
        case ds::RS400_MM_PID:
        case ds::RS430_MM_RGB_PID:
        case ds::RS460_PID:
        case ds::RS435_RGB_PID:
        case ds::RS405U_PID:
        case ds::RS435I_PID:
        case ds::RS416_PID:
        case ds::RS430I_PID:
        case ds::RS465_PID:
        case ds::RS416_RGB_PID:
        case ds::RS405_PID:
        case ds::RS455_PID:
            group = LOGS_COMMANDS_GROUP_DS;
            break;
        case L500_RECOVERY_PID:
        case L500_PID:
        case L515_PID_PRE_PRQ:
        case L515_PID:
            group = LOGS_COMMANDS_GROUP_L5;
            break;
        case SR300_PID:
        case SR300v2_PID:
        case SR300_RECOVERY:
            group = LOGS_COMMANDS_GROUP_SR;
            break;
        default:
            break;
        }
        return group;
    }

    firmware_logger_device::firmware_logger_device(std::shared_ptr<hw_monitor> hardware_monitor,
        const synthetic_sensor& depth_sensor) :
        _hw_monitor(hardware_monitor),
        _fw_logs(),
        _flash_logs(),
        _flash_logs_initialized(false),
        _parser(nullptr)
    {
        std::string pid_str (depth_sensor.get_info(RS2_CAMERA_INFO_PRODUCT_ID));
        std::stringstream ss;
        ss << std::hex << pid_str;
        ss >> _device_pid;
    }

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


    void firmware_logger_device::get_fw_logs_from_hw_monitor()
    {
        logs_commands_group group = get_logs_commands_group(_device_pid);
        if (group == LOGS_COMMANDS_GROUP_NONE)
        {
            LOG_INFO("Firmware logs not set for this device!");
            return;
        }
        auto it = _logs_commands_per_group.find(group);
        if (it == _logs_commands_per_group.end())
        {
            LOG_INFO("Firmware logs not set for this device!");
            return;
        }
        auto res = _hw_monitor->send(it->second.fw_logs_command);
        if (res.empty())
        {
            return;
        }

        auto beginOfLogIterator = res.begin();
        // convert bytes to fw_logs_binary_data
        for (int i = 0; i < res.size() / fw_logs::BINARY_DATA_SIZE; ++i)
        {
            if (*beginOfLogIterator == 0)
                break;
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
        logs_commands_group group = get_logs_commands_group(_device_pid);
        if (group == LOGS_COMMANDS_GROUP_NONE)
        {
            LOG_INFO("Flash logs not set for this device!");
            return;
        }
        auto it = _logs_commands_per_group.find(group);
        if (it == _logs_commands_per_group.end())
        {
            LOG_INFO("Flash logs not set for this device!");
            return;
        }
        auto res = _hw_monitor->send(it->second.flash_logs_command);

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
