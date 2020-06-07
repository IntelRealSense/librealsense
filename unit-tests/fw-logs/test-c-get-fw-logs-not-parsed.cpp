// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file fw-logs-common.h
#include "fw-logs-common.h"

/* Include the librealsense C header files */
#include <librealsense2/rs.h>

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

    bool were_fw_logs_received_once = false;
    while (1)//!were_fw_logs_received_once)
    {
        rs2_error* e = nullptr;
        rs2_firmware_log_message* fw_log_msg = rs2_get_firmware_log(dev, &e);
            
        REQUIRE(fw_log_msg);

        int size = rs2_firmware_log_message_size(fw_log_msg, &e);

        auto start = rs2_firmware_log_message_data(fw_log_msg, &e);

        if (size > 0)
        {
            were_fw_logs_received_once = true;
            //get time
            time_t rawtime;
            struct tm* timeinfo;

            time_t mytime = time(NULL);
            char* time_str = ctime(&mytime);
            time_str[strlen(time_str) - 1] = '\0';
            printf("message received %s - ", time_str);

            rs2_error* e = nullptr;
            rs2_log_severity severity = rs2_get_fw_log_severity(fw_log_msg, &e);
            printf("%s ", rs2_log_severity_to_string(severity));

            for (int i = 0; i < size; ++i)
            {
                printf("%d ", *(start + i));
            }
            printf("\n");
        }

        rs2_delete_firmware_log_message(fw_log_msg);
    }
    
}
