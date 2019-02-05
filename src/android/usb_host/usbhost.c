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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

// #define DEBUG 1
#if DEBUG

#ifdef USE_LIBLOG
#define LOG_TAG "usbhost"
#include "log/log.h"
#define D ALOGD
#else
#define D printf
#endif

#else
#define D(...)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stddef.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/inotify.h>
#include <dirent.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include <poll.h>
#include <pthread.h>

#include <linux/usbdevice_fs.h>
#include <asm/byteorder.h>

#include "usbhost.h"

#define DEV_DIR             "/dev"
#define DEV_BUS_DIR         DEV_DIR "/bus"
#define USB_FS_DIR          DEV_BUS_DIR "/usb"
#define USB_FS_ID_SCANNER   USB_FS_DIR "/%d/%d"
#define USB_FS_ID_FORMAT    USB_FS_DIR "/%03d/%03d"

// Some devices fail to send string descriptors if we attempt reading > 255 bytes
#define MAX_STRING_DESCRIPTOR_LENGTH    255

#define MAX_USBFS_WD_COUNT      10

struct usb_host_context {
    int                         fd;
    usb_device_added_cb         cb_added;
    usb_device_removed_cb       cb_removed;
    void                        *data;
    int                         wds[MAX_USBFS_WD_COUNT];
    int                         wdd;
    int                         wddbus;
};

#define MAX_DESCRIPTORS_LENGTH 4096

struct usb_device {
    char dev_name[64];
    unsigned char desc[MAX_DESCRIPTORS_LENGTH];
    int desc_length;
    int fd;
    int writeable;
};

static inline int badname(const char *name)
{
    while(*name) {
        if(!isdigit(*name++)) return 1;
    }
    return 0;
}

static int find_existing_devices_bus(char *busname,
                                     usb_device_added_cb added_cb,
                                     void *client_data)
{
    char devname[32];
    DIR *devdir;
    struct dirent *de;
    int done = 0;

    devdir = opendir(busname);
    if(devdir == 0) return 0;

    while ((de = readdir(devdir)) && !done) {
        if(badname(de->d_name)) continue;

        snprintf(devname, sizeof(devname), "%s/%s", busname, de->d_name);
        done = added_cb(devname, client_data);
    } // end of devdir while
    closedir(devdir);

    return done;
}

/* returns true if one of the callbacks indicates we are done */
static int find_existing_devices(usb_device_added_cb added_cb,
                                 void *client_data)
{
    char busname[32];
    DIR *busdir;
    struct dirent *de;
    int done = 0;

    busdir = opendir(USB_FS_DIR);
    if(busdir == 0) return 0;

    while ((de = readdir(busdir)) != 0 && !done) {
        if(badname(de->d_name)) continue;

        snprintf(busname, sizeof(busname), USB_FS_DIR "/%s", de->d_name);
        done = find_existing_devices_bus(busname, added_cb,
                                         client_data);
    } //end of busdir while
    closedir(busdir);

    return done;
}

static void watch_existing_subdirs(struct usb_host_context *context,
                                   int *wds, int wd_count)
{
    char path[100];
    int i, ret;

    wds[0] = inotify_add_watch(context->fd, USB_FS_DIR, IN_CREATE | IN_DELETE);
    if (wds[0] < 0)
        return;

    /* watch existing subdirectories of USB_FS_DIR */
    for (i = 1; i < wd_count; i++) {
        snprintf(path, sizeof(path), USB_FS_DIR "/%03d", i);
        ret = inotify_add_watch(context->fd, path, IN_CREATE | IN_DELETE);
        if (ret >= 0)
            wds[i] = ret;
    }
}

struct usb_host_context *usb_host_init()
{
    struct usb_host_context *context = calloc(1, sizeof(struct usb_host_context));
    if (!context) {
        fprintf(stderr, "out of memory in usb_host_context\n");
        return NULL;
    }
    context->fd = inotify_init();
    if (context->fd < 0) {
        fprintf(stderr, "inotify_init failed\n");
        free(context);
        return NULL;
    }
    return context;
}

