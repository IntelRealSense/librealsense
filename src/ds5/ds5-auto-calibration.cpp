// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "../third-party/json.hpp"
#include "ds5-device.h"
#include "ds5-private.h"
#include "ds5-thermal-monitor.h"
#include "ds5-auto-calibration.h"

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
        get_py_rx_plus_fl_calib_result = 0x14,
        set_coefficients = 0x19
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
                        restart_tl = static_cast<bool>(snr->get_option(RS2_OPTION_THERMAL_COMPENSATION).query());
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
                    LOG_WARNING(ex.what());
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

            res = get_PyRxFL_calibration_results(&h_1 , &h_2);

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
                auto res = _hw_monitor->send(command{ ds::AUTO_CALIB, tare_calib_check_status });
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
                progress_callback->on_update_progress(count++ * (2.f * speed)); //curently this number does not reflect the actual progress

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

        if(_curr_calibration.size() < sizeof(table_header))
            throw std::runtime_error("Write calibration can be called only after set calibration table was called");

        command write_calib( ds::SETINTCAL, set_coefficients );
        write_calib.data = _curr_calibration;
        _hw_monitor->send(write_calib);
    }

    void auto_calibrated::set_calibration_table(const std::vector<uint8_t>& calibration)
    {
        using namespace ds;

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
