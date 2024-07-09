// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "d500-auto-calibration-handler.h"
#include "d500-device.h"


namespace librealsense
{
    d500_calibration_answer d500_auto_calibrated_handler_hw_monitor::get_status() const
    {
        if (auto hwm = _hw_monitor.lock())
        {
            auto res = hwm->send(command{ ds::GET_CALIB_STATUS });
            // checking size of received buffer
            if (!check_buffer_size_from_get_calib_status(res))
                throw std::runtime_error("GET_CALIB_STATUS returned struct with wrong size");

            return *reinterpret_cast<d500_calibration_answer*>(res.data());
        }
        throw std::runtime_error("hw monitor has not been set");
    }

    std::vector<uint8_t> d500_auto_calibrated_handler_hw_monitor::run_auto_calibration(d500_calibration_mode _mode)
    {
        if (auto hwm = _hw_monitor.lock())
        {
            return hwm->send(command{ ds::SET_CALIB_MODE, static_cast<uint32_t>(_mode), 1 /*always*/ });
        }
        throw std::runtime_error("hw monitor has not been set");
    }

    void d500_auto_calibrated_handler_hw_monitor::set_hw_monitor_for_auto_calib(std::shared_ptr<hw_monitor> hwm)
    {
        _hw_monitor = hwm;
    }

    static bool check_buffer_size_from_get_calib_status_method(std::vector<uint8_t> res)
    {
        // the GET_CALIB_STATUS command will return:
        // - 3 bytes during the whole process
        // - 515 bytes (3 bytes + 512 bytes of the depth calibration) when the state is Complete

        bool is_size_ok = false;
        if (res.size() > 1)
        {
            if (res[0] < static_cast<int>(d500_calibration_state::RS2_D500_CALIBRATION_STATE_COMPLETE) &&
                res.size() == (sizeof(d500_calibration_answer) - sizeof(ds::d500_coefficients_table)))
                is_size_ok = true;

            if (res[0] == static_cast<int>(d500_calibration_state::RS2_D500_CALIBRATION_STATE_COMPLETE) &&
                res.size() == sizeof(d500_calibration_answer))
                is_size_ok = true;
        }
        return is_size_ok;
    }

    bool d500_auto_calibrated_handler_hw_monitor::check_buffer_size_from_get_calib_status(std::vector<uint8_t> res) const
    {
        return check_buffer_size_from_get_calib_status_method(res);
    }

    rs2_calibration_config d500_auto_calibrated_handler_hw_monitor::get_calibration_config() const
    {        
        if (auto hwm = _hw_monitor.lock())
        {
            rs2_calibration_config_with_header* calib_config_with_header;

            // prepare command
            using namespace ds;
            command cmd(GET_HKR_CONFIG_TABLE,
                static_cast<int>(d500_calib_location::d500_calib_flash_memory),
                static_cast<int>(d500_calibration_table_id::calib_cfg_id),
                static_cast<int>(d500_calib_type::d500_calib_dynamic));

            auto res = hwm->send(cmd);

            if (res.size() < sizeof(rs2_calibration_config_with_header))
            {
                throw io_exception(rsutils::string::from() << "Calibration config reading failed");
            }
            calib_config_with_header = reinterpret_cast<rs2_calibration_config_with_header*>(res.data());

            // check CRC before returning result       
            auto computed_crc32 = rsutils::number::calc_crc32(res.data() + sizeof(rs2_calibration_config_header), sizeof(rs2_calibration_config));
            if (computed_crc32 != calib_config_with_header->header.crc32)
            {
                throw invalid_value_exception(rsutils::string::from() << "Invalid CRC value for calibration config table");
            }

            return calib_config_with_header->payload;
        }
        throw std::runtime_error("hw monitor has not been set"); 
    }

    static std::vector<uint8_t> add_header_to_calib_config(const rs2_calibration_config& calib_config)
    {
        // calculate CRC
        uint32_t computed_crc32 = rsutils::number::calc_crc32(reinterpret_cast<const uint8_t*>(&calib_config),
            sizeof(rs2_calibration_config));

        // prepare vector of data to be sent (header + sp)
        rs2_calibration_config_with_header calib_config_with_header;
        uint16_t version = ((uint16_t)0x01 << 8) | 0x01;  // major=0x01, minor=0x01 --> ver = major.minor
        uint32_t calib_version = 0;  // ignoring this field, as requested by sw architect
        calib_config_with_header.header = { version, static_cast<uint16_t>(ds::d500_calibration_table_id::calib_cfg_id),
            sizeof(rs2_calibration_config), calib_version, computed_crc32 };
        calib_config_with_header.payload = calib_config;
        auto data_as_ptr = reinterpret_cast<uint8_t*>(&calib_config_with_header);
        return std::vector<uint8_t>(data_as_ptr, data_as_ptr + sizeof(rs2_calibration_config_with_header));
    }

