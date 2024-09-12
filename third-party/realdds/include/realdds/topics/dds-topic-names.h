// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <string>


namespace realdds {
namespace topics {


// The character used to separate topic path elements
constexpr char SEPARATOR = '/';

// Every topic should be under ROOT (which includes the separator):
constexpr char const * ROOT = "realsense/";
constexpr size_t ROOT_LEN = 10;
// NOTE: actual streams will be ROS-compatible, meaning rt/ROOT

constexpr char const * DEVICE_INFO_TOPIC_NAME = "realsense/device-info";

// The next topic names should be concatenated to a topic root
constexpr char const * NOTIFICATION_TOPIC_NAME = "/notification";
constexpr char const * CONTROL_TOPIC_NAME = "/control";
constexpr char const * METADATA_TOPIC_NAME = "/metadata";
constexpr char const * DFU_TOPIC_NAME = "/dfu";


namespace notification {
    namespace key {
        extern std::string const id;
    }
    namespace device_header {
        extern std::string const id;
        namespace key {
            extern std::string const n_streams;
            extern std::string const extrinsics;
        }
    }
    namespace device_options {
        extern std::string const id;
        namespace key {
            extern std::string const options;
        }
    }
    namespace stream_header {
        extern std::string const id;
        namespace key {
            extern std::string const type;
            extern std::string const name;
            extern std::string const sensor_name;
            extern std::string const profiles;
            extern std::string const default_profile_index;
            extern std::string const metadata_enabled;
        }
    }
    namespace stream_options {
        extern std::string const id;
        namespace key {
            extern std::string const stream_name;
            extern std::string const options;
            extern std::string const intrinsics;
            extern std::string const recommended_filters;
        }
        namespace intrinsics {
            namespace key {
                extern std::string const accel;
                extern std::string const gyro;
            }
        }
    }
    namespace log {
        extern std::string const id;
        namespace key {
            extern std::string const entries;
        }
    }
    namespace query_options {
        extern std::string const id;
        namespace key {
            extern std::string const option_values;
        }
    }
    namespace dfu_ready {
        extern std::string const id;
        namespace key {
        }
    }
    namespace dfu_apply {
        extern std::string const id;
        namespace key {
            extern std::string const progress;
        }
    }
}

namespace control {
    namespace key {
        using notification::key::id;
    }
    namespace set_option {
        extern std::string const id;
        namespace key {
            extern std::string const value;
            extern std::string const option_name;
            extern std::string const stream_name;
        }
    }
    namespace query_option {
        extern std::string const id;
        namespace key {
            using control::set_option::key::option_name;
            using control::set_option::key::stream_name;
        }
    }
    namespace query_options {
        using notification::query_options::id;
        namespace key {
            using control::set_option::key::option_name;
            using control::set_option::key::stream_name;
            extern std::string const sensor_name;
        }
    }
    namespace open_streams {
        extern std::string const id;
        namespace key {
            extern std::string const stream_profiles;
            extern std::string const reset;
            extern std::string const commit;
        }
    }
    namespace hwm {
        extern std::string const id;
        namespace key {
            extern std::string const opcode;
            extern std::string const param1;
            extern std::string const param2;
            extern std::string const param3;
            extern std::string const param4;
            extern std::string const data;
            extern std::string const build_command;
        }
    }
    namespace hw_reset {
        extern std::string const id;
    }
    namespace dfu_start {
        extern std::string const id;
    }
    namespace dfu_apply {
        using notification::dfu_apply::id;
        namespace key {
            extern std::string const cancel;
        }
    }
}

namespace reply {
    namespace key {
        using control::key::id;
        extern std::string const sample;
        extern std::string const control;
        extern std::string const status;
        extern std::string const explanation;
    }
    namespace status {
        extern std::string const ok;
    }
    namespace set_option {
        using control::set_option::id;
        namespace key {
            using control::set_option::key::value;
        }
    }
    namespace query_option {
        using control::query_option::id;
        namespace key {
            using control::set_option::key::value;
        }
    }
    namespace query_options {
        using control::query_options::id;
        namespace key {
            using notification::query_options::key::option_values;
        }
    }
    namespace open_streams {
        using control::open_streams::id;
        namespace key {
            using control::open_streams::key::stream_profiles;
        }
    }
    namespace hwm {
        using control::hwm::id;
        namespace key {
            using control::hwm::key::data;
        }
    }
    namespace dfu_start {
        using control::dfu_start::id;
        namespace key {
        }
    }
    namespace dfu_apply {
        using control::dfu_apply::id;
        namespace key {
        }
    }
}

namespace metadata {
    namespace key {
        extern std::string const stream_name;
        extern std::string const header;
        extern std::string const metadata;
    }
    namespace header {
        namespace key {
            extern std::string const frame_number;
            extern std::string const timestamp;
            extern std::string const timestamp_domain;
            extern std::string const depth_units;
        }
    }
    namespace metadata {
        namespace key {
        }
    }
}


}  // namespace topics
}  // namespace realdds
