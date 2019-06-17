// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_ANDROID_BACKEND

#include "android_uvc.h"
#include "../device_watcher.h"

#include "../../concurrency.h"
#include "../../types.h"
#include "../../libuvc/uvc_types.h"
#include "../../libuvc/utlist.h"
#include "../../usb/usb-types.h"
#include "../../usb/usb-device.h"

#include <vector>
#include <thread>
#include <atomic>
#include <zconf.h>

#define DEQUEUE_TIMEOUT 50
#define STREAMING_BULK_TRANSFER_TIMEOUT 1000

// Data structures for Backend-Frontend queue:
struct frame;
// We keep no more then 2 frames in between frontend and backend
typedef librealsense::small_heap<frame, 2> frames_archive;

struct frame {
    frame() {}

    // We don't want the frames to be overwritten at any point
    // (deallocate resets item to "default" that we want to avoid)
    frame(const frame &other) {}

    frame(frame &&other) {}

    frame &operator=(const frame &other) { return *this; }

    std::vector<uint8_t> pixels;
    librealsense::platform::frame_object fo;
    frames_archive *owner; // Keep pointer to owner for light-deleter
};

void cleanup_frame(frame *ptr) {
    if (ptr) ptr->owner->deallocate(ptr);
};

typedef void(*cleanup_ptr)(frame *);

// Unique_ptr is used as the simplest RAII, with static deleter
typedef std::unique_ptr<frame, cleanup_ptr> frame_ptr;
typedef single_consumer_queue<frame_ptr> frames_queue;

uvc_error_t usbhost_uvc_scan_streaming(usbhost_uvc_device *dev, usbhost_uvc_device_info_t *info,
                                       int interface_idx);

uvc_error_t usbhost_uvc_parse_vs(usbhost_uvc_device *dev, usbhost_uvc_device_info_t *info,
                                 usbhost_uvc_streaming_interface_t *stream_if,
                                 const unsigned char *block, size_t block_size);

void usbhost_uvc_free_device_info(usbhost_uvc_device_info_t *info) {
    uvc_input_terminal_t *input_term, *input_term_tmp;
    uvc_processing_unit_t *proc_unit, *proc_unit_tmp;
    uvc_extension_unit_t *ext_unit, *ext_unit_tmp;

    usbhost_uvc_streaming_interface_t *stream_if, *stream_if_tmp;
    uvc_format_desc_t *format, *format_tmp;
    uvc_frame_desc_t *frame, *frame_tmp;

    DL_FOREACH_SAFE(info->ctrl_if.input_term_descs, input_term, input_term_tmp) {
        DL_DELETE(info->ctrl_if.input_term_descs, input_term);
        free(input_term);
    }

    DL_FOREACH_SAFE(info->ctrl_if.processing_unit_descs, proc_unit, proc_unit_tmp) {
        DL_DELETE(info->ctrl_if.processing_unit_descs, proc_unit);
        free(proc_unit);
    }

    DL_FOREACH_SAFE(info->ctrl_if.extension_unit_descs, ext_unit, ext_unit_tmp) {
        DL_DELETE(info->ctrl_if.extension_unit_descs, ext_unit);
        free(ext_unit);
    }

    DL_FOREACH_SAFE(info->stream_ifs, stream_if, stream_if_tmp) {
        DL_FOREACH_SAFE(stream_if->format_descs, format, format_tmp) {
            DL_FOREACH_SAFE(format->frame_descs, frame, frame_tmp) {
                if (frame->intervals)
                    free(frame->intervals);

                DL_DELETE(format->frame_descs, frame);
                free(frame);
            }

            DL_DELETE(stream_if->format_descs, format);
            free(format);
        }

        DL_DELETE(info->stream_ifs, stream_if);
        free(stream_if);
    }

    if (info->interfaces) {
        for (int i = 0; i < MAX_USB_INTERFACES; i++) {
            if (info->interfaces->iface[i].extra != NULL) {
                //free(info->interfaces->iface[i].extra);
                info->interfaces->iface[i].extra = NULL;
            }
        }

        free(info->interfaces);
    }
}


/** @internal
* @brief Parse a VideoControl header.
* @ingroup device
*/
uvc_error_t uvc_parse_vc_header(usbhost_uvc_device *dev,
                                usbhost_uvc_device_info_t *info,
                                const unsigned char *block, size_t block_size) {
    size_t i;
    uvc_error_t scan_ret, ret = UVC_SUCCESS;


    info->ctrl_if.bcdUVC = SW_TO_SHORT(&block[3]);

    switch (info->ctrl_if.bcdUVC) {
        case 0x0100:
            info->ctrl_if.dwClockFrequency = DW_TO_INT(block + 7);
        case 0x010a:
            info->ctrl_if.dwClockFrequency = DW_TO_INT(block + 7);
        case 0x0110:
        case 0x0150:
            info->ctrl_if.dwClockFrequency = 0;
            break;
        default:
            return UVC_ERROR_NOT_SUPPORTED;
    }

    for (i = 12; i < block_size; ++i) {
        scan_ret = usbhost_uvc_scan_streaming(dev, info, block[i]);
        if (scan_ret != UVC_SUCCESS) {
            ret = scan_ret;
            break;
        }
    }

    return ret;
}

/** @internal
* @brief Parse a VideoControl input terminal.
* @ingroup device
*/
uvc_error_t uvc_parse_vc_input_terminal(usbhost_uvc_device *dev,
                                        usbhost_uvc_device_info_t *info,
                                        const unsigned char *block, size_t block_size) {
    uvc_input_terminal_t *term;
    size_t i;

    /* only supporting camera-type input terminals */
    if (SW_TO_SHORT(&block[4]) != UVC_ITT_CAMERA) {
        return UVC_SUCCESS;
    }

    term = (uvc_input_terminal_t *) calloc(1, sizeof(*term));

    term->bTerminalID = block[3];
    term->wTerminalType = (uvc_it_type) SW_TO_SHORT(&block[4]);
    term->wObjectiveFocalLengthMin = SW_TO_SHORT(&block[8]);
    term->wObjectiveFocalLengthMax = SW_TO_SHORT(&block[10]);
    term->wOcularFocalLength = SW_TO_SHORT(&block[12]);

    for (i = 14 + block[14]; i >= 15; --i)
        term->bmControls = block[i] + (term->bmControls << 8);

    DL_APPEND(info->ctrl_if.input_term_descs, term);

    return UVC_SUCCESS;
}

/** @internal
* @brief Parse a VideoControl processing unit.
* @ingroup device
*/
uvc_error_t uvc_parse_vc_processing_unit(usbhost_uvc_device *dev,
                                         usbhost_uvc_device_info_t *info,
                                         const unsigned char *block, size_t block_size) {
    uvc_processing_unit_t *unit;
    size_t i;

    unit = (uvc_processing_unit_t *) calloc(1, sizeof(*unit));
    unit->bUnitID = block[3];
    unit->bSourceID = block[4];

    for (i = 7 + block[7]; i >= 8; --i)
        unit->bmControls = block[i] + (unit->bmControls << 8);

    DL_APPEND(info->ctrl_if.processing_unit_descs, unit);

    return UVC_SUCCESS;
}

/** @internal
* @brief Parse a VideoControl selector unit.
* @ingroup device
*/
uvc_error_t uvc_parse_vc_selector_unit(usbhost_uvc_device *dev,
                                       usbhost_uvc_device_info_t *info,
                                       const unsigned char *block, size_t block_size) {
    uvc_selector_unit_t *unit;

    unit = (uvc_selector_unit_t *) calloc(1, sizeof(*unit));
    unit->bUnitID = block[3];

    DL_APPEND(info->ctrl_if.selector_unit_descs, unit);

    return UVC_SUCCESS;
}

/** @internal
* @brief Parse a VideoControl extension unit.
* @ingroup device
*/
uvc_error_t uvc_parse_vc_extension_unit(usbhost_uvc_device *dev,
                                        usbhost_uvc_device_info_t *info,
                                        const unsigned char *block, size_t block_size) {
    uvc_extension_unit_t *unit = (uvc_extension_unit_t *) calloc(1, sizeof(*unit));
    const uint8_t *start_of_controls;
    int size_of_controls, num_in_pins;
    int i;

    unit->bUnitID = block[3];
    memcpy(unit->guidExtensionCode, &block[4], 16);

    num_in_pins = block[21];
    size_of_controls = block[22 + num_in_pins];
    start_of_controls = &block[23 + num_in_pins];

    for (i = size_of_controls - 1; i >= 0; --i)
        unit->bmControls = start_of_controls[i] + (unit->bmControls << 8);

    DL_APPEND(info->ctrl_if.extension_unit_descs, unit);

    return UVC_SUCCESS;
}

