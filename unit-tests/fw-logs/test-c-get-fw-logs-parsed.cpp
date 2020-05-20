// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file fw-logs-common.h
#include "fw-logs-common.h"

/* Include the librealsense C header files */
#include <librealsense2/rs.h>
#include <librealsense2/h/rs_device.h>
#include <librealsense2/h/rs_pipeline.h>
#include <librealsense2/h/rs_option.h>
#include <librealsense2/h/rs_frame.h>


#include <direct.h>
#define GetCurrentDir _getcwd

char* rs2_firmware_log_message_to_string(rs2_firmware_log_message msg)
{
    char* buffer = (char*)(malloc(50 * sizeof(char)));
    sprintf(buffer, "sev = %d, file = %d, line = %d", msg._severity, msg._file_id, msg._line);
    return buffer;
}


std::string get_current_dir() {
    char buff[FILENAME_MAX]; //create string buffer to hold path
    GetCurrentDir(buff, FILENAME_MAX);
    std::string current_working_dir(buff);
    return current_working_dir;
}

//get fw logs using c api
TEST_CASE( "Getting FW logs in C", "[fw-logs]" ) {
    
    rs2_error* e = 0;

    // Create a context object. This object owns the handles to all connected realsense devices.
    // The returned object should be released with rs2_delete_context(...)
    rs2_context* ctx = rs2_create_context(RS2_API_VERSION, &e);
    REQUIRE(e == 0);

    /* Get a list of all the connected devices. */
    // The returned object should be released with rs2_delete_device_list(...)
    rs2_device_list* device_list = rs2_query_devices(ctx, &e);
    REQUIRE(e == 0);

    int dev_count = rs2_get_device_count(device_list, &e);
    REQUIRE(e == 0);
    printf("There are %d connected RealSense devices.\n", dev_count);
    REQUIRE(dev_count > 0);

    // Get the first connected device
    // The returned object should be released with rs2_delete_device(...)
    rs2_device* dev = rs2_create_device(device_list, 0, &e);
    REQUIRE(e == 0);

    bool device_extendable_to_fw_logger = false;
    if (rs2_is_device_extendable_to(dev, RS2_EXTENSION_FW_LOGGER, &e) != 0 && !e)
    {
        device_extendable_to_fw_logger = true;
    }
    REQUIRE(device_extendable_to_fw_logger);

    const char* xml_path_const = "HWLoggerEventsDS5.xml";

    /*std::string cf = get_current_dir();
    std::string xml_path(xml_path_const);
    
    if (!xml_path.empty())
    {
        std::ifstream f(xml_path);
        if (f.good())
        {
            int a = 1;
        }
        else {

            int b = 1;
        }
    }*/

    rs2_firmware_logs_parser* parser = rs2_create_firmware_logs_parser(xml_path_const, &e);
    REQUIRE(parser);

    bool were_fw_logs_received_once = false;
    while (!were_fw_logs_received_once)
    {
        rs2_firmware_log_message_list* fw_logs_list = rs2_get_firmware_logs_list(dev, &e);
        REQUIRE(fw_logs_list);

        if (fw_logs_list->_number_of_messages > 0)
        {
            were_fw_logs_received_once = true;
            for (int i = 0; i < fw_logs_list->_number_of_messages; ++i)
            {
                rs2_firmware_log_message* msg = &fw_logs_list->_messages[i];
                rs2_raw_data_buffer* parsed_log = rs2_parse_firmware_log(parser, msg->_event_id, msg->_p1, msg->_p2, msg->_p3,
                    msg->_file_id, msg->_thread_id, &e);
                // call as it must be: rs2_parse_firmware_log(parser, &msg);
                rs2::error::handle(e);

                int size = rs2_get_raw_data_size(parsed_log, &e);
                rs2::error::handle(e);

                if (size > 0)
                {
                    auto start = rs2_get_raw_data(parsed_log, &e);

                    unsigned char message_size = *start;
                    printf("size of message = %d", message_size);
                }

                printf("message received: %s\n", rs2_firmware_log_message_to_string(fw_logs_list->_messages[i]));
            }
        }
        rs2_delete_firmware_logs_list(fw_logs_list);
    }

    rs2_delete_firmware_logs_parser(parser);
    
}
