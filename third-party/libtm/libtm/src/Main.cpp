/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "Protocol.h"
#include "Message.h"

#define PRODUCT_ID 0xF63B
#define VENDOR_ID 0x040E
#define MAX_MESSAGE_SIZE 1024

void send_bulk_message_get_device_info(libusb_device_handle * device_handle)
{
    bulk_message_request_get_device_info request;
    bulk_message_response_get_device_info response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 6, DEV_GET_DEVICE_INFO);

    /* Print message request */
    printf("DEV_GET_DEVICE_INFO request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("DEV_GET_DEVICE_INFO response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_get_time(libusb_device_handle * device_handle)
{
    bulk_message_request_get_time request;
    bulk_message_response_get_time response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 6, DEV_GET_TIME);

    /* Print message request */
    printf("DEV_GET_TIME request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("DEV_GET_TIME response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_get_supported_raw_streams(libusb_device_handle * device_handle)
{
    bulk_message_request_get_supported_raw_streams request;
    uint8_t response[MAX_MESSAGE_SIZE];
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 6, DEV_GET_SUPPORTED_RAW_STREAMS);

    /* Print message request */
    printf("DEV_GET_SUPPORTED_RAW_STREAMS request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("DEV_GET_SUPPORTED_RAW_STREAMS response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_raw_streams_control(libusb_device_handle * device_handle)
{
    bulk_message_request_raw_streams_control* request = NULL;
    bulk_message_response_raw_streams_control response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;
    int request_length = sizeof(bulk_message_request_raw_streams_control) + sizeof(supported_raw_stream_libtm_message);

    /* request message will include one stream */
    request = (bulk_message_request_raw_streams_control*)malloc(request_length);

    /* Prepare get device info message */
    memset(request, 0, request_length);
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)request, request_length, DEV_RAW_STREAMS_CONTROL);
    request->wNumEnabledStreams = 1;
    request->stream[0].bSensorID = SET_SENSOR_ID(SENSOR_TYPE_COLOR, 0);
    request->stream[0].wFramesPerSecond = 30;
    request->stream[0].wWidth = 1920;
    request->stream[0].wHeight = 1080;
    request->stream[0].bReserved = 0;

    /* Print message request */
    printf("DEV_RAW_STREAMS_CONTROL request\n");
    print_message((unsigned char *)request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)request, request_length, &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("DEV_RAW_STREAMS_CONTROL response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_get_camera_intrinsics(libusb_device_handle * device_handle)
{
    bulk_message_request_get_camera_intrinsics request;
    bulk_message_response_get_camera_intrinsics response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 7, DEV_GET_CAMERA_INTRINSICS);
    request.bCameraID = SET_SENSOR_ID(SENSOR_TYPE_COLOR, 0);

    /* Print message request */
    printf("DEV_GET_CAMERA_INTRINSICS request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("DEV_GET_CAMERA_INTRINSICS response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_get_motion_intrinsics(libusb_device_handle * device_handle)
{
    bulk_message_request_get_motion_intrinsics request;
    bulk_message_response_get_motion_intrinsics response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 7, DEV_GET_MOTION_INTRINSICS);
    request.bMotionID = SET_SENSOR_ID(SENSOR_TYPE_GYRO, 0);

    /* Print message request */
    printf("DEV_GET_MOTION_INTRINSICS request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("DEV_GET_MOTION_INTRINSICS response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_get_extrinsics(libusb_device_handle * device_handle)
{
    bulk_message_request_get_extrinsics request;
    bulk_message_response_get_extrinsics response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 8, DEV_GET_EXTRINSICS);
    request.bSensorID = SET_SENSOR_ID(SENSOR_TYPE_COLOR, 0);
    request.bReferenceSensorID = SET_SENSOR_ID(SENSOR_TYPE_DEPTH, 1);

    /* Print message request */
    printf("DEV_GET_EXTRINSICS request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("DEV_GET_EXTRINSICS response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_set_camera_intrinsics(libusb_device_handle * device_handle)
{
    bulk_message_request_set_camera_intrinsics request;
    bulk_message_response_set_camera_intrinsics response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;
    int i = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 56, DEV_SET_CAMERA_INTRINSICS);
    request.bCameraID = SET_SENSOR_ID(SENSOR_TYPE_COLOR, 0);
    request.intrinsics.dwWidth = 1;
    request.intrinsics.dwHeight = 2;
    request.intrinsics.flPpx = 3;
    request.intrinsics.flPpy = 4;
    request.intrinsics.flFx = 5;
    request.intrinsics.flFy = 6;
    request.intrinsics.dwDistortionModel = 7;
    for (i = 0; i < 5; i++)
    {
        request.intrinsics.flCoeffs[i] = i+8;
    }

    /* Print message request */
    printf("DEV_SET_CAMERA_INTRINSICS request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("DEV_SET_CAMERA_INTRINSICS response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_set_motion_intrinsics(libusb_device_handle * device_handle)
{
    bulk_message_request_set_motion_intrinsics request;
    bulk_message_response_set_motion_intrinsics response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;
    int i = 0;
    int j = 0;
    int k = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 80, DEV_SET_MOTION_INTRINSICS);
    request.bMotionID = SET_SENSOR_ID(SENSOR_TYPE_GYRO, 0);

    for (i = 0; i < 3; i++, k++)
    {
        for (j = 0; j < 4; j++, k++)
        {
            request.intrinsics.flData[i][j] = k;
        }
    }

    for (i = 0; i < 3; i++, k++)
    {
        request.intrinsics.flNoiseVariances[i] = k;
    }

    for (i = 0; i < 3; i++, k++)
    {
        request.intrinsics.flBiasVariances[i] = k;
    }

    /* Print message request */
    printf("DEV_SET_MOTION_INTRINSICS request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("DEV_SET_MOTION_INTRINSICS response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_set_extrinsics(libusb_device_handle * device_handle)
{
    bulk_message_request_set_extrinsics request;
    bulk_message_response_set_extrinsics response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;
    int i = 0;
    int j = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 56, DEV_SET_EXTRINSICS);
    request.bSensorID = SET_SENSOR_ID(SENSOR_TYPE_COLOR, 0);
    request.bReferenceSensorID = SET_SENSOR_ID(SENSOR_TYPE_DEPTH, 1);

    for (i = 0; i < 9; i++, j++)
    {
        request.extrinsics.flRotation[i] = j;
    }

    for (i = 0; i < 3; i++, j++)
    {
        request.extrinsics.flTranslation[i] = j;
    }

    /* Print message request */
    printf("DEV_SET_EXTRINSICS request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("DEV_SET_EXTRINSICS response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_log_control(libusb_device_handle * device_handle)
{
    bulk_message_request_log_control request;
    bulk_message_response_log_control response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 56, DEV_LOG_CONTROL);
    request.bVerbosity = 0;
    request.bLogMode = 0;

    /* Print message request */
    printf("DEV_LOG_CONTROL request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("DEV_LOG_CONTROL response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_get_pose(libusb_device_handle * device_handle)
{
    bulk_message_request_get_pose request;
    bulk_message_response_get_pose response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 6, DEV_GET_POSE);

    /* Print message request */
    printf("DEV_GET_POSE request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("DEV_GET_POSE response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_get_occupancy_map_tiles(libusb_device_handle * device_handle)
{
    bulk_message_request_get_occupancy_map_tiles request;
    bulk_message_response_get_occupancy_map_tiles response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 6, SLAM_GET_OCCUPANCY_MAP_TILES);

    /* Print message request */
    printf("SLAM_GET_OCCUPANCY_MAP_TILES request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("SLAM_GET_OCCUPANCY_MAP_TILES response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_get_localization_data(libusb_device_handle * device_handle)
{
    bulk_message_request_get_localization_data request;
    bulk_message_response_get_localization_data response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 6, SLAM_GET_LOCALIZATION_DATA);

    /* Print message request */
    printf("SLAM_GET_LOCALIZATION_DATA request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("SLAM_GET_LOCALIZATION_DATA response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_set_localization_data(libusb_device_handle * device_handle)
{
    bulk_message_request_set_localization_data request;
    bulk_message_response_set_localization_data response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 7, SLAM_SET_LOCALIZATION_DATA);

    /* Print message request */
    printf("SLAM_SET_LOCALIZATION_DATA request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("SLAM_SET_LOCALIZATION_DATA response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_set_6dof_interrupt_rate(libusb_device_handle * device_handle)
{
    bulk_message_request_set_6dof_interrupt_rate request;
    bulk_message_response_set_6dof_interrupt_rate response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 7, SLAM_SET_6DOF_INTERRUPT_RATE);
    request.message.bInterruptRate = 1;

    /* Print message request */
    printf("SLAM_SET_6DOF_INTERRUPT_RATE request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("SLAM_SET_6DOF_INTERRUPT_RATE response\n");
    print_message((unsigned char *)&response);
}

void send_bulk_message_6dof_control_enable(libusb_device_handle * device_handle)
{
    bulk_message_request_6dof_control request;
    bulk_message_response_6dof_control response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 7, SLAM_6DOF_CONTROL);
    request.bEnable = 1;

    /* Print message request */
    printf("SLAM_6DOF_CONTROL request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("SLAM_6DOF_CONTROL response\n");
    print_message((unsigned char *)&response);
}

int receive_interrupt_message_6dof_pose_data(libusb_device_handle * device_handle)
{
    interrupt_message_get_pose response;
    int host_data_in_bytes_received = 0;
    int result = 0;

    /* Prepare get device info message */
    memset(&response, 0, sizeof(response));

    /* receive interrupt 6dof pose data */
    result = interrupt_transfer_read_message(device_handle, (unsigned char *)&response, &host_data_in_bytes_received);
    if (result == 0)
    {
        /* Print message response */
        printf("Interrupt 6dof pose data message\n");
        print_message((unsigned char *)&response);
    }

    return result;
}

void send_bulk_message_6dof_control_disable(libusb_device_handle * device_handle)
{
    bulk_message_request_6dof_control request;
    bulk_message_response_6dof_control response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;
    int i = 0;
    int result = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 7, SLAM_6DOF_CONTROL);
    request.bEnable = 0;

    /* Print message request */
    printf("SLAM_6DOF_CONTROL request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    result = bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("SLAM_6DOF_CONTROL response\n");
    print_message((unsigned char *)&response);

    if (result == 0)
    {
        printf("Waiting for interrupt endpoint to be cleaned\n");
        while (result == 0)
        {
            result = receive_interrupt_message_6dof_pose_data(device_handle);
            printf("Received interrupt 6dof pose data message %d\n",i);
            i++;
        }
        printf("interrupt endpoint is now cleaned\n");

    }
    else
    {
       printf("Error disabling 6DOF\n");
    }
}

void send_bulk_message_enable_occupancy_map(libusb_device_handle * device_handle)
{
    bulk_message_request_occupancy_map_control request;
    bulk_message_response_occupancy_map_control response;
    int host_data_out_bytes_sent = 0;
    int host_data_in_bytes_received = 0;

    /* Prepare get device info message */
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    init_message_request_header((unsigned char *)&request, 7, SLAM_OCCUPANCY_MAP_CONTROL);
    request.message.bEnable = 1;

    /* Print message request */
    printf("SLAM_OCCUPANCY_MAP_CONTROL request\n");
    print_message((unsigned char *)&request);

    /* Send and receive device info */
    bulk_transfer_exchange_message(device_handle, (unsigned char *)&request, sizeof(request), &host_data_out_bytes_sent, (unsigned char *)&response, &host_data_in_bytes_received);

    /* Print message response */
    printf("SLAM_OCCUPANCY_MAP_CONTROL response\n");
    print_message((unsigned char *)&response);
}


int main(void)
{
    struct libusb_device_handle * device_handle = NULL;
    int result = 0;
    int i = 0;
    int max_interrupt_message = 100;

    printf("TM2 Demo (version %d)\n", TM2_API_VERSION);

    /* Initialise libusb, must be called before other libusb routines are used */
    result = libusb_init(NULL);
    if (result < 0)
    {
        fprintf(stderr, "Error initializing libusb: %s\n", libusb_error_name(result));
        exit(1);
    }

    /* Finding a device with a particular idVendor/idProduct combination */
    device_handle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    if (device_handle == NULL)
    {
        fprintf(stderr, "Error finding USB device\n");
        goto out;
    }

    /* Claim an interface on a given device handle -  must claim the interface before performing I/O on any of its endpoints */
    result = libusb_claim_interface(device_handle, INTERFACE_INDEX);
    if (result < 0)
    {
        fprintf(stderr, "Error claiming interface: %s\n", libusb_error_name(result));
        goto out;
    }

    /* Sending all supported messages */
    send_bulk_message_get_device_info(device_handle);
    send_bulk_message_get_time(device_handle);
    send_bulk_message_get_supported_raw_streams(device_handle);
    send_bulk_message_raw_streams_control(device_handle);
    send_bulk_message_set_camera_intrinsics(device_handle);
    send_bulk_message_get_camera_intrinsics(device_handle);
    send_bulk_message_set_motion_intrinsics(device_handle);
    send_bulk_message_get_motion_intrinsics(device_handle);
    send_bulk_message_set_extrinsics(device_handle);
    send_bulk_message_get_extrinsics(device_handle);
    send_bulk_message_log_control(device_handle);
    send_bulk_message_get_pose(device_handle);
    send_bulk_message_get_occupancy_map_tiles(device_handle);
    send_bulk_message_get_localization_data(device_handle);
    send_bulk_message_set_localization_data(device_handle);
    send_bulk_message_set_6dof_interrupt_rate(device_handle);
    send_bulk_message_6dof_control_enable(device_handle);
    for (i = 0; i<max_interrupt_message; i++)
    {
        printf("Receive interrupt 6dof pose data message %d/%d\n", i, max_interrupt_message);
        receive_interrupt_message_6dof_pose_data(device_handle);
    }
    send_bulk_message_6dof_control_disable(device_handle);
    send_bulk_message_enable_occupancy_map(device_handle);

    /* Finished using the device - release an interface previously claimed */
    libusb_release_interface(device_handle, INTERFACE_INDEX);

out:
    if (device_handle != NULL)
    {
        /* Close a device handle */
        libusb_close(device_handle);
    }

    /* Deinitialise libusb, other libusb routines may not be called after this function */
    libusb_exit(NULL);

    return result;
}
