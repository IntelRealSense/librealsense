// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <numeric>
#include "../third-party/json.hpp"
#include "d400-device.h"
#include "d400-private.h"
#include "d400-thermal-monitor.h"
#include "d400-auto-calibration.h"
#include "librealsense2/rsutil.h"
#include "algo.h"

#include <rsutils/string/from.h>


#undef UCAL_PROFILE
#ifdef UCAL_PROFILE
#define LOG_OCC_WARN(...)   do { CLOG(WARNING   ,"librealsense") << __VA_ARGS__; } while(false)
#else
#define LOG_OCC_WARN(...)
#endif //UCAL_PROFILE

namespace librealsense
{
#pragma pack(push, 1)
#pragma pack(1)
    struct TareCalibrationResult
    {
        uint16_t status;  // DscStatus
        uint32_t tareDepth;  // Tare depth in 1/100 of depth unit
        uint32_t aveDepth;  // Average depth in 1/100 of depth unit
        int32_t curPx;    // Current Px in 1/1000000 of normalized unit
        int32_t calPx;    // Calibrated Px in 1/1000000 of normalized unit
        float curRightRotation[9]; // Current right rotation
        float calRightRotation[9]; // Calibrated right rotation
        uint16_t accuracyLevel;  // [0-3] (Very High/High/Medium/Low)
        uint16_t iterations;        // Number of iterations it took to converge
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
        py_rx_calib_check_status        = 0x03,
        interactive_scan_control        = 0x05,
        py_rx_calib_begin               = 0x08,
        tare_calib_begin                = 0x0b,
        tare_calib_check_status         = 0x0c,
        get_calibration_result          = 0x0d,
        focal_length_calib_begin        = 0x11,
        get_focal_legth_calib_result    = 0x12,
        py_rx_plus_fl_calib_begin       = 0x13,
        get_py_rx_plus_fl_calib_result  = 0x14,
        set_coefficients                = 0x19
    };

    enum auto_calib_speed : uint8_t
    {
        speed_very_fast = 0,
        speed_fast = 1,
        speed_medium = 2,
        speed_slow = 3,
        speed_white_wall = 4
    };

    enum class host_assistance_type
    {
        no_assistance = 0,
        assistance_start,
        assistance_first_feed,
        assistance_second_feed,
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
        uint32_t param3;
    };

    union param4
    {
        params4 param4_struct;
        uint32_t param_4;
    };

    const int DEFAULT_CALIB_TYPE = 0;

    const int DEFAULT_AVERAGE_STEP_COUNT = 20;
    const int DEFAULT_STEP_COUNT = 20;
    const int DEFAULT_ACCURACY = subpixel_accuracy::medium;
    const auto_calib_speed DEFAULT_SPEED = auto_calib_speed::speed_slow;
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

    auto_calibrated::auto_calibrated()
        : _interactive_state(interactive_calibration_state::RS2_OCC_STATE_NOT_ACTIVE),
          _interactive_scan(false),
          _action(auto_calib_action::RS2_OCC_ACTION_ON_CHIP_CALIB),
          _average_step_count(-1),
          _collected_counter(-1),
          _collected_frame_num(-1),
          _collected_sum(-1.0),
          _min_valid_depth(0),
          _max_valid_depth(uint16_t(-1)),
          _resize_factor(5),
          _skipped_frames(0)
    {}

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

