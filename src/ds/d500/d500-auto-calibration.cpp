// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.


#include "d500-auto-calibration.h"
#include "d500-private.h"
#include <src/ds/ds-calib-common.h>

#include <rsutils/string/from.h>
#include <rsutils/json.h>
#include "d500-types/calibration-config.h"

namespace librealsense
{
    static const std::string d500_calibration_state_strings[] = {
        "Idle",
        "In Process",
        "Done Success",
        "Done Failure",
        "Flash Update",
        "Complete"
    };

    static const std::string d500_calibration_result_strings[] = {
        "Unkown",
        "Success",
        "Failed to Converge",
        "Failed to Run"
    };

#pragma pack(push, 1)
    struct d500_calibration_answer
    {
        uint8_t calibration_state;
        int8_t calibration_progress;
        uint8_t calibration_result;
        ds::d500_coefficients_table depth_calibration;
    };
#pragma pack(pop)


    d500_auto_calibrated::d500_auto_calibrated() :
        _mode (d500_calibration_mode::RS2_D500_CALIBRATION_MODE_RESERVED),
        _state (d500_calibration_state::RS2_D500_CALIBRATION_STATE_IDLE),
        _result(d500_calibration_result::RS2_D500_CALIBRATION_RESULT_UNKNOWN)
    {}

    bool d500_auto_calibrated::check_buffer_size_from_get_calib_status(std::vector<uint8_t> res) const
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

    void d500_auto_calibrated::check_preconditions_and_set_state()
    {
        // moreover, relevant only for d585s, not for d555e
        if (_mode == d500_calibration_mode::RS2_D500_CALIBRATION_MODE_RUN ||
            _mode == d500_calibration_mode::RS2_D500_CALIBRATION_MODE_DRY_RUN)
        {
            // calibration state to be IDLE or COMPLETE
            auto res = _hw_monitor->send(command{ ds::GET_CALIB_STATUS });
            
            // checking size of received buffer
            if (!check_buffer_size_from_get_calib_status(res))
                throw std::runtime_error("GET_CALIB_STATUS returned struct with wrong size");

            d500_calibration_answer calib_result = *reinterpret_cast<d500_calibration_answer*>(res.data());
            _state = static_cast<d500_calibration_state>(calib_result.calibration_state);
            if (!(_state == d500_calibration_state::RS2_D500_CALIBRATION_STATE_IDLE ||
                _state == d500_calibration_state::RS2_D500_CALIBRATION_STATE_COMPLETE))
            {
                LOG_ERROR("Calibration State is not Idle nor Complete - pleare restart the device");
                throw std::runtime_error("OCC triggerred when Calibration State is not Idle not Complete");
            }
        }
        
        if (_mode == d500_calibration_mode::RS2_D500_CALIBRATION_MODE_ABORT)
        {
            // calibration state to be IN_PROCESS
            auto res = _hw_monitor->send(command{ ds::GET_CALIB_STATUS });
            if (!check_buffer_size_from_get_calib_status(res))
                throw std::runtime_error("GET_CALIB_STATUS returned struct with wrong size");

            d500_calibration_answer calib_result = *reinterpret_cast<d500_calibration_answer*>(res.data());
            _state = static_cast<d500_calibration_state>(calib_result.calibration_state);
            if (!(_state == d500_calibration_state::RS2_D500_CALIBRATION_STATE_PROCESS))
            {
                LOG_ERROR("Calibration State is not In Process - so it could not be aborted");
                throw std::runtime_error("OCC aborted when Calibration State is not In Process");
            }
        }
    }

    void d500_auto_calibrated::get_mode_from_json(const std::string& json)
    {
        _mode = d500_calibration_mode::RS2_D500_CALIBRATION_MODE_RUN;
        if (json.find("dry run") != std::string::npos)
            _mode = d500_calibration_mode::RS2_D500_CALIBRATION_MODE_DRY_RUN;
        else if (json.find("abort") != std::string::npos)
            _mode = d500_calibration_mode::RS2_D500_CALIBRATION_MODE_ABORT;
    }