void usb_host_cleanup(struct usb_host_context *context)
{
    close(context->fd);
    free(context);
}

int usb_host_get_fd(struct usb_host_context *context)
{
    return context->fd;
} /* usb_host_get_fd() */

int usb_host_load(struct usb_host_context *context,
                  usb_device_added_cb added_cb,
                  usb_device_removed_cb removed_cb,
                  usb_discovery_done_cb discovery_done_cb,
                  void *client_data)
{
    int done = 0;
    int i;

    context->cb_added = added_cb;
    context->cb_removed = removed_cb;
    context->data = client_data;

    D("Created device discovery thread\n");

    /* watch for files added and deleted within USB_FS_DIR */
    context->wddbus = -1;
    for (i = 0; i < MAX_USBFS_WD_COUNT; i++)
        context->wds[i] = -1;

    /* watch the root for new subdirectories */
    context->wdd = inotify_add_watch(context->fd, DEV_DIR, IN_CREATE | IN_DELETE);
    if (context->wdd < 0) {
        fprintf(stderr, "inotify_add_watch failed\n");
        if (discovery_done_cb)
            discovery_done_cb(client_data);
        return done;
    }

    watch_existing_subdirs(context, context->wds, MAX_USBFS_WD_COUNT);

    /* check for existing devices first, after we have inotify set up */
    done = find_existing_devices(added_cb, client_data);
    if (discovery_done_cb)
        done |= discovery_done_cb(client_data);

    return done;
} /* usb_host_load() */

int usb_host_read_event(struct usb_host_context *context)
{
    struct inotify_event* event;
    char event_buf[512];
    char path[100];
    int i, ret, done = 0;
    int offset = 0;
    int wd;

    ret = read(context->fd, event_buf, sizeof(event_buf));
    if (ret >= (int)sizeof(struct inotify_event)) {
        while (offset < ret && !done) {
            event = (struct inotify_event*)&event_buf[offset];
            done = 0;
            wd = event->wd;
            if (wd == context->wdd) {
                if ((event->mask & IN_CREATE) && !strcmp(event->name, "bus")) {
                    context->wddbus = inotify_add_watch(context->fd, DEV_BUS_DIR, IN_CREATE | IN_DELETE);
                    if (context->wddbus < 0) {
                        done = 1;
                    } else {
                        watch_existing_subdirs(context, context->wds, MAX_USBFS_WD_COUNT);
                        done = find_existing_devices(context->cb_added, context->data);
                    }
                }
            } else if (wd == context->wddbus) {
                if ((event->mask & IN_CREATE) && !strcmp(event->name, "usb")) {
                    watch_existing_subdirs(context, context->wds, MAX_USBFS_WD_COUNT);
                    done = find_existing_devices(context->cb_added, context->data);
                } else if ((event->mask & IN_DELETE) && !strcmp(event->name, "usb")) {
                    for (i = 0; i < MAX_USBFS_WD_COUNT; i++) {
                        if (context->wds[i] >= 0) {
                            inotify_rm_watch(context->fd, context->wds[i]);
                            context->wds[i] = -1;
                        }
                    }
                }
            } else if (wd == context->wds[0]) {
                i = atoi(event->name);
                snprintf(path, sizeof(path), USB_FS_DIR "/%s", event->name);
                D("%s subdirectory %s: index: %d\n", (event->mask & IN_CREATE) ?
                                                     "new" : "gone", path, i);
                if (i > 0 && i < MAX_USBFS_WD_COUNT) {
                    int local_ret = 0;
                    if (event->mask & IN_CREATE) {
                        local_ret = inotify_add_watch(context->fd, path,
                                                      IN_CREATE | IN_DELETE);
                        if (local_ret >= 0)
                            context->wds[i] = local_ret;
                        done = find_existing_devices_bus(path, context->cb_added,
                                                         context->data);
                    } else if (event->mask & IN_DELETE) {
                        inotify_rm_watch(context->fd, context->wds[i]);
                        context->wds[i] = -1;
                    }
                }
            } else {
                for (i = 1; (i < MAX_USBFS_WD_COUNT) && !done; i++) {
                    if (wd == context->wds[i]) {
                        snprintf(path, sizeof(path), USB_FS_DIR "/%03d/%s", i, event->name);
                        if (event->mask == IN_CREATE) {
                            D("new device %s\n", path);
                            done = context->cb_added(path, context->data);
                        } else if (event->mask == IN_DELETE) {
                            D("gone device %s\n", path);
                            done = context->cb_removed(path, context->data);
                        }
                    }
                }
            }

            offset += sizeof(struct inotify_event) + event->len;
        }
    }

    return done;
} /* usb_host_read_event() */

