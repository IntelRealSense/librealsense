// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.


#include "d500-auto-calibration.h"
#include <src/ds/ds-calib-common.h>

#include <rsutils/string/from.h>
#include <rsutils/json.h>
#include "d500-device.h"
#include <src/ds/d500/d500-debug-protocol-calibration-engine.h>

namespace librealsense
{
    static const std::string calibration_state_strings[] = {
        "Idle",
        "In Process",
        "Done Success",
        "Done Failure",
        "Flash Update",
        "Complete"
    };

    static const std::string calibration_result_strings[] = {
        "Unkown",
        "Success",
        "Failed to Converge",
        "Failed to Run"
    };

    d500_auto_calibrated::d500_auto_calibrated(std::shared_ptr<d500_debug_protocol_calibration_engine> calib_engine) :
        _calib_engine(calib_engine),
        _mode (calibration_mode::RESERVED),
        _state (calibration_state::IDLE),
        _result(calibration_result::UNKNOWN)
    {}

    void d500_auto_calibrated::check_preconditions_and_set_state()
    {
        if (_mode == calibration_mode::RUN ||
            _mode == calibration_mode::DRY_RUN)
        {
            // calibration state to be IDLE or COMPLETE
            _calib_engine->update_triggered_calibration_status();

            _state = _calib_engine->get_triggered_calibration_state();
            if (!(_state == calibration_state::IDLE ||
                _state == calibration_state::COMPLETE))
            {
                LOG_ERROR("Calibration State is not Idle nor Complete - pleare restart the device");
                throw std::runtime_error("OCC triggerred when Calibration State is not Idle not Complete");
            }
        }
        
        if (_mode == calibration_mode::ABORT)
        {
            // calibration state to be IN_PROCESS
            _calib_engine->update_triggered_calibration_status();
            _state = _calib_engine->get_triggered_calibration_state();
            if (!(_state == calibration_state::PROCESS))
            {
                LOG_ERROR("Calibration State is not In Process - so it could not be aborted");
                throw std::runtime_error("OCC aborted when Calibration State is not In Process");
            }
        }
    }

    void d500_auto_calibrated::get_mode_from_json(const std::string& json)
    {
        if (json.find("calib run") != std::string::npos)
            _mode = calibration_mode::RUN;
        else if (json.find("calib dry run") != std::string::npos)
            _mode = calibration_mode::DRY_RUN;
        else if (json.find("calib abort") != std::string::npos)
            _mode = calibration_mode::ABORT;
        else
            throw std::runtime_error("run_on_chip_calibration called with wrong content in json file");
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
            res = _calib_engine->run_triggered_calibration(_mode);

            if (_mode == calibration_mode::RUN ||
                _mode == calibration_mode::DRY_RUN)
            {
                res = update_calibration_status(timeout_ms, progress_callback);
            }
            else if (_mode == calibration_mode::ABORT)
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
            if (_mode == calibration_mode::DRY_RUN)
                error_message_prefix = "\nDRY RUN OCC ";
            else if (_mode == calibration_mode::ABORT)
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
        do
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            _calib_engine->update_triggered_calibration_status();

            _state = _calib_engine->get_triggered_calibration_state();
            _result = _calib_engine->get_triggered_calibration_result();
            std::stringstream ss;
            ss << "Calibration in progress - State = " << calibration_state_strings[static_cast<int>(_state)];
            if (_state == calibration_state::PROCESS)
            {
                ss << ", progress = " << static_cast<int>(_calib_engine->get_triggered_calibration_progress());
                ss << ", result = " << calibration_result_strings[static_cast<int>(_result)];
            }
            LOG_INFO(ss.str().c_str());
            if (progress_callback)
            {
                progress_callback->on_update_progress(_calib_engine->get_triggered_calibration_progress());
            }
            
            if (_result == calibration_result::FAILED_TO_RUN)
            {
                break;
            }
            bool is_timed_out(std::chrono::high_resolution_clock::now() - start_time > std::chrono::milliseconds(timeout_ms));
            if (is_timed_out)
            {
                throw std::runtime_error("OCC Calibration Timeout");
            }
        } while (_state != calibration_state::COMPLETE &&
            // if state is back to idle, it means that Abort action has been called
            _state != calibration_state::IDLE);

