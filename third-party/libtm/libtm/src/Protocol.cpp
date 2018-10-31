/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#include <stdio.h>
#include "Protocol.h"
#include "Common.h"

#define TIMEOUT_MS 5000

/* Bulk Endpoint Protocol */
#define BULK_IN_ENDPOINT 0x81
#define BULK_OUT_ENDPOINT 0x01
#define BULK_IN_MAX_TRANSFER_SIZE 1024
#define BULK_OUT_MAX_TRANSFER_SIZE 1024

/* Control Endpoint Protocol */
#define CONTROL_IN_MAX_TRANSFER_SIZE 1024
#define CONTROL_OUT_MAX_TRANSFER_SIZE 1024
#define CONTROL_IN_REQUEST_TYPE (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE)
#define CONTROL_OUT_REQUEST_TYPE (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE)
#define CONTROL_REQUEST_1 0x01
#define CONTROL_REQUEST_2 0x02

/* Interrupt Endpoint Protocol */
#define INTERRUPT_IN_ENDPOINT 0x82
#define INTERRUPT_OUT_ENDPOINT 0x02
#define INTERRUPT_IN_MAX_TRANSFER_SIZE 1024
#define INTERRUPT_OUT_MAX_TRANSFER_SIZE 1024


/* Description: This function uses synchronous bulk transfers to write message to the device */
/* Parameters:  IN device_handle - USB device handle                                         */
/*              IN host_data_out - Host buffer with the message to send                      */
/*              IN host_data_out_len - Host buffer length                                    */
/*              OUT host_data_out_bytes_sent - Actual buffer size transferred                */
/* Returns:     Zero on success, libusb_error code on failure                                */
int bulk_transfer_write_message(IN libusb_device_handle * device_handle, IN unsigned char * host_data_out, IN unsigned int host_data_out_len, OUT int * host_data_out_bytes_sent)
{
    int result = 0;

    *host_data_out_bytes_sent = 0;

    /* Write message to the device */
    result = libusb_bulk_transfer(device_handle, BULK_OUT_ENDPOINT, host_data_out, host_data_out_len, host_data_out_bytes_sent, TIMEOUT_MS);
    if (result >= 0)
    {
        printf("Message sent via bulk transfer:\n");
        print_data(host_data_out, *host_data_out_bytes_sent);
    }
    else
    {
        fprintf(stderr, "Error sending message via bulk transfer (%s)\n", libusb_error_name(result));
    }

    return result;
}


/* Description: This function uses synchronous bulk transfers to receive message from the device */
/* Parameters:  IN device_handle - USB device handle                                             */
/*              OUT host_data_in - Host buffer to save the message                               */
/*              OUT host_data_in_bytes_received - Actual buffer size received                    */
/* Returns:     Zero on success, libusb_error code on failure                                    */
int bulk_transfer_read_message(IN libusb_device_handle * device_handle, OUT unsigned char * host_data_in, OUT int * host_data_in_bytes_received)
{
    int result = 0;

    *host_data_in_bytes_received = 0;

    /* Read message from the device */
    result = libusb_bulk_transfer(device_handle, BULK_IN_ENDPOINT, host_data_in, BULK_IN_MAX_TRANSFER_SIZE, host_data_in_bytes_received, TIMEOUT_MS);
    if (result >= 0)
    {
        if (*host_data_in_bytes_received > 0)
        {
            printf("Message received via bulk transfer:\n");
            print_data(host_data_in, *host_data_in_bytes_received);
        }
        else
        {
            fprintf(stderr, "No message received via bulk transfer (%d)\n", result);
            return LIBUSB_ERROR_OTHER;
        }
    }
    else
    {
        fprintf(stderr, "Error receiving message via bulk transfer (%s)\n", libusb_error_name(result));
    }

    return result;
}



