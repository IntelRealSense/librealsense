// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <string>
#include "../include/librealsense/rs.hpp"

namespace rs
{

    error::error(rs_error * err) : std::runtime_error(rs_get_error_message(err))
    {
        function = (nullptr != rs_get_failed_function(err)) ? rs_get_failed_function(err) : std::string();
        args = (nullptr != rs_get_failed_args(err)) ? rs_get_failed_args(err) : std::string();
        rs_free_error(err);
    }

    const std::string & error::get_failed_function() const { return function; }
    const std::string & error::get_failed_args() const { return args; }
    void error::handle(rs_error * e) { if(e) throw error(e); }

}

