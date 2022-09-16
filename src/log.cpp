// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "types.h"
#include "log.h"

#include <fstream>

#ifdef BUILD_EASYLOGGINGPP
INITIALIZE_EASYLOGGINGPP

namespace librealsense
{
    char log_name[] = "librealsense";
    static logger_type<log_name> logger;
}

void librealsense::log_to_console(rs2_log_severity min_severity)
{
    logger.log_to_console(min_severity);
}

void librealsense::log_to_file(rs2_log_severity min_severity, const char * file_path)
{
    logger.log_to_file(min_severity, file_path);
}

void librealsense::log_to_callback( rs2_log_severity min_severity, log_callback_ptr callback )
{
    logger.log_to_callback( min_severity, callback );
}

void librealsense::reset_logger()
{
    logger.reset_logger();
}

void librealsense::enable_rolling_log_file( unsigned max_size )
{
    logger.enable_rolling_log_file( max_size );
}

#else // BUILD_EASYLOGGINGPP

void librealsense::log_to_console(rs2_log_severity min_severity)
{
    throw std::runtime_error("log_to_console is not supported without BUILD_EASYLOGGINGPP");
}

void librealsense::log_to_file(rs2_log_severity min_severity, const char * file_path)
{
    throw std::runtime_error("log_to_file is not supported without BUILD_EASYLOGGINGPP");
}

void librealsense::log_to_callback(rs2_log_severity min_severity, log_callback_ptr callback)
{
    throw std::runtime_error("log_to_callback is not supported without BUILD_EASYLOGGINGPP");
}

void librealsense::reset_logger()
{
    throw std::runtime_error("reset_logger is not supported without BUILD_EASYLOGGINGPP");
}

void librealsense::enable_rolling_log_file( unsigned max_size )
{
    throw std::runtime_error("enable_rolling_log_file is not supported without BUILD_EASYLOGGINGPP");
}
#endif // BUILD_EASYLOGGINGPP