void usb_host_run(struct usb_host_context *context,
                  usb_device_added_cb added_cb,
                  usb_device_removed_cb removed_cb,
                  usb_discovery_done_cb discovery_done_cb,
                  void *client_data)
{
    int done;

    done = usb_host_load(context, added_cb, removed_cb, discovery_done_cb, client_data);

    while (!done) {

        done = usb_host_read_event(context);
    }
} /* usb_host_run() */

struct usb_device *usb_device_open(const char *dev_name)
{
    int fd, attempts, writeable = 1;
    const int SLEEP_BETWEEN_ATTEMPTS_US = 100000; /* 100 ms */
    const int64_t MAX_ATTEMPTS = 10;              /* 1s */
    D("usb_device_open %s\n", dev_name);

    /* Hack around waiting for permissions to be set on the USB device node.
     * Should really be a timeout instead of attempt count, and should REALLY
     * be triggered by the perm change via inotify rather than polling.
     */
    for (attempts = 0; attempts < MAX_ATTEMPTS; ++attempts) {
        if (access(dev_name, R_OK | W_OK) == 0) {
            writeable = 1;
            break;
        } else {
            if (access(dev_name, R_OK) == 0) {
                /* double check that write permission didn't just come along too! */
                writeable = (access(dev_name, R_OK | W_OK) == 0);
                break;
            }
        }
        /* not writeable or readable - sleep and try again. */
        D("usb_device_open no access sleeping\n");
        usleep(SLEEP_BETWEEN_ATTEMPTS_US);
    }

    if (writeable) {
        fd = open(dev_name, O_RDWR);
    } else {
        fd = open(dev_name, O_RDONLY);
    }
    D("usb_device_open open returned %d writeable %d errno %d\n", fd, writeable, errno);
    if (fd < 0) return NULL;

    struct usb_device* result = usb_device_new(dev_name, fd);
    if (result)
        result->writeable = writeable;
    return result;
}

void usb_device_close(struct usb_device *device)
{
    close(device->fd);
    free(device);
}

struct usb_device *usb_device_new(const char *dev_name, int fd)
{
    struct usb_device *device = calloc(1, sizeof(struct usb_device));
    int length;

    D("usb_device_new %s fd: %d\n", dev_name, fd);

    if (lseek(fd, 0, SEEK_SET) != 0)
        goto failed;
    length = read(fd, device->desc, sizeof(device->desc));
    D("usb_device_new read returned %d errno %d\n", length, errno);
    if (length < 0)
        goto failed;

    strncpy(device->dev_name, dev_name, sizeof(device->dev_name) - 1);
    device->fd = fd;
    device->desc_length = length;
    // assume we are writeable, since usb_device_get_fd will only return writeable fds
    device->writeable = 1;
    return device;

    failed:
    // TODO It would be more appropriate to have callers do this
    // since this function doesn't "own" this file descriptor.
    close(fd);
    free(device);
    return NULL;
}

static int usb_device_reopen_writeable(struct usb_device *device)
{
    if (device->writeable)
        return 1;

    int fd = open(device->dev_name, O_RDWR);
    if (fd >= 0) {
        close(device->fd);
        device->fd = fd;
        device->writeable = 1;
        return 1;
    }
    D("usb_device_reopen_writeable failed errno %d\n", errno);
    return 0;
}