    std::vector<uint8_t> d500_auto_calibrated::run_on_chip_calibration(int timeout_ms, std::string json, 
        float* const health, rs2_update_progress_callback_sptr progress_callback)
    {
        std::vector<uint8_t> res;
        try
        {
            get_mode_from_json(json);

            // checking preconditions
            check_preconditions_and_set_state();

            // sending command to start calibration
            res = _hw_monitor->send(command{ ds::SET_CALIB_MODE, static_cast<uint32_t>(_mode), 1 /*always*/});

            if (_mode == d500_calibration_mode::RS2_D500_CALIBRATION_MODE_RUN ||
                _mode == d500_calibration_mode::RS2_D500_CALIBRATION_MODE_DRY_RUN)
            {
                res = update_calibration_status(timeout_ms, progress_callback);
            }
            else if (_mode == d500_calibration_mode::RS2_D500_CALIBRATION_MODE_ABORT)
            {
                res = update_abort_status();
            }
        }
        catch (std::runtime_error&)
        {
            throw;
        }
        catch(...)
        {
            std::string error_message_prefix = "\nRUN OCC ";
            if (_mode == d500_calibration_mode::RS2_D500_CALIBRATION_MODE_DRY_RUN)
                error_message_prefix = "\nDRY RUN OCC ";
            else if (_mode == d500_calibration_mode::RS2_D500_CALIBRATION_MODE_ABORT)
                error_message_prefix = "\nABORT OCC ";

            throw std::runtime_error(rsutils::string::from() << error_message_prefix + "Could not be triggered");
        }
        return res;
    }

    std::vector<uint8_t> d500_auto_calibrated::update_calibration_status(int timeout_ms,
        rs2_update_progress_callback_sptr progress_callback)
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        std::vector<uint8_t> res;
        d500_calibration_answer calib_answer;
        do
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            res = _hw_monitor->send(command{ ds::GET_CALIB_STATUS });
            if (!check_buffer_size_from_get_calib_status(res))
                throw std::runtime_error("GET_CALIB_STATUS returned struct with wrong size");

