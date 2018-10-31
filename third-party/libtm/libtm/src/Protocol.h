/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#ifndef __TM2_PROTOCOL_H
#define __TM2_PROTOCOL_H

#include <libusb-1.0/libusb.h>
#include "Common.h"

#define INTERFACE_NUMBER 0

/* Synchronous device I/O */
int bulk_transfer_write_message(IN libusb_device_handle * device_handle, IN unsigned char * host_data_out, IN unsigned int host_data_out_len, OUT int * host_data_out_bytes_sent);
int bulk_transfer_read_message(IN libusb_device_handle * device_handle, OUT unsigned char * host_data_in, OUT int * host_data_in_bytes_received);
int bulk_transfer_exchange_message(IN libusb_device_handle *device_handle, IN unsigned char * host_data_out, IN unsigned int host_data_out_len, OUT int * host_data_out_bytes_sent, OUT unsigned char * host_data_in, OUT int * host_data_in_bytes_received);

int control_transfer_write_message(IN libusb_device_handle *device_handle, IN unsigned char * host_data_out, IN unsigned int host_data_out_len, OUT int * host_data_out_bytes_sent);
int control_transfer_read_message(IN libusb_device_handle *device_handle, OUT unsigned char * host_data_in, OUT int * host_data_in_bytes_received);
int control_transfer_exchange_message(IN libusb_device_handle *device_handle, IN unsigned char * host_data_out, IN unsigned int host_data_out_len, OUT int * host_data_out_bytes_sent, OUT unsigned char * host_data_in, OUT int * host_data_in_bytes_received);

int interrupt_transfer_write_message(IN libusb_device_handle *device_handle, IN unsigned char * host_data_out, IN unsigned int host_data_out_len, OUT int * host_data_out_bytes_sent);
int interrupt_transfer_read_message(IN libusb_device_handle *device_handle, OUT unsigned char * host_data_in, OUT int * host_data_in_bytes_received);
int interrupt_transfer_exchange_message(IN libusb_device_handle *device_handle, IN unsigned char * host_data_out, IN unsigned int host_data_out_len, OUT int * host_data_out_bytes_sent, OUT unsigned char * host_data_in, OUT int * host_data_in_bytes_received);

/* Asynchronous device I/O */
/* TBD */

#endif // __TM2_PROTOCOL_H