/** @internal
* Process a single VideoControl descriptor block
* @ingroup device
*/
uvc_error_t uvc_parse_vc(
        usbhost_uvc_device *dev,
        usbhost_uvc_device_info_t *info,
        const unsigned char *block, size_t block_size) {
    int descriptor_subtype;
    uvc_error_t ret = UVC_SUCCESS;


    if (block[1] != 36) { // not a CS_INTERFACE descriptor??
        return UVC_SUCCESS; // UVC_ERROR_INVALID_DEVICE;
    }

    descriptor_subtype = block[2];

    switch (descriptor_subtype) {
        case UVC_VC_HEADER:
            ret = uvc_parse_vc_header(dev, info, block, block_size);
            break;
        case UVC_VC_INPUT_TERMINAL:
            ret = uvc_parse_vc_input_terminal(dev, info, block, block_size);
            break;
        case UVC_VC_OUTPUT_TERMINAL:
            break;
        case UVC_VC_SELECTOR_UNIT:
            ret = uvc_parse_vc_selector_unit(dev, info, block, block_size);
            break;
        case UVC_VC_PROCESSING_UNIT:
            ret = uvc_parse_vc_processing_unit(dev, info, block, block_size);
            break;
        case UVC_VC_EXTENSION_UNIT:
            ret = uvc_parse_vc_extension_unit(dev, info, block, block_size);
            break;
        default:
            ret = UVC_ERROR_INVALID_DEVICE;
    }

    return ret;
}

uvc_error_t usbhost_uvc_scan_control(usbhost_uvc_device *dev, usbhost_uvc_device_info_t *info,
                                     int InterfaceNumber) {
    usb_interface_descriptor *if_desc;
    uvc_error_t parse_ret, ret;
    int interface_idx;
    const unsigned char *buffer;
    size_t buffer_left, block_size;

    ret = UVC_SUCCESS;
    if_desc = NULL;

    for (interface_idx = 0; interface_idx < MAX_USB_INTERFACES; ++interface_idx) {
        if_desc = &info->interfaces->iface[interface_idx].desc;

        if (if_desc->bInterfaceClass == 14 && if_desc->bInterfaceSubClass == 1 &&
            if_desc->bInterfaceNumber == InterfaceNumber) // Video, Control
            break;
        //std::cout<<"";

        if_desc = NULL;
    }

    if (if_desc == NULL) {
        LOG_DEBUG("Error uvc scan control invalid device!");
        return UVC_ERROR_INVALID_DEVICE;
    }

    info->ctrl_if.bInterfaceNumber = interface_idx;
    if (if_desc->bNumEndpoints != 0) {
        info->ctrl_if.bEndpointAddress = info->interfaces->iface[interface_idx].endpoints[0].bEndpointAddress;
    }

    buffer = info->interfaces->iface[interface_idx].extra;
    buffer_left = info->interfaces->iface[interface_idx].extraLength;

    while (buffer_left >= 3) { // parseX needs to see buf[0,2] = length,type
        block_size = buffer[0];
        //printf("%d %x %d\n", buffer[0], buffer[1],buffer_left);
        parse_ret = uvc_parse_vc(dev, info, buffer, block_size);

        if (parse_ret != UVC_SUCCESS) {
            ret = parse_ret;
            break;
        }

        buffer_left -= block_size;
        buffer += block_size;
    }
    //LOG_DEBUG("usbhost_uvc_scan_control() complete!");
    return ret;
}

/** @internal
* Process a VideoStreaming interface
* @ingroup device
*/
uvc_error_t usbhost_uvc_scan_streaming(usbhost_uvc_device *dev,
                                       usbhost_uvc_device_info_t *info,
                                       int interface_idx) {
    const struct usbhost_uvc_interface *if_desc;
    const unsigned char *buffer;
    int buffer_left, block_size;
    uvc_error_t ret, parse_ret;
    usbhost_uvc_streaming_interface_t *stream_if;


    ret = UVC_SUCCESS;

    if_desc = &(info->interfaces->iface[interface_idx]);
    buffer = if_desc->extra;
    buffer_left = if_desc->extraLength - sizeof(usb_interface_descriptor);

    stream_if = (usbhost_uvc_streaming_interface_t *) calloc(1, sizeof(*stream_if));
    memset(stream_if, 0, sizeof(usbhost_uvc_streaming_interface_t));
    stream_if->parent = info;
    stream_if->bInterfaceNumber = if_desc->desc.bInterfaceNumber;
    DL_APPEND(info->stream_ifs, stream_if);

    while (buffer_left >= 3) {
        block_size = buffer[0];
        parse_ret = usbhost_uvc_parse_vs(dev, info, stream_if, buffer, block_size);

        if (parse_ret != UVC_SUCCESS) {
            ret = parse_ret;
            break;
        }

        buffer_left -= block_size;
        //printf("%d %x %x %d\n", buffer[0], buffer[1], buffer[2], buffer_left);
        buffer += block_size;
    }

    return ret;
}

/** @internal
* @brief Parse a VideoStreaming header block.
* @ingroup device
*/
uvc_error_t usbhost_uvc_parse_vs_input_header(usbhost_uvc_streaming_interface_t *stream_if,
                                              const unsigned char *block,
                                              size_t block_size) {

    stream_if->bEndpointAddress = block[6] & 0x8f;
    stream_if->bTerminalLink = block[8];

    return UVC_SUCCESS;
}

/** @internal
* @brief Parse a VideoStreaming uncompressed format block.
* @ingroup device
*/
uvc_error_t usbhost_uvc_parse_vs_format_uncompressed(usbhost_uvc_streaming_interface_t *stream_if,
                                                     const unsigned char *block,
                                                     size_t block_size) {

    uvc_format_desc_t *format = (uvc_format_desc_t *) calloc(1, sizeof(*format));
    memset(format, 0, sizeof(uvc_format_desc_t));
    format->parent = stream_if;
    format->bDescriptorSubtype = (uvc_vs_desc_subtype) block[2];
    format->bFormatIndex = block[3];
    //format->bmCapabilities = block[4];
    //format->bmFlags = block[5];
    memcpy(format->guidFormat, &block[5], 16);
    format->bBitsPerPixel = block[21];
    format->bDefaultFrameIndex = block[22];
    format->bAspectRatioX = block[23];
    format->bAspectRatioY = block[24];
    format->bmInterlaceFlags = block[25];
    format->bCopyProtect = block[26];

    DL_APPEND(stream_if->format_descs, format);

    return UVC_SUCCESS;
}

/** @internal
* @brief Parse a VideoStreaming frame format block.
* @ingroup device
*/
uvc_error_t usbhost_uvc_parse_vs_frame_format(usbhost_uvc_streaming_interface_t *stream_if,
                                              const unsigned char *block,
                                              size_t block_size) {

    uvc_format_desc_t *format = (uvc_format_desc_t *) malloc(sizeof(uvc_format_desc_t));
    memset(format, 0, sizeof(uvc_format_desc_t));
    format->parent = stream_if;
    format->bDescriptorSubtype = (uvc_vs_desc_subtype) block[2];
    format->bFormatIndex = block[3];
    format->bNumFrameDescriptors = block[4];
    memcpy(format->guidFormat, &block[5], 16);
    format->bBitsPerPixel = block[21];
    format->bDefaultFrameIndex = block[22];
    format->bAspectRatioX = block[23];
    format->bAspectRatioY = block[24];
    format->bmInterlaceFlags = block[25];
    format->bCopyProtect = block[26];
    format->bVariableSize = block[27];

    DL_APPEND(stream_if->format_descs, format);

    return UVC_SUCCESS;
}

