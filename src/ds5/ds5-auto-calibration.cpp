// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <numeric>
#include "../third-party/json.hpp"
#include "ds5-device.h"
#include "ds5-private.h"
#include "ds5-thermal-monitor.h"
#include "ds5-auto-calibration.h"
#include "librealsense2/rsutil.h"
#include "../algo.h"

namespace librealsense
{
#pragma pack(push, 1)
#pragma pack(1)
    struct DirectSearchCalibrationResult
    {
        uint16_t status;      // DscStatus
        uint16_t stepCount;
        uint16_t stepSize; // 1/1000 of a pixel
        uint32_t pixelCountThreshold; // minimum number of pixels in
                                      // selected bin
        uint16_t minDepth;  // Depth range for FWHM
        uint16_t maxDepth;
        uint32_t rightPy;   // 1/1000000 of normalized unit
        float healthCheck;
        float rightRotation[9]; // Right rotation
    };

    struct FocalLengthCalibrationResult
    {
        uint16_t status;    // DscStatus
        uint16_t stepCount;
        uint16_t scanRange; // 1/1000 of a pixel
        float rightFy;
        float rightFx;
        float leftFy;
        float leftFx;
        float FL_healthCheck;
        uint16_t fillFactor0; // first of the setCount number of fill factor, 1/100 of a percent
    };

    struct DscPyRxFLCalibrationTableResult
    {
        uint16_t headerSize; // 10 bytes for status & health numbers
        uint16_t status;      // DscStatus
        float healthCheck;
        float FL_heathCheck;
        uint16_t tableSize;  // 512 bytes
    };

    struct DscResultParams
    {
        uint16_t m_status;
        float    m_healthCheck;
    };

    struct DscResultBuffer
    {
        uint16_t m_paramSize;
        DscResultParams m_dscResultParams;
        uint16_t m_tableSize;
    };

    enum rs2_dsc_status : uint16_t
    {
        RS2_DSC_STATUS_SUCCESS = 0, /**< Self calibration succeeded*/
        RS2_DSC_STATUS_RESULT_NOT_READY = 1, /**< Self calibration result is not ready yet*/
        RS2_DSC_STATUS_FILL_FACTOR_TOO_LOW = 2, /**< There are too little textures in the scene*/
        RS2_DSC_STATUS_EDGE_TOO_CLOSE = 3, /**< Self calibration range is too small*/
        RS2_DSC_STATUS_NOT_CONVERGE = 4, /**< For tare calibration only*/
        RS2_DSC_STATUS_BURN_SUCCESS = 5,
        RS2_DSC_STATUS_BURN_ERROR = 6,
        RS2_DSC_STATUS_NO_DEPTH_AVERAGE = 7
    };

#pragma pack(pop)

    enum auto_calib_sub_cmd : uint8_t
    {
        py_rx_calib_begin = 0x08,
        py_rx_calib_check_status = 0x03,
        tare_calib_begin = 0x0b,
        tare_calib_check_status = 0x0c,
        get_calibration_result = 0x0d,
        focal_length_calib_begin = 0x11,
        get_focal_legth_calib_result = 0x12,
        py_rx_plus_fl_calib_begin = 0x13,
        get_py_rx_plus_fl_calib_result = 0x14
    };

    enum auto_calib_speed
    {
        speed_very_fast = 0,
        speed_fast = 1,
        speed_medium = 2,
        speed_slow = 3,
        speed_white_wall = 4
    };

    enum subpixel_accuracy
    {
        very_high = 0, //(0.025%)
        high = 1, //(0.05%)
        medium = 2, //(0.1%)
        low = 3 //(0.2%)
    };

    enum data_sampling
    {
        polling = 0,
        interrupt = 1
    };

    enum scan_parameter
    {
        py_scan = 0,
        rx_scan = 1
    };

    struct tare_params3
    {
        byte average_step_count;
        byte step_count;
        byte accuracy;
        byte reserved;
    };

    struct params4
    {
        int scan_parameter : 1;
        int reserved : 2;
        int data_sampling : 1;
    };

    union tare_calibration_params
    {
        tare_params3 param3_struct;
        int param3;
    };

    union param4
    {
        params4 param4_struct;
        int param_4;
    };

    const int DEFAULT_CALIB_TYPE = 0;

    const int DEFAULT_AVERAGE_STEP_COUNT = 20;
    const int DEFAULT_STEP_COUNT = 20;
    const int DEFAULT_ACCURACY = subpixel_accuracy::medium;
    const int DEFAULT_SPEED = auto_calib_speed::speed_slow;
    const int DEFAULT_SCAN = scan_parameter::py_scan;
    const int DEFAULT_SAMPLING = data_sampling::interrupt;

    const int DEFAULT_FL_STEP_COUNT = 100;
    const int DEFAULT_FY_SCAN_RANGE = 40;
    const int DEFAULT_KEEP_NEW_VALUE_AFTER_SUCESSFUL_SCAN = 1;
    const int DEFAULT_FL_SAMPLING = data_sampling::interrupt;
    const int DEFAULT_ADJUST_BOTH_SIDES = 0;

    const int DEFAULT_TARE_SAMPLING = data_sampling::polling;

    const int DEFAULT_OCC_FL_SCAN_LOCATION = 0;
    const int DEFAULT_FY_SCAN_DIRECTION = 0;
    const int DEFAULT_WHITE_WALL_MODE = 0;

    auto_calibrated::auto_calibrated(std::shared_ptr<hw_monitor>& hwm)
        : _hw_monitor(hwm){}

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

    void try_fetch(std::map<std::string, int> jsn, std::string key, int* value)
    {
        std::replace(key.begin(), key.end(), '_', ' '); // Treat _ as space
        if (jsn.find(key) != jsn.end())
        {
            *value = jsn[key];
        }
    }

    // RAII to handle auto-calibration with the thermal compensation disabled
    class thermal_compensation_guard
    {
    public:
        thermal_compensation_guard(auto_calibrated_interface* handle) :
            restart_tl(false), snr(nullptr)
        {
            if (Is<librealsense::device>(handle))
            {
                try
                {
                    // The depth sensor is assigned first by design
                    snr = &(As<librealsense::device>(handle)->get_sensor(0));

                    if (snr->supports_option(RS2_OPTION_THERMAL_COMPENSATION))
                        restart_tl = static_cast<bool>(snr->get_option(RS2_OPTION_THERMAL_COMPENSATION).query() != 0);
                    if (restart_tl)
                    {
                        snr->get_option(RS2_OPTION_THERMAL_COMPENSATION).set(0.f);
                        // Allow for FW changes to propagate
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }
                }
                catch(...) {
                    LOG_WARNING("Thermal Compensation guard failed to invoke");
                }
            }
        }
        virtual ~thermal_compensation_guard()
        {
            try
            {
                if (snr && restart_tl)
                    snr->get_option(RS2_OPTION_THERMAL_COMPENSATION).set(1.f);
            }
            catch (...) {
                LOG_WARNING("Thermal Compensation guard failed to complete");
            }
        }