    DirectSearchCalibrationResult auto_calibrated::get_calibration_status(int timeout_ms, std::function<void(const int count)> progress_func, bool wait_for_final_results)
    {
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
            try
            {
                // Check calibration status
                auto res = _hw_monitor->send(command{ ds::AUTO_CALIB, py_rx_calib_check_status });
                LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__  << " check occ status, res size = " << res.size()));
                if (res.size() < sizeof(DirectSearchCalibrationResult))
                {
                    if (!((retries++) % 5)) // Add log debug once in a sec
                    {
                        LOG_DEBUG("Not enough data from CALIB_STATUS!");
                    }
                }
                else
                {
                    result = *reinterpret_cast<DirectSearchCalibrationResult*>(res.data());
                    done = !wait_for_final_results || result.status != RS2_DSC_STATUS_RESULT_NOT_READY;
                }
            }
            catch (const invalid_value_exception& e)
            {
                LOG_DEBUG("error: " << e.what());
                // Asked for status while firmware is still in progress.
            }

            if (progress_func)
            {
                progress_func(count);
            }

            now = std::chrono::high_resolution_clock::now();

        } while (now - start < std::chrono::milliseconds(timeout_ms) && !done);


        // If we exit due to timeout, report timeout
        if (!done)
        {
            throw std::runtime_error("Operation timed-out!\n"
                "Calibration state did not converge on time");
        }
        return result;
    }

    std::vector<uint8_t> auto_calibrated::run_on_chip_calibration(int timeout_ms, std::string json, float* const health, update_progress_callback_ptr progress_callback)
    {
        int calib_type = DEFAULT_CALIB_TYPE;

        auto_calib_speed speed(DEFAULT_SPEED);
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

        host_assistance_type host_assistance(host_assistance_type::no_assistance);
        int scan_only_v3 = 0;
        int interactive_scan_v3 = 0;
        uint16_t step_count_v3 = 0;
        uint16_t fill_factor[256] = { 0 };
        
        float h_1 = 0.0f;
        float h_2 = 0.0f;

        //Enforce Thermal Compensation off during OCC
        volatile thermal_compensation_guard grd(this);

        if (json.size() > 0)
        {
            int tmp_speed(DEFAULT_SPEED);
            int tmp_host_assistance(0);
            auto jsn = parse_json(json);
            try_fetch(jsn, "calib type", &calib_type);
            if (calib_type == 0)
            {
                try_fetch(jsn, "speed", &tmp_speed);
                if (tmp_speed < speed_very_fast || tmp_speed >  speed_white_wall)
                    throw invalid_value_exception( rsutils::string::from()
                                                   << "Auto calibration failed! Given value of 'speed' " << speed
                                                   << " is out of range (0 - 4)." );
                speed = auto_calib_speed(tmp_speed);
            }
            else
                try_fetch(jsn, "speed", &speed_fl);

            try_fetch(jsn, "host assistance", &tmp_host_assistance);
            if (tmp_host_assistance < (int)host_assistance_type::no_assistance || tmp_host_assistance >  (int)host_assistance_type::assistance_second_feed)
                throw invalid_value_exception( rsutils::string::from()
                                               << "Auto calibration failed! Given value of 'host assistance' "
                                               << tmp_host_assistance << " is out of range (0 - "
                                               << (int)host_assistance_type::assistance_second_feed << ")." );
            host_assistance = host_assistance_type(tmp_host_assistance);

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

            try_fetch(jsn, "scan only", &scan_only_v3);
            try_fetch(jsn, "interactive scan", &interactive_scan_v3);

            int val = 0;
            try_fetch(jsn, "step count v3", &val);

            step_count_v3 = static_cast<uint16_t>(val);
            if (step_count_v3 > 0)
            {
                for (int i = 0; i < step_count_v3; ++i)
                {
                    val = 0;
                    std::stringstream ss;
                    ss << "fill factor " << i;
                    try_fetch(jsn, ss.str(), &val);
                    fill_factor[i] = static_cast<uint16_t>(val);
                }
            }
            try_fetch(jsn, "resize factor", &_resize_factor);
        }

        std::vector<uint8_t> res;

        if (host_assistance != host_assistance_type::no_assistance && _interactive_state == interactive_calibration_state::RS2_OCC_STATE_NOT_ACTIVE)
        {
            _json = json;
            _action = auto_calib_action::RS2_OCC_ACTION_ON_CHIP_CALIB;
            _interactive_state = interactive_calibration_state::RS2_OCC_STATE_WAIT_TO_CAMERA_START;
            _interactive_scan = false; // Production code must enforce non-interactive runs. Interactive scans for development only
            switch (speed)
            {
            case auto_calib_speed::speed_very_fast:
                _total_frames = 60;
                break;
            case auto_calib_speed::speed_fast:
                _total_frames = 120;
                break;
            case auto_calib_speed::speed_medium:
                _total_frames = 256;
                break;
            case auto_calib_speed::speed_slow:
                _total_frames = 256;
                break;
            case auto_calib_speed::speed_white_wall:
                _total_frames = 120;
                break;
            }
            std::fill_n(_fill_factor, 256, 0);
            DirectSearchCalibrationResult result = get_calibration_status(timeout_ms, [progress_callback, host_assistance, speed](int count)
            {
                if (progress_callback)
                {
                    if (host_assistance != host_assistance_type::no_assistance)
                        if (count < 20) progress_callback->on_update_progress(static_cast<float>(80 + count++));
                        else
                            progress_callback->on_update_progress(count++ * (2.f * static_cast<int>(speed))); //curently this number does not reflect the actual progress
                }
            }, false);
            // Handle errors from firmware
            rs2_dsc_status status = (rs2_dsc_status)result.status;

            if (result.maxDepth == 0)
            {
                throw std::runtime_error("Firmware calibration values are not yet set.");
            }
            _min_valid_depth = result.minDepth;
            _max_valid_depth = result.maxDepth;
            return res;
        }

        std::shared_ptr<ds_advanced_mode_base> preset_recover;
        if (calib_type == 0)
        {
            LOG_DEBUG("run_on_chip_calibration with parameters: speed = " << speed << " scan_parameter = " << scan_parameter << " data_sampling = " << data_sampling);
            check_params(speed, scan_parameter, data_sampling);

            uint32_t p4 = 0;
            if (scan_parameter)
                p4 |= 1;
            if (host_assistance != host_assistance_type::no_assistance)
                p4 |= (1 << 1);
            if (data_sampling)
                p4 |= (1 << 3);
            if (scan_only_v3)
                p4 |= (1 << 8);
            if (interactive_scan_v3)
                p4 |= (1 << 9);

            if (speed == speed_white_wall && apply_preset)
            {
                preset_recover = change_preset();
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }

            // Begin auto-calibration
            if (host_assistance == host_assistance_type::no_assistance || host_assistance == host_assistance_type::assistance_start)
            {
                auto res = _hw_monitor->send(command{ ds::AUTO_CALIB, py_rx_calib_begin, speed, 0, p4 });
                LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__
                    << " send occ py_rx_calib_begin, speed = " << speed << ", p4 = " << p4 << " res size = " << res.size()));
            }

            if (host_assistance != host_assistance_type::assistance_start)
            {
                if (host_assistance == host_assistance_type::assistance_first_feed)
                {
                    command cmd(ds::AUTO_CALIB, interactive_scan_control, 0, 0);
                    LOG_OCC_WARN(" occ interactive_scan_control 0,0 - save statistics ");
                    uint8_t* p = reinterpret_cast<uint8_t*>(&step_count_v3);
                    cmd.data.push_back(p[0]);
                    cmd.data.push_back(p[1]);
                    for (uint16_t i = 0; i < step_count_v3; ++i)
                    {
                        p = reinterpret_cast<uint8_t*>(fill_factor + i);
                        cmd.data.push_back(p[0]);
                        cmd.data.push_back(p[1]);
                    }
                    bool success = false;
                    int iter =0;
                    do // Retries are needed to overcome MIPI SKU occasionaly reporting busy state
                    {
                        try
                        {
                            if (iter==0) // apply only in the first iteration
                                std::this_thread::sleep_for(std::chrono::milliseconds(1000));   // Sending shorter request may fail with MIPI SKU
                            auto res = _hw_monitor->send(cmd);
                            success = true;
                        }
                        catch(...)
                        {
                            LOG_OCC_WARN("occ Save Statistics result failed");
                            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // For the FW to recuperate
                        };
                    }
                    while(( ++iter < 3) && (!success));
                }

                DirectSearchCalibrationResult result = get_calibration_status(timeout_ms, [progress_callback, host_assistance, speed](int count)
                    {
                        if (progress_callback)
                        {
                            if (host_assistance != host_assistance_type::no_assistance)
                                if (count < 20) progress_callback->on_update_progress(static_cast<float>(80 + count++));
                            else
                                progress_callback->on_update_progress(count++ * (2.f * static_cast<int>(speed))); //curently this number does not reflect the actual progress
                        }
                    });
                std::this_thread::sleep_for(std::chrono::milliseconds(100));

                auto status = (rs2_dsc_status)result.status;

                // Handle errors from firmware
                if (status != RS2_DSC_STATUS_SUCCESS)
                {
                    handle_calibration_error(status);
                }
                if (progress_callback)
                    progress_callback->on_update_progress(static_cast<float>(100));
                res = get_calibration_results(health);
                LOG_OCC_WARN(std::string(rsutils::string::from() << "Py: " << result.rightPy));
            }
        }
        else if (calib_type == 1)
        {
            LOG_DEBUG("run_on_chip_focal_length_calibration with parameters: step count = " << fl_step_count
                << ", fy scan range = " << fy_scan_range << ", keep new value after sucessful scan = " << keep_new_value_after_sucessful_scan
                << ", interrrupt data sampling " << fl_data_sampling << ", adjust both sides = " << adjust_both_sides
                << ", fl scan location = " << fl_scan_location << ", fy scan direction = " << fy_scan_direction << ", white wall mode = " << white_wall_mode);
            check_focal_length_params(fl_step_count, fy_scan_range, keep_new_value_after_sucessful_scan, fl_data_sampling, adjust_both_sides, fl_scan_location, fy_scan_direction, white_wall_mode);

            // Begin auto-calibration
            uint32_t p4 = 0;
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
            if (scan_only_v3)
                p4 |= (1 << 8);
            if (interactive_scan_v3)
                p4 |= (1 << 9);

            if (speed == speed_white_wall && apply_preset)
            {
                preset_recover = change_preset();
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }

            if( host_assistance == host_assistance_type::no_assistance || host_assistance == host_assistance_type::assistance_start )
            {
                _hw_monitor->send( command{ ds::AUTO_CALIB,
                                            focal_length_calib_begin,
                                            static_cast< uint32_t >( fl_step_count ),
                                            static_cast< uint32_t >( fy_scan_range ),
                                            p4 } );
            }

            if (host_assistance != host_assistance_type::assistance_start)
            {
                if (host_assistance == host_assistance_type::assistance_first_feed)
                {
                    command cmd(ds::AUTO_CALIB, interactive_scan_control, 0, 0);
                    LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__ << "occ interactive_scan_control 0,0 " << " res size = " << res.size()));
                    uint8_t* p = reinterpret_cast<uint8_t*>(&step_count_v3);
                    cmd.data.push_back(p[0]);
                    cmd.data.push_back(p[1]);
                    for (uint16_t i = 0; i < step_count_v3; ++i)
                    {
                        p = reinterpret_cast<uint8_t*>(fill_factor + i);
                        cmd.data.push_back(p[0]);
                        cmd.data.push_back(p[1]);
                    }

                    _hw_monitor->send(cmd);
                }

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
                    auto res = _hw_monitor->send(command{ ds::AUTO_CALIB, get_focal_legth_calib_result });

                    if (res.size() < sizeof(FocalLengthCalibrationResult))
                        LOG_WARNING("Not enough data from CALIB_STATUS!");
                    else
                    {
                        result = *reinterpret_cast<FocalLengthCalibrationResult*>(res.data());
                        done = result.status != RS2_DSC_STATUS_RESULT_NOT_READY;
                    }

                    if (progress_callback)
                    {
                        if (host_assistance != host_assistance_type::no_assistance)
                            if (count < 20) progress_callback->on_update_progress(static_cast<float>(80 + count++));
                        else
                            progress_callback->on_update_progress(count++* (2.f * 3)); //curently this number does not reflect the actual progress
                    }

                    now = std::chrono::high_resolution_clock::now();

                } while (now - start < std::chrono::milliseconds(timeout_ms) && !done);


                // If we exit due to timeout, report timeout
                if (!done)
                {
                    throw std::runtime_error("Operation timed-out!\n"
                        "Calibration did not converge on time");
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
        }
        else if (calib_type == 2)
        {
            LOG_DEBUG("run_on_chip_calibration with parameters: speed = " << speed_fl
                << ", keep new value after sucessful scan = " << keep_new_value_after_sucessful_scan
                << " data_sampling = " << data_sampling << ", adjust both sides = " << adjust_both_sides
                << ", fl scan location = " << fl_scan_location << ", fy scan direction = " << fy_scan_direction << ", white wall mode = " << white_wall_mode);
            check_one_button_params(speed, keep_new_value_after_sucessful_scan, data_sampling, adjust_both_sides, fl_scan_location, fy_scan_direction, white_wall_mode);

            uint32_t p4 = 0;
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
            if (scan_only_v3)
                p4 |= (1 << 8);
            if (interactive_scan_v3)
                p4 |= (1 << 9);

            if (speed == speed_white_wall && apply_preset)
            {
                preset_recover = change_preset();
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
            }

            // Begin auto-calibration
            if (host_assistance == host_assistance_type::no_assistance || host_assistance == host_assistance_type::assistance_start)
            {
                _hw_monitor->send( command{ ds::AUTO_CALIB,
                                            py_rx_plus_fl_calib_begin,
                                            static_cast< uint32_t >( speed_fl ),
                                            0,
                                            p4 } );
                LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__ << "occ py_rx_plus_fl_calib_begin speed_fl = " << speed_fl <<  " res size = " << res.size()));
            }

            if (host_assistance != host_assistance_type::assistance_start)
            {
                if ((host_assistance == host_assistance_type::assistance_first_feed) || (host_assistance == host_assistance_type::assistance_second_feed))
                {
                    command cmd(ds::AUTO_CALIB, interactive_scan_control, 0, 0);
                    LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__ << "occ interactive_scan_control 0,0"));
                    uint8_t* p = reinterpret_cast<uint8_t*>(&step_count_v3);
                    cmd.data.push_back(p[0]);
                    cmd.data.push_back(p[1]);
                    for (uint16_t i = 0; i < step_count_v3; ++i)
                    {
                        p = reinterpret_cast<uint8_t*>(fill_factor + i);
                        cmd.data.push_back(p[0]);
                        cmd.data.push_back(p[1]);
                    }

                    _hw_monitor->send(cmd);
                }

                if (host_assistance != host_assistance_type::assistance_first_feed)
                {
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
                            LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__ << "occ get_py_rx_plus_fl_calib_result res size = " << res.size() ));

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
                            if (host_assistance != host_assistance_type::no_assistance)
                                if (count < 20) progress_callback->on_update_progress(static_cast<float>(80 + count++));
                            else
                                progress_callback->on_update_progress(count++* (2.f * static_cast<int>(speed))); //curently this number does not reflect the actual progress
                        }

                        now = std::chrono::high_resolution_clock::now();

                    } while (now - start < std::chrono::milliseconds(timeout_ms) && !done);

                    // If we exit due to timeout, report timeout
                    if (!done)
                    {
                        throw std::runtime_error("Operation timed-out!\n"
                            "Calibration state did not converge on time");
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
            }
        }

        return res;
    }

    std::vector<uint8_t> auto_calibrated::run_tare_calibration(int timeout_ms, float ground_truth_mm, std::string json, float* const health, update_progress_callback_ptr progress_callback)
    {
        int average_step_count = DEFAULT_AVERAGE_STEP_COUNT;
        int step_count = DEFAULT_STEP_COUNT;
        int accuracy = DEFAULT_ACCURACY;
        int speed = DEFAULT_SPEED;
        int scan_parameter = DEFAULT_SCAN;
        int data_sampling = DEFAULT_TARE_SAMPLING;
        int apply_preset = 1;
        int depth = 0;
        host_assistance_type host_assistance(host_assistance_type::no_assistance);
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
            int tmp_host_assistance(0);
            try_fetch(jsn, "host assistance", &tmp_host_assistance);
            if (tmp_host_assistance < (int)host_assistance_type::no_assistance || tmp_host_assistance >(int)host_assistance_type::assistance_second_feed)
                throw invalid_value_exception( rsutils::string::from()
                                               << "Auto calibration failed! Given value of 'host assistance' "
                                               << tmp_host_assistance << " is out of range (0 - "
                                               << (int)host_assistance_type::assistance_second_feed << ")." );
            host_assistance = host_assistance_type(tmp_host_assistance);
            try_fetch(jsn, "depth", &depth);
            try_fetch(jsn, "resize factor", &_resize_factor);
        }

        if (host_assistance != host_assistance_type::no_assistance && _interactive_state == interactive_calibration_state::RS2_OCC_STATE_NOT_ACTIVE)
        {
            _json = json;
            _ground_truth_mm = ground_truth_mm;
            _total_frames = step_count;
            _average_step_count = average_step_count;
            _action = auto_calib_action::RS2_OCC_ACTION_TARE_CALIB;
            _interactive_state = interactive_calibration_state::RS2_OCC_STATE_WAIT_TO_CAMERA_START;

            DirectSearchCalibrationResult result = get_calibration_status(timeout_ms, [progress_callback, host_assistance, speed](int count)
                {
                    if (progress_callback)
                    {
                        if (host_assistance != host_assistance_type::no_assistance)
                            if (count < 20) progress_callback->on_update_progress(static_cast<float>(80 + count++));
                            else
                                progress_callback->on_update_progress(count++ * (2.f * speed)); //curently this number does not reflect the actual progress
                    }
                }, false);
            // Handle errors from firmware
            rs2_dsc_status status = (rs2_dsc_status)result.status;

            if (result.maxDepth == 0)
            {
                throw std::runtime_error("Firmware calibration values are not yet set.");
            }
            _min_valid_depth = result.minDepth;
            _max_valid_depth = result.maxDepth;

            return res;
        }

        if (depth > 0)
        {
            LOG_OCC_WARN("run_tare_calibration interactive control (2) with parameters: depth = " << depth);
            _hw_monitor->send( command{ ds::AUTO_CALIB,
                                        interactive_scan_control,
                                        2,
                                        static_cast< uint32_t >( depth ) } );
        }
        else
        {
            std::shared_ptr<ds_advanced_mode_base> preset_recover;
            if (depth == 0)
            {
                if (apply_preset)
                {
                    if (host_assistance != host_assistance_type::no_assistance)
                        change_preset_and_stay();
                    else
                        preset_recover = change_preset();
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }

                LOG_DEBUG("run_tare_calibration with parameters: speed = " << speed << " average_step_count = " << average_step_count << " step_count = " << step_count << " accuracy = " << accuracy << " scan_parameter = " << scan_parameter << " data_sampling = " << data_sampling);
                check_tare_params(speed, scan_parameter, data_sampling, average_step_count, step_count, accuracy);

                auto param2 = static_cast< uint32_t >( ground_truth_mm ) * 100;

                tare_calibration_params param3{ static_cast< byte >( average_step_count ),
                                                static_cast< byte >( step_count ),
                                                static_cast< byte >( accuracy ),
                                                0 };

                param4 param{ static_cast< byte >( scan_parameter ),
                              0,
                              static_cast< byte >( data_sampling ) };

                if (host_assistance != host_assistance_type::no_assistance)
                    param.param_4 |= (1 << 8);

                // Log the current preset
                auto advanced_mode = dynamic_cast<ds_advanced_mode_base*>(this);
                if (advanced_mode)
                {
                    auto cur_preset = (rs2_rs400_visual_preset)(int)advanced_mode->_preset_opt->query();
                    LOG_DEBUG("run_tare_calibration with preset: " << rs2_rs400_visual_preset_to_string(cur_preset));
                }

                if (depth == 0)
                    _hw_monitor->send(command{ ds::AUTO_CALIB, tare_calib_begin, param2, param3.param3, param.param_4 });
            }

            if (host_assistance == host_assistance_type::no_assistance || depth < 0)
            {
                TareCalibrationResult result;

                // While not ready...
                int count = 0;
                bool done = false;

                auto start = std::chrono::high_resolution_clock::now();
                auto now = start;
                do
                {
                    memset(&result, 0, sizeof(TareCalibrationResult));
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));

                    // Check calibration status
                    try
                    {
                        res = _hw_monitor->send(command{ ds::AUTO_CALIB, tare_calib_check_status });
                        if (res.size() < sizeof(TareCalibrationResult))
                        {
                            if (depth < 0)
                                restore_preset();
                            throw std::runtime_error("Not enough data from CALIB_STATUS!");
                        }

                        result = *reinterpret_cast<TareCalibrationResult*>(res.data());
                        done = result.status != RS2_DSC_STATUS_RESULT_NOT_READY;
                    }
                    catch (const std::exception& ex)
                    {
                        LOG_INFO(ex.what());
                    }

                    if (progress_callback)
                    {
                        if (depth < 0 && count < 20)
                            progress_callback->on_update_progress(static_cast<float>(80 + count++));
                        else if (depth == 0)
                            progress_callback->on_update_progress(count++ * (2.f * speed)); //curently this number does not reflect the actual progress
                    }

                    now = std::chrono::high_resolution_clock::now();

                } while (now - start < std::chrono::milliseconds(timeout_ms) && !done);

                // If we exit due to timeout, report timeout
                if (!done)
                {
                    if (depth < 0)
                        restore_preset();

                    throw std::runtime_error("Operation timed-out!\n"
                        "Calibration did not converge on time");
                }

                auto status = (rs2_dsc_status)result.status;
                
                uint8_t* p = res.data() + sizeof(TareCalibrationResult) + 2 * result.iterations * sizeof(uint32_t);
                float* ph = reinterpret_cast<float*>(p);
                health[0] = ph[0];
                health[1] = ph[1];

                LOG_INFO("Ground truth: " << ground_truth_mm << "mm");
                LOG_INFO("Health check numbers from TareCalibrationResult(0x0C): before=" << ph[0] << ", after=" << ph[1]);
                LOG_INFO("Z calculated from health check numbers : before=" << (ph[0] + 1) * ground_truth_mm << ", after=" << (ph[1] + 1) * ground_truth_mm);

                // Handle errors from firmware
                if (status != RS2_DSC_STATUS_SUCCESS)
                    handle_calibration_error(status);

                res = get_calibration_results();

                if (depth < 0)
                {
                    restore_preset();
                    std::this_thread::sleep_for(std::chrono::milliseconds(200));
                }
                if (progress_callback)
                    progress_callback->on_update_progress(static_cast<float>(100));
            }
        }

        return res;
    }

    uint16_t auto_calibrated::calc_fill_rate(const rs2_frame* f)
    {
        auto frame = ((video_frame*)f);
        int width = frame->get_width();
        int height = frame->get_height();
        int roi_w = width / _resize_factor;
        int roi_h = height / _resize_factor;
        int roi_start_w = (width - roi_w) / 2;
        int roi_start_h = (height - roi_h) / 2;
        int from = roi_start_h;
        int to = roi_start_h + roi_h;
        int roi_size = roi_w * roi_h;
        int data_size = roi_size;
        const uint16_t* p = reinterpret_cast<const uint16_t*>(frame->get_frame_data());

#ifdef SAVE_RAW_IMAGE
        std::vector<uint16_t> cropped_image(width * height, 0);
        int cropped_idx(0);
        cropped_idx += from * width + roi_start_w;
#endif

        p += from * width + roi_start_w;

        int counter(0);
        for (int j = from; j < to; ++j)
        {
            for (int i = 0; i < roi_w; ++i)
            {
#ifdef SAVE_RAW_IMAGE
                cropped_image[cropped_idx] = (*p);
                cropped_idx++;
#endif
                if ((*p) >= _min_valid_depth && (*p) <= _max_valid_depth)
                    ++counter;
                ++p;
            }
            p += (width - roi_w);
#ifdef SAVE_RAW_IMAGE
            cropped_idx += (width - roi_w);
#endif
        }
#ifdef SAVE_RAW_IMAGE
        {
            unsigned long milliseconds_since_epoch =
                std::chrono::duration_cast<std::chrono::milliseconds>
                (std::chrono::system_clock::now().time_since_epoch()).count();

            std::stringstream name_s;
            name_s << "cropped_image_" << std::setfill('0') << std::setw(4) << milliseconds_since_epoch << "_" << frame->get_frame_number() << ".raw";
            std::ofstream fout(name_s.str(), std::ios::out | std::ios::binary);
            fout.write((char*)&cropped_image[0], cropped_image.size() * sizeof(uint16_t));
            fout.close();
        }
#endif
        double tmp = static_cast<double>(counter) / static_cast<double>(data_size) * 10000.0;
        return static_cast<uint16_t>(tmp + 0.5f);

    }

    // fill_missing_data:
    // Fill every zeros section linearly based on the section's edges.
    void auto_calibrated::fill_missing_data(uint16_t data[256], int size)
    {
        int counter = 0;
        int start = 0;
        while (data[start++] == 0)
            ++counter;

        if (start + 2 > size)
            throw std::runtime_error( rsutils::string::from() << "There is no enought valid data in the array!" );

        for (int i = 0; i < counter; ++i)
            data[i] = data[counter];

        start = 0;
        int end = 0;
        float tmp = 0;
        for (int i = 0; i < size; ++i)
        {
            if (data[i] == 0)
                start = i;

            if (start != 0 && data[i] != 0)
                end = i;

            if (start != 0 && end != 0)
            {
                tmp = static_cast<float>(data[end] - data[start - 1]);
                tmp /= end - start + 1;
                for (int j = start; j < end; ++j)
                    data[j] = static_cast<uint16_t>(tmp * (j - start + 1) + data[start - 1] + 0.5f);
                start = 0;
                end = 0;
            }
        }

        if (start != 0 && end == 0)
        {
            for (int i = start; i < size; ++i)
                data[i] = data[start - 1];
        }
    }

    void auto_calibrated::remove_outliers(uint16_t data[256], int size)
    {
        //Due to the async between the preset activation and the flow, the initial frames of the sample may include unrelated data.
        // the purpose of the function is to eliminate those by replacing them with a single value. 
        // This assumes that the fill_rate is contiguous during scan (i.e. grows or shrinks monotonically)
        // Additionally, this function rectifies singular sporadic outliers which are in the top 5% that may skew the results
        uint16_t base_fr = 0;
        for (int i = 255; i >= 0; i--)
        {
            // Initialize reference value
            if (!base_fr)
            {
                if (data[i])
                    base_fr = data[i];
                continue;
            }

            // Rectify missing values
            if (!data[i])
                data[i] = base_fr;
        }

        static const int _outlier_percentile = 9500; // The outlier value is expected to be significantly above this value
        for (int i = 0; i <= 253; i++) // Check for single outliers by assessing triples
        {
            auto val1 = data[i];
            auto val2 = data[i+1];
            auto val3 = data[i+2];

            // Check for rectification candidate
            if ((val2 > val1) && (val2 > val3))
            {
                auto diff = val3-val1;
                auto delta = std::max(std::abs(val2-val1),std::abs(val2-val3));
                // Actual outlier is a
                // - spike 3 times or more than the expected gap
                // - in the 5 top percentile
                // - with neighbour values being smaller by at least 500 points to avoid clamping around the peak
                if ((delta > 500) && (delta > (std::abs(diff) * 3)) && (val2 > _outlier_percentile))
                {
                    data[i+1] = data[i] + diff/2;
                    LOG_OCC_WARN(std::string(rsutils::string::from() << "Outlier with value " << val2 << " was changed to be " << data[i+1] ));
                }
            }
        }
    }

    // get_depth_frame_sum:
    // Function sums the pixels in the image ROI - Add the collected avarage parameters to _collected_counter, _collected_sum. 
    void auto_calibrated::collect_depth_frame_sum(const rs2_frame* f)
    {
        auto frame = ((video_frame*)f);
        int width = frame->get_width();
        int height = frame->get_height();
        int roi_w = width / _resize_factor;
        int roi_h = height / _resize_factor;
        int roi_start_w = (width - roi_w) / 2;
        int roi_start_h = (height - roi_h) / 2;

        const uint16_t* p = reinterpret_cast<const uint16_t*>(frame->get_frame_data());

#ifdef SAVE_RAW_IMAGE
        std::vector<uint16_t> origin_image(width * height, 0);
        for (int ii = 0; ii < width * height; ii++)
            origin_image[ii] = *(p + ii);

        {
            unsigned long milliseconds_since_epoch =
                std::chrono::duration_cast<std::chrono::milliseconds>
                (std::chrono::system_clock::now().time_since_epoch()).count();

            std::stringstream name_s;
            name_s << "origin_tare_image_" << std::setfill('0') << std::setw(4) << milliseconds_since_epoch << "_" << frame->get_frame_number() << ".raw";

            std::ofstream fout(name_s.str(), std::ios::out | std::ios::binary);
            fout.write((char*)&origin_image[0], origin_image.size() * sizeof(uint16_t));
            fout.close();
        }
        std::vector<uint16_t> cropped_image(width * height, 0);
        int cropped_idx(0);
        cropped_idx += roi_start_h * width + roi_start_w;
#endif

        p += roi_start_h * width + roi_start_w;

        for (int j = 0; j < roi_h; ++j)
        {
            for (int i = 0; i < roi_w; ++i)
            {
#ifdef SAVE_RAW_IMAGE
                cropped_image[cropped_idx] = (*p);
                cropped_idx++;
#endif
                if ((*p) >= _min_valid_depth && (*p) <= _max_valid_depth)
                {
                    ++_collected_counter;
                    _collected_sum += *p;
                }
                ++p;
            }
            p += (width- roi_w);
#ifdef SAVE_RAW_IMAGE
            cropped_idx += (width - roi_w);
#endif
        }
#ifdef SAVE_RAW_IMAGE
        {
            unsigned long milliseconds_since_epoch =
                std::chrono::duration_cast<std::chrono::milliseconds>
                (std::chrono::system_clock::now().time_since_epoch()).count();

            std::stringstream name_s;
            name_s << "cropped_tare_image_" << std::setfill('0') << std::setw(4) << milliseconds_since_epoch << "_" << frame->get_frame_number() << ".raw";

            std::ofstream fout(name_s.str(), std::ios::out | std::ios::binary);
            fout.write((char*)&cropped_image[0], cropped_image.size() * sizeof(uint16_t));
            fout.close();
        }
#endif
    }

    std::vector<uint8_t> auto_calibrated::process_calibration_frame(int timeout_ms, const rs2_frame* f, float* const health, update_progress_callback_ptr progress_callback)
    {
        try
        {
            std::vector<uint8_t> res;
            rs2_metadata_type frame_counter = ((frame_interface*)f)->get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
            rs2_metadata_type frame_ts = ((frame_interface*)f)->get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP);
            bool tare_fc_workaround = (_action == auto_calib_action::RS2_OCC_ACTION_TARE_CALIB); //Tare calib shall use rolling frame counter
            bool mipi_sku = ((frame_interface*)f)->supports_frame_metadata(RS2_FRAME_METADATA_CALIB_INFO);
            if (mipi_sku)
                frame_counter = ((frame_interface*)f)->get_frame_metadata(RS2_FRAME_METADATA_CALIB_INFO);

            if (_interactive_state == interactive_calibration_state::RS2_OCC_STATE_WAIT_TO_CAMERA_START)
            {
                if (frame_counter <= 2)
                {
                    return res;
                }
                if (progress_callback)
                {
                    progress_callback->on_update_progress(static_cast<float>(10));
                }
                _interactive_state = interactive_calibration_state::RS2_OCC_STATE_INITIAL_FW_CALL;
            }
            if (_interactive_state == interactive_calibration_state::RS2_OCC_STATE_INITIAL_FW_CALL)
            {
                if (_action == auto_calib_action::RS2_OCC_ACTION_TARE_CALIB)
                {
                    res = run_tare_calibration(timeout_ms, _ground_truth_mm, _json, health, progress_callback);
                }
                else if (_action == auto_calib_action::RS2_OCC_ACTION_ON_CHIP_CALIB)
                {
                    res = run_on_chip_calibration(timeout_ms, _json, health, progress_callback);
                }
                _prev_frame_counter = frame_counter;
                if ((!tare_fc_workaround) && mipi_sku) _prev_frame_counter +=10; //  Compensate for MIPI OCC calib. invoke delay
                _interactive_state = interactive_calibration_state::RS2_OCC_STATE_WAIT_TO_CALIB_START;
                LOG_OCC_WARN(std::string(rsutils::string::from() << "switch INITIAL_FW_CALL=>WAIT_TO_CALIB_START, prev_fc is reset to " << _prev_frame_counter));
                return res;
            }
            if (_interactive_state == interactive_calibration_state::RS2_OCC_STATE_WAIT_TO_CALIB_START)
            {
                bool still_waiting(frame_counter >= _prev_frame_counter || frame_counter >= _total_frames);
                _prev_frame_counter = frame_counter;
                if (still_waiting)
                {
                    if (progress_callback)
                    {
                        progress_callback->on_update_progress(static_cast<float>(15));
                    }
                    return res;
                }
                if (progress_callback)
                {
                    progress_callback->on_update_progress(static_cast<float>(20));
                }
                _collected_counter = 0;
                _collected_sum = 0;
                _collected_frame_num = 0;
                _skipped_frames = 0;
                _prev_frame_counter = -1;
                _interactive_state = interactive_calibration_state::RS2_OCC_STATE_DATA_COLLECT;
                LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__ << " switch to RS2_OCC_STATE_DATA_COLLECT"));
            }
            if (_interactive_state == interactive_calibration_state::RS2_OCC_STATE_DATA_COLLECT)
            {
                if (_action == auto_calib_action::RS2_OCC_ACTION_ON_CHIP_CALIB)
                {
#ifdef SAVE_RAW_IMAGE
                    {
                        unsigned long milliseconds_since_epoch =
                            std::chrono::duration_cast<std::chrono::milliseconds>
                            (std::chrono::system_clock::now().time_since_epoch()).count();

                        std::stringstream name_s;
                        name_s << "all_frame_numbers.txt";
                        std::ofstream fout(name_s.str(), std::ios::app);
                        fout << frame_counter << ", " << _prev_frame_counter << ", " << _total_frames << ", " 
                             << ((frame_interface*)f)->get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP) << ", " << milliseconds_since_epoch << std::endl;
                        fout.close();
                    }
#endif
#ifdef SAVE_RAW_IMAGE
                    {
                        auto frame = ((video_frame*)f);
                        int width = frame->get_width();
                        int height = frame->get_height();
                        const uint16_t* p = reinterpret_cast<const uint16_t*>(frame->get_frame_data());
                        {
                            unsigned long milliseconds_since_epoch =
                                std::chrono::duration_cast<std::chrono::milliseconds>
                                (std::chrono::system_clock::now().time_since_epoch()).count();

                            std::stringstream name_s;
                            name_s << "origin_image_" << std::setfill('0') << std::setw(4) << milliseconds_since_epoch << "_" << frame->get_frame_number() << ".raw";

                            std::ofstream fout(name_s.str(), std::ios::out | std::ios::binary);
                            fout.write((char*)p, width * height * sizeof(uint16_t));
                            fout.close();
                        }
                    }
#endif

                    static const int FRAMES_TO_SKIP(_interactive_scan ? 1 : 0);
                    int fw_host_offset = (_interactive_scan ? 0 : 1);

                    if (frame_counter + fw_host_offset < _total_frames)
                    {
                        if (frame_counter < _prev_frame_counter)
                        {
                            throw std::runtime_error("Frames arrived in a wrong order!");
                        }
                        if (_skipped_frames < FRAMES_TO_SKIP || frame_counter == _prev_frame_counter)
                        {
                            _skipped_frames++;
                        }
                        else
                        {
                            if (progress_callback)
                            {
                                progress_callback->on_update_progress(static_cast<float>(20 + static_cast<int>(frame_counter * 60.0 / _total_frames)));
                            }
                            auto fill_rate  = calc_fill_rate(f);
                            if (frame_counter < 10) // handle discrepancy on stream/preset activation
                                fill_rate = 0;
                            if (frame_counter + fw_host_offset < 256)
                            {
                                _fill_factor[frame_counter + fw_host_offset] = fill_rate;
                                LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__ << " fc = " << frame_counter <<  ", _fill_factor[" << frame_counter + fw_host_offset << "] = " << fill_rate));
                            }

                            if (_interactive_scan)
                            {
                                auto res = _hw_monitor->send(command{ ds::AUTO_CALIB, interactive_scan_control, 1});
                                LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__ << " occ interactive_scan_control 1,"));
                            }
                            _skipped_frames = 0;
                            _prev_frame_counter = frame_counter;
                        }
                    }
                    else
                    {
                        _interactive_state = interactive_calibration_state::RS2_OCC_STATE_WAIT_FOR_FINAL_FW_CALL;
                        LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__ << " go to final FW call"));
                    }
                }
                else if (_action == auto_calib_action::RS2_OCC_ACTION_TARE_CALIB)
                {
                    static const int FRAMES_TO_SKIP(1);
                    if (frame_counter != _prev_frame_counter)
                    {
                        _collected_counter = 0;
                        _collected_sum = 0.0;
                        _collected_frame_num = 0;
                        _skipped_frames = 0;
                        if (progress_callback)
                        {
                            double progress_rate = std::min(1.0, static_cast<double>(frame_counter) / _total_frames);
                            progress_callback->on_update_progress(static_cast<float>(20 + static_cast<int>(progress_rate * 60.0)));
                        }
                    }
                    LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__ << " fr_c = " << frame_counter
                        << " fr_ts = " << frame_ts << " _c_f_num = " << _collected_frame_num));

                    if (frame_counter < _total_frames)
                    {
                        if (_skipped_frames < FRAMES_TO_SKIP)
                        {
                            _skipped_frames++;
                        }
                        else
                        {
                            if (_collected_frame_num < _average_step_count)
                            {
                                collect_depth_frame_sum(f);
                                if (_collected_counter && (_collected_frame_num + 1) == _average_step_count)
                                {
                                    _collected_sum = (_collected_sum / _collected_counter) * 10000;
                                    int depth = static_cast<int>(_collected_sum + 0.5);

                                    std::stringstream ss;
                                    ss << "{\n \"depth\":" << depth << "}";

                                    std::string json = ss.str();
                                    run_tare_calibration(timeout_ms, _ground_truth_mm, json, health, progress_callback);
                                }
                            }
                            ++_collected_frame_num;
                        }
                        _prev_frame_counter = frame_counter;
                    }
                    else
                    {
                        _interactive_state = interactive_calibration_state::RS2_OCC_STATE_WAIT_FOR_FINAL_FW_CALL;
                    }
                }
            }
            if (_interactive_state == interactive_calibration_state::RS2_OCC_STATE_WAIT_FOR_FINAL_FW_CALL)
            {
                LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__ << " :RS2_OCC_STATE_WAIT_FOR_FINAL_FW_CALL"));
                if (frame_counter > _total_frames)
                {
                    _interactive_state = interactive_calibration_state::RS2_OCC_STATE_FINAL_FW_CALL;
                }
                else if (_interactive_scan)
                {
                    _hw_monitor->send(command{ ds::AUTO_CALIB, interactive_scan_control, 1 });
                    LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__ << "Call for interactive_scan_control, 1"));
                }
            }
            if (_interactive_state == interactive_calibration_state::RS2_OCC_STATE_FINAL_FW_CALL)
            {
                LOG_OCC_WARN(std::string(rsutils::string::from() << __LINE__ << " :RS2_OCC_STATE_FINAL_FW_CALL"));
                if (progress_callback)
                {
                    progress_callback->on_update_progress(static_cast<float>(80));
                }
                if (_action == auto_calib_action::RS2_OCC_ACTION_ON_CHIP_CALIB)
                {
#ifdef SAVE_RAW_IMAGE
                    {
                        std::stringstream ss;
                        ss << "{\n \"calib type\":" << 0 <<
                            ",\n \"host assistance\":" << 2 <<
                            ",\n \"step count v3\":" << _total_frames;
                        for (int i = 0; i < _total_frames; ++i)
                            ss << ",\n \"fill factor " << i << "\":" << _fill_factor[i];
                        ss << "}";

                        std::stringstream name_s;
                        name_s << "fill_factor_before_fill.txt";
                        std::ofstream fout(name_s.str(), std::ios::out);
                        fout << ss.str();
                        fout.close();
                    }
#endif
                    // Do not delete - to be used to improve algo robustness
                    fill_missing_data(_fill_factor, _total_frames);
                    remove_outliers(_fill_factor, _total_frames);
                    std::stringstream ss;
                    ss << "{\n \"calib type\":" << 0 <<
                        ",\n \"host assistance\":" << 2 <<
                        ",\n \"step count v3\":" << _total_frames;
                    for (int i = 0; i < _total_frames; ++i)
                        ss << ",\n \"fill factor " << i << "\":" << _fill_factor[i];
                    ss << "}";

                    std::string json = ss.str();
#ifdef SAVE_RAW_IMAGE
                    {
                        std::stringstream name_s;
                        name_s << "fill_factor.txt";
                        std::ofstream fout(name_s.str(), std::ios::out);
                        fout << json;
                        fout.close();
                    }
#endif

                    res = run_on_chip_calibration(timeout_ms, json, health, progress_callback);
                }
                else if (_action == auto_calib_action::RS2_OCC_ACTION_TARE_CALIB)
                {
                    std::stringstream ss;
                    ss << "{\n \"depth\":" << -1 << "}";

                    std::string json = ss.str();
                    res = run_tare_calibration(timeout_ms, _ground_truth_mm, json, health, progress_callback);
                }
                if (progress_callback)
                {
                    progress_callback->on_update_progress(static_cast<float>(100));
                }
                _interactive_state = interactive_calibration_state::RS2_OCC_STATE_NOT_ACTIVE;
            }
            return res;
        }
        catch (const std::exception&)
        {
            _interactive_state = interactive_calibration_state::RS2_OCC_STATE_NOT_ACTIVE;
            throw;
        }
    }


    std::shared_ptr<ds_advanced_mode_base> auto_calibrated::change_preset()
    {
        preset old_preset_values;
        rs2_rs400_visual_preset old_preset;

        auto advanced_mode = dynamic_cast<ds_advanced_mode_base*>(this);
        if (advanced_mode)
        {
            old_preset = (rs2_rs400_visual_preset)(int)advanced_mode->_preset_opt->query();
            if (old_preset == RS2_RS400_VISUAL_PRESET_CUSTOM)
                old_preset_values = advanced_mode->get_all();
            advanced_mode->_preset_opt->set(RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY);
        }

        std::shared_ptr<ds_advanced_mode_base> recover_preset(advanced_mode, [old_preset, advanced_mode, old_preset_values](ds_advanced_mode_base* adv)
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

    void auto_calibrated::change_preset_and_stay()
    {
        auto advanced_mode = dynamic_cast<ds_advanced_mode_base*>(this);
        if (advanced_mode)
        {
            _old_preset = (rs2_rs400_visual_preset)(int)advanced_mode->_preset_opt->query();
            if (_old_preset == RS2_RS400_VISUAL_PRESET_CUSTOM)
                _old_preset_values = advanced_mode->get_all();
            advanced_mode->_preset_opt->set(RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY);
            _preset_change = true;
        }
    }

    void auto_calibrated::restore_preset()
    {
        if (_preset_change)
        {
            auto advanced_mode = dynamic_cast<ds_advanced_mode_base*>(this);
            if (_old_preset == RS2_RS400_VISUAL_PRESET_CUSTOM)
            {
                advanced_mode->_preset_opt->set(RS2_RS400_VISUAL_PRESET_CUSTOM);
                advanced_mode->set_all(_old_preset_values);
            }
            else
                advanced_mode->_preset_opt->set(static_cast<float>(_old_preset));
        }
        _preset_change = false;
    }

    void auto_calibrated::check_params(int speed, int scan_parameter, int data_sampling) const
    {
        if (speed < speed_very_fast || speed >  speed_white_wall)
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'speed' " << speed
                                           << " is out of range (0 - 4)." );
       if (scan_parameter != py_scan && scan_parameter != rx_scan)
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'scan parameter' "
                                           << scan_parameter << " is out of range (0 - 1)." );
        if (data_sampling != polling && data_sampling != interrupt)
           throw invalid_value_exception( rsutils::string::from()
                                          << "Auto calibration failed! Given value of 'data sampling' " << data_sampling
                                          << " is out of range (0 - 1)." );
    }

    void auto_calibrated::check_tare_params(int speed, int scan_parameter, int data_sampling, int average_step_count, int step_count, int accuracy)
    {
        check_params(speed, scan_parameter, data_sampling);

        if (average_step_count < 1 || average_step_count > 30)
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'number of frames to average' "
                                           << average_step_count << " is out of range (1 - 30)." );
        if (step_count < 5 || step_count > 30)
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'max iteration steps' "
                                           << step_count << " is out of range (5 - 30)." );
        if (accuracy < very_high || accuracy > low)
            throw invalid_value_exception( rsutils::string::from()
                                           << "Auto calibration failed! Given value of 'subpixel accuracy' " << accuracy
                                           << " is out of range (0 - 3)." );
    }

    void auto_calibrated::check_focal_length_params(int step_count, int fy_scan_range, int keep_new_value_after_sucessful_scan, int interrrupt_data_samling, int adjust_both_sides, int fl_scan_location, int fy_scan_direction, int white_wall_mode) const
    {
        if (step_count < 8 || step_count >  256)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'step_count' " << step_count << " is out of range (8 - 256).");
        if (fy_scan_range < 1 || fy_scan_range >  60000)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'fy_scan_range' " << fy_scan_range << " is out of range (1 - 60000).");
        if (keep_new_value_after_sucessful_scan < 0 || keep_new_value_after_sucessful_scan >  1)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'keep_new_value_after_sucessful_scan' " << keep_new_value_after_sucessful_scan << " is out of range (0 - 1).");
        if (interrrupt_data_samling < 0 || interrrupt_data_samling >  1)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'interrrupt_data_samling' " << interrrupt_data_samling << " is out of range (0 - 1).");
        if (adjust_both_sides < 0 || adjust_both_sides >  1)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'adjust_both_sides' " << adjust_both_sides << " is out of range (0 - 1).");
        if (fl_scan_location < 0 || fl_scan_location >  1)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'fl_scan_location' " << fl_scan_location << " is out of range (0 - 1).");
        if (fy_scan_direction < 0 || fy_scan_direction >  1)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'fy_scan_direction' " << fy_scan_direction << " is out of range (0 - 1).");
        if (white_wall_mode < 0 || white_wall_mode >  1)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'white_wall_mode' " << white_wall_mode << " is out of range (0 - 1).");
    }

    void auto_calibrated::check_one_button_params(int speed, int keep_new_value_after_sucessful_scan, int data_sampling, int adjust_both_sides, int fl_scan_location, int fy_scan_direction, int white_wall_mode) const
    {
        if (speed < speed_very_fast || speed >  speed_white_wall)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'speed' " << speed << " is out of range (0 - 4).");
        if (keep_new_value_after_sucessful_scan < 0 || keep_new_value_after_sucessful_scan >  1)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'keep_new_value_after_sucessful_scan' " << keep_new_value_after_sucessful_scan << " is out of range (0 - 1).");
        if (data_sampling != polling && data_sampling != interrupt)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'data sampling' " << data_sampling << " is out of range (0 - 1).");
        if (adjust_both_sides < 0 || adjust_both_sides >  1)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'adjust_both_sides' " << adjust_both_sides << " is out of range (0 - 1).");
        if (fl_scan_location < 0 || fl_scan_location >  1)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'fl_scan_location' " << fl_scan_location << " is out of range (0 - 1).");
        if (fy_scan_direction < 0 || fy_scan_direction >  1)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'fy_scan_direction' " << fy_scan_direction << " is out of range (0 - 1).");
        if (white_wall_mode < 0 || white_wall_mode >  1)
            throw invalid_value_exception( rsutils::string::from() << "Auto calibration failed! Given value of 'white_wall_mode' " << white_wall_mode << " is out of range (0 - 1).");
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
        else
            throw std::runtime_error( rsutils::string::from()
                                      << "Calibration didn't converge! (RESULT=" << int( status ) << ")" );
    }

    std::vector<uint8_t> auto_calibrated::get_calibration_results(float* const health) const
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

    std::vector<uint8_t> auto_calibrated::get_PyRxFL_calibration_results(float* const health, float* health_fl) const
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
        command cmd(ds::GETINTCAL, static_cast<int>(ds::d400_calibration_table_id::coefficients_table_id));
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
        d400_calibration_table_id  tbl_id = static_cast<d400_calibration_table_id>(hd->table_type);
        fw_cmd cmd{};
        uint32_t param2 = 0;
        switch (tbl_id)
        {
        case d400_calibration_table_id::coefficients_table_id:
            cmd = SETINTCAL;
            break;
        case d400_calibration_table_id::rgb_calibration_id:
            cmd = SETINTCALNEW;
            param2 = 1;
            break;
        default:
            throw std::runtime_error( rsutils::string::from() << "Flashing calibration table type 0x" << std::hex
                                                              << static_cast<int>(tbl_id) << " is not supported" );
        }

        command write_calib(cmd, static_cast<int>(tbl_id), param2);
        write_calib.data = _curr_calibration;
        _hw_monitor->send(write_calib);

        LOG_DEBUG("Flashing " << ((tbl_id == d400_calibration_table_id::coefficients_table_id) ? "Depth" : "RGB") << " calibration table");

    }

    void auto_calibrated::set_calibration_table(const std::vector<uint8_t>& calibration)
    {
        using namespace ds;

        table_header* hd = (table_header*)(calibration.data());
        ds::d400_calibration_table_id  tbl_id = static_cast<ds::d400_calibration_table_id >(hd->table_type);

        switch (tbl_id)
        {
            case d400_calibration_table_id::coefficients_table_id: // Load the modified depth calib table into flash RAM
            {
                uint8_t* table = (uint8_t*)(calibration.data() + sizeof(table_header));
                command write_calib(ds::CALIBRECALC, 0, 0, 0, 0xcafecafe);
                write_calib.data.insert(write_calib.data.end(), (uint8_t*)table, ((uint8_t*)table) + hd->table_size);
                try
                {
                    _hw_monitor->send(write_calib);
                }
                catch(...)
                {
                    LOG_ERROR("Flashing coefficients_table_id failed");
                }
            }
            case d400_calibration_table_id::rgb_calibration_id: // case fall-through by design. For RGB skip loading to RAM (not supported)
                _curr_calibration = calibration;
                break;
            default:
                throw std::runtime_error( rsutils::string::from()
                                          << "the operation is not defined for calibration table type " << static_cast<int>(tbl_id));
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
        auto table = (librealsense::ds::d400_coefficients_table*)calib_table.data();

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
            if (counter > 0)
            {
                left_z_tl[i] /= counter;
                left_z_tr[i] /= counter;
                left_z_bl[i] /= counter;
                left_z_br[i] /= counter;
            }
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
        float* const health, int health_size, update_progress_callback_ptr progress_callback)
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
            ret = _hw_monitor->send(command{ds::GETINTCAL, static_cast<int>(ds::d400_calibration_table_id::rgb_calibration_id) });
            auto table = reinterpret_cast<librealsense::ds::d400_rgb_calibration_table*>(ret.data());

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
                throw std::runtime_error( rsutils::string::from() << "Target distance calculation requires at least "
                                                                  << min_frames_required << " frames, aborting" );
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

    void auto_calibrated::set_hw_monitor_for_auto_calib(std::shared_ptr<hw_monitor> hwm)
    {
        _hw_monitor = hwm;
    }
}
