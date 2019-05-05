// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "types.h"
#include "log.h"

#include <fstream>

#if BUILD_EASYLOGGINGPP
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

#else // BUILD_EASYLOGGINGPP

void librealsense::log_to_console(rs2_log_severity min_severity)
{
}

void librealsense::log_to_file(rs2_log_severity min_severity, const char * file_path)
{
}

#endif // BUILD_EASYLOGGINGPP