/* Description: This function uses synchronous bulk transfers to write data to the device and receive data from the device */
/* Parameters:  IN device_handle - USB device handle                                                                       */
/*              IN host_data_out - Host buffer with the message to send                                                    */
/*              IN host_data_out_len - Host buffer length                                                                  */
/*              OUT host_data_out_bytes_sent - Actual buffer size transferred                                              */
/*              OUT host_data_in - Host buffer to save the message                                                         */
/*              OUT host_data_in_bytes_received - Actual buffer size received                                              */
/* Returns:     Zero on success, libusb_error code on failure                                                              */
int bulk_transfer_exchange_message(IN libusb_device_handle *device_handle, IN unsigned char * host_data_out, IN unsigned int host_data_out_len, OUT int * host_data_out_bytes_sent, OUT unsigned char * host_data_in, OUT int * host_data_in_bytes_received)
{
    int result = 0;

    *host_data_out_bytes_sent = 0;
    *host_data_in_bytes_received = 0;

    /* Write data to the device */
    result = libusb_bulk_transfer(device_handle, BULK_OUT_ENDPOINT, host_data_out, host_data_out_len, host_data_out_bytes_sent, TIMEOUT_MS);
    if (result >= 0)
    {
        printf("Message sent via bulk transfer:\n");
        print_data(host_data_out, *host_data_out_bytes_sent);

        /* Read data from the device */
        result = libusb_bulk_transfer(device_handle, BULK_IN_ENDPOINT, host_data_in, BULK_IN_MAX_TRANSFER_SIZE, host_data_in_bytes_received, TIMEOUT_MS);
        if (result >= 0)
        {
            if (*host_data_in_bytes_received > 0)
            {
                printf("Message received via bulk transfer:\n");
                print_data(host_data_in, *host_data_in_bytes_received);
            }
            else
            {
                fprintf(stderr, "No message received via bulk transfer (%d)\n", result);
                result = LIBUSB_ERROR_OTHER;
            }
        }
        else
        {
            fprintf(stderr, "Error receiving message via bulk transfer (%s)\n", libusb_error_name(result));
        }
    }
    else
    {
        fprintf(stderr, "Error sending message via bulk transfer (%s)\n", libusb_error_name(result));
    }

    return result;
}


/* Description: This function uses synchronous control transfers to write data to the device */
/* Parameters:  IN device_handle - USB device handle                                         */
/*              IN host_data_out - Host buffer with the message to send                      */
/*              IN host_data_out_len - Host buffer length                                    */
/*              OUT host_data_out_bytes_sent - Actual buffer size transferred                */
/* Returns:     Zero on success, libusb_error code on failure                                */
int control_transfer_write_message(IN libusb_device_handle *device_handle, IN unsigned char * host_data_out, IN unsigned int host_data_out_len, OUT int * host_data_out_bytes_sent)
{
    int result = 0;

    *host_data_out_bytes_sent = 0;

    /* Write data to the device */
    result = libusb_control_transfer(device_handle, CONTROL_OUT_REQUEST_TYPE, CONTROL_REQUEST_1, 0, INTERFACE_NUMBER, host_data_out, host_data_out_len, TIMEOUT_MS);
    if (result >= 0)
    {
        *host_data_out_bytes_sent = result;
        printf("Message sent via control transfer:\n");
        print_data(host_data_out, *host_data_out_bytes_sent);
    }
    else
    {
        fprintf(stderr, "Error sending data via control transfer (%s)\n", libusb_error_name(result));
    }

    return result;
}