            calib_answer = *reinterpret_cast<d500_calibration_answer*>(res.data());
            _state = static_cast<d500_calibration_state>(calib_answer.calibration_state);
            _result = static_cast<d500_calibration_result>(calib_answer.calibration_result);
            std::stringstream ss;
            ss << "Calibration in progress - State = " << d500_calibration_state_strings[static_cast<int>(_state)];
            if (_state == d500_calibration_state::RS2_D500_CALIBRATION_STATE_PROCESS)
            {
                ss << ", progress = " << static_cast<int>(calib_answer.calibration_progress);
                ss << ", result = " << d500_calibration_result_strings[static_cast<int>(_result)];
            }
            LOG_INFO(ss.str().c_str());
            if (progress_callback)
            {
                progress_callback->on_update_progress(calib_answer.calibration_progress);
            }
            if (_result == d500_calibration_result::RS2_D500_CALIBRATION_RESULT_FAILED_TO_RUN)
            {
                break;
            }
            bool is_timed_out(std::chrono::high_resolution_clock::now() - start_time > std::chrono::milliseconds(timeout_ms));
            if (is_timed_out)
            {
                throw std::runtime_error("OCC Calibration Timeout");
            }
        } while (_state != d500_calibration_state::RS2_D500_CALIBRATION_STATE_COMPLETE &&
            // if state is back to idle, it means that Abort action has been called
            _state != d500_calibration_state::RS2_D500_CALIBRATION_STATE_IDLE);

        // printing new calibration to log
        if (_state == d500_calibration_state::RS2_D500_CALIBRATION_STATE_COMPLETE)
        {
            if (_result == d500_calibration_result::RS2_D500_CALIBRATION_RESULT_SUCCESS)
            {
                auto depth_calib = *reinterpret_cast<ds::d500_coefficients_table*>(&calib_answer.depth_calibration);
                LOG_INFO("Depth new Calibration = \n" + depth_calib.to_string());
            }
            else if (_result == d500_calibration_result::RS2_D500_CALIBRATION_RESULT_FAILED_TO_CONVERGE)
            {
                LOG_ERROR("Calibration completed but algorithm failed");
                throw std::runtime_error("Calibration completed but algorithm failed");
            }
        }
        else
        {
            if (_result == d500_calibration_result::RS2_D500_CALIBRATION_RESULT_FAILED_TO_RUN)
            {
                LOG_ERROR("Calibration failed to run");
                throw std::runtime_error("Calibration failed to run");
            }
        }

        return res;
    }

    std::vector<uint8_t> d500_auto_calibrated::update_abort_status()
    {
        std::vector<uint8_t> ans;
        auto res = _hw_monitor->send(command{ ds::GET_CALIB_STATUS });
        if (!check_buffer_size_from_get_calib_status(res))
            throw std::runtime_error("GET_CALIB_STATUS returned struct with wrong size");

        d500_calibration_answer calib_answer = *reinterpret_cast<d500_calibration_answer*>(res.data());
        if (calib_answer.calibration_state == static_cast<uint8_t>(d500_calibration_state::RS2_D500_CALIBRATION_STATE_PROCESS))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            res = _hw_monitor->send(command{ ds::GET_CALIB_STATUS });
            if (!check_buffer_size_from_get_calib_status(res))
                throw std::runtime_error("GET_CALIB_STATUS returned struct with wrong size");

            calib_answer = *reinterpret_cast<d500_calibration_answer*>(res.data());
        }
        if (calib_answer.calibration_state == static_cast<uint8_t>(d500_calibration_state::RS2_D500_CALIBRATION_STATE_IDLE))
        {
            LOG_INFO("Depth Calibration Successfully Aborted");
            // returning success
            ans.push_back(1);
        }
        else
        {
            LOG_INFO("Depth Calibration Could not be Aborted");
            // returning failure
            ans.push_back(0);
        }
        return ans;
    }

    std::vector<uint8_t> d500_auto_calibrated::run_tare_calibration(int timeout_ms, float ground_truth_mm, std::string json, float* const health, rs2_update_progress_callback_sptr progress_callback)
    {
        throw std::runtime_error(rsutils::string::from() << "Tare Calibration not applicable for this device");
    }

    std::vector<uint8_t> d500_auto_calibrated::process_calibration_frame(int timeout_ms, const rs2_frame* f, float* const health, rs2_update_progress_callback_sptr progress_callback)
    {
        throw std::runtime_error(rsutils::string::from() << "Process Calibration Frame not applicable for this device");
    }

    std::vector<uint8_t> d500_auto_calibrated::get_calibration_table() const
    {
        // Getting depth calibration table. RGB table is currently not supported by auto_calibrated_interface API
        std::vector< uint8_t > res;

        command cmd( ds::GET_HKR_CONFIG_TABLE,
                     static_cast< int >( ds::d500_calib_location::d500_calib_flash_memory ),
                     static_cast< int >( ds::d500_calibration_table_id::depth_calibration_id ),
                     static_cast< int >( ds::d500_calib_type::d500_calib_dynamic ) );
        auto calib = _hw_monitor->send( cmd );

        if( calib.size() < sizeof( ds::table_header ) )
            throw std::runtime_error( "GET_HKR_CONFIG_TABLE response is smaller then calibration header!" );

        auto header = (ds::table_header *)( calib.data() );

        if( calib.size() < sizeof( ds::table_header ) + header->table_size )
            throw std::runtime_error( "GET_HKR_CONFIG_TABLE response is smaller then expected table size!" );

        // Backwards compalibility dictates that we will return the table without the header, but we need the header
        // details like versions to later set back the table. Save it at the start of _curr_calibration.
        _curr_calibration.assign( calib.begin(), calib.begin() + sizeof( ds::table_header ) );

        res.assign( calib.begin() + sizeof( ds::table_header ), calib.end() );

        return res;
    }

    void d500_auto_calibrated::write_calibration() const
    {
        auto table_header = reinterpret_cast< ds::table_header * >( _curr_calibration.data() );
        table_header->crc32 = rsutils::number::calc_crc32( _curr_calibration.data() + sizeof( ds::table_header ),
                                                           _curr_calibration.size() - sizeof( ds::table_header ) );

        command cmd( ds::SET_HKR_CONFIG_TABLE,
                     static_cast< int >( ds::d500_calib_location::d500_calib_flash_memory ),
                     static_cast< int >( table_header->table_type ),
                     static_cast< int >( ds::d500_calib_type::d500_calib_dynamic ) );
        cmd.data.assign( _curr_calibration.begin(), _curr_calibration.end() );
        cmd.require_response = false;

        _hw_monitor->send( cmd );
    }

    void d500_auto_calibrated::set_calibration_table(const std::vector<uint8_t>& calibration)
    {
        if( _curr_calibration.size() != sizeof( ds::table_header ) && // First time setting table, only header set by get_calibration_table
            _curr_calibration.size() != sizeof( ds::d500_coefficients_table ) ) // Table was previously set
            throw std::runtime_error( rsutils::string::from() <<
                                      "Current calibration table has unexpected size " << _curr_calibration.size() );

        if( calibration.size() != sizeof( ds::d500_coefficients_table ) - sizeof( ds::table_header ) )
            throw std::runtime_error( rsutils::string::from() <<
                                      "Setting calibration table with unexpected size" << calibration.size() );

        _curr_calibration.resize( sizeof( ds::table_header ) ); // Remove previously set calibration, keep header.
        _curr_calibration.insert( _curr_calibration.end(), calibration.begin(), calibration.end() );
    }

    void d500_auto_calibrated::reset_to_factory_calibration() const
    {
        throw std::runtime_error(rsutils::string::from() << "Tare Calibration not applicable for this device");
    }

    std::vector< uint8_t > d500_auto_calibrated::run_focal_length_calibration( rs2_frame_queue * left,
                                                                               rs2_frame_queue * right,
                                                                               float target_w,
                                                                               float target_h,
                                                                               int adjust_both_sides,
                                                                               float * ratio,
                                                                               float * angle,
                                                                               rs2_update_progress_callback_sptr progress_callback )
    {
        float fx[2] = { -1.0f, -1.0f };
        float fy[2] = { -1.0f, -1.0f };

        float left_rect_sides[4] = { 0.f };
        ds_calib_common::get_target_rect_info( left,
                                               left_rect_sides,
                                               fx[0],
                                               fy[0],
                                               50,
                                               progress_callback );  // Report 50% progress

        float right_rect_sides[4] = { 0.f };
        ds_calib_common::get_target_rect_info( right, right_rect_sides, fx[1], fy[1], 75, progress_callback );

        std::vector< uint8_t > ret;
        const float correction_factor = 0.5f;

        auto table_data = get_calibration_table(); // Table is returned without the header
        auto full_table = _curr_calibration; // During get_calibration_table header is saved in _curr_calibration
        full_table.insert( full_table.end(), table_data.begin(), table_data.end() );
        auto table = reinterpret_cast< librealsense::ds::d500_coefficients_table * >( full_table.data() );

        float ratio_to_apply = ds_calib_common::get_focal_length_correction_factor( left_rect_sides,
                                                                                    right_rect_sides,
                                                                                    fx,
                                                                                    fy,
                                                                                    target_w,
                                                                                    target_h,
                                                                                    table->baseline,
                                                                                    *ratio,
                                                                                    *angle );

        if( adjust_both_sides )
        {
            float ratio_to_apply_2 = sqrtf( ratio_to_apply );
            table->left_coefficients_table.base_instrinsics.fx /= ratio_to_apply_2;
            table->left_coefficients_table.base_instrinsics.fy /= ratio_to_apply_2;
            table->right_coefficients_table.base_instrinsics.fx *= ratio_to_apply_2;
            table->right_coefficients_table.base_instrinsics.fy *= ratio_to_apply_2;
        }
        else
        {
            table->right_coefficients_table.base_instrinsics.fx *= ratio_to_apply;
            table->right_coefficients_table.base_instrinsics.fy *= ratio_to_apply;
        }

        //Return data without header
        table_data.assign( full_table.begin() + sizeof( ds::table_header ), full_table.end() );
        return table_data;
    }

    std::vector<uint8_t> d500_auto_calibrated::run_uv_map_calibration(rs2_frame_queue* left, rs2_frame_queue* color, rs2_frame_queue* depth, int py_px_only,
        float* const health, int health_size, rs2_update_progress_callback_sptr progress_callback)
    {
        throw std::runtime_error(rsutils::string::from() << "UV Map Calibration not applicable for this device");
    }

    float d500_auto_calibrated::calculate_target_z(rs2_frame_queue* queue1, rs2_frame_queue* queue2, rs2_frame_queue* queue3,
        float target_w, float target_h, rs2_update_progress_callback_sptr progress_callback)
    {
        throw std::runtime_error(rsutils::string::from() << "Calculate T not applicable for this device");
    }

    std::string d500_auto_calibrated::get_calibration_config() const
    {
        calibration_config_with_header* result;

        // prepare command
        command cmd(ds::GET_HKR_CONFIG_TABLE,
            static_cast<int>(ds::d500_calib_location::d500_calib_flash_memory),
            static_cast<int>(ds::d500_calibration_table_id::calib_cfg_id),
            static_cast<int>(ds::d500_calib_type::d500_calib_dynamic));
        cmd.require_response = true;

        // send command to device and get response (calibration config entry + header)
        std::vector< uint8_t > response = _hw_monitor->send(cmd);
        if (response.size() < sizeof(calibration_config_with_header))
        {
            throw io_exception(rsutils::string::from() << "Calibration Config Read Failed");
        }

        // check CRC before returning result
        auto computed_crc32 = rsutils::number::calc_crc32(response.data() + sizeof(table_header),
            sizeof(calibration_config));
        result = reinterpret_cast<calibration_config_with_header*>(response.data());
        if (computed_crc32 != result->get_table_header().get_crc32())
        {
            throw invalid_value_exception(rsutils::string::from() << "Calibration Config Invalid CRC Value");
        }

        rsutils::json j = result->get_calibration_config().to_json();
        return j.dump();
    }

    void d500_auto_calibrated::set_calibration_config(const std::string& calibration_config_json_str) const
    {
        rsutils::json json_data = rsutils::json::parse(calibration_config_json_str);
        calibration_config calib_config(json_data["calibration_config"]);

        // calculate CRC
        uint32_t computed_crc32 = rsutils::number::calc_crc32(reinterpret_cast<const uint8_t*>(&calib_config), sizeof(calibration_config));

        // prepare vector of data to be sent (header + calibration_config)
        uint16_t version = ((uint16_t)0x01 << 8) | 0x01;  // major=0x01, minor=0x01 --> ver = major.minor
        uint32_t calib_version = 0;  // ignoring this field, as requested by sw architect
        table_header header(version, static_cast<uint16_t>(ds::d500_calibration_table_id::calib_cfg_id), sizeof(calibration_config),
            calib_version, computed_crc32);
        calibration_config_with_header calib_config_with_header(header, calib_config);
        auto data_as_ptr = reinterpret_cast<const uint8_t*>(&calib_config_with_header);

        // prepare command
        command cmd(ds::SET_HKR_CONFIG_TABLE,
            static_cast<int>(ds::d500_calib_location::d500_calib_flash_memory),
            static_cast<int>(ds::d500_calibration_table_id::calib_cfg_id),
            static_cast<int>(ds::d500_calib_type::d500_calib_dynamic));
        cmd.data.insert(cmd.data.end(), data_as_ptr, data_as_ptr + sizeof(calibration_config_with_header));
        cmd.require_response = false;

        // send command 
        _hw_monitor->send(cmd);
    }

    void d500_auto_calibrated::set_hw_monitor_for_auto_calib(std::shared_ptr<hw_monitor> hwm)
    {
        _hw_monitor = hwm;
    }
}