    protected:
        bool restart_tl;
        librealsense::sensor_interface* snr;

    private:
        // Disable copy/assignment ctors
        thermal_compensation_guard(const thermal_compensation_guard&);
        thermal_compensation_guard& operator=(const thermal_compensation_guard&);
    };

    std::vector<uint8_t> auto_calibrated::run_on_chip_calibration(int timeout_ms, std::string json, float* health, update_progress_callback_ptr progress_callback)
    {
        int calib_type = DEFAULT_CALIB_TYPE;

        int speed = DEFAULT_SPEED;
        int speed_fl = auto_calib_speed::speed_slow;
        int scan_parameter = DEFAULT_SCAN;
        int data_sampling = DEFAULT_SAMPLING;
        int apply_preset = 1;

        int fl_step_count = DEFAULT_FL_STEP_COUNT;
        int fy_scan_range = DEFAULT_FY_SCAN_RANGE;
        int keep_new_value_after_sucessful_scan = DEFAULT_KEEP_NEW_VALUE_AFTER_SUCESSFUL_SCAN;
        int fl_data_sampling = DEFAULT_FL_SAMPLING;
        int adjust_both_sides = DEFAULT_ADJUST_BOTH_SIDES;

        int fl_scan_location = DEFAULT_OCC_FL_SCAN_LOCATION;
        int fy_scan_direction = DEFAULT_FY_SCAN_DIRECTION;
        int white_wall_mode = DEFAULT_WHITE_WALL_MODE;

        float h_1 = 0.0f;
        float h_2 = 0.0f;

        //Enforce Thermal Compensation off during OCC
        volatile thermal_compensation_guard grd(this);

        if (json.size() > 0)
        {
            auto jsn = parse_json(json);
            try_fetch(jsn, "calib type", &calib_type);

            if (calib_type == 0)
                try_fetch(jsn, "speed", &speed);
            else
                try_fetch(jsn, "speed", &speed_fl);

            try_fetch(jsn, "scan parameter", &scan_parameter);
            try_fetch(jsn, "data sampling", &data_sampling);
            try_fetch(jsn, "apply preset", &apply_preset);

            try_fetch(jsn, "fl step count", &fl_step_count);
            try_fetch(jsn, "fy scan range", &fy_scan_range);
            try_fetch(jsn, "keep new value after sucessful scan", &keep_new_value_after_sucessful_scan);
            try_fetch(jsn, "fl data sampling", &fl_data_sampling);
            try_fetch(jsn, "adjust both sides", &adjust_both_sides);

            try_fetch(jsn, "fl scan location", &fl_scan_location);
            try_fetch(jsn, "fy scan direction", &fy_scan_direction);
            try_fetch(jsn, "white wall mode", &white_wall_mode);
        }

        std::vector<uint8_t> res;
        if (calib_type == 0)
        {
            LOG_INFO("run_on_chip_calibration with parameters: speed = " << speed << " scan_parameter = " << scan_parameter << " data_sampling = " << data_sampling);
            check_params(speed, scan_parameter, data_sampling);

            param4 param{ (byte)scan_parameter, 0, (byte)data_sampling };

            std::shared_ptr<ds5_advanced_mode_base> preset_recover;
            if (speed == speed_white_wall && apply_preset)
                preset_recover = change_preset();

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            // Begin auto-calibration
            _hw_monitor->send(command{ ds::AUTO_CALIB, py_rx_calib_begin, speed, 0, param.param_4 });

            DirectSearchCalibrationResult result{};

            int count = 0;
            int retries = 0;
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
                    auto res = _hw_monitor->send(command{ ds::AUTO_CALIB, py_rx_calib_check_status });

                    if (res.size() < sizeof(DirectSearchCalibrationResult))
                        throw std::runtime_error("Not enough data from CALIB_STATUS!");

                    result = *reinterpret_cast<DirectSearchCalibrationResult*>(res.data());
                    done = result.status != RS2_DSC_STATUS_RESULT_NOT_READY;
                }
                catch (const std::exception& ex)
                {
                    if (!((retries++) % 5)) // Add log debug once in a sec
                    {
                        LOG_DEBUG(ex.what());
                    }
                }

                if (progress_callback)
                    progress_callback->on_update_progress(count++ * (2.f * speed)); //curently this number does not reflect the actual progress

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

            res = get_calibration_results(health);
        }
        else if (calib_type == 1)
        {
            LOG_INFO("run_on_chip_focal_length_calibration with parameters: step count = " << fl_step_count
                << ", fy scan range = " << fy_scan_range << ", keep new value after sucessful scan = " << keep_new_value_after_sucessful_scan
                << ", interrrupt data sampling " << fl_data_sampling << ", adjust both sides = " << adjust_both_sides
                << ", fl scan location = " << fl_scan_location << ", fy scan direction = " << fy_scan_direction << ", white wall mode = " << white_wall_mode);
            check_focal_length_params(fl_step_count, fy_scan_range, keep_new_value_after_sucessful_scan, fl_data_sampling, adjust_both_sides, fl_scan_location, fy_scan_direction, white_wall_mode);

            // Begin auto-calibration
            int p4 = 0;
            if (keep_new_value_after_sucessful_scan)
                p4 |= (1 << 1);
            if (fl_data_sampling)
                p4 |= (1 << 3);
            if (adjust_both_sides)
                p4 |= (1 << 4);
            if (fl_scan_location)
                p4 |= (1 << 5);
            if (fy_scan_direction)
                p4 |= (1 << 6);
            if (white_wall_mode)
                p4 |= (1 << 7);
            _hw_monitor->send(command{ ds::AUTO_CALIB, focal_length_calib_begin, fl_step_count, fy_scan_range, p4 });

            FocalLengthCalibrationResult result{};

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
                    auto res = _hw_monitor->send(command{ ds::AUTO_CALIB, get_focal_legth_calib_result });

                    if (res.size() < sizeof(FocalLengthCalibrationResult))
                        throw std::runtime_error("Not enough data from CALIB_STATUS!");

                    result = *reinterpret_cast<FocalLengthCalibrationResult*>(res.data());
                    done = result.status != RS2_DSC_STATUS_RESULT_NOT_READY;
                }
                catch (const std::exception& ex)
                {
                    LOG_WARNING(ex.what());
                }

                if (progress_callback)
                    progress_callback->on_update_progress(count++ * (2.f * 3)); //curently this number does not reflect the actual progress

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

