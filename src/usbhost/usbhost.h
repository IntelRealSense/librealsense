/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __USB_HOST_H
#define __USB_HOST_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#include <linux/version.h>
#if LINUX_VERSION_CODE > KERNEL_VERSION(2, 6, 20)
#include <linux/usb/ch9.h>
#else
#include <linux/usb_ch9.h>
#endif

struct usb_host_context;
struct usb_endpoint_descriptor;

struct usb_descriptor_iter {
    unsigned char*  config;
    unsigned char*  config_end;
    unsigned char*  curr_desc;
};

struct usb_request
{
    struct usb_device *dev;
    void* buffer;
    int buffer_length;
    int actual_length;
    int max_packet_size;
    void *private_data; /* struct usbdevfs_urb* */
    int endpoint;
    void *client_data;  /* free for use by client */
};

/* Callback for notification when new USB devices are attached.
 * Return true to exit from usb_host_run.
 */
typedef int (* usb_device_added_cb)(const char *dev_name, void *client_data);

/* Callback for notification when USB devices are removed.
 * Return true to exit from usb_host_run.
 */
typedef int (* usb_device_removed_cb)(const char *dev_name, void *client_data);

/* Callback indicating that initial device discovery is done.
 * Return true to exit from usb_host_run.
 */
typedef int (* usb_discovery_done_cb)(void *client_data);

/* Call this to initialize the USB host library. */
struct usb_host_context *usb_host_init(void);

/* Call this to cleanup the USB host library. */
void usb_host_cleanup(struct usb_host_context *context);

/* Call this to get the inotify file descriptor. */
int usb_host_get_fd(struct usb_host_context *context);

/* Call this to initialize the usb host context. */
int usb_host_load(struct usb_host_context *context,
                  usb_device_added_cb added_cb,
                  usb_device_removed_cb removed_cb,
                  usb_discovery_done_cb discovery_done_cb,
                  void *client_data);

/* Call this to read and handle occuring usb event. */
int usb_host_read_event(struct usb_host_context *context);

/* Call this to monitor the USB bus for new and removed devices.
 * This is intended to be called from a dedicated thread,
 * as it will not return until one of the callbacks returns true.
 * added_cb will be called immediately for each existing USB device,
 * and subsequently each time a new device is added.
 * removed_cb is called when USB devices are removed from the bus.
 * discovery_done_cb is called after the initial discovery of already
 * connected devices is complete.
 */
void usb_host_run(struct usb_host_context *context,
                  usb_device_added_cb added_cb,
                  usb_device_removed_cb removed_cb,
                  usb_discovery_done_cb discovery_done_cb,
                  void *client_data);

/* Creates a usb_device object for a USB device */
struct usb_device *usb_device_open(const char *dev_name);

/* Releases all resources associated with the USB device */
void usb_device_close(struct usb_device *device);

/* Creates a usb_device object for already open USB device */
struct usb_device *usb_device_new(const char *dev_name, int fd);

/* Returns the file descriptor for the usb_device */
int usb_device_get_fd(struct usb_device *device);

/* Returns the name for the USB device, which is the same as
 * the dev_name passed to usb_device_open()
 */
const char* usb_device_get_name(struct usb_device *device);

/* Returns a unique ID for the device.
 *Currently this is generated from the dev_name path.
 */
int usb_device_get_unique_id(struct usb_device *device);

/* Returns a unique ID for the device name.
 * Currently this is generated from the device path.
 */
int usb_device_get_unique_id_from_name(const char* name);

/* Returns the device name for the unique ID.
 * Call free() to deallocate the returned string */
char* usb_device_get_name_from_unique_id(int id);

/* Returns the USB vendor ID from the device descriptor for the USB device */
uint16_t usb_device_get_vendor_id(struct usb_device *device);

/* Returns the USB product ID from the device descriptor for the USB device */
uint16_t usb_device_get_product_id(struct usb_device *device);

/* Returns a pointer to device descriptor */
const struct usb_device_descriptor* usb_device_get_device_descriptor(struct usb_device *device);

/* Returns a USB descriptor string for the given string ID.
 * Return value: < 0 on error.  0 on success.
 * The string is returned in ucs2_out in USB-native UCS-2 encoding.
 *
 * parameters:
 *  id - the string descriptor index.
 *  timeout - in milliseconds (see Documentation/driver-api/usb/usb.rst)
 *  ucs2_out - Must point to null on call.
 *             Will be filled in with a buffer on success.
 *             If this is non-null on return, it must be free()d.
 *  response_size - size, in bytes, of ucs-2 string in ucs2_out.
 *                  The size isn't guaranteed to include null termination.
 * Call free() to free the result when you are done with it.
 */