/** @internal
* @brief Parse a VideoStreaming MJPEG format block.
* @ingroup device
*/
uvc_error_t usbhost_uvc_parse_vs_format_mjpeg(usbhost_uvc_streaming_interface_t *stream_if,
                                              const unsigned char *block,
                                              size_t block_size) {

    uvc_format_desc_t *format = (uvc_format_desc_t *) calloc(1, sizeof(*format));

    format->parent = stream_if;
    format->bDescriptorSubtype = (uvc_vs_desc_subtype) block[2];
    format->bFormatIndex = block[3];
    memcpy(format->fourccFormat, "MJPG", 4);
    format->bmFlags = block[5];
    format->bBitsPerPixel = 0;
    format->bDefaultFrameIndex = block[6];
    format->bAspectRatioX = block[7];
    format->bAspectRatioY = block[8];
    format->bmInterlaceFlags = block[9];
    format->bCopyProtect = block[10];

    DL_APPEND(stream_if->format_descs, format);

    return UVC_SUCCESS;
}

/** @internal
* @brief Parse a VideoStreaming uncompressed frame block.
* @ingroup device
*/
uvc_error_t usbhost_uvc_parse_vs_frame_frame(usbhost_uvc_streaming_interface_t *stream_if,
                                             const unsigned char *block,
                                             size_t block_size) {
    uvc_format_desc_t *format;
    uvc_frame_desc_t *frame;

    const unsigned char *p;
    int i;

    format = stream_if->format_descs->prev;
    frame = (uvc_frame_desc_t *) malloc(sizeof(uvc_frame_desc_t));
    memset(frame, 0, sizeof(uvc_frame_desc_t));
    frame->parent = format;

    frame->bDescriptorSubtype = (uvc_vs_desc_subtype) block[2];
    frame->bFrameIndex = block[3];
    frame->bmCapabilities = block[4];
    frame->wWidth = block[5] + (block[6] << 8);
    frame->wHeight = block[7] + (block[8] << 8);
    frame->dwMinBitRate = DW_TO_INT(&block[9]);
    frame->dwMaxBitRate = DW_TO_INT(&block[13]);
    frame->dwDefaultFrameInterval = DW_TO_INT(&block[17]);
    frame->bFrameIntervalType = block[21];
    frame->dwBytesPerLine = DW_TO_INT(&block[22]);

    if (block[21] == 0) {
        frame->dwMinFrameInterval = DW_TO_INT(&block[26]);
        frame->dwMaxFrameInterval = DW_TO_INT(&block[30]);
        frame->dwFrameIntervalStep = DW_TO_INT(&block[34]);
    } else {
        frame->intervals = (uint32_t *) malloc((block[21] + 1) * sizeof(uint32_t));
        p = &block[26];

        for (i = 0; i < block[21]; ++i) {
            frame->intervals[i] = DW_TO_INT(p);
            p += 4;
        }
        frame->intervals[block[21]] = 0;
    }

    DL_APPEND(format->frame_descs, frame);

    return UVC_SUCCESS;
}

/** @internal
* @brief Parse a VideoStreaming uncompressed frame block.
* @ingroup device
*/
uvc_error_t usbhost_uvc_parse_vs_frame_uncompressed(usbhost_uvc_streaming_interface_t *stream_if,
                                                    const unsigned char *block,
                                                    size_t block_size) {
    uvc_format_desc_t *format;
    uvc_frame_desc_t *frame;

    const unsigned char *p;
    int i;

    format = stream_if->format_descs->prev;
    frame = (uvc_frame_desc_t *) calloc(1, sizeof(*frame));
    memset(frame, 0, sizeof(uvc_frame_desc_t));
    frame->parent = format;

    frame->bDescriptorSubtype = (uvc_vs_desc_subtype) block[2];
    frame->bFrameIndex = block[3];
    frame->bmCapabilities = block[4];
    frame->wWidth = block[5] + (block[6] << 8);
    frame->wHeight = block[7] + (block[8] << 8);
    frame->dwMinBitRate = DW_TO_INT(&block[9]);
    frame->dwMaxBitRate = DW_TO_INT(&block[13]);
    frame->dwMaxVideoFrameBufferSize = DW_TO_INT(&block[17]);
    frame->dwDefaultFrameInterval = DW_TO_INT(&block[21]);
    frame->bFrameIntervalType = block[25];

    if (block[25] == 0) {
        frame->dwMinFrameInterval = DW_TO_INT(&block[26]);
        frame->dwMaxFrameInterval = DW_TO_INT(&block[30]);
        frame->dwFrameIntervalStep = DW_TO_INT(&block[34]);
    } else {
        frame->intervals = (uint32_t *) calloc(block[25] + 1, sizeof(frame->intervals[0]));
        p = &block[26];

        for (i = 0; i < block[25]; ++i) {
            frame->intervals[i] = DW_TO_INT(p);
            p += 4;
        }
        frame->intervals[block[25]] = 0;
    }

    DL_APPEND(format->frame_descs, frame);

    return UVC_SUCCESS;
}

/** @internal
* Process a single VideoStreaming descriptor block
* @ingroup device
*/
uvc_error_t usbhost_uvc_parse_vs(
        usbhost_uvc_device *dev,
        usbhost_uvc_device_info_t *info,
        usbhost_uvc_streaming_interface_t *stream_if,
        const unsigned char *block, size_t block_size) {
    uvc_error_t ret;
    int descriptor_subtype;

    ret = UVC_SUCCESS;
    descriptor_subtype = block[2];

    switch (descriptor_subtype) {
        case UVC_VS_INPUT_HEADER:
            ret = usbhost_uvc_parse_vs_input_header(stream_if, block, block_size);
            break;
        case UVC_VS_FORMAT_UNCOMPRESSED:
            ret = usbhost_uvc_parse_vs_format_uncompressed(stream_if, block, block_size);
            break;
        case UVC_VS_FORMAT_MJPEG:
            ret = usbhost_uvc_parse_vs_format_mjpeg(stream_if, block, block_size);
            break;
        case UVC_VS_FRAME_UNCOMPRESSED:
        case UVC_VS_FRAME_MJPEG:
            ret = usbhost_uvc_parse_vs_frame_uncompressed(stream_if, block, block_size);
            break;
        case UVC_VS_FORMAT_FRAME_BASED:
            ret = usbhost_uvc_parse_vs_frame_format(stream_if, block, block_size);
            break;
        case UVC_VS_FRAME_FRAME_BASED:
            ret = usbhost_uvc_parse_vs_frame_frame(stream_if, block, block_size);
            break;
        case UVC_VS_COLORFORMAT:
            break;

        default:
            /** @todo handle JPEG and maybe still frames or even DV... */
            /*fprintf(stderr, "unsupported descriptor subtype: %d\n",
            descriptor_subtype);*/
            break;
    }

    return ret;
}

uvc_error_t update_stream_if_handle(usbhost_uvc_device *devh, int interface_idx) {
    usbhost_uvc_streaming_interface_t *stream_if;

    DL_FOREACH(devh->deviceData.stream_ifs, stream_if) {
        if (stream_if->bInterfaceNumber == interface_idx)
            if (!stream_if->interface && interface_idx < MAX_USB_INTERFACES) {
                // usbhost_GetAssociatedInterface returns the associated interface (Video stream interface which is associated to Video control interface)
                // A value of 0 indicates the first associated interface (Video Stream 1), a value of 1 indicates the second associated interface (video stream 2)
                // WinUsbInterfaceNumber is the actual interface number taken from the USB config descriptor
                // A value of 0 indicates the first interface (Video Control Interface), A value of 1 indicates the first associated interface (Video Stream 1)
                // For this reason, when calling to usbhost_GetAssociatedInterface, we must decrease 1 to receive the associated interface
                auto intfs = devh->device->get_interfaces();
                auto it = std::find_if(intfs.begin(), intfs.end(),
                                       [&](const librealsense::platform::rs_usb_interface& i) { return i->get_number() == interface_idx; });
                if (it != intfs.end())
                    stream_if->interface = *it;
            }
    }
    return UVC_SUCCESS;
}

/* Return only Video stream interfaces */
static usbhost_uvc_streaming_interface_t* usbhost_uvc_get_stream_if(usbhost_uvc_device *devh, int interface_idx)
{
    usbhost_uvc_streaming_interface_t *stream_if;

    DL_FOREACH(devh->deviceData.stream_ifs, stream_if)
    {
        if (stream_if->bInterfaceNumber == interface_idx)
        {
            return stream_if;
        }
    }

    return NULL;
}

/** Return all device formats available
* @ingroup streaming
*
* @param[in] devh Device Handle
* @param[out] formats returned
*/

#define SWAP_UINT32(x) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))

