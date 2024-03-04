// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <realdds/topics/dds-topic-names.h>


namespace realdds {
namespace topics {


namespace notification {
    namespace key {
        std::string const id( "id", 2 );
    }
    namespace device_header {
        std::string const id( "device-header", 13 );
        namespace key {
            std::string const n_streams( "n-streams", 9 );
            std::string const extrinsics( "extrinsics", 10 );
        }
    }
    namespace device_options {
        std::string const id( "device-options", 14 );
        namespace key {
            std::string const options( "options", 7 );
        }
    }
    namespace stream_header {
        std::string const id( "stream-header", 13 );
        namespace key {
            std::string const type( "type", 4 );
            std::string const name( "name", 4 );
            std::string const sensor_name( "sensor-name", 11 );
            std::string const profiles( "profiles", 8 );
            std::string const default_profile_index( "default-profile-index", 21 );
            std::string const metadata_enabled( "metadata-enabled", 16 );
        }
    }
    namespace stream_options {
        std::string const id( "stream-options", 14 );
        namespace key {
            std::string const stream_name( "stream-name", 11 );
            std::string const options( "options", 7 );
            std::string const intrinsics( "intrinsics", 10 );
            std::string const recommended_filters( "recommended-filters", 19 );
        }
        namespace intrinsics {
            namespace key {
                std::string const accel( "accel", 5 );
                std::string const gyro( "gyro", 4 );
            }
        }
    }
    namespace log {
        std::string const id( "log", 3 );
        namespace key {
            std::string const entries( "entries", 7 );
        }
    }
    namespace query_options {
        std::string const id( "query-options", 13 );
        namespace key {
            std::string const option_values( "option-values", 13 );
        }
    }
    namespace dfu_ready {
        std::string const id( "dfu-ready", 9 );
        namespace key {
        }
    }
    namespace dfu_apply {
        std::string const id( "dfu-apply", 9 );
        namespace key {
            std::string const progress( "progress", 8 );
        }
    }
}

namespace control {
    namespace key {
        //using notification::key::id;
    }
    namespace set_option {
        std::string const id( "set-option", 10 );
        namespace key {
            std::string const value( "value", 5 );
            std::string const option_name( "option-name", 11 );
            std::string const stream_name( "stream-name", 11 );
        }
    }
    namespace query_option {
        std::string const id( "query-option", 12 );
        namespace key {
            //using control::set_option::key::option_name;
            //using control::set_option::key::stream_name;
        }
    }
    namespace query_options {
        //using notification::query_options::id;
        namespace key {
            //using control::set_option::key::option_name;
            //using control::set_option::key::stream_name;
            //using notification::query_options::key::option_values;
            std::string const sensor_name( "sensor-name", 11 );
        }
    }
    namespace open_streams {
        std::string const id( "open-streams", 12 );
        namespace key {
            std::string const stream_profiles( "stream-profiles", 15 );
            std::string const reset( "reset", 5 );
            std::string const commit( "commit", 6 );
        }
    }
    namespace hwm {
        std::string const id( "hwm", 3 );
        namespace key {
            std::string const opcode( "opcode", 6 );
            std::string const param1( "param1", 6 );
            std::string const param2( "param2", 6 );
            std::string const param3( "param3", 6 );
            std::string const param4( "param4", 6 );
            std::string const data( "data", 4 );
            std::string const build_command( "build-command", 13 );
        }
    }
    namespace hw_reset {
        std::string const id( "hw-reset", 8 );
    }
    namespace dfu_start {
        std::string const id( "dfu-start", 9 );
    }
    namespace dfu_apply {
        //using notification::dfu_apply::id;
        namespace key {
            std::string const cancel( "cancel", 6 );
        }
    }
}

namespace reply {
    namespace key {
        //using control::key::id;
        std::string const sample( "sample", 6 );
        std::string const control( "control", 7 );
        std::string const status( "status", 6 );
        std::string const explanation( "explanation", 11 );
    }
    namespace status {
        std::string const ok( "ok", 2 );
    }
    namespace set_option {
        //using control::set_option::id;
        namespace key {
            //using control::set_option::key::value;
        }
    }
    namespace query_option {
        //using control::query_option::id;
        namespace key {
            //using control::set_option::key::value;
        }
    }
    namespace query_options {
        //using control::query_options::id;
        namespace key {
            //using notification::query_options::key::option_values;
        }
    }
    namespace open_streams {
        //using control::open_streams::id;
        namespace key {
            //using control::open_streams::key::stream_profiles;
        }
    }
    namespace hwm {
        //using control::hwm::id;
        namespace key {
            //using control::hwm::key::data;
        }
    }
    namespace dfu_start {
        //using control::dfu_start::id;
        namespace key {
        }
    }
    namespace dfu_apply {
        //using control::dfu_apply::id;
        namespace key {
        }
    }
}

namespace metadata {
    namespace key {
        std::string const stream_name( "stream-name", 11 );
        std::string const header( "header", 6 );
        std::string const metadata( "metadata", 8 );
    }
    namespace header {
        namespace key {
            std::string const frame_number( "frame-number", 12 );
            std::string const timestamp( "timestamp", 9 );
            std::string const timestamp_domain( "timestamp-domain", 16 );
            std::string const depth_units( "depth-units", 11 );
        }
    }
    namespace metadata {
        namespace key {
        }
    }
}


}  // namespace topics
}  // namespace realdds