        // printing new calibration to log
        if (_state == calibration_state::COMPLETE)
        {
            if (_result == calibration_result::SUCCESS)
            {
                auto depth_calib = _calib_engine->get_depth_calibration();
                LOG_INFO("Depth new Calibration = \n" + depth_calib.to_string());
                auto depth_calib_start = reinterpret_cast<uint8_t*>(&depth_calib);
                res.insert(res.begin(), depth_calib_start, depth_calib_start + sizeof(ds::d500_coefficients_table));
            }
            else if (_result == calibration_result::FAILED_TO_CONVERGE)
            {
                LOG_ERROR("Calibration completed but algorithm failed");
                throw std::runtime_error("Calibration completed but algorithm failed");
            }
        }
        else
        {
            if (_result == calibration_result::FAILED_TO_RUN)
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
        _calib_engine->update_triggered_calibration_status();
        if (_calib_engine->get_triggered_calibration_state() == calibration_state::PROCESS)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            _calib_engine->update_triggered_calibration_status();
        }
        if (_calib_engine->get_triggered_calibration_state() == calibration_state::IDLE)
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
        throw not_implemented_exception(rsutils::string::from() << "Tare Calibration not applicable for this device");
    }

    std::vector<uint8_t> d500_auto_calibrated::process_calibration_frame(int timeout_ms, const rs2_frame* f, float* const health, rs2_update_progress_callback_sptr progress_callback)
    {
        throw not_implemented_exception(rsutils::string::from() << "Process Calibration Frame not applicable for this device");
    }

    std::vector<uint8_t> d500_auto_calibrated::get_calibration_table() const
    {
        return _calib_engine->get_calibration_table(_curr_calibration);
    }

    void d500_auto_calibrated::write_calibration() const
    {
        _calib_engine->write_calibration(_curr_calibration);
    }

    void d500_auto_calibrated::set_calibration_table(const std::vector<uint8_t>& calibration)
    {
        if (_curr_calibration.size() != sizeof(ds::table_header) && // First time setting table, only header set by get_calibration_table
            _curr_calibration.size() != sizeof(ds::d500_coefficients_table)) // Table was previously set
            throw std::runtime_error(rsutils::string::from() <<
                "Current calibration table has unexpected size " << _curr_calibration.size());

        if (calibration.size() != sizeof(ds::d500_coefficients_table) - sizeof(ds::table_header))
            throw std::runtime_error(rsutils::string::from() <<
                "Setting calibration table with unexpected size" << calibration.size());

        _curr_calibration.resize(sizeof(ds::table_header)); // Remove previously set calibration, keep header.
        _curr_calibration.insert(_curr_calibration.end(), calibration.begin(), calibration.end());
    }

    void d500_auto_calibrated::reset_to_factory_calibration() const
    {
        throw not_implemented_exception(rsutils::string::from() << "Reset to factory Calibration not applicable for this device");
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
        ds_calib_common::get_target_rect_info( left, left_rect_sides, fx[0], fy[0], 50, progress_callback );  // Report 50% progress

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
        throw not_implemented_exception(rsutils::string::from() << "UV Map Calibration not applicable for this device");
    }

    float d500_auto_calibrated::calculate_target_z( rs2_frame_queue* queue1, rs2_frame_queue* queue2, rs2_frame_queue* queue3,
                                                    float target_w, float target_h,
                                                    rs2_update_progress_callback_sptr progress_callback )
    {
        constexpr size_t min_frames_required = 10;

        rs2_error * e = nullptr;
        int queue_size = rs2_frame_queue_size( queue1, &e );
        if( queue_size < min_frames_required )
            throw std::runtime_error( rsutils::string::from() << "Target distance calculation requires at least "
                                                                << min_frames_required << " frames, aborting" );

        float target_fw = 0;
        float target_fh = 0;
        std::array< float, 4 > rect_sides{};
        ds_calib_common::get_target_rect_info( queue1, rect_sides.data(), target_fw, target_fh, 50, progress_callback ); // Report 50% progress

        float gt[4] = { 0 };
        gt[0] = target_fw * target_w / rect_sides[0];
        gt[1] = target_fw * target_w / rect_sides[1];
        gt[2] = target_fh * target_h / rect_sides[2];
        gt[3] = target_fh * target_h / rect_sides[3];

        if( gt[0] <= 0.1f || gt[1] <= 0.1f || gt[2] <= 0.1f || gt[3] <= 0.1f )
            throw std::runtime_error( "Target distance calculation failed" );

        // Target's plane Z value is the average of the four calculated corners
        float target_z_value = 0.f;
        for( int i = 0; i < 4; ++i )
            target_z_value += gt[i];
        target_z_value /= 4.f;

        return target_z_value;
    }

    std::string d500_auto_calibrated::get_calibration_config() const
    {
        return _calib_engine->get_calibration_config();
    }

    void d500_auto_calibrated::set_calibration_config(const std::string& calibration_config_json_str) const
    {
        _calib_engine->set_calibration_config(calibration_config_json_str);
    }

}
