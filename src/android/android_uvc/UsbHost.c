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

#define DEBUG 1
#if DEBUG

//#ifdef USE_LIBLOG
//#define LOG_TAG "usbhost"
//#include "utils/Log.h"
//#define D ALOGD
//#else
//#define D printf
//#endif

#include <android/log.h>

#define D(...)  __android_log_print(ANDROID_LOG_INFO, "LIBUSBHOST", __VA_ARGS__)

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

// From drivers/usb/core/devio.c
// I don't know why this isn't in a kernel header
#define MAX_USBFS_BUFFER_SIZE   8*1024*1024

#define MAX_USBFS_WD_COUNT      10

struct usb_host_context {
    int fd;
    usb_device_added_cb cb_added;
    usb_device_removed_cb cb_removed;
    void *data;
    int wds[MAX_USBFS_WD_COUNT];
    int wdd;
    int wddbus;
};

static inline int badname(const char *name) {
    while (*name) {
        if (!isdigit(*name++)) return 1;
    }
    return 0;
}

static int find_existing_devices_bus(char *busname,
                                     usb_device_added_cb added_cb,
                                     void *client_data) {
    char devname[32];
    DIR *devdir;
    struct dirent *de;
    int done = 0;

    devdir = opendir(busname);
    if (devdir == 0) return 0;

    while ((de = readdir(devdir)) && !done) {
        if (badname(de->d_name)) continue;

        snprintf(devname, sizeof(devname), "%s/%s", busname, de->d_name);
        done = added_cb(devname, client_data);
    } // end of devdir while
    closedir(devdir);

    return done;
}

/* returns true if one of the callbacks indicates we are done */
static int find_existing_devices(usb_device_added_cb added_cb,
                                 void *client_data) {
    char busname[32];
    DIR *busdir;
    struct dirent *de;
    int done = 0;

    busdir = opendir(USB_FS_DIR);
    if (busdir == 0) return 0;

    while ((de = readdir(busdir)) != 0 && !done) {
        if (badname(de->d_name)) continue;

        snprintf(busname, sizeof(busname), USB_FS_DIR "/%s", de->d_name);
        done = find_existing_devices_bus(busname, added_cb,
                                         client_data);
    } //end of busdir while
    closedir(busdir);

    return done;
}

