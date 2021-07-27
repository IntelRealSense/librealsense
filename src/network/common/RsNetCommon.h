// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include <librealsense2/hpp/rs_internal.hpp>
#include <librealsense2/rs.hpp>

#define RS2_NET_MAJOR_VERSION    2
#define RS2_NET_MINOR_VERSION    0
#define RS2_NET_PATCH_VERSION    0

#define COMPRESSION_ENABLED
#define COMPRESSION_ZSTD

#define CHUNK_SIZE (1024*2)
#pragma pack (push, 1)
typedef struct chunk_header{
    uint16_t size;
    uint32_t offset;
    uint32_t index;
    uint8_t  status;
    uint8_t  meta_id;
    uint64_t meta_data;
} chunk_header_t;
#pragma pack(pop)
#define CHUNK_HLEN (sizeof(chunk_header_t))

using StreamIndex     = std::pair<rs2_stream, int>;
using StreamPair      = std::pair<StreamIndex, StreamIndex>;

using MetaMap         = std::map<uint8_t, rs2_metadata_type>;

typedef struct {
    uint32_t index;
    float    value;

    struct rs2::option_range range;

    bool     ro;
} RsOption;

using OptMap          = std::map<std::string, std::map<uint32_t, RsOption>>;

class RsOptionsList {
public:
    RsOptionsList(rs2::device dev) : m_dev(dev), m_remote_changed(false), m_local_changed(false) {};
    RsOptionsList(rs2::software_device sw_dev) : m_dev(NULL), m_sw_dev(sw_dev), m_remote_changed(false), m_local_changed(false) {};
   ~RsOptionsList() {};

    std::string serialize_full() {
        const std::lock_guard<std::mutex> lock(m_local_mutex);

        std::stringstream options;
        for (auto sensor : m_local) {
            options << sensor.first; // sensor name
            for (auto option : sensor.second) {
                options << "|";
                options << option.first << ",";

                options << option.second.value << ",";

                options << option.second.range.min << ",";
                options << option.second.range.max << ",";
                options << option.second.range.def << ",";
                options << option.second.range.step << ',';

                if (option.second.ro) options << '1';
                else options << '0';
            }
            options << "\r\n";
        }
        return options.str();
    };

    std::string serialize_changes() {
        const std::lock_guard<std::mutex> lock(m_local_mutex);

        std::stringstream options;
        for (auto sensor : m_local) {
            std::stringstream values;
            for (auto option : sensor.second) {
                if (m_local_prev[sensor.first][option.first].value != option.second.value) {
                    values << "|";
                    values << option.first << ",";

                    values << option.second.value << ",";

                    values << option.second.range.min << ",";
                    values << option.second.range.max << ",";
                    values << option.second.range.def << ",";
                    values << option.second.range.step << ',';

                    if (option.second.ro) values << '1';
                    else values << '0';
                }
            }

            if (!options.str().empty()) {
                options << sensor.first; // sensor name
                options << values.str();
                options << "\r\n";
            }
        }
        return options.str();
    };

    void scan() {
        const std::lock_guard<std::mutex> lock(m_local_mutex);

        // save the previous state
        m_local_prev = m_local;

        m_local_changed = false;

        rs2::device dev = m_dev;
        if (dev == NULL) dev = m_sw_dev;
        for (rs2::sensor sensor : dev.query_sensors()) {
            std::string sensor_name(sensor.supports(RS2_CAMERA_INFO_NAME) ? sensor.get_info(RS2_CAMERA_INFO_NAME) : "Unknown");
            // Prepare options list
            std::map<uint32_t, RsOption> opts;
            for (int32_t i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++) {
                RsOption option;
                rs2_option option_type = static_cast<rs2_option>(i);
                try {
                    // Get the current value of the option
                    option.index = i;
                    option.value = sensor.get_option(option_type);
                    option.range = sensor.get_option_range(option_type);
                    option.ro    = sensor.is_option_read_only(option_type);

                    opts[i] = option;
                } catch(...) {
                    ;
                }

                if (m_local_prev[sensor_name][option.index].value != option.value) m_local_changed = true;
            }
            m_local[sensor_name] = opts;
        }
    };