uvc_error_t usbhost_get_available_formats(
        usbhost_uvc_device *devh,
        int interface_idx,
        uvc_format_t **formats) {

    usbhost_uvc_streaming_interface_t *stream_if = NULL;
    uvc_format_t *prev_format = NULL;
    uvc_format_desc_t *format;

    stream_if = usbhost_uvc_get_stream_if(devh, interface_idx);
    if (stream_if == NULL) {
        return UVC_ERROR_INVALID_PARAM;
    }

    DL_FOREACH(stream_if->format_descs, format) {
        uint32_t *interval_ptr;
        uvc_frame_desc_t *frame_desc;

        DL_FOREACH(format->frame_descs, frame_desc) {
            for (interval_ptr = frame_desc->intervals;
                 *interval_ptr;
                 ++interval_ptr) {
                uvc_format_t *cur_format = (uvc_format_t *) malloc(sizeof(uvc_format_t));
                cur_format->height = frame_desc->wHeight;
                cur_format->width = frame_desc->wWidth;
                cur_format->fourcc = SWAP_UINT32(*(const uint32_t *) format->guidFormat);

                cur_format->fps = 10000000 / *interval_ptr;
                cur_format->next = NULL;

                if (prev_format != NULL) {
                    prev_format->next = cur_format;
                } else {
                    *formats = cur_format;
                }

                prev_format = cur_format;
            }
        }
    }
    return UVC_SUCCESS;
}

// Return linked list of uvc_format_t of all available formats inside winusb device
uvc_error_t usbhost_get_available_formats_all(usbhost_uvc_device *devh, uvc_format_t **formats) {

    usbhost_uvc_streaming_interface_t *stream_if = NULL;
    uvc_format_t *prev_format = NULL;
    uvc_format_desc_t *format;

    DL_FOREACH(devh->deviceData.stream_ifs, stream_if) {
        DL_FOREACH(stream_if->format_descs, format) {
            uint32_t *interval_ptr;
            uvc_frame_desc_t *frame_desc;

            DL_FOREACH(format->frame_descs, frame_desc) {
                for (interval_ptr = frame_desc->intervals;
                     *interval_ptr;
                     ++interval_ptr) {
                    uvc_format_t *cur_format = (uvc_format_t *) malloc(sizeof(uvc_format_t));
                    cur_format->height = frame_desc->wHeight;
                    cur_format->width = frame_desc->wWidth;
                    auto temp = SWAP_UINT32(*(const uint32_t *) format->guidFormat);
                    cur_format->fourcc = fourcc_map.count(temp) ? fourcc_map.at(temp) : temp;
                    cur_format->interfaceNumber = stream_if->bInterfaceNumber;

                    cur_format->fps = 10000000 / *interval_ptr;
                    cur_format->next = NULL;

                    if (prev_format != NULL) {
                        prev_format->next = cur_format;
                    } else {
                        *formats = cur_format;
                    }

                    prev_format = cur_format;
                }
            }
        }
    }
    return UVC_SUCCESS;
}

uvc_error_t usbhost_free_formats(uvc_format_t *formats) {
    uvc_format_t *cur_format = formats;
    while (cur_format != NULL) {
        uvc_format_t *format = cur_format;
        cur_format = cur_format->next;
        free(format);
    }

    return UVC_SUCCESS;
}

/** @internal
* @brief Find the descriptor for a specific frame configuration
* @param stream_if Stream interface
* @param format_id Index of format class descriptor
* @param frame_id Index of frame descriptor
*/
static uvc_frame_desc_t *
usbhost_uvc_find_frame_desc_stream_if(usbhost_uvc_streaming_interface_t *stream_if,
                                      uint16_t format_id, uint16_t frame_id) {

    uvc_format_desc_t *format = NULL;
    uvc_frame_desc_t *frame = NULL;

    DL_FOREACH(stream_if->format_descs, format) {
        if (format->bFormatIndex == format_id) {
            DL_FOREACH(format->frame_descs, frame) {
                if (frame->bFrameIndex == frame_id)
                    return frame;
            }
        }
    }

    return NULL;
}

uvc_frame_desc_t *usbhost_uvc_find_frame_desc_stream(usbhost_uvc_stream_handle_t *strmh,
                                                     uint16_t format_id, uint16_t frame_id) {
    return usbhost_uvc_find_frame_desc_stream_if(strmh->stream_if, format_id, frame_id);
}


void usbhost_uvc_swap_buffers(usbhost_uvc_stream_handle_t *strmh, size_t leftover = 0) {
    uint8_t *tmp_buf;

    //pthread_mutex_lock(&strmh->cb_mutex);

    /* swap the buffers */
    tmp_buf = strmh->holdbuf;
    strmh->hold_bytes = strmh->got_bytes;
    strmh->holdbuf = strmh->outbuf;
    strmh->outbuf = tmp_buf;
    strmh->hold_last_scr = strmh->last_scr;
    strmh->hold_pts = strmh->pts;
    strmh->hold_seq = strmh->seq;

    //pthread_cond_broadcast(&strmh->cb_cond);
    //pthread_mutex_unlock(&strmh->cb_mutex);

    strmh->seq++;
    strmh->got_bytes = leftover;
    strmh->last_scr = 0;
    strmh->pts = 0;
}

void usbhost_uvc_process_payload(usbhost_uvc_stream_handle_t *strmh,
                                 frames_archive *archive,
                                 frames_queue *queue) {
    uint8_t header_info;
    size_t payload_len = strmh->got_bytes;
    uint8_t *payload = strmh->outbuf;

    /* ignore empty payload transfers */
    if (payload_len == 0)
        return;
    uint8_t header_len = payload[0];

    if (header_len > payload_len)
    {
        LOG_ERROR("bogus packet: actual_len=" << payload_len << ", header_len=" << header_len);
        return;
    }

    size_t data_len = strmh->got_bytes - header_len;

    if (header_len < 2) {
        header_info = 0;
    }
    else {
        /** @todo we should be checking the end-of-header bit */
        size_t variable_offset = 2;

        header_info = payload[1];

        if (header_info & 0x40) {
            LOG_ERROR("bad packet: error bit set");
            return;
        }

        if (strmh->fid != (header_info & 1) && strmh->got_bytes != 0) {
            /* The frame ID bit was flipped, but we have image data sitting
            around from prior transfers. This means the camera didn't send
            an EOF for the last transfer of the previous frame. */
//            LOG_DEBUG("complete buffer : length " << strmh->got_bytes);
            usbhost_uvc_swap_buffers(strmh);
        }

        strmh->fid = header_info & 1;

        if (header_info & (1 << 2)) {
            strmh->pts = DW_TO_INT(payload + variable_offset);
            variable_offset += 4;
        }

        if (header_info & (1 << 3)) {
            /** @todo read the SOF token counter */
            strmh->last_scr = DW_TO_INT(payload + variable_offset);
            variable_offset += 6;
        }
    }

    if ((data_len > 0) && (strmh->cur_ctrl.dwMaxVideoFrameSize == (data_len)))
    {
        //if (header_info & (1 << 1)) { // Temp patch to allow old firmware
        /* The EOF bit is set, so publish the complete frame */
        usbhost_uvc_swap_buffers(strmh);

        auto frame_p = archive->allocate();
        if (frame_p)
        {
            frame_ptr fp(frame_p, &cleanup_frame);

            memcpy(fp->pixels.data(), payload, data_len + header_len);

            LOG_DEBUG("Passing packet to user CB with size " << (data_len + header_len));
            librealsense::platform::frame_object fo{ data_len, header_len,
                                                     fp->pixels.data() + header_len , fp->pixels.data() };
            fp->fo = fo;

            queue->enqueue(std::move(fp));
        }
        else
        {
            LOG_WARNING("usbhost backend is dropping a frame because librealsense wasn't fast enough");
        }
    }
}