static void watch_existing_subdirs(struct usb_host_context *context,
                                   int *wds, int wd_count) {
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

struct usb_host_context *usb_host_init() {
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

void usb_host_cleanup(struct usb_host_context *context) {
    close(context->fd);
    free(context);
}

int usb_host_get_fd(struct usb_host_context *context) {
    return context->fd;
} /* usb_host_get_fd() */

int usb_host_load(struct usb_host_context *context,
                  usb_device_added_cb added_cb,
                  usb_device_removed_cb removed_cb,
                  usb_discovery_done_cb discovery_done_cb,
                  void *client_data) {
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

int usb_host_read_event(struct usb_host_context *context) {
    struct inotify_event *event;
    char event_buf[512];
    char path[100];
    int i, ret, done = 0;
    int offset = 0;
    int wd;

    ret = read(context->fd, event_buf, sizeof(event_buf));
    if (ret >= (int) sizeof(struct inotify_event)) {
        while (offset < ret && !done) {
            event = (struct inotify_event *) &event_buf[offset];
            done = 0;
            wd = event->wd;
            if (wd == context->wdd) {
                if ((event->mask & IN_CREATE) && !strcmp(event->name, "bus")) {
                    context->wddbus = inotify_add_watch(context->fd, DEV_BUS_DIR,
                                                        IN_CREATE | IN_DELETE);
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
                  void *client_data) {
    int done;

    done = usb_host_load(context, added_cb, removed_cb, discovery_done_cb, client_data);

    while (!done) {

        done = usb_host_read_event(context);
    }
} /* usb_host_run() */

struct usb_device_handle *usb_device_open(const char *dev_name) {
    int fd, did_retry = 0, writeable = 1;

    D("usb_device_open %s\n", dev_name);

    retry:
    fd = open(dev_name, O_RDWR);
    if (fd < 0) {
        /* if we fail, see if have read-only access */
        fd = open(dev_name, O_RDONLY);
        D("usb_device_open open returned %d errno %d\n", fd, errno);
        if (fd < 0 && (errno == EACCES || errno == ENOENT) && !did_retry) {
            /* work around race condition between inotify and permissions management */
            sleep(1);
            did_retry = 1;
            goto retry;
        }

        if (fd < 0)
            return NULL;
        writeable = 0;
        D("[ usb open read-only %s fd = %d]\n", dev_name, fd);
    }

    struct usb_device_handle *result = usb_device_new(dev_name, fd);
    if (result)
        result->writeable = writeable;
    return result;
}

void usb_device_close(struct usb_device_handle *device) {
    close(device->fd);
    free(device);
}

struct usb_device_handle *usb_device_new(const char *dev_name, int fd) {
    struct usb_device_handle *device = calloc(1, sizeof(struct usb_device_handle));
    int length;

    D("usb_device_new %s fd: %d\n", dev_name, fd);

    if (lseek(fd, 0, SEEK_SET) != 0)
        goto failed;
    D("usb_device_new read. errno before read: %d\n", errno);
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
    close(fd);
    free(device);
    return NULL;
}

static int usb_device_reopen_writeable(struct usb_device_handle *device) {
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

int usb_device_get_fd(struct usb_device_handle *device) {
    if (!usb_device_reopen_writeable(device))
        return -1;
    return device->fd;
}

const char *usb_device_get_name(struct usb_device_handle *device) {
    return device->dev_name;
}

int usb_device_get_unique_id(struct usb_device_handle *device) {
    int bus = 0, dev = 0;
    sscanf(device->dev_name, USB_FS_ID_SCANNER, &bus, &dev);
    return bus * 1000 + dev;
}

int usb_device_get_unique_id_from_name(const char *name) {
    int bus = 0, dev = 0;
    sscanf(name, USB_FS_ID_SCANNER, &bus, &dev);
    return bus * 1000 + dev;
}

char *usb_device_get_name_from_unique_id(int id) {
    int bus = id / 1000;
    int dev = id % 1000;
    char *result = (char *) calloc(1, strlen(USB_FS_ID_FORMAT));
    snprintf(result, strlen(USB_FS_ID_FORMAT) - 1, USB_FS_ID_FORMAT, bus, dev);
    return result;
}

uint16_t usb_device_get_vendor_id(struct usb_device_handle *device) {
    struct usb_device_descriptor *desc = (struct usb_device_descriptor *) device->desc;
    return __le16_to_cpu(desc->idVendor);
}

uint16_t usb_device_get_product_id(struct usb_device_handle *device) {
    struct usb_device_descriptor *desc = (struct usb_device_descriptor *) device->desc;
    return __le16_to_cpu(desc->idProduct);
}

const struct usb_device_descriptor *
usb_device_get_device_descriptor(struct usb_device_handle *device) {
    return (struct usb_device_descriptor *) device->desc;
}

char *usb_device_get_string(struct usb_device_handle *device, int id) {
    char string[256];
    __u16 buffer[MAX_STRING_DESCRIPTOR_LENGTH / sizeof(__u16)];
    __u16 languages[MAX_STRING_DESCRIPTOR_LENGTH / sizeof(__u16)];
    int i, result;
    int languageCount = 0;

    if (id == 0) return NULL;

    string[0] = 0;
    memset(languages, 0, sizeof(languages));

    // read list of supported languages
    result = usb_device_control_transfer(device,
                                         USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
                                         USB_REQ_GET_DESCRIPTOR,
                                         (USB_DT_STRING << 8) | 0, 0, languages, sizeof(languages),
                                         0);
    if (result > 0)
        languageCount = (result - 2) / 2;

    for (i = 1; i <= languageCount; i++) {
        memset(buffer, 0, sizeof(buffer));

        result = usb_device_control_transfer(device,
                                             USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE,
                                             USB_REQ_GET_DESCRIPTOR,
                                             (USB_DT_STRING << 8) | id, languages[i], buffer,
                                             sizeof(buffer), 0);
        if (result > 0) {
            int i;
            // skip first word, and copy the rest to the string, changing shorts to bytes.
            result /= 2;
            for (i = 1; i < result; i++)
                string[i - 1] = buffer[i];
            string[i - 1] = 0;
            return strdup(string);
        }
    }

    return NULL;
}

char *usb_device_get_manufacturer_name(struct usb_device_handle *device) {
    struct usb_device_descriptor *desc = (struct usb_device_descriptor *) device->desc;
    return usb_device_get_string(device, desc->iManufacturer);
}

char *usb_device_get_product_name(struct usb_device_handle *device) {
    struct usb_device_descriptor *desc = (struct usb_device_descriptor *) device->desc;
    return usb_device_get_string(device, desc->iProduct);
}

int usb_device_get_version(struct usb_device_handle *device) {
    struct usb_device_descriptor *desc = (struct usb_device_descriptor *) device->desc;
    return desc->bcdUSB;
}

char *usb_device_get_serial(struct usb_device_handle *device) {
    struct usb_device_descriptor *desc = (struct usb_device_descriptor *) device->desc;
    return usb_device_get_string(device, desc->iSerialNumber);
}

int usb_device_is_writeable(struct usb_device_handle *device) {
    return device->writeable;
}

void usb_descriptor_iter_init(struct usb_device_handle *device, struct usb_descriptor_iter *iter) {
    iter->config = device->desc;
    iter->config_end = device->desc + device->desc_length;
    iter->curr_desc = device->desc;
}

struct usb_descriptor_header *usb_descriptor_iter_next(struct usb_descriptor_iter *iter) {
    struct usb_descriptor_header *next;
    if (iter->curr_desc >= iter->config_end)
        return NULL;
    next = (struct usb_descriptor_header *) iter->curr_desc;
    iter->curr_desc += next->bLength;
    return next;
}

int usb_device_claim_interface(struct usb_device_handle *device, unsigned int interface) {
    int ret = ioctl(device->fd, USBDEVFS_CLAIMINTERFACE, &interface);
    if (ret < 0) {
        usb_device_connect_kernel_driver(device, interface, 0);
        ret = ioctl(device->fd, USBDEVFS_CLAIMINTERFACE, &interface);
    }
    return ret;
}

int usb_device_release_interface(struct usb_device_handle *device, unsigned int interface) {
    return ioctl(device->fd, USBDEVFS_RELEASEINTERFACE, &interface);
}

int usb_device_connect_kernel_driver(struct usb_device_handle *device,
                                     unsigned int interface, int connect) {
    struct usbdevfs_ioctl ctl;

    ctl.ifno = interface;
    ctl.ioctl_code = (connect ? USBDEVFS_CONNECT : USBDEVFS_DISCONNECT);
    ctl.data = NULL;
    return ioctl(device->fd, USBDEVFS_IOCTL, &ctl);
}

int usb_device_set_configuration(struct usb_device_handle *device, int configuration) {
    return ioctl(device->fd, USBDEVFS_SETCONFIGURATION, &configuration);
}

int usb_device_set_interface(struct usb_device_handle *device, unsigned int interface,
                             unsigned int alt_setting) {
    struct usbdevfs_setinterface ctl;

    ctl.interface = interface;
    ctl.altsetting = alt_setting;
    return ioctl(device->fd, USBDEVFS_SETINTERFACE, &ctl);
}

int usb_device_control_transfer(struct usb_device_handle *device,
                                int requestType,
                                int request,
                                int value,
                                int index,
                                void *buffer,
                                int length,
                                unsigned int timeout) {
    struct usbdevfs_ctrltransfer ctrl;

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

int usb_device_bulk_transfer(struct usb_device_handle *device,
                             int endpoint,
                             void *buffer,
                             unsigned long length,
                             unsigned int timeout) {
    struct usbdevfs_bulktransfer ctrl;

    // need to limit request size to avoid EINVAL
    if (length > MAX_USBFS_BUFFER_SIZE)
        length = MAX_USBFS_BUFFER_SIZE;

    memset(&ctrl, 0, sizeof(ctrl));
    ctrl.ep = endpoint;
    ctrl.len = length;
    ctrl.data = buffer;
    ctrl.timeout = timeout;
    return ioctl(device->fd, USBDEVFS_BULK, &ctrl);
}

int usb_device_reset(struct usb_device_handle *device) {
    return ioctl(device->fd, USBDEVFS_RESET);
}

int usb_endpoint_reset(struct usb_device_handle *device , uint8_t endpoint){
    return ioctl(device->fd, USBDEVFS_CLEAR_HALT,endpoint);
}

struct usb_request *usb_request_new(struct usb_device_handle *dev,
                                    const struct usb_endpoint_descriptor *ep_desc) {
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
    req->max_packet_size = MAX_USBFS_BUFFER_SIZE;//__le16_to_cpu(ep_desc->wMaxPacketSize);
    req->private_data = urb;
    req->endpoint = urb->endpoint;
    urb->usercontext = req;

    return req;
}

struct usb_request *usb_request_reset(struct usb_request *req, const struct usb_endpoint_descriptor *ep_desc) {
    if (!req) {
        return NULL;
    }
    struct usbdevfs_urb *urb = req->private_data;
    memset(urb,0,sizeof(struct usbdevfs_urb));
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
    req->actual_length = 0;
    return req;
}

void usb_request_free(struct usb_request *req) {
    free(req->private_data);
    free(req);
}

int usb_request_queue(struct usb_request *req) {
    struct usbdevfs_urb *urb = (struct usbdevfs_urb *) req->private_data;
    int res;

    urb->status = -1;
    urb->buffer = req->buffer;
    // need to limit request size to avoid EINVAL
    if (req->buffer_length > MAX_USBFS_BUFFER_SIZE)
        urb->buffer_length = MAX_USBFS_BUFFER_SIZE;
    else
        urb->buffer_length = req->buffer_length;

    do {
        res = ioctl(req->dev->fd, USBDEVFS_SUBMITURB, urb);
        if(res<0)
        D("Unable to queue request - %s",strerror(errno));
    } while ((res < 0) && (errno == EINTR));

    return res;
}

struct usb_request *usb_request_wait(struct usb_device_handle *dev) {
    struct usbdevfs_urb *urb = NULL;
    struct usb_request *req = NULL;

    while (1) {
        int res = ioctl(dev->fd, USBDEVFS_REAPURB, &urb);
        //D("USBDEVFS_REAPURB returned %d\n", res);
        if (res < 0) {
            if (errno == EINTR) {
                continue;
            }
            //D("[ reap urb - error ]\n");
            return NULL;
        } else {
            //D("[ urb @%p status = %s, actual = %d ]\n",urb, strerror(urb->status), urb->actual_length);
            req = (struct usb_request *) urb->usercontext;
            req->actual_length = urb->actual_length;
        }
        break;
    }
    return req;
}

int usb_request_cancel(struct usb_request *req) {
    struct usbdevfs_urb *urb = ((struct usbdevfs_urb *) req->private_data);
    return ioctl(req->dev->fd, USBDEVFS_DISCARDURB, urb);
}