            res = get_calibration_results(health);
        }
        else
        {
            LOG_INFO("run_on_chip_calibration with parameters: speed = " << speed_fl
                << ", keep new value after sucessful scan = " << keep_new_value_after_sucessful_scan
                << " data_sampling = " << data_sampling << ", adjust both sides = " << adjust_both_sides
                << ", fl scan location = " << fl_scan_location << ", fy scan direction = " << fy_scan_direction << ", white wall mode = " << white_wall_mode);
            check_one_button_params(speed, keep_new_value_after_sucessful_scan, data_sampling, adjust_both_sides, fl_scan_location, fy_scan_direction, white_wall_mode);

            int p4 = 0;
            if (scan_parameter)
                p4 |= 1;
            if (keep_new_value_after_sucessful_scan)
                p4 |= (1 << 1);
            if (fl_data_sampling)
                p4 |= (1 << 3);
            if (adjust_both_sides)
                p4 |= (1 << 4);
            if (fl_scan_location)
                p4 |= (1 << 5);
            if (fy_scan_direction)
                p4 |= (1 << 6);
            if (white_wall_mode)
                p4 |= (1 << 7);

            std::shared_ptr<ds5_advanced_mode_base> preset_recover;
            if (speed == speed_white_wall && apply_preset)
                preset_recover = change_preset();

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            // Begin auto-calibration
            _hw_monitor->send(command{ ds::AUTO_CALIB, py_rx_plus_fl_calib_begin, speed_fl, 0, p4 });

            DscPyRxFLCalibrationTableResult result{};

            int count = 0;
            bool done = false;

            auto start = std::chrono::high_resolution_clock::now();
            auto now = start;
            float progress = 0.0f;

            // While not ready...
            do
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));

                // Check calibration status
                try
                {
                    auto res = _hw_monitor->send(command{ ds::AUTO_CALIB, get_py_rx_plus_fl_calib_result });

                    if (res.size() < sizeof(DscPyRxFLCalibrationTableResult))
                        throw std::runtime_error("Not enough data from CALIB_STATUS!");

                    result = *reinterpret_cast<DscPyRxFLCalibrationTableResult*>(res.data());
                    done = result.status != RS2_DSC_STATUS_RESULT_NOT_READY;
                }
                catch (const std::exception& ex)
                {
                    LOG_WARNING(ex.what());
                }

                if (progress_callback)
                {
                    progress = count++ * (2.f * speed);
                    progress_callback->on_update_progress(progress); //curently this number does not reflect the actual progress
                }

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
                handle_calibration_error(status);

            res = get_PyRxFL_calibration_results(&h_1, &h_2);

            int health_1 = static_cast<int>(abs(h_1) * 1000.0f + 0.5f);
            health_1 &= 0xFFF;

            int health_2 = static_cast<int>(abs(h_2) * 1000.0f + 0.5f);
            health_2 &= 0xFFF;

            int sign = 0;
            if (h_1 < 0.0f)
                sign = 1;
            if (h_2 < 0.0f)
                sign |= 2;

            int h = health_1;
            h |= health_2 << 12;
            h |= sign << 24;
            *health = static_cast<float>(h);
        }

        return res;
    }

    std::vector<uint8_t> auto_calibrated::run_tare_calibration(int timeout_ms, float ground_truth_mm, std::string json, update_progress_callback_ptr progress_callback)
    {
        int average_step_count = DEFAULT_AVERAGE_STEP_COUNT;
        int step_count = DEFAULT_STEP_COUNT;
        int accuracy = DEFAULT_ACCURACY;
        int speed = DEFAULT_SPEED;
        int scan_parameter = DEFAULT_SCAN;
        int data_sampling = DEFAULT_TARE_SAMPLING;
        int apply_preset = 1;
        std::vector<uint8_t> res;

        //Enforce Thermal Compensation off during Tare calibration
        volatile thermal_compensation_guard grd(this);

        if (json.size() > 0)
        {
            auto jsn = parse_json(json);
            try_fetch(jsn, "speed", &speed);
            try_fetch(jsn, "average step count", &average_step_count);
            try_fetch(jsn, "step count", &step_count);
            try_fetch(jsn, "accuracy", &accuracy);
            try_fetch(jsn, "scan parameter", &scan_parameter);
            try_fetch(jsn, "data sampling", &data_sampling);
            try_fetch(jsn, "apply preset", &apply_preset);
        }

        LOG_INFO("run_tare_calibration with parameters: speed = " << speed << " average_step_count = " << average_step_count << " step_count = " << step_count << " accuracy = " << accuracy << " scan_parameter = " << scan_parameter << " data_sampling = " << data_sampling);
        check_tare_params(speed, scan_parameter, data_sampling, average_step_count, step_count, accuracy);

        std::shared_ptr<ds5_advanced_mode_base> preset_recover;
        if (apply_preset)
            preset_recover = change_preset();

        auto param2 = (int)ground_truth_mm * 100;

        tare_calibration_params param3{ (byte)average_step_count, (byte)step_count, (byte)accuracy, 0};

        param4 param{ (byte)scan_parameter, 0, (byte)data_sampling };

        _hw_monitor->send(command{ ds::AUTO_CALIB, tare_calib_begin, param2, param3.param3, param.param_4});

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
                res = _hw_monitor->send(command{ ds::AUTO_CALIB, tare_calib_check_status });
                if (res.size() < sizeof(DirectSearchCalibrationResult))
                    throw std::runtime_error("Not enough data from CALIB_STATUS!");

                result = *reinterpret_cast<DirectSearchCalibrationResult*>(res.data());
                done = result.status != RS2_DSC_STATUS_RESULT_NOT_READY;
            }
            catch (const std::exception& ex)
            {
                LOG_INFO(ex.what());
            }

            if (progress_callback)
                progress_callback->on_update_progress(count++ * (2.f * speed)); //curently this number does not reflect the actual progress

            now = std::chrono::high_resolution_clock::now();

        } while (now - start < std::chrono::milliseconds(timeout_ms) && !done);

        // If we exit due to timeout, report timeout
        if (!done)
            throw std::runtime_error("Operation timed-out!\nCalibration state did not converged in time");

        auto status = (rs2_dsc_status)result.status;

        // Handle errors from firmware
        if (status != RS2_DSC_STATUS_SUCCESS)
            handle_calibration_error(status);

        return get_calibration_results();
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
                advanced_mode->_preset_opt->set(static_cast<float>(old_preset));
        });

        return recover_preset;
    }

    void auto_calibrated::check_params(int speed, int scan_parameter, int data_sampling) const
    {
        if (speed < speed_very_fast || speed >  speed_white_wall)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'speed' " << speed << " is out of range (0 - 4).");
       if (scan_parameter != py_scan && scan_parameter != rx_scan)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'scan parameter' " << scan_parameter << " is out of range (0 - 1).");
        if (data_sampling != polling && data_sampling != interrupt)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'data sampling' " << data_sampling << " is out of range (0 - 1).");
    }

    void auto_calibrated::check_tare_params(int speed, int scan_parameter, int data_sampling, int average_step_count, int step_count, int accuracy)
    {
        check_params(speed, scan_parameter, data_sampling);

        if (average_step_count < 1 || average_step_count > 30)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'number of frames to average' " << average_step_count << " is out of range (1 - 30).");
        if (step_count < 5 || step_count > 30)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'max iteration steps' " << step_count << " is out of range (5 - 30).");
        if (accuracy < very_high || accuracy > low)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'subpixel accuracy' " << accuracy << " is out of range (0 - 3).");
    }

    void auto_calibrated::check_focal_length_params(int step_count, int fy_scan_range, int keep_new_value_after_sucessful_scan, int interrrupt_data_samling, int adjust_both_sides, int fl_scan_location, int fy_scan_direction, int white_wall_mode) const
    {
        if (step_count < 8 || step_count >  256)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'step_count' " << step_count << " is out of range (8 - 256).");
        if (fy_scan_range < 1 || fy_scan_range >  60000)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'fy_scan_range' " << fy_scan_range << " is out of range (1 - 60000).");
        if (keep_new_value_after_sucessful_scan < 0 || keep_new_value_after_sucessful_scan >  1)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'keep_new_value_after_sucessful_scan' " << keep_new_value_after_sucessful_scan << " is out of range (0 - 1).");
        if (interrrupt_data_samling < 0 || interrrupt_data_samling >  1)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'interrrupt_data_samling' " << interrrupt_data_samling << " is out of range (0 - 1).");
        if (adjust_both_sides < 0 || adjust_both_sides >  1)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'adjust_both_sides' " << adjust_both_sides << " is out of range (0 - 1).");
        if (fl_scan_location < 0 || fl_scan_location >  1)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'fl_scan_location' " << fl_scan_location << " is out of range (0 - 1).");
        if (fy_scan_direction < 0 || fy_scan_direction >  1)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'fy_scan_direction' " << fy_scan_direction << " is out of range (0 - 1).");
        if (white_wall_mode < 0 || white_wall_mode >  1)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'white_wall_mode' " << white_wall_mode << " is out of range (0 - 1).");
    }

    void auto_calibrated::check_one_button_params(int speed, int keep_new_value_after_sucessful_scan, int data_sampling, int adjust_both_sides, int fl_scan_location, int fy_scan_direction, int white_wall_mode) const
    {
        if (speed < speed_very_fast || speed >  speed_white_wall)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'speed' " << speed << " is out of range (0 - 4).");
        if (keep_new_value_after_sucessful_scan < 0 || keep_new_value_after_sucessful_scan >  1)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'keep_new_value_after_sucessful_scan' " << keep_new_value_after_sucessful_scan << " is out of range (0 - 1).");
        if (data_sampling != polling && data_sampling != interrupt)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'data sampling' " << data_sampling << " is out of range (0 - 1).");
        if (adjust_both_sides < 0 || adjust_both_sides >  1)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'adjust_both_sides' " << adjust_both_sides << " is out of range (0 - 1).");
        if (fl_scan_location < 0 || fl_scan_location >  1)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'fl_scan_location' " << fl_scan_location << " is out of range (0 - 1).");
        if (fy_scan_direction < 0 || fy_scan_direction >  1)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'fy_scan_direction' " << fy_scan_direction << " is out of range (0 - 1).");
        if (white_wall_mode < 0 || white_wall_mode >  1)
            throw invalid_value_exception(to_string() << "Auto calibration failed! Given value of 'white_wall_mode' " << white_wall_mode << " is out of range (0 - 1).");
    }

    void auto_calibrated::handle_calibration_error(int status) const
    {
        if (status == RS2_DSC_STATUS_EDGE_TOO_CLOSE)
        {
            throw std::runtime_error("Calibration didn't converge! - edges too close\n"
                "Please retry in different lighting conditions");
        }
        else if (status == RS2_DSC_STATUS_FILL_FACTOR_TOO_LOW)
        {
            throw std::runtime_error("Not enough depth pixels! - low fill factor)\n"
                "Please retry in different lighting conditions");
        }
        else if (status == RS2_DSC_STATUS_NOT_CONVERGE)
        {
            throw std::runtime_error("Calibration failed to converge\n"
                "Please retry in different lighting conditions");
        }
        else if (status == RS2_DSC_STATUS_NO_DEPTH_AVERAGE)
        {
            throw std::runtime_error("Calibration didn't converge! - no depth average\n"
                "Please retry in different lighting conditions");
        }
        else throw std::runtime_error(to_string() << "Calibration didn't converge! (RESULT=" << int(status) << ")");
    }

    std::vector<uint8_t> auto_calibrated::get_calibration_results(float* health) const
    {
        using namespace ds;

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

        if(health)
            *health = reslt->m_dscResultParams.m_healthCheck;

        return calib;
    }

    std::vector<uint8_t> auto_calibrated::get_PyRxFL_calibration_results(float* health, float* health_fl) const
    {
        using namespace ds;

        // Get new calibration from the firmware
        auto res = _hw_monitor->send(command{ ds::AUTO_CALIB, get_py_rx_plus_fl_calib_result });
        if (res.size() < sizeof(DscPyRxFLCalibrationTableResult))
            throw std::runtime_error("Not enough data from CALIB_STATUS!");

        auto reslt = (DscPyRxFLCalibrationTableResult*)(res.data());

        table_header* header = reinterpret_cast<table_header*>(res.data() + sizeof(DscPyRxFLCalibrationTableResult));

        if (res.size() < sizeof(DscPyRxFLCalibrationTableResult) + sizeof(table_header) + header->table_size)
            throw std::runtime_error("Table truncated in CALIB_STATUS!");

        std::vector<uint8_t> calib;
        calib.resize(sizeof(table_header) + header->table_size, 0);
        memcpy(calib.data(), header, calib.size()); // Copy to new_calib

        if (health_fl)
            *health_fl = reslt->FL_heathCheck;

        if (health)
            *health = reslt->healthCheck;

        return calib;
    }

    std::vector<uint8_t> auto_calibrated::get_calibration_table() const
    {
        using namespace ds;

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
        using namespace ds;
        if(_curr_calibration.size() <= sizeof(table_header))
            throw std::runtime_error("Write calibration can be called only after set calibration table was called");

        table_header* hd = (table_header*)(_curr_calibration.data());
        calibration_table_id tbl_id = static_cast<calibration_table_id>(hd->table_type);
        fw_cmd cmd{};
        int param2 = 0;
        switch (tbl_id)
        {
        case coefficients_table_id:
            cmd = SETINTCAL;
            break;
        case rgb_calibration_id:
            cmd = SETINTCALNEW;
            param2 = 1;
            break;
        default:
            throw std::runtime_error(to_string() << "Flashing calibration table type 0x" << std::hex << tbl_id << " is not supported");
        }

        command write_calib(cmd, tbl_id, param2);
        write_calib.data = _curr_calibration;
        _hw_monitor->send(write_calib);

        LOG_DEBUG("Flashing " << ((tbl_id == coefficients_table_id) ? "Depth" : "RGB") << " calibration table");

    }

    void auto_calibrated::set_calibration_table(const std::vector<uint8_t>& calibration)
    {
        using namespace ds;

        table_header* hd = (table_header*)(calibration.data());
        ds::calibration_table_id tbl_id = static_cast<ds::calibration_table_id>(hd->table_type);

        switch (tbl_id)
        {
            case coefficients_table_id: // Load the modified depth calib table into flash RAM
            {
                uint8_t* table = (uint8_t*)(calibration.data() + sizeof(table_header));
                command write_calib(ds::CALIBRECALC, 0, 0, 0, 0xcafecafe);
                write_calib.data.insert(write_calib.data.end(), (uint8_t*)table, ((uint8_t*)table) + hd->table_size);
                _hw_monitor->send(write_calib);
            }
            case rgb_calibration_id: // case fall-through by design. For RGB skip loading to RAM (not supported)
                _curr_calibration = calibration;
                break;
            default:
                throw std::runtime_error(to_string() << "the operation is not defined for calibration table type " << tbl_id);
        }
    }

    void auto_calibrated::reset_to_factory_calibration() const
    {
        command cmd(ds::fw_cmd::CAL_RESTORE_DFLT);
        _hw_monitor->send(cmd);
    }

    void auto_calibrated::get_target_rect_info(rs2_frame_queue* frames, float rect_sides[4], float& fx, float& fy, int progress, update_progress_callback_ptr progress_callback)
    {
        fx = -1.0f;
        std::vector<std::array<float, 4>> rect_sides_arr;

        rs2_error* e = nullptr;
        rs2_frame* f = nullptr;

        int queue_size = rs2_frame_queue_size(frames, &e);
        if (queue_size==0)
            throw std::runtime_error("Extract target rectangle info - no frames in input queue!");
        int fc = 0;
        while ((fc++ < queue_size) && rs2_poll_for_frame(frames, &f, &e))
        {
            rs2::frame ff(f);
            if (ff.get_data())
            {
                if (fx < 0.0f)
                {
                    auto p = ff.get_profile();
                    auto vsp = p.as<rs2::video_stream_profile>();
                    rs2_intrinsics intrin = vsp.get_intrinsics();
                    fx = intrin.fx;
                    fy = intrin.fy;
                }

                std::array<float, 4> rec_sides_cur;
                rs2_extract_target_dimensions(f, RS2_CALIB_TARGET_ROI_RECT_GAUSSIAN_DOT_VERTICES, rec_sides_cur.data(), 4, &e);
                if (e)
                    throw std::runtime_error("Failed to extract target information\nfrom the captured frames!");
                rect_sides_arr.emplace_back(rec_sides_cur);
            }

            rs2_release_frame(f);

            if (progress_callback)
                progress_callback->on_update_progress(static_cast<float>(++progress));
        }

        if (rect_sides_arr.size())
        {
            for (int i = 0; i < 4; ++i)
                rect_sides[i] = rect_sides_arr[0][i];

            for (int j = 1; j < rect_sides_arr.size(); ++j)
            {
                for (int i = 0; i < 4; ++i)
                    rect_sides[i] += rect_sides_arr[j][i];
            }

            for (int i = 0; i < 4; ++i)
                rect_sides[i] /= rect_sides_arr.size();
        }
        else
            throw std::runtime_error("Failed to extract the target rectangle info!");
    }

    std::vector<uint8_t> auto_calibrated::run_focal_length_calibration(rs2_frame_queue* left, rs2_frame_queue* right, float target_w, float target_h,
        int adjust_both_sides, float *ratio, float * angle, update_progress_callback_ptr progress_callback)
    {
        float fx[2] = { -1.0f, -1.0f };
        float fy[2] = { -1.0f, -1.0f };

        float left_rect_sides[4] = {0.f};
        get_target_rect_info(left, left_rect_sides, fx[0], fy[0], 50, progress_callback); // Report 50% progress

        float right_rect_sides[4] = {0.f};
        get_target_rect_info(right, right_rect_sides, fx[1], fy[1], 75, progress_callback);

        std::vector<uint8_t> ret;
        const float correction_factor = 0.5f;

        auto calib_table = get_calibration_table();
        auto table = (librealsense::ds::coefficients_table*)calib_table.data();

        float ar[2] = { 0 };
        float tmp = left_rect_sides[2] + left_rect_sides[3];
        if (tmp > 0.1f)
            ar[0] = (left_rect_sides[0] + left_rect_sides[1]) / tmp;

        tmp = right_rect_sides[2] + right_rect_sides[3];
        if (tmp > 0.1f)
            ar[1] = (right_rect_sides[0] + right_rect_sides[1]) / tmp;

        float align = 0.0f;
        if (ar[0] > 0.0f)
            align = ar[1] / ar[0] - 1.0f;

        float ta[2] = { 0 };
        float gt[4] = { 0 };
        float ave_gt = 0.0f;

        if (left_rect_sides[0] > 0)
            gt[0] = fx[0] * target_w / left_rect_sides[0];

        if (left_rect_sides[1] > 0)
            gt[1] = fx[0] * target_w / left_rect_sides[1];

        if (left_rect_sides[2] > 0)
            gt[2] = fy[0] * target_h / left_rect_sides[2];

        if (left_rect_sides[3] > 0)
            gt[3] = fy[0] * target_h / left_rect_sides[3];

        ave_gt = 0.0f;
        for (int i = 0; i < 4; ++i)
            ave_gt += gt[i];
        ave_gt /= 4.0;

        ta[0] = atanf(align * ave_gt / abs(table->baseline));
        ta[0] = rad2deg(ta[0]);

        if (right_rect_sides[0] > 0)
            gt[0] = fx[1] * target_w / right_rect_sides[0];

        if (right_rect_sides[1] > 0)
            gt[1] = fx[1] * target_w / right_rect_sides[1];

        if (right_rect_sides[2] > 0)
            gt[2] = fy[1] * target_h / right_rect_sides[2];

        if (right_rect_sides[3] > 0)
            gt[3] = fy[1] * target_h / right_rect_sides[3];

        ave_gt = 0.0f;
        for (int i = 0; i < 4; ++i)
            ave_gt += gt[i];
        ave_gt /= 4.0;

        ta[1] = atanf(align * ave_gt / abs(table->baseline));
        ta[1] = rad2deg(ta[1]);

        *angle = (ta[0] + ta[1]) / 2;

        align *= 100;

        float r[4] = { 0 };
        float c = fx[0] / fx[1];

        if (left_rect_sides[0] > 0.1f)
            r[0] = c * right_rect_sides[0] / left_rect_sides[0];

        if (left_rect_sides[1] > 0.1f)
            r[1] = c * right_rect_sides[1] / left_rect_sides[1];

        c = fy[0] / fy[1];
        if (left_rect_sides[2] > 0.1f)
            r[2] = c * right_rect_sides[2] / left_rect_sides[2];

        if (left_rect_sides[3] > 0.1f)
            r[3] = c * right_rect_sides[3] / left_rect_sides[3];

        float ra = 0.0f;
        for (int i = 0; i < 4; ++i)
            ra += r[i];
        ra /= 4;

        ra -= 1.0f;
        ra *= 100;

        *ratio = ra - correction_factor * align;
        float ratio_to_apply = *ratio / 100.0f + 1.0f;

        if (adjust_both_sides)
        {
            float ratio_to_apply_2 = sqrtf(ratio_to_apply);
            table->intrinsic_left.x.x /= ratio_to_apply_2;
            table->intrinsic_left.x.y /= ratio_to_apply_2;
            table->intrinsic_right.x.x *= ratio_to_apply_2;
            table->intrinsic_right.x.y *= ratio_to_apply_2;
        }
        else
        {
            table->intrinsic_right.x.x *= ratio_to_apply;
            table->intrinsic_right.x.y *= ratio_to_apply;
        }

        auto actual_data = calib_table.data() + sizeof(librealsense::ds::table_header);
        auto actual_data_size = calib_table.size() - sizeof(librealsense::ds::table_header);
        auto crc = librealsense::calc_crc32(actual_data, actual_data_size);
        table->header.crc32 = crc;

        return calib_table;
    }

    void auto_calibrated::undistort(uint8_t* img, const rs2_intrinsics& intrin, int roi_ws, int roi_hs, int roi_we, int roi_he)
    {
        assert(intrin.model == RS2_DISTORTION_INVERSE_BROWN_CONRADY);

        int width = intrin.width;
        int height = intrin.height;

        if (roi_ws < 0) roi_ws = 0;
        if (roi_hs < 0) roi_hs = 0;
        if (roi_we > width) roi_we = width;
        if (roi_he > height) roi_he = height;

        int size3 = width * height * 3;
        std::vector<uint8_t> tmp(size3);
        memset(tmp.data(), 0, size3);

        float x = 0;
        float y = 0;
        int m = 0;
        int n = 0;

        int width3 = width * 3;
        int idx_from = 0;
        int idx_to = 0;
        for (int j = roi_hs; j < roi_he; ++j)
        {
            for (int i = roi_ws; i < roi_we; ++i)
            {
                x = static_cast<float>(i);
                y = static_cast<float>(j);

                if (abs(intrin.fx) > 0.00001f && abs(intrin.fy) > 0.0001f)
                {
                    x = (x - intrin.ppx) / intrin.fx;
                    y = (y - intrin.ppy) / intrin.fy;

                    float r2 = x * x + y * y;
                    float f = 1 + intrin.coeffs[0] * r2 + intrin.coeffs[1] * r2 * r2 + intrin.coeffs[4] * r2 * r2 * r2;
                    float ux = x * f + 2 * intrin.coeffs[2] * x * y + intrin.coeffs[3] * (r2 + 2 * x * x);
                    float uy = y * f + 2 * intrin.coeffs[3] * x * y + intrin.coeffs[2] * (r2 + 2 * y * y);
                    x = ux;
                    y = uy;

                    x = x * intrin.fx + intrin.ppx;
                    y = y * intrin.fy + intrin.ppy;
                }

                m = static_cast<int>(x + 0.5f);
                if (m >= 0 && m < width)
                {
                    n = static_cast<int>(y + 0.5f);
                    if (n >= 0 && n < height)
                    {
                        idx_from = j * width3 + i * 3;
                        idx_to = n * width3 + m * 3;
                        tmp[idx_to++] = img[idx_from++];
                        tmp[idx_to++] = img[idx_from++];
                        tmp[idx_to++] = img[idx_from++];
                    }
                }
            }
        }

        memmove(img, tmp.data(), size3);
    }

    void auto_calibrated::get_target_dots_info(rs2_frame_queue* frames, float dots_x[4], float dots_y[4], rs2::stream_profile& profile, rs2_intrinsics& intrin, int progress, update_progress_callback_ptr progress_callback)
    {
        bool got_intrinsics = false;
        std::vector<std::array<float, 4>> dots_x_arr;
        std::vector<std::array<float, 4>> dots_y_arr;

        rs2_error* e = nullptr;
        rs2_frame* f = nullptr;

        int queue_size = rs2_frame_queue_size(frames, &e);
        int fc = 0;

        while ((fc++ < queue_size) && rs2_poll_for_frame(frames, &f, &e))
        {
            rs2::frame ff(f);
            if (ff.get_data())
            {
                if (!got_intrinsics)
                {
                    profile = ff.get_profile();
                    auto vsp = profile.as<rs2::video_stream_profile>();
                    intrin = vsp.get_intrinsics();
                    got_intrinsics = true;
                }

                if (ff.get_profile().format() == RS2_FORMAT_RGB8)
                {
                    undistort(const_cast<uint8_t *>(static_cast<const uint8_t *>(ff.get_data())), intrin,
                            librealsense::_roi_ws - librealsense::_patch_size, librealsense::_roi_hs - librealsense::_patch_size,
                            librealsense::_roi_we + librealsense::_patch_size, librealsense::_roi_he + librealsense::_patch_size);
                }

                float dots[8] = { 0 };
                rs2_extract_target_dimensions(f, RS2_CALIB_TARGET_POS_GAUSSIAN_DOT_VERTICES, dots, 8, &e);
                if (e)
                    throw std::runtime_error("Failed to extract target information\nfrom the captured frames!");

                std::array<float, 4> dots_x_cur;
                std::array<float, 4> dots_y_cur;
                int j = 0;
                for (int i = 0; i < 4; ++i)
                {
                    j = i << 1;
                    dots_x_cur[i] = dots[j];
                    dots_y_cur[i] = dots[j + 1];
                }
                dots_x_arr.emplace_back(dots_x_cur);
                dots_y_arr.emplace_back(dots_y_cur);
            }

            rs2_release_frame(f);

            if (progress_callback)
                progress_callback->on_update_progress(static_cast<float>(++progress));
        }

        for (int i = 0; i < 4; ++i)
        {
            dots_x[i] = dots_x_arr[0][i];
            dots_y[i] = dots_y_arr[0][i];
        }

        for (int j = 1; j < dots_x_arr.size(); ++j)
        {
            for (int i = 0; i < 4; ++i)
            {
                dots_x[i] += dots_x_arr[j][i];
                dots_y[i] += dots_y_arr[j][i];
            }
        }

        for (int i = 0; i < 4; ++i)
        {
            dots_x[i] /= dots_x_arr.size();
            dots_y[i] /= dots_y_arr.size();
        }
    }

    void auto_calibrated::find_z_at_corners(float left_x[4], float left_y[4], rs2_frame_queue* frames, float left_z[4])
    {
        int x1[4] = { 0 };
        int y1[4] = { 0 };
        int x2[4] = { 0 };
        int y2[4] = { 0 };
        int pos_tl[4] = { 0 };
        int pos_tr[4] = { 0 };
        int pos_bl[4] = { 0 };
        int pos_br[4] = { 0 };

        float left_z_tl[4] = { 0 };
        float left_z_tr[4] = { 0 };
        float left_z_bl[4] = { 0 };
        float left_z_br[4] = { 0 };

        bool got_width = false;
        int width = 0;
        int counter = 0;
        rs2_error* e = nullptr;
        rs2_frame* f = nullptr;

        int queue_size = rs2_frame_queue_size(frames, &e);
        int fc = 0;

        while ((fc++ < queue_size) && rs2_poll_for_frame(frames, &f, &e))
        {
            rs2::frame ff(f);
            if (ff.get_data())
            {
                if (!got_width)
                {
                    auto p = ff.get_profile();
                    auto vsp = p.as<rs2::video_stream_profile>();
                    width = vsp.get_intrinsics().width;
                    got_width = true;

                    for (int i = 0; i < 4; ++i)
                    {
                        x1[i] = static_cast<int>(left_x[i]);
                        y1[i] = static_cast<int>(left_y[i]);

                        x2[i] = static_cast<int>(left_x[i] + 1.0f);
                        y2[i] = static_cast<int>(left_y[i] + 1.0f);

                        pos_tl[i] = y1[i] * width + x1[i];
                        pos_tr[i] = y1[i] * width + x2[i];
                        pos_bl[i] = y2[i] * width + x1[i];
                        pos_br[i] = y2[i] * width + x2[i];
                    }
                }

                const uint16_t* depth = reinterpret_cast<const uint16_t*>(ff.get_data());
                for (int i = 0; i < 4; ++i)
                {
                    left_z_tl[i] += static_cast<float>(depth[pos_tl[i]]);
                    left_z_tr[i] += static_cast<float>(depth[pos_tr[i]]);
                    left_z_bl[i] += static_cast<float>(depth[pos_bl[i]]);
                    left_z_br[i] += static_cast<float>(depth[pos_br[i]]);
                }

                ++counter;
            }

            rs2_release_frame(f);
        }

        for (int i = 0; i < 4; ++i)
        {
            left_z_tl[i] /= counter;
            left_z_tr[i] /= counter;
            left_z_bl[i] /= counter;
            left_z_br[i] /= counter;
        }

        float z_1 = 0.0f;
        float z_2 = 0.0f;
        float s = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            s = left_x[i] - x1[i];
            z_1 = (1.0f - s) * left_z_bl[i] + s * left_z_br[i];
            z_2 = (1.0f - s) * left_z_tl[i] + s * left_z_tr[i];

            s = left_y[i] - y1[i];
            left_z[i] = (1.0f - s) * z_2 + s * z_1;
            left_z[i] *= 0.001f;
        }
    }

    std::vector<uint8_t> auto_calibrated::run_uv_map_calibration(rs2_frame_queue* left, rs2_frame_queue* color, rs2_frame_queue* depth, int py_px_only,
        float* health, int health_size, update_progress_callback_ptr progress_callback)
    {
        float left_dots_x[4];
        float left_dots_y[4];
        rs2_intrinsics left_intrin;
        rs2::stream_profile left_profile;
        get_target_dots_info(left, left_dots_x, left_dots_y, left_profile, left_intrin, 50, progress_callback);

        float color_dots_x[4];
        float color_dots_y[4];
        rs2_intrinsics color_intrin;
        rs2::stream_profile color_profile;
        get_target_dots_info(color, color_dots_x, color_dots_y, color_profile, color_intrin, 75, progress_callback);

        float z[4] = { 0 };
        find_z_at_corners(left_dots_x, left_dots_y, depth, z);

        rs2_extrinsics extrin = left_profile.get_extrinsics_to(color_profile);

        float pixel_left[4][2] = { 0 };
        float point_left[4][3] = { 0 };

        float pixel_color[4][2] = { 0 };
        float pixel_color_norm[4][2] = { 0 };
        float point_color[4][3] = { 0 };

        for (int i = 0; i < 4; ++i)
        {
            pixel_left[i][0] = left_dots_x[i];
            pixel_left[i][1] = left_dots_y[i];

            rs2_deproject_pixel_to_point(point_left[i], &left_intrin, pixel_left[i], z[i]);
            rs2_transform_point_to_point(point_color[i], &extrin, point_left[i]);

            pixel_color_norm[i][0] = point_color[i][0] / point_color[i][2];
            pixel_color_norm[i][1] = point_color[i][1] / point_color[i][2];
            pixel_color[i][0] = pixel_color_norm[i][0] * color_intrin.fx + color_intrin.ppx;
            pixel_color[i][1] = pixel_color_norm[i][1] * color_intrin.fy + color_intrin.ppy;
        }

        float diff[4] = { 0 };
        float tmp = 0.0f;
        for (int i = 0; i < 4; ++i)
        {
            tmp = (pixel_color[i][0] - color_dots_x[i]);
            tmp *= tmp;
            diff[i] = tmp;

            tmp = (pixel_color[i][1] - color_dots_y[i]);
            tmp *= tmp;
            diff[i] += tmp;

            diff[i] = sqrtf(diff[i]);
        }

        float err_before = 0.0f;
        for (int i = 0; i < 4; ++i)
            err_before += diff[i];
        err_before /= 4;

        float ppx = 0.0f;
        float ppy = 0.0f;
        float fx = 0.0f;
        float fy = 0.0f;

        if (py_px_only)
        {
            fx = color_intrin.fx;
            fy = color_intrin.fy;

            ppx = 0.0f;
            ppy = 0.0f;
            for (int i = 0; i < 4; ++i)
            {
                ppx += color_dots_x[i] - pixel_color_norm[i][0] * fx;
                ppy += color_dots_y[i] - pixel_color_norm[i][1] * fy;
            }
            ppx /= 4.0f;
            ppy /= 4.0f;
        }
        else
        {
            double x = 0;
            double y = 0;
            double c_x = 0;
            double c_y = 0;
            double x_2 = 0;
            double y_2 = 0;
            double c_xc = 0;
            double c_yc = 0;
            for (int i = 0; i < 4; ++i)
            {
                x += pixel_color_norm[i][0];
                y += pixel_color_norm[i][1];
                c_x += color_dots_x[i];
                c_y += color_dots_y[i];
                x_2 += pixel_color_norm[i][0] * pixel_color_norm[i][0];
                y_2 += pixel_color_norm[i][1] * pixel_color_norm[i][1];
                c_xc += color_dots_x[i] * pixel_color_norm[i][0];
                c_yc += color_dots_y[i] * pixel_color_norm[i][1];
            }

            double d_x = 4 * x_2 - x * x;
            if (d_x > 0.01)
            {
                d_x = 1 / d_x;
                fx = static_cast<float>(d_x * (4 * c_xc - x * c_x));
                ppx = static_cast<float>(d_x * (x_2 * c_x - x * c_xc));
            }

            double d_y = 4 * y_2 - y * y;
            if (d_y > 0.01)
            {
                d_y = 1 / d_y;
                fy = static_cast<float>(d_y * (4 * c_yc - y * c_y));
                ppy = static_cast<float>(d_y * (y_2 * c_y - y * c_yc));
            }
        }

        float err_after = 0.0f;
        float tmpx = 0;
        float tmpy = 0;
        for (int i = 0; i < 4; ++i)
        {
            tmpx = pixel_color_norm[i][0] * fx + ppx - color_dots_x[i];
            tmpx *= tmpx;

            tmpy = pixel_color_norm[i][1] * fy + ppy - color_dots_y[i];
            tmpy *= tmpy;

            err_after += sqrtf(tmpx + tmpy);
        }

        err_after /= 4.0f;

        std::vector<uint8_t> ret;
        const float max_change = 16.0f;
        if (fabs(color_intrin.ppx - ppx) < max_change && fabs(color_intrin.ppy - ppy) < max_change && fabs(color_intrin.fx - fx) < max_change && fabs(color_intrin.fy - fy) < max_change)
        {
            ret = _hw_monitor->send(command{ds::GETINTCAL, ds::rgb_calibration_id });
            auto table = reinterpret_cast<librealsense::ds::rgb_calibration_table*>(ret.data());

            health[0] = table->intrinsic(2, 0); // px
            health[1] = table->intrinsic(2, 1); // py
            health[2] = table->intrinsic(0, 0); // fx
            health[3] = table->intrinsic(1, 1); // fy

            table->intrinsic(2, 0) = 2.0f * ppx / color_intrin.width - 1;  // ppx
            table->intrinsic(2, 1) = 2.0f * ppy / color_intrin.height - 1; // ppy
            table->intrinsic(0, 0) = 2.0f * fx / color_intrin.width;       // fx
            table->intrinsic(1, 1) = 2.0f * fy / color_intrin.height;      // fy

            float calib_aspect_ratio = 9.f / 16.f; // shall be overwritten with the actual calib resolution
            if (table->calib_width && table->calib_height)
                calib_aspect_ratio = float(table->calib_height) / float(table->calib_width);

            float actual_aspect_ratio = color_intrin.height / (float)color_intrin.width;
            if (actual_aspect_ratio < calib_aspect_ratio)
            {
                table->intrinsic(1, 1) /= calib_aspect_ratio / actual_aspect_ratio; // fy
                table->intrinsic(2, 1) /= calib_aspect_ratio / actual_aspect_ratio; // ppy
            }
            else
            {
                table->intrinsic(0, 0) /= actual_aspect_ratio / calib_aspect_ratio; // fx
                table->intrinsic(2, 0) /= actual_aspect_ratio / calib_aspect_ratio; // ppx
            }

            table->header.crc32 = calc_crc32(ret.data() + sizeof(librealsense::ds::table_header), ret.size() - sizeof(librealsense::ds::table_header));

            health[0] = (abs(table->intrinsic(2, 0) / health[0]) - 1) * 100; // px
            health[1] = (abs(table->intrinsic(2, 1) / health[1]) - 1) * 100; // py
            health[2] = (abs(table->intrinsic(0, 0) / health[2]) - 1) * 100; // fx
            health[3] = (abs(table->intrinsic(1, 1) / health[3]) - 1) * 100; // fy
        }

        return ret;
    }

    float auto_calibrated::calculate_target_z(rs2_frame_queue* queue1, rs2_frame_queue* queue2, rs2_frame_queue* queue3,
        float target_w, float target_h, update_progress_callback_ptr progress_callback)
    {
        constexpr size_t min_frames_required = 10;
        bool created = false;

        float4 rect_sides{};
        float target_fw = 0;
        float target_fh = 0;

        float target_z_value = -1.f;
        std::vector<float4>  rec_sides_data;
        rect_calculator target_z_calculator(true);

        int frm_idx = 0;
        int progress = 0;
        rs2_error* e = nullptr;
        rs2_frame* f = nullptr;

        int queue_size = rs2_frame_queue_size(queue1, &e);
        int fc = 0;

        while ((fc++ < queue_size) && rs2_poll_for_frame(queue1, &f, &e))
        {
            rs2::frame ff(f);
            if (ff.get_data())
            {
                if (!created)
                {
                    auto vsp = ff.get_profile().as<rs2::video_stream_profile>();

                    target_fw = vsp.get_intrinsics().fx * target_w;
                    target_fh = vsp.get_intrinsics().fy * target_h;
                    created = true;
                }

                // retirieve target size and accumulate results, skip frame if target could not be found
                if (target_z_calculator.extract_target_dims(f, rect_sides))
                {
                    rec_sides_data.push_back(rect_sides);
                }

                frm_idx++;
            }

            rs2_release_frame(f);

            if (progress_callback)
                progress_callback->on_update_progress(static_cast<float>(progress++));

        }

        if (rec_sides_data.size())
        {
            // Verify that at least TBD  valid extractions were made
            if ((frm_idx < min_frames_required))
                throw std::runtime_error(to_string() << "Target distance calculation requires at least " << min_frames_required << " frames, aborting");
            if (float(rec_sides_data.size() / frm_idx) < 0.5f)
                throw std::runtime_error("Please re-adjust the camera position \nand make sure the specific target is \nin the middle of the camera image!");

            rect_sides = {};
            auto avg_data = std::accumulate(rec_sides_data.begin(), rec_sides_data.end(), rect_sides);
            for (auto i = 0UL; i < 4; i++)
                avg_data[i] /= rec_sides_data.size();

            float gt[4] = { 0 };

            gt[0] = target_fw / avg_data[0];

            gt[1] = target_fw / avg_data[1];

            gt[2] = target_fh / avg_data[2];

            gt[3] = target_fh / avg_data[3];

            if (gt[0] <= 0.1f || gt[1] <= 0.1f || gt[2] <= 0.1f || gt[3] <= 0.1f)
                throw std::runtime_error("Target distance calculation failed");

            // Target's plane Z value is the average of the four calculated corners
            target_z_value = 0.f;
            for (int i = 0; i < 4; ++i)
                target_z_value += gt[i];
            target_z_value /= 4.f;

            return target_z_value;
        }
        else
            throw std::runtime_error("Failed to extract target dimension info!");
    }
}