void stream_thread(usbhost_uvc_stream_context *strctx) {
    auto inf = strctx->stream->stream_if->interface;
    auto dev = strctx->stream->devh->device;
    auto messenger = dev->open();

    auto read_ep = inf->first_endpoint(librealsense::platform::RS2_USB_ENDPOINT_DIRECTION_READ);
    uint32_t reset_ep_timeout = 100;
    messenger->reset_endpoint(read_ep, reset_ep_timeout);

    frames_archive archive;
    std::atomic_bool keep_sending_callbacks(true);
    frames_queue queue;

    // Get all pointers from archive and initialize their content
    std::vector<frame *> frames;
    for (auto i = 0; i < frames_archive::CAPACITY; i++) {
        auto ptr = archive.allocate();
        ptr->pixels.resize(strctx->maxVideoBufferSize, 0);
        ptr->owner = &archive;
        frames.push_back(ptr);
    }

    for (auto ptr : frames) {
        archive.deallocate(ptr);
    }

    std::thread t([&]() {
        while (keep_sending_callbacks) {
            frame_ptr fp(nullptr, [](frame *) {});
            if (queue.dequeue(&fp, DEQUEUE_TIMEOUT)) {
                strctx->stream->user_cb(&fp->fo, strctx->stream->user_ptr);
            }
        }
    });
    LOG_DEBUG("Transfer thread started for endpoint address: " << strctx->endpoint);
    bool disconnect = false;
    do {
        auto i = strctx->stream->stream_if->interface;
        uint32_t transferred = 0;
        auto sts = messenger->bulk_transfer(read_ep, strctx->stream->outbuf, LIBUVC_XFER_BUF_SIZE, transferred, STREAMING_BULK_TRANSFER_TIMEOUT);
        switch(sts)
        {
            case librealsense::platform::RS2_USB_STATUS_NO_DEVICE:
                disconnect = true;
                break;
            case librealsense::platform::RS2_USB_STATUS_OVERFLOW:
                messenger->reset_endpoint(read_ep, reset_ep_timeout);
                break;
            case librealsense::platform::RS2_USB_STATUS_SUCCESS:
                strctx->stream->got_bytes = transferred;
                usbhost_uvc_process_payload(strctx->stream, &archive, &queue);
                break;
            default:
                break;
        }
    } while (!disconnect && strctx->stream->running);

    int ep = strctx->endpoint;
    messenger->reset_endpoint(read_ep, reset_ep_timeout);
    free(strctx);

    queue.clear();
    archive.stop_allocation();
    archive.wait_until_empty();
    keep_sending_callbacks = false;

    LOG_DEBUG("start join stream_thread");
    t.join();

    LOG_DEBUG("Transfer thread stopped for endpoint address: " << ep);
};

uvc_error_t usbhost_uvc_stream_start(
        usbhost_uvc_stream_handle_t *strmh,
        usbhost_uvc_frame_callback_t *cb,
        void *user_ptr,
        uint8_t flags
) {
    /* USB interface we'll be using */
    usbhost_uvc_interface *iface;
    int interface_id;
    uvc_frame_desc_t *frame_desc;
    uvc_format_desc_t *format_desc;
    uvc_stream_ctrl_t *ctrl;
    uvc_error_t ret;
    /* Total amount of data per transfer */

    ctrl = &strmh->cur_ctrl;

    if (strmh->running) {
        return UVC_ERROR_BUSY;
    }

    strmh->running = 1;
    strmh->seq = 1;
    strmh->fid = 0;
    strmh->pts = 0;
    strmh->last_scr = 0;

    frame_desc = usbhost_uvc_find_frame_desc_stream(strmh, ctrl->bFormatIndex, ctrl->bFrameIndex);
    if (!frame_desc) {
        ret = UVC_ERROR_INVALID_PARAM;
        strmh->running = 0;
        return ret;
    }
    format_desc = frame_desc->parent;

    // Get the interface that provides the chosen format and frame configuration
    interface_id = strmh->stream_if->bInterfaceNumber;
    iface = &strmh->devh->deviceData.interfaces->iface[interface_id];

    usbhost_uvc_stream_context *streamctx = new usbhost_uvc_stream_context;

    //printf("Starting stream on EP = 0x%X, interface 0x%X\n", format_desc->parent->bEndpointAddress, iface);

    streamctx->stream = strmh;
    streamctx->endpoint = format_desc->parent->bEndpointAddress;
    streamctx->iface = iface;
    streamctx->maxPayloadTransferSize = strmh->cur_ctrl.dwMaxPayloadTransferSize;
    streamctx->maxVideoBufferSize = strmh->cur_ctrl.dwMaxVideoFrameSize;
    strmh->user_ptr = user_ptr;
    strmh->cb_thread = std::thread(stream_thread, streamctx);
    strmh->user_cb = cb;

    return UVC_SUCCESS;

}

uvc_error_t usbhost_uvc_stream_stop(usbhost_uvc_stream_handle_t *strmh) {
    if (!strmh->running)
        return UVC_ERROR_INVALID_PARAM;

    strmh->running = false;
    LOG_DEBUG("start join cb_thread");
    strmh->cb_thread.join();
    LOG_DEBUG("cb_thread joined");

    return UVC_SUCCESS;
}

void usbhost_uvc_stream_close(usbhost_uvc_stream_handle_t *strmh) {
    if (strmh->running) {
        usbhost_uvc_stream_stop(strmh);
    }

    free(strmh->outbuf);
    free(strmh->holdbuf);

    DL_DELETE(strmh->devh->streams, strmh);

    free(strmh->user_ptr);
    free(strmh);
}

uvc_error_t usbhost_uvc_stream_open_ctrl(usbhost_uvc_device *devh, usbhost_uvc_stream_handle_t **strmhp,
                             uvc_stream_ctrl_t *ctrl);

uvc_error_t usbhost_start_streaming(usbhost_uvc_device *devh, uvc_stream_ctrl_t *ctrl,
                                    usbhost_uvc_frame_callback_t *cb, void *user_ptr,
                                    uint8_t flags) {
    uvc_error_t ret;
    usbhost_uvc_stream_handle_t *strmh;

    ret = usbhost_uvc_stream_open_ctrl(devh, &strmh, ctrl);
    if (ret != UVC_SUCCESS) {
        return ret;
    }

    ret = usbhost_uvc_stream_start(strmh, cb, user_ptr, flags);
    if (ret != UVC_SUCCESS) {
        usbhost_uvc_stream_close(strmh);
        return ret;
    }

    return UVC_SUCCESS;
}

void usbhost_stop_streaming(usbhost_uvc_device *devh) {
    usbhost_uvc_stream_handle_t *strmh, *strmh_tmp;

    DL_FOREACH_SAFE(devh->streams, strmh, strmh_tmp) {
        usbhost_uvc_stream_close(strmh);
    }
}


/** @internal
* @brief Find the descriptor for a specific frame configuration
* @param devh UVC device
* @param format_id Index of format class descriptor
* @param frame_id Index of frame descriptor
*/
uvc_frame_desc_t *usbhost_uvc_find_frame_desc(usbhost_uvc_device *devh,
                                              uint16_t format_id, uint16_t frame_id) {

    usbhost_uvc_streaming_interface_t *stream_if;
    uvc_frame_desc_t *frame;

    DL_FOREACH(devh->deviceData.stream_ifs, stream_if) {
        frame = usbhost_uvc_find_frame_desc_stream_if(stream_if, format_id, frame_id);
        if (frame)
            return frame;
    }

    return NULL;
}