    void d500_auto_calibrated_handler_hw_monitor::set_calibration_config(const rs2_calibration_config& calib_config)
    {
        if (auto hwm = _hw_monitor.lock())
        {
            auto calib_config_with_header = add_header_to_calib_config(calib_config);

            // prepare command
            command cmd(ds::SET_HKR_CONFIG_TABLE,
                static_cast<int>(ds::d500_calib_location::d500_calib_flash_memory),
                static_cast<int>(ds::d500_calibration_table_id::calib_cfg_id),
                static_cast<int>(ds::d500_calib_type::d500_calib_dynamic));
            cmd.data = calib_config_with_header;
            cmd.require_response = false;

            // send command 
            hwm->send(cmd);
        }
        else
        {
            throw std::runtime_error("hw monitor has not been set");
        }
    }

    bool d500_auto_calibrated_handler_debug_protocol::check_buffer_size_from_get_calib_status(std::vector<uint8_t> res) const
    {
        return check_buffer_size_from_get_calib_status_method(res);
    }

    void d500_auto_calibrated_handler_debug_protocol::set_device_for_auto_calib(std::shared_ptr<d500_device> device)
    {
        _dev = device;
    }

    d500_calibration_answer d500_auto_calibrated_handler_debug_protocol::get_status() const
    {
        if (auto device = _dev.lock())
        {
            auto cmd = device->build_command(ds::GET_CALIB_STATUS);
            auto res = device->send_receive_raw_data(cmd);

            // checking size of received buffer
            if (!check_buffer_size_from_get_calib_status(res))
                throw std::runtime_error("GET_CALIB_STATUS returned struct with wrong size");

            return *reinterpret_cast<d500_calibration_answer*>(res.data());
        }
        throw std::runtime_error("device has not been set");
    }

    std::vector<uint8_t> d500_auto_calibrated_handler_debug_protocol::run_auto_calibration(d500_calibration_mode _mode)
    {
        if (auto device = _dev.lock())
        {
            auto cmd = device->build_command(ds::SET_CALIB_MODE, static_cast<uint32_t>(_mode), 1 /*always*/);
            return device->send_receive_raw_data(cmd);
        }
        throw std::runtime_error("device has not been set");
    }

    void d500_auto_calibrated_handler_debug_protocol::set_calibration_config(const rs2_calibration_config& calib_config)
    {
        if (auto device = _dev.lock())
        {
            auto calib_config_with_header = add_header_to_calib_config(calib_config);

            // prepare command
            auto cmd = device->build_command(ds::SET_HKR_CONFIG_TABLE,
                static_cast<int>(ds::d500_calib_location::d500_calib_flash_memory),
                static_cast<int>(ds::d500_calibration_table_id::calib_cfg_id),
                static_cast<int>(ds::d500_calib_type::d500_calib_dynamic), 0,
                calib_config_with_header.data(), sizeof(rs2_calibration_config_with_header));

            // send command 
            auto res = device->send_receive_raw_data(cmd);
        }
        else
        {
            throw std::runtime_error("device has not been set");
        }
    }

    rs2_calibration_config d500_auto_calibrated_handler_debug_protocol::get_calibration_config() const
    {
        if (auto device = _dev.lock())
        {
            rs2_calibration_config_with_header* calib_config_with_header;

            // prepare command
            using namespace ds;
            auto cmd = device->build_command(GET_HKR_CONFIG_TABLE,
                static_cast<int>(d500_calib_location::d500_calib_flash_memory),
                static_cast<int>(d500_calibration_table_id::calib_cfg_id),
                static_cast<int>(d500_calib_type::d500_calib_dynamic));

            // sending command
            auto res = device->send_receive_raw_data(cmd);

            if (res.size() < sizeof(rs2_calibration_config_with_header))
            {
                throw io_exception(rsutils::string::from() << "Calibration config reading failed");
            }
            calib_config_with_header = reinterpret_cast<rs2_calibration_config_with_header*>(res.data());

            // check CRC before returning result       
            auto computed_crc32 = rsutils::number::calc_crc32(res.data() + sizeof(rs2_calibration_config_header), sizeof(rs2_calibration_config));
            if (computed_crc32 != calib_config_with_header->header.crc32)
            {
                throw invalid_value_exception(rsutils::string::from() << "Invalid CRC value for calibration config table");
            }

            return calib_config_with_header->payload;
        }
        throw std::runtime_error("device has not been set");
    }

}// namespace librealsense