    void parse(std::string options) {
        const std::lock_guard<std::mutex> lock(m_remote_mutex);

        // save the previous state
        m_remote_prev = m_remote;

        // Parse the request, it is in form
        // <sensor_name>|<opt1_index>,<opt1_value>,<opt1_value>,<opt1_min>,<opt1_max>,<opt1_default>,<opt1_step>|...|<optN_index>,<optN_value>,<optN_value>,<optN_min>,<optN_max>,<optN_default>,<optN_step>
        while (!options.empty()) {
            std::map<uint32_t, RsOption> opts;

            // get the sensor line
            uint32_t line_pos = options.find("\r\n");
            std::string sensor = options.substr(0, line_pos) + "|";
            options.erase(0, line_pos + 2);

            // get the sensor name
            uint32_t pos = sensor.find("|");
            std::string sensor_name = sensor.substr(0, pos);
            sensor.erase(0, pos + 1);

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

                opts[option.index] = option;
                
                if (m_local[sensor_name][option.index].value != option.value) m_remote_changed = true;
            }

            m_remote[sensor_name] = opts;
        }
    };

    void set() {
        const std::lock_guard<std::mutex> local_lock(m_local_mutex);
        const std::lock_guard<std::mutex> remote_lock(m_remote_mutex);

        for (auto sensor : m_remote) {
            // locate the proper sensor
            rs2::device dev = m_dev;
            if (dev == NULL) dev = m_sw_dev;
            auto sensors = m_dev.query_sensors();
            auto s = sensors.begin();
            for (; s != sensors.end(); s++) {
                std::string sname(s->supports(RS2_CAMERA_INFO_NAME) ? s->get_info(RS2_CAMERA_INFO_NAME) : "Unknown");
                if (std::strcmp(sname.c_str(), sensor.first.c_str()) == 0) {
                    break;
                }
            }

            for (auto option : sensor.second) {
                if (m_local[sensor.first][option.first].value != option.second.value) {
                    try {
                        if (m_dev == NULL) {
                            LOG_ERROR("Setting option for software sensor" << (rs2_option)option.first << " #" << std::dec << option.first << " to " << option.second.value << ", old value is " << m_local[sensor.first][option.first].value);
                        } else {
                            if (!s->is_option_read_only((rs2_option)option.first)) {
                                LOG_INFO("Setting option " << (rs2_option)option.first << " #" << std::dec << option.first << " to " << option.second.value << ", old value is " << m_local[sensor.first][option.first].value);
                                s->set_option((rs2_option)option.first, option.second.value);
                            }
                        }
                        // update the local map
                        m_local[sensor.first][option.first].value = option.second.value;
                    } catch(const rs2::error& e) {
                        // LOG_WARNING("Failed to set option #" << option.first);
                        LOG_ERROR("Failed to set option " << (rs2_option)option.first << " #" << std::dec << option.first << " for sensor " << sensor.first << " to " << option.second.value 
                            << ". Range is [" << option.second.range.min << ", " << option.second.range.max << "], default value is " << option.second.range.def << ", step is " << option.second.range.step 
                            << "(" << e.what() << ")");
                    } catch(...) {
                        // LOG_WARNING("Failed to set option #" << option.first);
                        LOG_ERROR("Failed to set option " << (rs2_option)option.first << " #" << std::dec << option.first << " for sensor " << sensor.first << " to " << option.second.value 
                            << ". Range is [" << option.second.range.min << ", " << option.second.range.max << "], default value is " << option.second.range.def << ", step is " << option.second.range.step);
                    }
                }
            }
        }

        m_remote_changed = false;
    };

    bool changes() { return m_local_changed; };
    bool remote_changes() { return m_remote_changed; };

private:
    rs2::device m_dev;
    rs2::software_device m_sw_dev;

    OptMap m_local_prev;
    OptMap m_local;

    OptMap m_remote_prev;
    OptMap m_remote;

    std::mutex m_local_mutex;
    std::mutex m_remote_mutex;

    bool m_local_changed;
    bool m_remote_changed;
};