/* Description: This function uses synchronous control transfers to receive data from the device */
/* Parameters:  IN device_handle - USB device handle                                             */
/*              OUT host_data_in - Host buffer to save the message                               */
/*              OUT host_data_in_bytes_received - Actual buffer size received                    */
/* Returns:     Zero on success, libusb_error code on failure                                    */
int control_transfer_read_message(IN libusb_device_handle *device_handle, OUT unsigned char * host_data_in, OUT int * host_data_in_bytes_received)
{
    int result = 0;

    *host_data_in_bytes_received = 0;

    /* Read message from the device */
    result = libusb_control_transfer(device_handle, CONTROL_IN_REQUEST_TYPE, CONTROL_REQUEST_2, 0, INTERFACE_NUMBER, host_data_in, CONTROL_IN_MAX_TRANSFER_SIZE, TIMEOUT_MS);
    if (result >= 0)
    {
        *host_data_in_bytes_received = result;
        printf("Message received via control transfer:\n");
        print_data(host_data_in, *host_data_in_bytes_received);
    }
    else
    {
        fprintf(stderr, "Error receiving message via control transfer (%s)\n", libusb_error_name(result));
    }

    return result;
}


/* Description: This function uses synchronous control transfers to write data to the device and receive data from the device */
/* Parameters:  IN device_handle - USB device handle                                                                          */
/*              IN host_data_out - Host buffer with the message to send                                                       */
/*              IN host_data_out_len - Host buffer length                                                                     */
/*              OUT host_data_out_bytes_sent - Actual buffer size transferred                                                 */
/*              OUT host_data_in - Host buffer to save the message                                                            */
/*              OUT host_data_in_bytes_received - Actual buffer size received                                                 */
/* Returns:     Zero on success, libusb_error code on failure                                                                 */
int control_transfer_exchange_message(IN libusb_device_handle *device_handle, IN unsigned char * host_data_out, IN unsigned int host_data_out_len, OUT int * host_data_out_bytes_sent, OUT unsigned char * host_data_in, OUT int * host_data_in_bytes_received)
{
    int result = 0;

    *host_data_out_bytes_sent = 0;
    *host_data_in_bytes_received = 0;

    /* Send message to the device */
    result = libusb_control_transfer(device_handle, CONTROL_OUT_REQUEST_TYPE, CONTROL_REQUEST_1, 0, INTERFACE_NUMBER, host_data_out, host_data_out_len, TIMEOUT_MS);
    if (result >= 0)
    {
        *host_data_out_bytes_sent = result;
        printf("Message sent via control transfer:\n");
        print_data(host_data_out, *host_data_out_bytes_sent);

        /* Read message from the device */
        result = libusb_control_transfer(device_handle, CONTROL_IN_REQUEST_TYPE, CONTROL_REQUEST_2, 0, INTERFACE_NUMBER, host_data_in, CONTROL_IN_MAX_TRANSFER_SIZE, TIMEOUT_MS);
        if (result >= 0)
        {
            *host_data_in_bytes_received = result;
            printf("Message received via control transfer:\n");
            print_data(host_data_in, *host_data_in_bytes_received);
        }
        else
        {
            fprintf(stderr, "Error receiving message via control transfer (%s)\n", libusb_error_name(result));
        }
    }
    else
    {
        fprintf(stderr, "Error sending data via control transfer (%s)\n", libusb_error_name(result));
    }

    return result;
}



/* Description: This function uses synchronous interrupt transfers to write data to the device */
/* Parameters:  IN device_handle - USB device handle                                           */
/*              IN host_data_out - Host buffer with the message to send                        */
/*              IN host_data_out_len - Host buffer length                                      */
/*              OUT host_data_out_bytes_sent - Actual buffer size transferred                  */
/* Returns:     Zero on success, libusb_error code on failure                                  */
int interrupt_transfer_write_message(IN libusb_device_handle *device_handle, IN unsigned char * host_data_out, IN unsigned int host_data_out_len, OUT int * host_data_out_bytes_sent)
{
    int result = 0;

    *host_data_out_bytes_sent = 0;

    /* Send message to the device */
    result = libusb_interrupt_transfer(device_handle, INTERRUPT_OUT_ENDPOINT, host_data_out, host_data_out_len, host_data_out_bytes_sent, TIMEOUT_MS);
    if (result >= 0)
    {
        printf("Message sent via interrupt transfer:\n");
        print_data(host_data_out, *host_data_out_bytes_sent);
    }
    else
    {
        fprintf(stderr, "Error sending message via interrupt transfer (%s)\n", libusb_error_name(result));
    }

    return result;
}