uvc_error_t
usbhost_uvc_query_stream_ctrl(usbhost_uvc_device *devh, uvc_stream_ctrl_t *ctrl, uint8_t probe,
                              int req) {
    uint8_t buf[48];
    size_t len;
    int err;

    memset(buf, 0, sizeof(buf));

    switch (devh->deviceData.ctrl_if.bcdUVC) {
        case 0x0110:
            len = 34;
            break;
        case 0x0150:
            len = 48;
            break;
        default:
            len = 26;
            break;
    }
    /* prepare for a SET transfer */
    if (req == UVC_SET_CUR) {
        SHORT_TO_SW(ctrl->bmHint, buf);
        buf[2] = ctrl->bFormatIndex;
        buf[3] = ctrl->bFrameIndex;
        INT_TO_DW(ctrl->dwFrameInterval, buf + 4);
        SHORT_TO_SW(ctrl->wKeyFrameRate, buf + 8);
        SHORT_TO_SW(ctrl->wPFrameRate, buf + 10);
        SHORT_TO_SW(ctrl->wCompQuality, buf + 12);
        SHORT_TO_SW(ctrl->wCompWindowSize, buf + 14);
        SHORT_TO_SW(ctrl->wDelay, buf + 16);
        INT_TO_DW(ctrl->dwMaxVideoFrameSize, buf + 18);
        INT_TO_DW(ctrl->dwMaxPayloadTransferSize, buf + 22);
        INT_TO_DW(0, buf + 18);
        INT_TO_DW(0, buf + 22);

        if (len >= 34) {
            INT_TO_DW(ctrl->dwClockFrequency, buf + 26);
            buf[30] = ctrl->bmFramingInfo;
            buf[31] = ctrl->bPreferredVersion;
            buf[32] = ctrl->bMinVersion;
            buf[33] = ctrl->bMaxVersion;
            /** @todo support UVC 1.1 */
        }

        if (len == 48) {
            buf[34] = ctrl->bUsage;
            buf[35] = ctrl->bBitDepthLuma;
            buf[36] = ctrl->bmSettings;
            buf[37] = ctrl->bMaxNumberOfRefFramesPlus1;
            SHORT_TO_SW(ctrl->bmRateControlModes, buf + 38);
            QUAD_TO_QW(ctrl->bmLayoutPerStream, buf + 40);
        }
    }
    int retries = 0;
    do {
        //auto str_if = usbhost_uvc_get_stream_if(devh, ctrl->bInterfaceNumber);
        /* do the transfer */
        err = usb_device_control_transfer(
                devh->device->get_handle(),
                req == UVC_SET_CUR ? UVC_REQ_TYPE_SET : UVC_REQ_TYPE_GET,
                req,
                probe ? (UVC_VS_PROBE_CONTROL << 8) : (UVC_VS_COMMIT_CONTROL << 8),
                ctrl->bInterfaceNumber, // When requestType is directed to an interface, WinUsb driver automatically passes the interface number in the low byte of index
                buf, len, 0);
    } while (err < 0 && retries++ < 5);

    if (err <= 0) {
        LOG_ERROR("Probe-commit control transfer failed with errno: " << errno << " - " << strerror(errno));
        return (uvc_error_t) err;
    }

    /* now decode following a GET transfer */
    if (req != UVC_SET_CUR) {
        ctrl->bmHint = SW_TO_SHORT(buf);
        ctrl->bFormatIndex = buf[2];
        ctrl->bFrameIndex = buf[3];
        ctrl->dwFrameInterval = DW_TO_INT(buf + 4);
        ctrl->wKeyFrameRate = SW_TO_SHORT(buf + 8);
        ctrl->wPFrameRate = SW_TO_SHORT(buf + 10);
        ctrl->wCompQuality = SW_TO_SHORT(buf + 12);
        ctrl->wCompWindowSize = SW_TO_SHORT(buf + 14);
        ctrl->wDelay = SW_TO_SHORT(buf + 16);
        ctrl->dwMaxVideoFrameSize = DW_TO_INT(buf + 18);
        ctrl->dwMaxPayloadTransferSize = DW_TO_INT(buf + 22);

        if (len >= 34) {
            ctrl->dwClockFrequency = DW_TO_INT(buf + 26);
            ctrl->bmFramingInfo = buf[30];
            ctrl->bPreferredVersion = buf[31];
            ctrl->bMinVersion = buf[32];
            ctrl->bMaxVersion = buf[33];
            /** @todo support UVC 1.1 */
        }

        if (len == 48) {
            ctrl->bUsage = buf[34];
            ctrl->bBitDepthLuma = buf[35];
            ctrl->bmSettings = buf[36];
            ctrl->bMaxNumberOfRefFramesPlus1 = buf[37];
            ctrl->bmRateControlModes = DW_TO_INT(buf + 38);
            ctrl->bmLayoutPerStream = QW_TO_QUAD(buf + 40);
        }
        //         else
        //             ctrl->dwClockFrequency = devh->info->ctrl_if.dwClockFrequency;

        /* fix up block for cameras that fail to set dwMax* */
        if (ctrl->dwMaxVideoFrameSize == 0) {
            uvc_frame_desc_t *frame = usbhost_uvc_find_frame_desc(devh, ctrl->bFormatIndex,
                                                                  ctrl->bFrameIndex);

            if (frame) {
                ctrl->dwMaxVideoFrameSize = frame->dwMaxVideoFrameBufferSize;
            }
        }
    }
    return UVC_SUCCESS;
}

/** @brief Reconfigure stream with a new stream format.
* @ingroup streaming
*
* This may be executed whether or not the stream is running.
*
* @param[in] strmh Stream handle
* @param[in] ctrl Control block, processed using {usbhost_uvc_probe_stream_ctrl} or
*             {uvc_get_stream_ctrl_format_size}
*/
uvc_error_t usbhost_uvc_stream_ctrl(usbhost_uvc_stream_handle_t *strmh, uvc_stream_ctrl_t *ctrl) {
    uvc_error_t ret = UVC_SUCCESS;

    if (strmh->stream_if->bInterfaceNumber != ctrl->bInterfaceNumber) {
        return UVC_ERROR_INVALID_PARAM;
    }

    /* @todo Allow the stream to be modified without restarting the stream */
    if (strmh->running) {
        return UVC_ERROR_BUSY;
    }

    // Set streaming control block
    ret = usbhost_uvc_query_stream_ctrl(strmh->devh, ctrl, 0, UVC_SET_CUR);
    if (ret != UVC_SUCCESS) {
        return ret;
    }

    strmh->cur_ctrl = *ctrl;
    return UVC_SUCCESS;
}

static usbhost_uvc_stream_handle_t *
usbhost_uvc_get_stream_by_interface(usbhost_uvc_device *devh, int interface_idx) {
    usbhost_uvc_stream_handle_t *strmh;

    DL_FOREACH(devh->streams, strmh) {
        if (strmh->stream_if->bInterfaceNumber == interface_idx)
            return strmh;
    }

    return NULL;
}

uvc_error_t
usbhost_uvc_stream_open_ctrl(usbhost_uvc_device *devh, usbhost_uvc_stream_handle_t **strmhp,
                             uvc_stream_ctrl_t *ctrl) {
    /* Chosen frame and format descriptors */
    usbhost_uvc_stream_handle_t *strmh = NULL;
    usbhost_uvc_streaming_interface_t *stream_if;
    uvc_error_t ret;

    if (usbhost_uvc_get_stream_by_interface(devh, ctrl->bInterfaceNumber) != NULL) {
        ret = UVC_ERROR_BUSY; /* Stream is already opened */
        goto fail;
    }

    stream_if = usbhost_uvc_get_stream_if(devh, ctrl->bInterfaceNumber);
    if (!stream_if) {
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }

    strmh = (usbhost_uvc_stream_handle_t *) calloc(1, sizeof(*strmh));
    if (!strmh) {
        ret = UVC_ERROR_NO_MEM;
        goto fail;
    }
    memset(strmh, 0, sizeof(usbhost_uvc_stream_handle_t));
    strmh->devh = devh;
    strmh->stream_if = stream_if;
    //strmh->frame.library_owns_data = 1;


    //TODO: Fix this
    ret = usbhost_uvc_stream_ctrl(strmh, ctrl);
    if (ret != UVC_SUCCESS)
        goto fail;

    // Set up the streaming status and data space
    strmh->running = 0;
    /** @todo take only what we need */
    strmh->outbuf = (uint8_t *) malloc(LIBUVC_XFER_BUF_SIZE);
    strmh->holdbuf = (uint8_t *) malloc(LIBUVC_XFER_BUF_SIZE);

    /*pthread_mutex_init(&strmh->cb_mutex, NULL);
    pthread_cond_init(&strmh->cb_cond, NULL);*/

    DL_APPEND(devh->streams, strmh);

    *strmhp = strmh;

    return UVC_SUCCESS;

    fail:
    if (strmh)
        free(strmh);
    return ret;
}

// Probe (Set and Get) streaming control block
uvc_error_t usbhost_uvc_probe_stream_ctrl(usbhost_uvc_device *devh, uvc_stream_ctrl_t *ctrl) {
    uvc_error_t ret = UVC_SUCCESS;

    // Sending probe SET request - UVC_SET_CUR request in a probe/commit structure containing desired values for VS Format index, VS Frame index, and VS Frame Interval
    // UVC device will check the VS Format index, VS Frame index, and Frame interval properties to verify if possible and update the probe/commit structure if feasible
    ret = usbhost_uvc_query_stream_ctrl(devh, ctrl, 1, UVC_SET_CUR);
    if (ret != UVC_SUCCESS) {
        return ret;
    }

    // Sending probe GET request - UVC_GET_CUR request to read the updated values from UVC device
    ret = usbhost_uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_CUR);
    if (ret != UVC_SUCCESS) {
        return ret;
    }

    /** @todo make sure that worked */
    return UVC_SUCCESS;
}