int usb_device_get_string_ucs2(struct usb_device* device, int id, int timeout, void** ucs2_out,
                               size_t* response_size);

/* Returns the length in bytes read into the raw descriptors array */
size_t usb_device_get_descriptors_length(const struct usb_device* device);

/* Returns a pointer to the raw descriptors array */
const unsigned char* usb_device_get_raw_descriptors(const struct usb_device* device);

/* Returns a USB descriptor string for the given string ID.
 * Used to implement usb_device_get_manufacturer_name,
 * usb_device_get_product_name and usb_device_get_serial.
 * Returns ascii - non ascii characters will be replaced with '?'.
 * Call free() to free the result when you are done with it.
 */
char* usb_device_get_string(struct usb_device *device, int id, int timeout);

/* Returns the manufacturer name for the USB device.
 * Call free() to free the result when you are done with it.
 */
char* usb_device_get_manufacturer_name(struct usb_device *device, int timeout);

/* Returns the product name for the USB device.
 * Call free() to free the result when you are done with it.
 */
char* usb_device_get_product_name(struct usb_device *device, int timeout);

/* Returns the version number for the USB device.
 */
int usb_device_get_version(struct usb_device *device);

/* Returns the USB serial number for the USB device.
 * Call free() to free the result when you are done with it.
 */
char* usb_device_get_serial(struct usb_device *device, int timeout);

/* Returns true if we have write access to the USB device,
 * and false if we only have access to the USB device configuration.
 */
int usb_device_is_writeable(struct usb_device *device);

/* Initializes a usb_descriptor_iter, which can be used to iterate through all
 * the USB descriptors for a USB device.
 */
void usb_descriptor_iter_init(struct usb_device *device, struct usb_descriptor_iter *iter);

/* Returns the next USB descriptor for a device, or NULL if we have reached the
 * end of the list.
 */
struct usb_descriptor_header *usb_descriptor_iter_next(struct usb_descriptor_iter *iter);

/* Claims the specified interface of a USB device */
int usb_device_claim_interface(struct usb_device *device, unsigned int interface);

/* Releases the specified interface of a USB device */
int usb_device_release_interface(struct usb_device *device, unsigned int interface);

/* Requests the kernel to connect or disconnect its driver for the specified interface.
 * This can be used to ask the kernel to disconnect its driver for a device
 * so usb_device_claim_interface can claim it instead.
 */
int usb_device_connect_kernel_driver(struct usb_device *device,
                                     unsigned int interface, int connect);

/* Sets the current configuration for the device to the specified configuration */
int usb_device_set_configuration(struct usb_device *device, int configuration);

/* Sets the specified interface of a USB device */
int usb_device_set_interface(struct usb_device *device, unsigned int interface,
                             unsigned int alt_setting);

/* Sends a control message to the specified device on endpoint zero */
int usb_device_control_transfer(struct usb_device *device,
                                int requestType,
                                int request,
                                int value,
                                int index,
                                void* buffer,
                                int length,
                                unsigned int timeout);

/* Reads or writes on a bulk endpoint.
 * Returns number of bytes transferred, or negative value for error.
 */
int usb_device_bulk_transfer(struct usb_device *device,
                             int endpoint,
                             void* buffer,
                             unsigned int length,
                             unsigned int timeout);

/** Reset USB bus for the device */
int usb_device_reset(struct usb_device *device);

/* Creates a new usb_request. */
struct usb_request *usb_request_new(struct usb_device *dev,
                                    const struct usb_endpoint_descriptor *ep_desc);

/* Releases all resources associated with the request */
void usb_request_free(struct usb_request *req);

/* Submits a read or write request on the specified device */
int usb_request_queue(struct usb_request *req);

/* Waits for the results of a previous usb_request_queue operation. timeoutMillis == -1 requests
 * to wait forever.
 * Returns a usb_request, or NULL for error.
 */
struct usb_request *usb_request_wait(struct usb_device *dev, int timeoutMillis);

/* Cancels a pending usb_request_queue() operation. */
int usb_request_cancel(struct usb_request *req);

#ifdef __cplusplus
}
#endif
#endif /* __USB_HOST_H */