int usb_device_get_fd(struct usb_device *device)
{
    if (!usb_device_reopen_writeable(device))
        return -1;
    return device->fd;
}

const char* usb_device_get_name(struct usb_device *device)
{
    return device->dev_name;
}

int usb_device_get_unique_id(struct usb_device *device)
{
    int bus = 0, dev = 0;
    sscanf(device->dev_name, USB_FS_ID_SCANNER, &bus, &dev);
    return bus * 1000 + dev;
}

int usb_device_get_unique_id_from_name(const char* name)
{
    int bus = 0, dev = 0;
    sscanf(name, USB_FS_ID_SCANNER, &bus, &dev);
    return bus * 1000 + dev;
}

char* usb_device_get_name_from_unique_id(int id)
{
    int bus = id / 1000;
    int dev = id % 1000;
    char* result = (char *)calloc(1, strlen(USB_FS_ID_FORMAT));
    snprintf(result, strlen(USB_FS_ID_FORMAT) - 1, USB_FS_ID_FORMAT, bus, dev);
    return result;
}

uint16_t usb_device_get_vendor_id(struct usb_device *device)
{
    struct usb_device_descriptor* desc = (struct usb_device_descriptor*)device->desc;
    return __le16_to_cpu(desc->idVendor);
}

uint16_t usb_device_get_product_id(struct usb_device *device)
{
    struct usb_device_descriptor* desc = (struct usb_device_descriptor*)device->desc;
    return __le16_to_cpu(desc->idProduct);
}

const struct usb_device_descriptor* usb_device_get_device_descriptor(struct usb_device* device) {
    return (struct usb_device_descriptor*)device->desc;
}

size_t usb_device_get_descriptors_length(const struct usb_device* device) {
    return device->desc_length;
}

const unsigned char* usb_device_get_raw_descriptors(const struct usb_device* device) {
    return device->desc;
}

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
                               size_t* response_size) {
    __u16 languages[MAX_STRING_DESCRIPTOR_LENGTH / sizeof(__u16)];
    char response[MAX_STRING_DESCRIPTOR_LENGTH];
    int result;
    int languageCount = 0;

    if (id == 0) return -1;
    if (*ucs2_out != NULL) return -1;

    memset(languages, 0, sizeof(languages));

    // read list of supported languages
    result = usb_device_control_transfer(device,
                                         USB_DIR_IN|USB_TYPE_STANDARD|USB_RECIP_DEVICE, USB_REQ_GET_DESCRIPTOR,
                                         (USB_DT_STRING << 8) | 0, 0, languages, sizeof(languages),
                                         timeout);
    if (result > 0)
        languageCount = (result - 2) / 2;

    for (int i = 1; i <= languageCount; i++) {
        memset(response, 0, sizeof(response));

        result = usb_device_control_transfer(
                device, USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE, USB_REQ_GET_DESCRIPTOR,
                (USB_DT_STRING << 8) | id, languages[i], response, sizeof(response), timeout);
        if (result >= 2) {  // string contents begin at offset 2.
            int descriptor_len = result - 2;
            char* out = malloc(descriptor_len + 3);
            if (out == NULL) {
                return -1;
            }
            memcpy(out, response + 2, descriptor_len);
            // trail with three additional NULLs, so that there's guaranteed
            // to be a UCS-2 NULL character beyond whatever USB returned.
            // The returned string length is still just what USB returned.
            memset(out + descriptor_len, '\0', 3);
            *ucs2_out = (void*)out;
            *response_size = descriptor_len;
            return 0;
        }
    }
    return -1;
}

/* Warning: previously this blindly returned the lower 8 bits of
 * every UCS-2 character in a USB descriptor.  Now it will replace
 * values > 127 with ascii '?'.
 */
