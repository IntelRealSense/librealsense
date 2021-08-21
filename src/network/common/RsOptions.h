// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include <librealsense2/hpp/rs_internal.hpp>
#include <librealsense2/rs.hpp>

#include "RsNetSensor.h"

#define RS_MAX_SENSORS_NUMBER 16

typedef struct {
    uint32_t index;
    float    value;

    struct rs2::option_range range;

    bool     ro;
} RsOption;

class RsSensorOptions {
public:
    RsSensorOptions() {};
   ~RsSensorOptions() {};

    std::string get_name() { return m_sensor_name; };

private:
    void init() {
        memset((void*)m_sensor_options, 0xFF, sizeof(RsOption) * RS2_OPTION_COUNT);

        // Prepare options list
        for (int32_t i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++) {
            rs2_option option_type = static_cast<rs2_option>(i);

            try {
                // Get the current value of the option
                m_sensor_options[i].value = m_sensor.get_option(option_type);
                m_sensor_options[i].range = m_sensor.get_option_range(option_type);
                m_sensor_options[i].ro    = m_sensor.is_option_read_only(option_type);

                m_sensor_options[i].index = i;

                LOG_DEBUG("Scanned sensor " << m_sensor_name << " for option #" << std::dec << m_sensor_options[i].index << " (" << (rs2_option)m_sensor_options[i].index << ")");
            } catch(...) {
                // LOG_WARNING("Scan failed for option " << " #" << std::dec << m_sensor_options[i].index << "(" << (rs2_option)m_sensor_options[i].index << ")");
            }
        }

        m_changed = true;
    };

public:
    // server
    void init(rs2::sensor sensor) {
        m_server = true;
        m_sensor = sensor;
        m_sensor_name = sensor.supports(RS2_CAMERA_INFO_NAME) ? sensor.get_info(RS2_CAMERA_INFO_NAME) : "Unknown";

        init();
    };

    // client
    void init(NetSensor sensor) {
        m_server = false;
        m_sensor = *(sensor->get_sensor().get());
        m_sensor_name = sensor->get_name();

        init();
    };

    bool scan() {
        m_changed = false;

        for (int32_t i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++) {
            rs2_option option_type = static_cast<rs2_option>(i);
            try {
                // Get the current value of the RO option
                if (m_sensor_options[i].ro == m_server) {
                    m_sensor_options[i].value = m_sensor.get_option(option_type);

                    if (m_sensor_options_prev[i].value != m_sensor_options[i].value) {
                        m_changed = true;
                        LOG_DEBUG("Server option change " << std::setw(30) << (rs2_option)i << " #" << std::dec << i << " to " << m_sensor_options[i].value << ", old value is " << m_sensor_options_prev[i].value);
                        m_sensor_options_prev[i].value = m_sensor_options[i].value;
                    }
                }
            } catch(...) {
                LOG_DEBUG("Get option failed for " << std::setw(30) << (rs2_option)i << " #" << std::dec << i << " to " << m_sensor_options[i].value << ", old value is " << m_sensor_options_prev[i].value);
            }
        }

        return m_changed;
    };

    RsOption& operator[](int i) {
        return m_sensor_options[i];
    };

private:
    rs2::sensor m_sensor;
    std::string m_sensor_name;
    RsOption    m_sensor_options[RS2_OPTION_COUNT];
    RsOption    m_sensor_options_prev[RS2_OPTION_COUNT];
    std::mutex  m_mutex;
    bool        m_changed;
    bool        m_server;
};

class RsOptions {
public:
    RsOptions() {};
    virtual ~RsOptions() {};

public:
    bool changed() { 
        bool changed = false;

        // scan function
        for (int s = 0; s < RS_MAX_SENSORS_NUMBER; ++s) {
            changed |= m_sensor_options[s].scan();
        }

        return changed;
    };

    std::string get_opt_str() {
        std::stringstream options;

        for (int s = 0; s < RS_MAX_SENSORS_NUMBER; ++s) {
            try {
                m_sensor_options[s].scan();

                std::string sensor_name = m_sensor_options[s].get_name();
                if (!sensor_name.empty()) {
                    options << sensor_name;

                    for (int32_t i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++) {
                        if (~m_sensor_options[s][i].index) {
                            options << "|";
                            options << i << ",";

                            options << m_sensor_options[s][i].value << ",";

                            options << m_sensor_options[s][i].range.min << ",";
                            options << m_sensor_options[s][i].range.max << ",";
                            options << m_sensor_options[s][i].range.def << ",";
                            options << m_sensor_options[s][i].range.step << ',';

                            if (m_sensor_options[s][i].ro) options << '1';
                            else options << '0';
                        }
                    }
                    options << "\r\n";
                }
            } catch(...) {};
        }

        return options.str();
    };

    void set_opt_str(std::string options) {
        // Parse the request, it is in form
        // <sensor_name>|<opt1_index>,<opt1_value>,<opt1_value>,<opt1_min>,<opt1_max>,<opt1_default>,<opt1_step>|...|<optN_index>,<optN_value>,<optN_value>,<optN_min>,<optN_max>,<optN_default>,<optN_step>
        while (!options.empty()) {
            // get the sensor line
            uint32_t line_pos = options.find("\r\n");
            std::string sensor = options.substr(0, line_pos) + "|";
            options.erase(0, line_pos + 2);

            // get the sensor name
            uint32_t pos = sensor.find("|");
            std::string sensor_name = sensor.substr(0, pos);
            sensor.erase(0, pos + 1);

            // find the sensor
            int sensor_index = 0;
            for (; sensor_index < RS_MAX_SENSORS_NUMBER; ++sensor_index) {
                std::string name = m_sensor_options[sensor_index].get_name();
                if (!name.empty()) {
                    if (name.compare(sensor_name) == 0) break;
                } else throw std::runtime_error("Error in server response: wrong sensor.");
            }

            while (!sensor.empty()) {
                RsOption option;

                pos = sensor.find(",");
                option.index = std::stoul(sensor.substr(0, pos).c_str());
                sensor.erase(0, pos + 1);
                
                pos = sensor.find("|");
                std::string vals = sensor.substr(0, pos + 1);
                sensor.erase(0, pos + 1);
                
                pos = vals.find(",");
                option.value = std::stof(vals.substr(0, pos).c_str());
                vals.erase(0, pos + 1);

                pos = vals.find(",");
                option.range.min = std::stof(vals.substr(0, pos).c_str());
                vals.erase(0, pos + 1);

                pos = vals.find(",");
                option.range.max = std::stof(vals.substr(0, pos).c_str());
                vals.erase(0, pos + 1);

                pos = vals.find(",");
                option.range.def = std::stof(vals.substr(0, pos).c_str());
                vals.erase(0, pos + 1);

                pos = vals.find(",");
                option.range.step = std::stof(vals.substr(0, pos).c_str());
                vals.erase(0, pos + 1);

                pos = vals.find("|");
                if (std::stoi(vals.substr(0, pos).c_str())) {
                    option.ro = true;
                } else {
                    option.ro = false;
                }
                vals.erase(0, pos + 1);

                // update and set if changed
                if (m_sensor_options[sensor_index][option.index].value != option.value) {
                    m_sensor_options[sensor_index][option.index] = option;

                    set_opt(sensor_name, option);
                }
            }
        }
    };

private:
    virtual void set_opt(std::string sensor_name, RsOption option) {
        LOG_DEBUG("Option change for " << sensor_name << " #" << std::dec << option.index << " (" << (rs2_option)option.index << ") to " << option.value);
    };
    
protected:
    RsSensorOptions m_sensor_options[RS_MAX_SENSORS_NUMBER];
};

