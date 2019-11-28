// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "ds5-auto-calibration.h"
#include "../third-party/json.hpp"
#include "ds5-device.h"

namespace librealsense
{
    enum auto_calib_sub_cmd : uint8_t
    {
        auto_calib_begin = 0x08,
        auto_calib_check_status = 0x03,
        tare_calib_begin = 0x0b,
        tare_calib_check_status = 0x0c,
        get_calibration_result = 0x0d,
        set_coefficients = 0x19
    };

    auto_calibrated::auto_calibrated(std::shared_ptr<hw_monitor>& hwm)
        :_hw_monitor(hwm){}

    std::map<std::string, int> auto_calibrated::parse_json(std::string json_content)
    {
        using json = nlohmann::json;

         auto j = json::parse(json_content);

        std::map<std::string, int> values;

        for (json::iterator it = j.begin(); it != j.end(); ++it)
        {
            values[it.key()] = it.value();
        }
        return values;
    }

    std::vector<uint8_t> auto_calibrated::run_on_chip_calibration(int timeout_ms, std::string json, float* health, update_progress_callback_ptr progress_callback)
    {
        if (json.size() > 0)
        {
            auto jsn = parse_json(json);
            if (jsn.find("speed") != jsn.end())
            {
                _speed = jsn["speed"];
            }
        }

        std::shared_ptr<ds5_advanced_mode_base> preset_recover;
        if (_speed == 4)
            preset_recover = change_preset();

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Begin auto-calibration
        _hw_monitor->send(command{ ds::AUTO_CALIB, auto_calib_begin, _speed });

        DirectSearchCalibrationResult result;
        memset(&result, 0, sizeof(DirectSearchCalibrationResult));

        int count = 0;
        bool done = false;

        auto start = std::chrono::high_resolution_clock::now();
        auto now = start;

        // While not ready...
        do
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            // Check calibration status
            try
            {
                auto res = _hw_monitor->send(command{ ds::AUTO_CALIB, auto_calib_check_status });

                if (res.size() < sizeof(DirectSearchCalibrationResult))
                    throw std::runtime_error("Not enough data from CALIB_STATUS!");

                result = *reinterpret_cast<DirectSearchCalibrationResult*>(res.data());
                done = result.status != RS2_DSC_STATUS_RESULT_NOT_READY;
            }
            catch (const std::exception& ex)
            {
                LOG_WARNING(ex.what());
            }
            if (progress_callback)
                progress_callback->on_update_progress(count++ * (2 * _speed));

            now = std::chrono::high_resolution_clock::now();

        } while (now - start < std::chrono::milliseconds(timeout_ms) && !done);


        // If we exit due to timeout, report timeout
        if (!done)
        {
            throw std::runtime_error("Operation timed-out!\n"
                "Calibration state did not converged in time");
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        auto status = (rs2_dsc_status)result.status;

        // Handle errors from firmware
        if (status != RS2_DSC_STATUS_SUCCESS)
        {
            handle_calibration_error(status);
        }

        auto res = get_calibration_results(health);
        return res;
    }

    std::vector<uint8_t> auto_calibrated::run_tare_calibration(int timeout_ms, float ground_truth_mm, std::string json, float* health, update_progress_callback_ptr progress_callback)
    {
        if (json.size() > 0)
        {
            auto jsn = parse_json(json);
            if (jsn.find("speed") != jsn.end())
            {
                _speed = jsn["speed"];
            }
            if (jsn.find("average_step_count") != jsn.end())
            {
                _average_step_count = jsn["average_step_count"];
            }
            if (jsn.find("step_count") != jsn.end())
            {
                _step_count = jsn["step_count"];
            }
            if (jsn.find("accuracy") != jsn.end())
            {
                _accuracy = jsn["accuracy"];
            }
        }

        auto preset_recover = change_preset();

        auto param2 = (int)ground_truth_mm * 100;
        auto param3 = (_average_step_count ) + (_step_count << 8) + (_accuracy << 16);

        _hw_monitor->send(command{ ds::AUTO_CALIB, tare_calib_begin, param2, param3 });

        DirectSearchCalibrationResult result;

        // While not ready...
        int count = 0;
        bool done = false;

        auto start = std::chrono::high_resolution_clock::now();
        auto now = start;

        do
        {
            memset(&result, 0, sizeof(DirectSearchCalibrationResult));

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            // Check calibration status
            try
            {
                auto res = _hw_monitor->send(command{ ds::AUTO_CALIB, tare_calib_check_status });
                if (res.size() < sizeof(DirectSearchCalibrationResult))
                    throw std::runtime_error("Not enough data from CALIB_STATUS!");

                auto result = *reinterpret_cast<DirectSearchCalibrationResult*>(res.data());
                done = result.status != RS2_DSC_STATUS_RESULT_NOT_READY;
            }

            catch (const std::exception& ex)
            {
                LOG_WARNING(ex.what());
            }

            if (progress_callback)
                progress_callback->on_update_progress(count * (2 * _speed));

            now = std::chrono::high_resolution_clock::now();

        } while (now - start < std::chrono::milliseconds(timeout_ms) && !done);

        // If we exit due to timeout, report timeout
        if (!done)
        {
            throw std::runtime_error("Operation timed-out!\n"
                "Calibration state did not converged in time");
        }

        auto status = (rs2_dsc_status)result.status;

        // Handle errors from firmware
        if (status != RS2_DSC_STATUS_SUCCESS)
        {
            handle_calibration_error(status);
        }

        return get_calibration_results(health);
    }