char* usb_device_get_string(struct usb_device* device, int id, int timeout) {
    char* ascii_string = NULL;
    size_t raw_string_len = 0;
    size_t i;
    if (usb_device_get_string_ucs2(device, id, timeout, (void**)&ascii_string, &raw_string_len) < 0)
        return NULL;
    if (ascii_string == NULL) return NULL;
    for (i = 0; i < raw_string_len / 2; ++i) {
        // wire format for USB is always little-endian.
        char lower = ascii_string[2 * i];
        char upper = ascii_string[2 * i + 1];
        if (upper || (lower & 0x80)) {
            ascii_string[i] = '?';
        } else {
            ascii_string[i] = lower;
        }
    }
    ascii_string[i] = '\0';
    return ascii_string;
}

char* usb_device_get_manufacturer_name(struct usb_device *device, int timeout)
{
    struct usb_device_descriptor *desc = (struct usb_device_descriptor *)device->desc;
    return usb_device_get_string(device, desc->iManufacturer, timeout);
}

char* usb_device_get_product_name(struct usb_device *device, int timeout)
{
    struct usb_device_descriptor *desc = (struct usb_device_descriptor *)device->desc;
    return usb_device_get_string(device, desc->iProduct, timeout);
}

int usb_device_get_version(struct usb_device *device)
{
    struct usb_device_descriptor *desc = (struct usb_device_descriptor *)device->desc;
    return desc->bcdUSB;
}

char* usb_device_get_serial(struct usb_device *device, int timeout)
{
    struct usb_device_descriptor *desc = (struct usb_device_descriptor *)device->desc;
    return usb_device_get_string(device, desc->iSerialNumber, timeout);
}

int usb_device_is_writeable(struct usb_device *device)
{
    return device->writeable;
}

void usb_descriptor_iter_init(struct usb_device *device, struct usb_descriptor_iter *iter)
{
    iter->config = device->desc;
    iter->config_end = device->desc + device->desc_length;
    iter->curr_desc = device->desc;
}

struct usb_descriptor_header *usb_descriptor_iter_next(struct usb_descriptor_iter *iter)
{
    struct usb_descriptor_header* next;
    if (iter->curr_desc >= iter->config_end)
        return NULL;
    next = (struct usb_descriptor_header*)iter->curr_desc;
    iter->curr_desc += next->bLength;
    return next;
}

int usb_device_claim_interface(struct usb_device *device, unsigned int interface)
{
    return ioctl(device->fd, USBDEVFS_CLAIMINTERFACE, &interface);
}

int usb_device_release_interface(struct usb_device *device, unsigned int interface)
{
    return ioctl(device->fd, USBDEVFS_RELEASEINTERFACE, &interface);
}

int usb_device_connect_kernel_driver(struct usb_device *device,
                                     unsigned int interface, int connect)
{
    struct usbdevfs_ioctl ctl;

    ctl.ifno = interface;
    ctl.ioctl_code = (connect ? USBDEVFS_CONNECT : USBDEVFS_DISCONNECT);
    ctl.data = NULL;
    return ioctl(device->fd, USBDEVFS_IOCTL, &ctl);
}

int usb_device_set_configuration(struct usb_device *device, int configuration)
{
    return ioctl(device->fd, USBDEVFS_SETCONFIGURATION, &configuration);
}

int usb_device_set_interface(struct usb_device *device, unsigned int interface,
                             unsigned int alt_setting)
{
    struct usbdevfs_setinterface ctl;

    ctl.interface = interface;
    ctl.altsetting = alt_setting;
    return ioctl(device->fd, USBDEVFS_SETINTERFACE, &ctl);
}

int usb_device_control_transfer(struct usb_device *device,
                                int requestType,
                                int request,
                                int value,
                                int index,
                                void* buffer,
                                int length,
                                unsigned int timeout)
{
    struct usbdevfs_ctrltransfer  ctrl;

    // this usually requires read/write permission
    if (!usb_device_reopen_writeable(device))
        return -1;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.bRequestType = requestType;
    ctrl.bRequest = request;
    ctrl.wValue = value;
    ctrl.wIndex = index;
    ctrl.wLength = length;
    ctrl.data = buffer;
    ctrl.timeout = timeout;
    return ioctl(device->fd, USBDEVFS_CONTROL, &ctrl);
}

