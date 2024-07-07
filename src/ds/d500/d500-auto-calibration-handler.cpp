// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "d500-auto-calibration-handler.h"


namespace librealsense
{
    d500_auto_calibrated_handler_hw_monitor::d500_auto_calibrated_handler_hw_monitor()
    {

    }

    d500_calibration_answer d500_auto_calibrated_handler_hw_monitor::get_status() const
    {
        auto res = _hw_monitor->send(command{ ds::GET_CALIB_STATUS });
        // checking size of received buffer
        if (!check_buffer_size_from_get_calib_status(res))
            throw std::runtime_error("GET_CALIB_STATUS returned struct with wrong size");

        return *reinterpret_cast<d500_calibration_answer*>(res.data());
    }

    std::vector<uint8_t, std::allocator<uint8_t>> d500_auto_calibrated_handler_hw_monitor::run_auto_calibration(d500_calibration_mode _mode)
    {
        return _hw_monitor->send(command{ ds::SET_CALIB_MODE, static_cast<uint32_t>(_mode), 1 /*always*/ });
    }

    void d500_auto_calibrated_handler_hw_monitor::set_hw_monitor_for_auto_calib(std::shared_ptr<hw_monitor> hwm)
    {
        _hw_monitor = hwm;
    }

    bool d500_auto_calibrated_handler_hw_monitor::check_buffer_size_from_get_calib_status(std::vector<uint8_t> res) const
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

    rs2_calibration_config d500_auto_calibrated_handler_hw_monitor::get_calibration_config() const
    {
        rs2_calibration_config_with_header* calib_config_with_header;

        // prepare command
        using namespace ds;
        command cmd(GET_HKR_CONFIG_TABLE,
            static_cast<int>(d500_calib_location::d500_calib_flash_memory),
            static_cast<int>(d500_calibration_table_id::calib_cfg_id),
            static_cast<int>(d500_calib_type::d500_calib_dynamic));
        auto res = _hw_monitor->send(cmd);

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

    void d500_auto_calibrated_handler_hw_monitor::set_calibration_config(const rs2_calibration_config& calib_config)
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
        auto data_as_ptr = reinterpret_cast<const uint8_t*>(&calib_config_with_header);

        // prepare command
        command cmd(ds::SET_HKR_CONFIG_TABLE,
            static_cast<int>(ds::d500_calib_location::d500_calib_flash_memory),
            static_cast<int>(ds::d500_calibration_table_id::calib_cfg_id),
            static_cast<int>(ds::d500_calib_type::d500_calib_dynamic));
        cmd.data.insert(cmd.data.end(), data_as_ptr, data_as_ptr + sizeof(rs2_calibration_config_with_header));
        cmd.require_response = false;

        // send command 
        _hw_monitor->send(cmd);
    }


}// namespace librealsense