/** Get a negotiated streaming control block for some common parameters for specific interface
* @ingroup streaming
*
* @param[in] devh Device handle
* @param[in,out] ctrl Control block
* @param[in] format_class Type of streaming format
* @param[in] width Desired frame width
* @param[in] height Desired frame height
* @param[in] fps Frame rate, frames per second
*/
uvc_error_t usbhost_get_stream_ctrl_format_size(
        usbhost_uvc_device *devh,
        uvc_stream_ctrl_t *ctrl,
        uint32_t fourcc,
        int width, int height,
        int fps,
        int interface_idx
) {
    usbhost_uvc_streaming_interface_t *stream_if;
    uvc_format_desc_t *format;
    uvc_error_t ret = UVC_SUCCESS;

    stream_if = usbhost_uvc_get_stream_if(devh, interface_idx);
    if (stream_if == NULL) {
        return UVC_ERROR_INVALID_PARAM;
    }

    DL_FOREACH(stream_if->format_descs, format) {
        uvc_frame_desc_t *frame;
        //TODO
        auto val = SWAP_UINT32(*(const uint32_t *) format->guidFormat);
        if(fourcc_map.count(val))
            val = fourcc_map.at(val);

        if (fourcc != val)
            continue;

        DL_FOREACH(format->frame_descs, frame) {
            if (frame->wWidth != width || frame->wHeight != height)
                continue;

            uint32_t *interval;
            if (frame->intervals) {
                for (interval = frame->intervals; *interval; ++interval) {
                    // allow a fps rate of zero to mean "accept first rate available"
                    if (10000000 / *interval == (unsigned int) fps || fps == 0) {

                        /* get the max values -- we need the interface number to be able
                        to do this */
                        ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
                        ret = usbhost_uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);
                        if (ret != UVC_SUCCESS) {
                            return ret;
                        }

                        ctrl->bmHint = (1 << 0); /* don't negotiate interval */
                        ctrl->bFormatIndex = format->bFormatIndex;
                        ctrl->bFrameIndex = frame->bFrameIndex;
                        ctrl->dwFrameInterval = *interval;

                        goto found;
                    }
                }
            } else {
                uint32_t interval_100ns = 10000000 / fps;
                uint32_t interval_offset = interval_100ns - frame->dwMinFrameInterval;

                if (interval_100ns >= frame->dwMinFrameInterval
                    && interval_100ns <= frame->dwMaxFrameInterval
                    && !(interval_offset
                         && (interval_offset % frame->dwFrameIntervalStep))) {

                    /* get the max values -- we need the interface number to be able
                    to do this */
                    ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
                    ret = usbhost_uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);
                    if (ret != UVC_SUCCESS) {
                        return ret;
                    }

                    ctrl->bmHint = (1 << 0);
                    ctrl->bFormatIndex = format->bFormatIndex;
                    ctrl->bFrameIndex = frame->bFrameIndex;
                    ctrl->dwFrameInterval = interval_100ns;

                    goto found;
                }
            }
        }
    }

    return UVC_ERROR_INVALID_MODE;

    found:
    return usbhost_uvc_probe_stream_ctrl(devh, ctrl);
}

/** Get a negotiated streaming control block for some common parameters for all interface
* @ingroup streaming
*
* @param[in] devh Device handle
* @param[in,out] ctrl Control block
* @param[in] format_class Type of streaming format
* @param[in] width Desired frame width
* @param[in] height Desired frame height
* @param[in] fps Frame rate, frames per second
*/
uvc_error_t usbhost_get_stream_ctrl_format_size_all(
        usbhost_uvc_device *devh,
        uvc_stream_ctrl_t *ctrl,
        uint32_t fourcc,
        int width, int height,
        int fps
) {
    usbhost_uvc_streaming_interface_t *stream_if;
    uvc_format_desc_t *format;
    uvc_error_t ret = UVC_SUCCESS;

    DL_FOREACH(devh->deviceData.stream_ifs, stream_if) {

        DL_FOREACH(stream_if->format_descs, format) {
            uvc_frame_desc_t *frame;

            if (SWAP_UINT32(fourcc) != *(const uint32_t *) format->guidFormat)
                continue;

            DL_FOREACH(format->frame_descs, frame) {
                if (frame->wWidth != width || frame->wHeight != height)
                    continue;

                uint32_t *interval;

                if (frame->intervals) {
                    for (interval = frame->intervals; *interval; ++interval) {
                        // allow a fps rate of zero to mean "accept first rate available"
                        if (10000000 / *interval == (unsigned int) fps || fps == 0) {

                            /* get the max values -- we need the interface number to be able
                            to do this */
                            ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
                            ret = usbhost_uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);
                            if (ret != UVC_SUCCESS) {
                                return ret;
                            }

                            ctrl->bmHint = (1 << 0); /* don't negotiate interval */
                            ctrl->bFormatIndex = format->bFormatIndex;
                            ctrl->bFrameIndex = frame->bFrameIndex;
                            ctrl->dwFrameInterval = *interval;

                            goto found;
                        }
                    }
                } else {
                    uint32_t interval_100ns = 10000000 / fps;
                    uint32_t interval_offset = interval_100ns - frame->dwMinFrameInterval;

                    if (interval_100ns >= frame->dwMinFrameInterval
                        && interval_100ns <= frame->dwMaxFrameInterval
                        && !(interval_offset
                             && (interval_offset % frame->dwFrameIntervalStep))) {

                        /* get the max values -- we need the interface number to be able
                        to do this */
                        ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
                        ret = usbhost_uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);
                        if (ret != UVC_SUCCESS) {
                            return ret;
                        }

                        ctrl->bmHint = (1 << 0);
                        ctrl->bFormatIndex = format->bFormatIndex;
                        ctrl->bFrameIndex = frame->bFrameIndex;
                        ctrl->dwFrameInterval = interval_100ns;

                        goto found;
                    }
                }
            }
        }
    }

    return UVC_ERROR_INVALID_MODE;

    found:
    return usbhost_uvc_probe_stream_ctrl(devh, ctrl);
}

/**
* @brief Get the length of a control on a terminal or unit.
*
* @param devh UVC device handle
* @param unit Unit or Terminal ID; obtain this from the uvc_extension_unit_t describing the extension unit
* @param ctrl Vendor-specific control number to query
* @return On success, the length of the control as reported by the device. Otherwise,
*   a uvc_error_t error describing the error encountered.
* @ingroup ctrl
*/
int uvc_get_ctrl_len(usbhost_uvc_device *devh, uint8_t unit, uint8_t ctrl) {
    unsigned char buf[2];

    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    int ret = usb_device_control_transfer(
            devh->device->get_handle(),
            UVC_REQ_TYPE_GET,
            UVC_GET_LEN,
            ctrl << 8,
            unit << 8 | devh->deviceData.ctrl_if.bInterfaceNumber,
            buf,
            2, 0);

    if (ret < 0)
        return ret;
    else
        return (unsigned short) SW_TO_SHORT(buf);
}

/**
* @brief Perform a GET_* request from an extension unit.
*
* @param devh UVC device handle
* @param unit Unit ID; obtain this from the uvc_extension_unit_t describing the extension unit
* @param ctrl Control number to query
* @param data Data buffer to be filled by the device
* @param len Size of data buffer
* @param req_code GET_* request to execute
* @return On success, the number of bytes actually transferred. Otherwise,
*   a uvc_error_t error describing the error encountered.
* @ingroup ctrl
*/
int uvc_get_ctrl(usbhost_uvc_device *devh, uint8_t unit, uint8_t ctrl, void *data, int len,
                 enum uvc_req_code req_code) {
    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    return usb_device_control_transfer(
            devh->device->get_handle(),
            UVC_REQ_TYPE_GET, req_code,
            ctrl << 8,
            unit << 8 | devh->deviceData.ctrl_if.bInterfaceNumber,        // XXX saki
            static_cast<unsigned char *>(data),
            len, 0);
}