int usb_device_bulk_transfer(struct usb_device *device,
                             int endpoint,
                             void* buffer,
                             unsigned int length,
                             unsigned int timeout)
{
    struct usbdevfs_bulktransfer  ctrl;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.ep = endpoint;
    ctrl.len = length;
    ctrl.data = buffer;
    ctrl.timeout = timeout;
    return ioctl(device->fd, USBDEVFS_BULK, &ctrl);
}

int usb_device_reset(struct usb_device *device)
{
    return ioctl(device->fd, USBDEVFS_RESET);
}

struct usb_request *usb_request_new(struct usb_device *dev,
                                    const struct usb_endpoint_descriptor *ep_desc)
{
    struct usbdevfs_urb *urb = calloc(1, sizeof(struct usbdevfs_urb));
    if (!urb)
        return NULL;

    if ((ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK)
        urb->type = USBDEVFS_URB_TYPE_BULK;
    else if ((ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_INT)
        urb->type = USBDEVFS_URB_TYPE_INTERRUPT;
    else {
        D("Unsupported endpoint type %d", ep_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK);
        free(urb);
        return NULL;
    }
    urb->endpoint = ep_desc->bEndpointAddress;

    struct usb_request *req = calloc(1, sizeof(struct usb_request));
    if (!req) {
        free(urb);
        return NULL;
    }

    req->dev = dev;
    req->max_packet_size = __le16_to_cpu(ep_desc->wMaxPacketSize);
    req->private_data = urb;
    req->endpoint = urb->endpoint;
    urb->usercontext = req;

    return req;
}

void usb_request_free(struct usb_request *req)
{
    free(req->private_data);
    free(req);
}

int usb_request_queue(struct usb_request *req)
{
    struct usbdevfs_urb *urb = (struct usbdevfs_urb*)req->private_data;
    int res;

    urb->status = -1;
    urb->buffer = req->buffer;
    urb->buffer_length = req->buffer_length;

    do {
        res = ioctl(req->dev->fd, USBDEVFS_SUBMITURB, urb);
    } while((res < 0) && (errno == EINTR));

    return res;
}

struct usb_request *usb_request_wait(struct usb_device *dev, int timeoutMillis)
{
    // Poll until a request becomes available if there is a timeout
    if (timeoutMillis > 0) {
        struct pollfd p = {.fd = dev->fd, .events = POLLOUT, .revents = 0};

        int res = poll(&p, 1, timeoutMillis);

        if (res != 1 || p.revents != POLLOUT) {
            D("[ poll - event %d, error %d]\n", p.revents, errno);
            return NULL;
        }
    }

    // Read the request. This should usually succeed as we polled before, but it can fail e.g. when
    // two threads are reading usb requests at the same time and only a single request is available.
    struct usbdevfs_urb *urb = NULL;
    int res = TEMP_FAILURE_RETRY(ioctl(dev->fd, timeoutMillis == -1 ? USBDEVFS_REAPURB :
                                                USBDEVFS_REAPURBNDELAY, &urb));
    D("%s returned %d\n", timeoutMillis == -1 ? "USBDEVFS_REAPURB" : "USBDEVFS_REAPURBNDELAY", res);

    if (res < 0) {
        D("[ reap urb - error %d]\n", errno);
        return NULL;
    } else {
        D("[ urb @%p status = %d, actual = %d ]\n", urb, urb->status, urb->actual_length);

        struct usb_request *req = (struct usb_request*)urb->usercontext;
        req->actual_length = urb->actual_length;

        return req;
    }
}

int usb_request_cancel(struct usb_request *req)
{
    struct usbdevfs_urb *urb = ((struct usbdevfs_urb*)req->private_data);
    return ioctl(req->dev->fd, USBDEVFS_DISCARDURB, urb);
}