/* Description: This function uses synchronous interrupt transfers to receive data from the device */
/* Parameters:  IN device_handle - USB device handle                                               */
/*              OUT host_data_in - Host buffer to save the message                                 */
/*              OUT host_data_in_bytes_received - Actual buffer size received                      */
/* Returns:     Zero on success, libusb_error code on failure                                      */
int interrupt_transfer_read_message(IN libusb_device_handle *device_handle, OUT unsigned char * host_data_in, OUT int * host_data_in_bytes_received)
{
    int result = 0;

    *host_data_in_bytes_received = 0;

    /* Read message from the device */
    result = libusb_interrupt_transfer(device_handle, INTERRUPT_IN_ENDPOINT, host_data_in, INTERRUPT_OUT_MAX_TRANSFER_SIZE, host_data_in_bytes_received, TIMEOUT_MS);
    if (result >= 0)
    {
        if (*host_data_in_bytes_received > 0)
        {
            printf("Message received via interrupt transfer:\n");
            print_data(host_data_in, *host_data_in_bytes_received);
        }
        else
        {
            fprintf(stderr, "No message received in interrupt transfer (%s)\n", libusb_error_name(result));
            result = LIBUSB_ERROR_OTHER;
        }
    }
    else
    {
        fprintf(stderr, "Error receiving message via interrupt transfer (%s)\n", libusb_error_name(result));
    }

    return result;
}


/* Description: This function uses synchronous interrupt transfers to write data to the device and receive data from the device */
/* Parameters:  IN device_handle - USB device handle                                                                            */
/*              IN host_data_out - Host buffer with the message to send                                                         */
/*              IN host_data_out_len - Host buffer length                                                                       */
/*              OUT host_data_out_bytes_sent - Actual buffer size transferred                                                   */
/*              OUT host_data_in - Host buffer to save the message                                                              */
/*              OUT host_data_in_bytes_received - Actual buffer size received                                                   */
/* Returns:     Zero on success, libusb_error code on failure                                                                   */
int interrupt_transfer_exchange_message(IN libusb_device_handle *device_handle, IN unsigned char * host_data_out, IN unsigned int host_data_out_len, OUT int * host_data_out_bytes_sent, OUT unsigned char * host_data_in, OUT int * host_data_in_bytes_received)
{
    int result = 0;

    *host_data_out_bytes_sent = 0;
    *host_data_in_bytes_received = 0;

    /* Send message to the device */
    result = libusb_interrupt_transfer(device_handle, INTERRUPT_OUT_ENDPOINT, host_data_out, host_data_out_len, host_data_out_bytes_sent, TIMEOUT_MS);
    if (result >= 0)
    {
        printf("Message sent via interrupt transfer:\n");
        print_data(host_data_out, *host_data_out_bytes_sent);

        /* Read message from the device */
        result = libusb_interrupt_transfer(device_handle, INTERRUPT_IN_ENDPOINT, host_data_in, INTERRUPT_OUT_MAX_TRANSFER_SIZE, host_data_in_bytes_received, TIMEOUT_MS);
        if (result >= 0)
        {
            if (*host_data_in_bytes_received > 0)
            {
                printf("Message received via interrupt transfer:\n");
                print_data(host_data_in, *host_data_in_bytes_received);
            }
            else
            {
                fprintf(stderr, "No message received in interrupt transfer (%s)\n", libusb_error_name(result));
                result = LIBUSB_ERROR_OTHER;
            }
        }
        else
        {
            fprintf(stderr, "Error receiving message via interrupt transfer (%s)\n", libusb_error_name(result));
        }
    }
    else
    {
        fprintf(stderr, "Error sending message via interrupt transfer (%s)\n", libusb_error_name(result));
    }

    return result;
}