    std::shared_ptr<ds5_advanced_mode_base> auto_calibrated::change_preset()
    {
        preset old_preset_values;
        rs2_rs400_visual_preset old_preset;

        auto advanced_mode = dynamic_cast<ds5_advanced_mode_base*>(this);
        if (advanced_mode)
        {
            old_preset = (rs2_rs400_visual_preset)(int)advanced_mode->_preset_opt->query();
            if (old_preset == RS2_RS400_VISUAL_PRESET_CUSTOM)
                old_preset_values = advanced_mode->get_all();
            advanced_mode->_preset_opt->set(RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY);
        }

        std::shared_ptr<ds5_advanced_mode_base> recover_preset(advanced_mode, [old_preset, advanced_mode, old_preset_values](ds5_advanced_mode_base* adv)
        {
            if (old_preset == RS2_RS400_VISUAL_PRESET_CUSTOM)
            {
                advanced_mode->_preset_opt->set(RS2_RS400_VISUAL_PRESET_CUSTOM);
                adv->set_all(old_preset_values);
            }
            else
                advanced_mode->_preset_opt->set(old_preset);
        });

        return recover_preset;
    }

    void auto_calibrated::handle_calibration_error(rs2_dsc_status status) const
    {
        if (status == RS2_DSC_STATUS_EDGE_TOO_CLOSE)
        {
            throw std::runtime_error("Calibration didn't converge! (EDGE_TO_CLOSE)\n"
                "Please retry in different lighting conditions");
        }
        else if (status == RS2_DSC_STATUS_FILL_FACTOR_TOO_LOW)
        {
            throw std::runtime_error("Not enough depth pixels! (FILL_FACTOR_LOW)\n"
                "Please retry in different lighting conditions");
        }
        else if (status == RS2_DSC_STATUS_NOT_CONVERGE)
        {
            throw std::runtime_error("Calibration didn't converge! (NOT_CONVERGE)\n"
                "Please retry in different lighting conditions");
        }
        else if (status == RS2_DSC_STATUS_NO_DEPTH_AVERAGE)
        {
            throw std::runtime_error("Calibration didn't converge! (NO_AVERAGE)\n"
                "Please retry in different lighting conditions");
        }
        else throw std::runtime_error(to_string() << "Calibration didn't converge! (RESULT=" << int(status) << ")");
    }

    std::vector<uint8_t> auto_calibrated::get_calibration_results(float* health) const
    {
        // Get new calibration from the firmware
        auto res = _hw_monitor->send(command{ ds::AUTO_CALIB, get_calibration_result});
        if (res.size() < sizeof(DscResultBuffer))
            throw std::runtime_error("Not enough data from CALIB_STATUS!");

        auto reslt = (DscResultBuffer*)(res.data());

        table_header* header = reinterpret_cast<table_header*>(res.data() + sizeof(DscResultBuffer));

        if (res.size() < sizeof(DscResultBuffer) + sizeof(table_header) + header->table_size)
            throw std::runtime_error("Table truncated in CALIB_STATUS!");

        std::vector<uint8_t> calib;

        calib.resize(sizeof(table_header) + header->table_size, 0);
        memcpy(calib.data(), header, calib.size()); // Copy to new_calib

        *health = abs(reslt->m_dscResultParams.m_healthCheck);

        return calib;
    }

    std::vector<uint8_t> auto_calibrated::get_calibration_table() const
    {
        std::vector<uint8_t> res;

        // Fetch current calibration using GETINITCAL command
        command cmd(ds::GETINTCAL, ds::coefficients_table_id);
        auto calib = _hw_monitor->send(cmd);

        if (calib.size() < sizeof(table_header)) throw std::runtime_error("Missing calibration header from GETINITCAL!");

        auto table = (uint8_t*)(calib.data() + sizeof(table_header));
        auto hd = (table_header*)(calib.data());

        if (calib.size() < sizeof(table_header) + hd->table_size)
            throw std::runtime_error("Table truncated from GETINITCAL!");

        res.resize(sizeof(table_header) + hd->table_size, 0);
        memcpy(res.data(), hd, res.size()); // Copy to old_calib

        return res;
    }

    void auto_calibrated::write_calibration() const
    {
        command write_calib( ds::SETINTCAL, set_coefficients );
        write_calib.data = _curr_calibration;
        _hw_monitor->send(write_calib);
    }

    void auto_calibrated::set_calibration_table(const std::vector<uint8_t>& calibration)
    {
        table_header* hd = (table_header*)(calibration.data());
        uint8_t* table = (uint8_t*)(calibration.data() + sizeof(table_header));
        command write_calib(ds::CALIBRECALC, 0, 0, 0, 0xcafecafe);
        write_calib.data.insert(write_calib.data.end(), (uint8_t*)table, ((uint8_t*)table) + hd->table_size);
        _hw_monitor->send(write_calib);

        _curr_calibration = calibration;
    }

    void auto_calibrated::reset_to_factory_calibration() const
    {
        command cmd(ds::fw_cmd::CAL_RESTORE_DFLT);
        _hw_monitor->send(cmd);
    }
}
