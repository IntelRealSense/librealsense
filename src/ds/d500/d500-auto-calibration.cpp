// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.


#include "d500-auto-calibration.h"
#include <rsutils/string/from.h>
#include <rsutils/json.h>
#include "d500-device.h"


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

    d500_auto_calibrated::d500_auto_calibrated() :
        _mode (d500_calibration_mode::RS2_D500_CALIBRATION_MODE_RESERVED),
        _state (d500_calibration_state::RS2_D500_CALIBRATION_STATE_IDLE),
        _result(d500_calibration_result::RS2_D500_CALIBRATION_RESULT_UNKNOWN)
    {
        // add here choice between hw_monitor or debug protocol
        // is hw monitor needed at all (since debug protocol uses hwm
        _ac_handler = std::make_shared <d500_auto_calibrated_handler>();
    }

    void d500_auto_calibrated::check_preconditions_and_set_state()
    {
        // moreover, relevant only for d585s, not for d555e
        if (_mode == d500_calibration_mode::RS2_D500_CALIBRATION_MODE_RUN ||
            _mode == d500_calibration_mode::RS2_D500_CALIBRATION_MODE_DRY_RUN)
        {
            // calibration state to be IDLE or COMPLETE
            auto calib_result = _ac_handler->get_status();

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
            d500_calibration_answer calib_result = _ac_handler->get_status();
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
            res = _ac_handler->run_auto_calibration(_mode);

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
            calib_answer = _ac_handler->get_status();

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
        auto calib_answer = _ac_handler->get_status();
        if (calib_answer.calibration_state == static_cast<uint8_t>(d500_calibration_state::RS2_D500_CALIBRATION_STATE_PROCESS))
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            calib_answer = _ac_handler->get_status();
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
        throw std::runtime_error(rsutils::string::from() << "Get Calibration Table not applicable for this device");
    }

    void d500_auto_calibrated::write_calibration() const
    {
        throw std::runtime_error(rsutils::string::from() << "Write Calibration not applicable for this device");
    }

    void d500_auto_calibrated::set_calibration_table(const std::vector<uint8_t>& calibration)
    {
        throw std::runtime_error(rsutils::string::from() << "Set Calibration Table not applicable for this device");
    }

    void d500_auto_calibrated::reset_to_factory_calibration() const
    {
        throw std::runtime_error(rsutils::string::from() << "Tare Calibration not applicable for this device");
    }

    std::vector<uint8_t> d500_auto_calibrated::run_focal_length_calibration(rs2_frame_queue* left, rs2_frame_queue* right, float target_w, float target_h,
        int adjust_both_sides, float *ratio, float * angle, rs2_update_progress_callback_sptr progress_callback)
    {
        throw std::runtime_error(rsutils::string::from() << "Focal Length Calibration not applicable for this device");
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

    rs2_calibration_config d500_auto_calibrated::get_calibration_config() const
    {
        return _ac_handler->get_calibration_config();
    }

    void d500_auto_calibrated::set_calibration_config(const rs2_calibration_config& calib_config)
    {
        _ac_handler->set_calibration_config(calib_config);
    }

    std::string d500_auto_calibrated::calibration_config_to_json_string(const rs2_calibration_config& calib_config) const
    {

        rsutils::json json_data;
        json_data["calibration_config"]["calib_roi_num_of_segments"] = calib_config.calib_roi_num_of_segments;

        for (int roi_index = 0; roi_index < 4; roi_index++)
        {
            for (int mask_index = 0; mask_index < 4; mask_index++)
            {
                json_data["calibration_config"]["roi"][roi_index][mask_index][0] = calib_config.roi[roi_index].mask_pixel[mask_index][0];
                json_data["calibration_config"]["roi"][roi_index][mask_index][1] = calib_config.roi[roi_index].mask_pixel[mask_index][1];
            }
        }

        auto& rotation = json_data["calibration_config"]["camera_position"]["rotation"];
        rotation[0][0] = calib_config.camera_position.rotation.x.x;
        rotation[0][1] = calib_config.camera_position.rotation.x.y;
        rotation[0][2] = calib_config.camera_position.rotation.x.z;
        rotation[1][0] = calib_config.camera_position.rotation.y.x;
        rotation[1][1] = calib_config.camera_position.rotation.y.y;
        rotation[1][2] = calib_config.camera_position.rotation.y.z;
        rotation[2][0] = calib_config.camera_position.rotation.z.x;
        rotation[2][1] = calib_config.camera_position.rotation.z.y;
        rotation[2][2] = calib_config.camera_position.rotation.z.z;

        auto& translation = json_data["calibration_config"]["camera_position"]["translation"];
        translation[0] = calib_config.camera_position.translation.x;
        translation[1] = calib_config.camera_position.translation.y;
        translation[2] = calib_config.camera_position.translation.z;

        // fill crypto signature array
        size_t number_of_elements = sizeof(calib_config.crypto_signature) / sizeof(calib_config.crypto_signature[0]);
        std::vector<uint8_t> crypto_signature_byte_array(number_of_elements);
        memcpy(crypto_signature_byte_array.data(), calib_config.crypto_signature, sizeof(calib_config.crypto_signature));
        json_data["calibration_config"]["crypto_signature"] = crypto_signature_byte_array;

        return json_data.dump();

    }

    rs2_calibration_config d500_auto_calibrated::json_string_to_calibration_config(const std::string& json_str) const
    {
        rsutils::json json_data = rsutils::json::parse(json_str);
        rs2_calibration_config calib_config;

        calib_config.calib_roi_num_of_segments = json_data["calibration_config"]["calib_roi_num_of_segments"];

        for (int roi_index = 0; roi_index < 4; roi_index++)
        {
            for (int mask_index = 0; mask_index < 4; mask_index++)
            {
                calib_config.roi[roi_index].mask_pixel[mask_index][0] = json_data["calibration_config"]["roi"][roi_index][mask_index][0];
                calib_config.roi[roi_index].mask_pixel[mask_index][1] = json_data["calibration_config"]["roi"][roi_index][mask_index][1];
            }
        }

        auto& rotation = json_data["calibration_config"]["camera_position"]["rotation"];
        calib_config.camera_position.rotation.x.x = rotation[0][0];
        calib_config.camera_position.rotation.x.y = rotation[0][1];
        calib_config.camera_position.rotation.x.z = rotation[0][2];
        calib_config.camera_position.rotation.y.x = rotation[1][0];
        calib_config.camera_position.rotation.y.y = rotation[1][1];
        calib_config.camera_position.rotation.y.z = rotation[1][2];
        calib_config.camera_position.rotation.z.x = rotation[2][0];
        calib_config.camera_position.rotation.z.y = rotation[2][1];
        calib_config.camera_position.rotation.z.z = rotation[2][2];

        auto& translation = json_data["calibration_config"]["camera_position"]["translation"];
        calib_config.camera_position.translation.x = translation[0];
        calib_config.camera_position.translation.y = translation[1];
        calib_config.camera_position.translation.z = translation[2];

        // fill crypto signature array
        std::vector<uint8_t> crypto_signature_vector = json_data["calibration_config"]["crypto_signature"].get<std::vector<uint8_t>>();
        std::memcpy(calib_config.crypto_signature, crypto_signature_vector.data(), crypto_signature_vector.size() * sizeof(uint8_t));

        return calib_config;
    }

    void d500_auto_calibrated::set_device_for_auto_calib(d500_device* device)
    {
        _ac_handler->set_device_for_auto_calib(device);
    }
}