/**
* @brief Perform a SET_CUR request to a terminal or unit.
*
* @param devh UVC device handle
* @param unit Unit or Terminal ID
* @param ctrl Control number to set
* @param data Data buffer to be sent to the device
* @param len Size of data buffer
* @return On success, the number of bytes actually transferred. Otherwise,
*   a uvc_error_t error describing the error encountered.
* @ingroup ctrl
*/
int uvc_set_ctrl(usbhost_uvc_device *devh, uint8_t unit, uint8_t ctrl, void *data, int len) {
    if (!devh)
        return UVC_ERROR_NO_DEVICE;
    auto ret = usb_device_control_transfer(
            devh->device->get_handle(),
            UVC_REQ_TYPE_SET, UVC_SET_CUR,
            ctrl<<8,
            unit << 8 | devh->deviceData.ctrl_if.bInterfaceNumber,
            static_cast<unsigned char *>(data),
            len, 10);
    int x =errno;
    return ret;
}

uvc_error_t uvc_get_power_mode(usbhost_uvc_device *devh, enum uvc_device_power_mode *mode,
                               enum uvc_req_code req_code) {
    uint8_t mode_char;
    int ret;
    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    ret = usb_device_control_transfer(
            devh->device->get_handle(),
            UVC_REQ_TYPE_GET, req_code,
            UVC_VC_VIDEO_POWER_MODE_CONTROL << 8,
            devh->deviceData.ctrl_if.bInterfaceNumber,
            (unsigned char *) &mode_char,
            sizeof(mode_char), 0);

    if (ret == 1) {
        *mode = static_cast<uvc_device_power_mode>(mode_char);
        return UVC_SUCCESS;
    } else {
        return static_cast<uvc_error_t>(ret);
    }
}

uvc_error_t uvc_set_power_mode(usbhost_uvc_device *devh, enum uvc_device_power_mode mode) {
    uint8_t mode_char = mode;
    int ret;

    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    ret = usb_device_control_transfer(
            devh->device->get_handle(),
            UVC_REQ_TYPE_SET, UVC_SET_CUR,
            UVC_VC_VIDEO_POWER_MODE_CONTROL << 8,
            devh->deviceData.ctrl_if.bInterfaceNumber,
            (unsigned char *) &mode_char,
            sizeof(mode_char), 0);

    if (ret == 1)
        return UVC_SUCCESS;
    else
        return static_cast<uvc_error_t>(ret);
}

// Iterate over all descriptors and parse all Interface and Endpoint descriptors
void usbhost_uvc_parse_config_descriptors(usb_descriptor_iter *it,
                                          usbhost_uvc_interfaces **interfaces) {

    usbhost_uvc_interfaces *descInterfaces = new usbhost_uvc_interfaces();
    memset(descInterfaces, 0, sizeof(usbhost_uvc_interfaces));
    usbhost_uvc_interface *curInterface = NULL;
    usb_interface_descriptor *curDescriptor = NULL;
    descInterfaces->numInterfaces = 0;


    // iterate over all descriptors
    usb_descriptor_header *h = nullptr;
    usb_descriptor_header *prev = nullptr;
    do {
        h = usb_descriptor_iter_next(it);
        if (h == NULL) {
            if (prev) {
                curInterface->extraLength = reinterpret_cast<unsigned char *>(it->config_end) -
                                            reinterpret_cast<unsigned char *>(prev);
                curInterface->extra =
                        reinterpret_cast<unsigned char *>(prev) + sizeof(usb_interface_descriptor);
                //LOG_DEBUG("Saved extra length in end: %d", curInterface->extraLength);
            }
        } else if (h->bDescriptorType == USB_DT_INTERFACE) {
            curDescriptor = (usb_interface_descriptor *) h;
            if (prev != nullptr) {
                curInterface->extraLength = reinterpret_cast<unsigned char *>(curDescriptor) -
                                            reinterpret_cast<unsigned char *>(prev);
                curInterface->extra =
                        reinterpret_cast<unsigned char *>(prev) + sizeof(usb_interface_descriptor);
                //LOG_DEBUG("Saved extra length: %d", curInterface->extraLength);
            }
            prev = h;
            curInterface = &descInterfaces->iface[curDescriptor->bInterfaceNumber];
            memcpy(&curInterface->desc, curDescriptor, curDescriptor->bLength);
            curInterface->winusbInterfaceNumber = descInterfaces->numInterfaces;
            //LOG_DEBUG("Found interface number %d ",curDescriptor->bInterfaceNumber);
            descInterfaces->numInterfaces++;
        } else if (h->bDescriptorType == USB_DT_ENDPOINT) {
            memcpy(&curInterface->endpoints[curInterface->numEndpoints], h, h->bLength);
            /*LOG_DEBUG("Found endpoint: %x for interface number %d ",
                 curInterface->endpoints[curInterface->numEndpoints].bEndpointAddress,
                 curInterface->winusbInterfaceNumber);*/
            curInterface->numEndpoints++;
        }
    } while (h);
    //LOG_DEBUG("Finished parsing descriptors!");
    *interfaces = descInterfaces;
}


usb_descriptor_header *
usbhost_get_descriptor(usb_device *pDevice, int descriptor_type, int descriptor_index) {
    int curr_index = 0;
    usb_descriptor_iter iter;
    usb_descriptor_iter_init(pDevice, &iter);
    usb_descriptor_header *h = usb_descriptor_iter_next(&iter);
    while (h != nullptr) {
        if (h->bDescriptorType == descriptor_type) {
            if (curr_index == descriptor_index) {
                return h;
            } else {
                curr_index++;
            }
        }
        h = usb_descriptor_iter_next(&iter);
    }
    return h;
}

uvc_error_t usbhost_open(usbhost_uvc_device *device, int InterfaceNumber) {
    usbhost_uvc_interfaces *interfaces = NULL;
    usb_config_descriptor *cfgDesc = NULL;
    byte *descriptors = NULL;
    uvc_error_t ret = UVC_SUCCESS;

    //LOG_DEBUG("usbhost_open() device: %s", device->deviceHandle->dev_name);
    device->streams = NULL;

    // Start by clearing deviceData, otherwise
    // usbhost_uvc_free_device_info(&device->deviceData);
    // will do something not good
    memset(&device->deviceData, 0, sizeof(usbhost_uvc_device_info_t));

    if (!device->device) {
        LOG_ERROR("usb_device_new() failed to create usb device!");
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }


    // Returns IVCAM configuration descriptor
    cfgDesc = (usb_config_descriptor *) usbhost_get_descriptor(device->device->get_handle(),
                                                               USB_DT_CONFIG,
                                                               0);
    if (cfgDesc == NULL) {
        LOG_ERROR("usb_device_new() failed cannot get config descriptor!");
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }
    device->deviceData.config = *cfgDesc;
    //LOG_DEBUG("usbhost_open() parsing descriptors: %s", device->deviceHandle->dev_name);
    // Iterate over all descriptors and parse all Interface and Endpoint descriptors
    usb_descriptor_iter it;
    usb_descriptor_iter_init(device->device->get_handle(), &it);
    usbhost_uvc_parse_config_descriptors(&it,
                                         &interfaces);
    //LOG_DEBUG("usbhost_open() parsing descriptors complete found %d interfaces",interfaces->numInterfaces);

    device->deviceData.interfaces = interfaces;


    //LOG_DEBUG("usbhost_open() usbhost_uvc_scan_control");

    // Fill fields of uvc_device_info on device
    ret = usbhost_uvc_scan_control(device, &device->deviceData, InterfaceNumber);
    if (ret != UVC_SUCCESS) {
        LOG_ERROR("uvc_scan_control failed\n");
        goto fail;
    }

    return ret;

    fail:

    if (descriptors) {
        delete descriptors;
    }

    if (device) {
        usbhost_uvc_free_device_info(&device->deviceData);

        if (device->device) {
            device->device = NULL;
        }
    }
    LOG_ERROR("usbhost_open() failed! " << ret);
    return ret;
}


uvc_error_t usbhost_close(usbhost_uvc_device *device) {
    uvc_error_t ret = UVC_SUCCESS;

    if (device != NULL) {
        usbhost_uvc_free_device_info(&device->deviceData);
        memset(&device->deviceData, 0, sizeof(usbhost_uvc_device_info_t));


        if (device->device) {
            //usb_device_close(device->deviceHandle->GetHandle()); //TODO: do we need to do this?
            device->device = NULL;
        }

        device->streams = NULL;
        free(device);
    } else {
        printf("Error: device == NULL\n");
        ret = UVC_ERROR_NO_DEVICE;
    }

    return ret;
}
#endif
