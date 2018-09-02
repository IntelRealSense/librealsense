// winusb_uvc.cpp : Defines the entry point for the console application.
//

#ifdef RS2_USE_WINUSB_UVC_BACKEND

#define NOMINMAX
#include "windows.h"
#include "SETUPAPI.H"
#include "winusb_uvc.h"
#include "libuvc/utlist.h"
#include "concurrency.h"
#include "types.h"
#include <vector>
#include <thread>
#include <atomic>

// Data structures for Backend-Frontend queue:
struct frame;
// We keep no more then 2 frames in between frontend and backend
typedef librealsense::small_heap<frame, 2> frames_archive;
struct frame
{
    frame() {}

    // We don't want the frames to be overwritten at any point
    // (deallocate resets item to "default" that we want to avoid)
    frame(const frame& other) {}
    frame(frame&& other) {}
    frame& operator=(const frame& other) { return *this; }

    std::vector<uint8_t> pixels; 
    librealsense::platform::frame_object fo;
    frames_archive* owner; // Keep pointer to owner for light-deleter
};
void cleanup_frame(frame* ptr) {
    if (ptr) ptr->owner->deallocate(ptr);
};
typedef void(*cleanup_ptr)(frame*);
// Unique_ptr is used as the simplest RAII, with static deleter
typedef std::unique_ptr<frame, cleanup_ptr> frame_ptr;
typedef single_consumer_queue<frame_ptr> frames_queue;

uvc_error_t winusb_uvc_scan_streaming(winusb_uvc_device *dev, winusb_uvc_device_info_t *info, int interface_idx);
uvc_error_t winusb_uvc_parse_vs(winusb_uvc_device *dev, winusb_uvc_device_info_t *info, winusb_uvc_streaming_interface_t *stream_if, const unsigned char *block, size_t block_size);

void winusb_uvc_free_device_info(winusb_uvc_device_info_t *info) {
    uvc_input_terminal_t *input_term, *input_term_tmp;
    uvc_processing_unit_t *proc_unit, *proc_unit_tmp;
    uvc_extension_unit_t *ext_unit, *ext_unit_tmp;

    winusb_uvc_streaming_interface_t *stream_if, *stream_if_tmp;
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

    if (info->interfaces)
    {
        for (int i = 0; i < MAX_USB_INTERFACES; i++)
        {
            if (info->interfaces->iface[i].extra != NULL)
            {
                free(info->interfaces->iface[i].extra);
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
uvc_error_t uvc_parse_vc_header(winusb_uvc_device *dev,
    winusb_uvc_device_info_t *info,
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
        scan_ret = winusb_uvc_scan_streaming(dev, info, block[i]);
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
uvc_error_t uvc_parse_vc_input_terminal(winusb_uvc_device *dev,
    winusb_uvc_device_info_t *info,
    const unsigned char *block, size_t block_size) {
    uvc_input_terminal_t *term;
    size_t i;

    /* only supporting camera-type input terminals */
    if (SW_TO_SHORT(&block[4]) != UVC_ITT_CAMERA) {
        return UVC_SUCCESS;
    }

    term = (uvc_input_terminal_t *)calloc(1, sizeof(*term));

    term->bTerminalID = block[3];
    term->wTerminalType = (uvc_it_type)SW_TO_SHORT(&block[4]);
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
uvc_error_t uvc_parse_vc_processing_unit(winusb_uvc_device *dev,
    winusb_uvc_device_info_t *info,
    const unsigned char *block, size_t block_size) {
    uvc_processing_unit_t *unit;
    size_t i;

    unit = (uvc_processing_unit_t *)calloc(1, sizeof(*unit));
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
uvc_error_t uvc_parse_vc_selector_unit(winusb_uvc_device *dev,
    winusb_uvc_device_info_t *info,
    const unsigned char *block, size_t block_size) {
    uvc_selector_unit_t *unit;

    unit = (uvc_selector_unit_t *)calloc(1, sizeof(*unit));
    unit->bUnitID = block[3];

    DL_APPEND(info->ctrl_if.selector_unit_descs, unit);

    return UVC_SUCCESS;
}

/** @internal
* @brief Parse a VideoControl extension unit.
* @ingroup device
*/
uvc_error_t uvc_parse_vc_extension_unit(winusb_uvc_device *dev,
    winusb_uvc_device_info_t *info,
    const unsigned char *block, size_t block_size) {
    uvc_extension_unit_t *unit = (uvc_extension_unit_t *)calloc(1, sizeof(*unit));
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
    winusb_uvc_device *dev,
    winusb_uvc_device_info_t *info,
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

uvc_error_t winusb_uvc_scan_control(winusb_uvc_device *dev, winusb_uvc_device_info_t *info) {
    USB_INTERFACE_DESCRIPTOR *if_desc;
    uvc_error_t parse_ret, ret;
    int interface_idx;
    const unsigned char *buffer;
    size_t buffer_left, block_size;

    ret = UVC_SUCCESS;
    if_desc = NULL;

    for (interface_idx = 0; interface_idx < MAX_USB_INTERFACES; ++interface_idx) {
        if_desc = &info->interfaces->iface[interface_idx].desc;

        if (if_desc->bInterfaceClass == 14 && if_desc->bInterfaceSubClass == 1) // Video, Control
            break;

        if_desc = NULL;
    }

    if (if_desc == NULL) {
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

    return ret;
}

/** @internal
* Process a VideoStreaming interface
* @ingroup device
*/
uvc_error_t winusb_uvc_scan_streaming(winusb_uvc_device *dev,
    winusb_uvc_device_info_t *info,
    int interface_idx) {
    const struct winusb_uvc_interface *if_desc;
    const unsigned char *buffer;
    int buffer_left, block_size;
    uvc_error_t ret, parse_ret;
    winusb_uvc_streaming_interface_t *stream_if;


    ret = UVC_SUCCESS;

    if_desc = &(info->interfaces->iface[interface_idx]);
    buffer = if_desc->extra;
    buffer_left = if_desc->extraLength - sizeof(USB_INTERFACE_DESCRIPTOR);

    stream_if = (winusb_uvc_streaming_interface_t *)calloc(1, sizeof(*stream_if));
    memset(stream_if, 0, sizeof(winusb_uvc_streaming_interface_t));
    stream_if->parent = info;
    stream_if->bInterfaceNumber = if_desc->desc.bInterfaceNumber;
    DL_APPEND(info->stream_ifs, stream_if);

    while (buffer_left >= 3) {
        block_size = buffer[0];
        parse_ret = winusb_uvc_parse_vs(dev, info, stream_if, buffer, block_size);

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
uvc_error_t winusb_uvc_parse_vs_input_header(winusb_uvc_streaming_interface_t *stream_if,
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
uvc_error_t winusb_uvc_parse_vs_format_uncompressed(winusb_uvc_streaming_interface_t *stream_if,
    const unsigned char *block,
    size_t block_size) {

    uvc_format_desc_t *format = (uvc_format_desc_t *)calloc(1, sizeof(*format));
    memset(format, 0, sizeof(uvc_format_desc_t));
    format->parent = stream_if;
    format->bDescriptorSubtype = (uvc_vs_desc_subtype)block[2];
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
uvc_error_t winusb_uvc_parse_vs_frame_format(winusb_uvc_streaming_interface_t *stream_if,
    const unsigned char *block,
    size_t block_size) {

    uvc_format_desc_t *format = (uvc_format_desc_t *)malloc(sizeof(uvc_format_desc_t));
    memset(format, 0, sizeof(uvc_format_desc_t));
    format->parent = stream_if;
    format->bDescriptorSubtype = (uvc_vs_desc_subtype)block[2];
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
uvc_error_t winusb_uvc_parse_vs_format_mjpeg(winusb_uvc_streaming_interface_t *stream_if,
    const unsigned char *block,
    size_t block_size) {

    uvc_format_desc_t *format = (uvc_format_desc_t *)calloc(1, sizeof(*format));

    format->parent = stream_if;
    format->bDescriptorSubtype = (uvc_vs_desc_subtype)block[2];
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
uvc_error_t winusb_uvc_parse_vs_frame_frame(winusb_uvc_streaming_interface_t *stream_if,
    const unsigned char *block,
    size_t block_size) {
    uvc_format_desc_t *format;
    uvc_frame_desc_t *frame;

    const unsigned char *p;
    int i;

    format = stream_if->format_descs->prev;
    frame = (uvc_frame_desc_t *)malloc(sizeof(uvc_frame_desc_t));
    memset(frame, 0, sizeof(uvc_frame_desc_t));
    frame->parent = format;

    frame->bDescriptorSubtype = (uvc_vs_desc_subtype)block[2];
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
    }
    else {
        frame->intervals = (uint32_t *)malloc((block[21] + 1) * sizeof(uint32_t));
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
uvc_error_t winusb_uvc_parse_vs_frame_uncompressed(winusb_uvc_streaming_interface_t *stream_if,
    const unsigned char *block,
    size_t block_size) {
    uvc_format_desc_t *format;
    uvc_frame_desc_t *frame;

    const unsigned char *p;
    int i;

    format = stream_if->format_descs->prev;
    frame = (uvc_frame_desc_t *)calloc(1, sizeof(*frame));
    memset(frame, 0, sizeof(uvc_frame_desc_t));
    frame->parent = format;

    frame->bDescriptorSubtype = (uvc_vs_desc_subtype)block[2];
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
    }
    else {
        frame->intervals = (uint32_t *)calloc(block[25] + 1, sizeof(frame->intervals[0]));
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
uvc_error_t winusb_uvc_parse_vs(
    winusb_uvc_device *dev,
    winusb_uvc_device_info_t *info,
    winusb_uvc_streaming_interface_t *stream_if,
    const unsigned char *block, size_t block_size) {
    uvc_error_t ret;
    int descriptor_subtype;

    ret = UVC_SUCCESS;
    descriptor_subtype = block[2];

    switch (descriptor_subtype) {
    case UVC_VS_INPUT_HEADER:
        ret = winusb_uvc_parse_vs_input_header(stream_if, block, block_size);
        break;
    case UVC_VS_FORMAT_UNCOMPRESSED:
        ret = winusb_uvc_parse_vs_format_uncompressed(stream_if, block, block_size);
        break;
    case UVC_VS_FORMAT_MJPEG:
        ret = winusb_uvc_parse_vs_format_mjpeg(stream_if, block, block_size);
        break;
    case UVC_VS_FRAME_UNCOMPRESSED:
    case UVC_VS_FRAME_MJPEG:
        ret = winusb_uvc_parse_vs_frame_uncompressed(stream_if, block, block_size);
        break;
    case UVC_VS_FORMAT_FRAME_BASED:
        ret = winusb_uvc_parse_vs_frame_format(stream_if, block, block_size);
        break;
    case UVC_VS_FRAME_FRAME_BASED:
        ret = winusb_uvc_parse_vs_frame_frame(stream_if, block, block_size);
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


/* Return only Video stream interfaces */
static winusb_uvc_streaming_interface_t *winusb_uvc_get_stream_if(winusb_uvc_device *devh, int interface_idx) {
    winusb_uvc_streaming_interface_t *stream_if;

    DL_FOREACH(devh->deviceData.stream_ifs, stream_if) {
        if (stream_if->bInterfaceNumber == interface_idx)
            return stream_if;
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

uvc_error_t winusb_get_available_formats(
    winusb_uvc_device *devh,
    int interface_idx,
    uvc_format_t **formats) {

    winusb_uvc_streaming_interface_t *stream_if = NULL;
    uvc_format_t *prev_format = NULL;
    uvc_format_desc_t *format;

    stream_if = winusb_uvc_get_stream_if(devh, interface_idx);
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
                uvc_format_t *cur_format = (uvc_format_t *)malloc(sizeof(uvc_format_t));
                cur_format->height = frame_desc->wHeight;
                cur_format->width = frame_desc->wWidth;
                cur_format->fourcc = SWAP_UINT32(*(const uint32_t *)format->guidFormat);

                cur_format->fps = 10000000 / *interval_ptr;
                cur_format->next = NULL;

                if (prev_format != NULL) {
                    prev_format->next = cur_format;
                }
                else {
                    *formats = cur_format;
                }

                prev_format = cur_format;
            }
        }
    }
    return UVC_SUCCESS;
}

// Return linked list of uvc_format_t of all available formats inside winusb device
uvc_error_t winusb_get_available_formats_all(winusb_uvc_device *devh, uvc_format_t **formats) {

    winusb_uvc_streaming_interface_t *stream_if = NULL;
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
                    uvc_format_t *cur_format = (uvc_format_t *)malloc(sizeof(uvc_format_t));
                    cur_format->height = frame_desc->wHeight;
                    cur_format->width = frame_desc->wWidth;
                    cur_format->fourcc = SWAP_UINT32(*(const uint32_t *)format->guidFormat);
                    cur_format->interfaceNumber = stream_if->bInterfaceNumber;

                    cur_format->fps = 10000000 / *interval_ptr;
                    cur_format->next = NULL;

                    if (prev_format != NULL) {
                        prev_format->next = cur_format;
                    }
                    else {
                        *formats = cur_format;
                    }

                    prev_format = cur_format;
                }
            }
        }
    }
    return UVC_SUCCESS;
}

uvc_error_t winusb_free_formats(uvc_format_t *formats) {
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
static uvc_frame_desc_t *winusb_uvc_find_frame_desc_stream_if(winusb_uvc_streaming_interface_t *stream_if,
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

uvc_frame_desc_t *winusb_uvc_find_frame_desc_stream(winusb_uvc_stream_handle_t *strmh,
    uint16_t format_id, uint16_t frame_id) {
    return winusb_uvc_find_frame_desc_stream_if(strmh->stream_if, format_id, frame_id);
}







void winusb_uvc_swap_buffers(winusb_uvc_stream_handle_t *strmh) {
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
    strmh->got_bytes = 0;
    strmh->last_scr = 0;
    strmh->pts = 0;
}

void winusb_uvc_process_payload(winusb_uvc_stream_handle_t *strmh, 
    uint8_t *payload, size_t payload_len, frames_archive* archive, frames_queue* queue) {
    uint8_t header_len;
    uint8_t header_info;
    size_t data_len;

    /* magic numbers for identifying header packets from some iSight cameras */
    static uint8_t isight_tag[] = {
        0x11, 0x22, 0x33, 0x44,
        0xde, 0xad, 0xbe, 0xef, 0xde, 0xad, 0xfa, 0xce
    };

    /* ignore empty payload transfers */
    if (payload_len == 0)
        return;

    /* Certain iSight cameras have strange behavior: They send header
    * information in a packet with no image data, and then the following
    * packets have only image data, with no more headers until the next frame.
    *
    * The iSight header: len(1), flags(1 or 2), 0x11223344(4),
    * 0xdeadbeefdeadface(8), ??(16)
    */

    header_len = payload[0];
    if (header_len > payload_len)
    {
        printf("bogus packet: actual_len=%zd, header_len=%d\n", payload_len, header_len);
        return;
    }
    else
    {
        //printf("Frame %d %d: header info = %08X\n", strmh->seq, counter, payload[1]);
    }

    data_len = payload_len - header_len;

    if (header_len < 2) {
        header_info = 0;
    }
    else {
        /** @todo we should be checking the end-of-header bit */
        size_t variable_offset = 2;

        header_info = payload[1];

        if (header_info & 0x40) {
            printf("bad packet: error bit set");
            return;
        }

        if (strmh->fid != (header_info & 1) && strmh->got_bytes != 0) {
            /* The frame ID bit was flipped, but we have image data sitting
            around from prior transfers. This means the camera didn't send
            an EOF for the last transfer of the previous frame. */
            printf("complete buffer : length %zd\n", strmh->got_bytes);
            winusb_uvc_swap_buffers(strmh);

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
            winusb_uvc_swap_buffers(strmh);

            auto frame_p = archive->allocate();
            if (frame_p)
            {
                frame_ptr fp(frame_p, &cleanup_frame);
                
                memcpy(fp->pixels.data(), payload, data_len + header_len);

                LOG_DEBUG("Passing packet to user CB with size " << data_len + header_len);
                librealsense::platform::frame_object fo{ data_len, header_len, 
                    fp->pixels.data() + header_len , fp->pixels.data() };
                fp->fo = fo;

                queue->enqueue(std::move(fp));
            }
            else
            {
                LOG_INFO("WinUSB backend is dropping a frame because librealsense wasn't fast enough");
            }
        //}
    }
}

void stream_thread(winusb_uvc_stream_context *strctx)
{
    PUCHAR buffer = (PUCHAR)malloc(strctx->maxPayloadTransferSize);
    memset(buffer, 0, sizeof(strctx->maxPayloadTransferSize));

    frames_archive archive;
    std::atomic_bool keep_sending_callbacks = true;
    frames_queue queue;

    // Get all pointers from archive and initialize their content
    std::vector<frame*> frames;
    for (auto i = 0; i < frames_archive::CAPACITY; i++)
    {
        auto ptr = archive.allocate();
        ptr->pixels.resize(strctx->maxPayloadTransferSize, 0);
        ptr->owner = &archive;
        frames.push_back(ptr);
    }
    for (auto ptr : frames)
    {
        archive.deallocate(ptr);
    }

    std::thread t([&]() {
        while (keep_sending_callbacks)
        {
            frame_ptr fp(nullptr, [](frame*) {});
            if (queue.dequeue(&fp, 50))
            {
                strctx->stream->user_cb(&fp->fo, strctx->stream->user_ptr);
            }
        }
    });

    do {
        DWORD transferred;
        if (!WinUsb_ReadPipe(strctx->stream->devh->associateHandle,
            strctx->endpoint,
            buffer,
            strctx->maxPayloadTransferSize,
            &transferred,
            NULL)) 
            {
                printf("WinUsb_ReadPipe Error: %d\n" + GetLastError());
                break;
            }

        LOG_DEBUG("Packet received with size " << transferred);
        winusb_uvc_process_payload(strctx->stream, buffer, transferred, &archive, &queue);
    } while (strctx->stream->running);

    // reseting pipe after use
    auto ret = WinUsb_ResetPipe(strctx->stream->devh->associateHandle, strctx->endpoint);

    free(buffer);
    free(strctx);

    queue.clear();
    archive.stop_allocation();
    archive.wait_until_empty();
    keep_sending_callbacks = false;
    t.join();
};

uvc_error_t winusb_uvc_stream_start(
    winusb_uvc_stream_handle_t *strmh,
    winusb_uvc_frame_callback_t *cb,
    void *user_ptr,
    uint8_t flags
) {
    /* USB interface we'll be using */
    winusb_uvc_interface *iface;
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

    frame_desc = winusb_uvc_find_frame_desc_stream(strmh, ctrl->bFormatIndex, ctrl->bFrameIndex);
    if (!frame_desc) {
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }
    format_desc = frame_desc->parent;

    // Get the interface that provides the chosen format and frame configuration
    interface_id = strmh->stream_if->bInterfaceNumber;
    iface = &strmh->devh->deviceData.interfaces->iface[interface_id];

    winusb_uvc_stream_context *streamctx = new winusb_uvc_stream_context;

    //printf("Starting stream on EP = 0x%X, interface 0x%X\n", format_desc->parent->bEndpointAddress, iface);

    streamctx->stream = strmh;
    streamctx->endpoint = format_desc->parent->bEndpointAddress;
    streamctx->iface = iface;
    streamctx->maxPayloadTransferSize = strmh->cur_ctrl.dwMaxPayloadTransferSize;
    strmh->user_ptr = user_ptr;
    strmh->cb_thread = std::thread(stream_thread, streamctx);
    strmh->user_cb = cb;


    return UVC_SUCCESS;
fail:
    strmh->running = 0;
    return ret;
}

uvc_error_t winusb_uvc_stream_stop(winusb_uvc_stream_handle_t *strmh)
{
    if (!strmh->running)
        return UVC_ERROR_INVALID_PARAM;

    strmh->running = 0;
    strmh->cb_thread.join();

    return UVC_SUCCESS;
}

void winusb_uvc_stream_close(winusb_uvc_stream_handle_t *strmh)
{
    if (strmh->running)
    {
        winusb_uvc_stream_stop(strmh);
    }

    free(strmh->outbuf);
    free(strmh->holdbuf);

    DL_DELETE(strmh->devh->streams, strmh);

    free(strmh->user_ptr);
    free(strmh);
}

uvc_error_t winusb_uvc_stream_open_ctrl(winusb_uvc_device *devh, winusb_uvc_stream_handle_t **strmhp, uvc_stream_ctrl_t *ctrl);

uvc_error_t winusb_start_streaming(winusb_uvc_device *devh, uvc_stream_ctrl_t *ctrl, winusb_uvc_frame_callback_t *cb, void *user_ptr, uint8_t flags)
{
    uvc_error_t ret;
    winusb_uvc_stream_handle_t *strmh;

    ret = winusb_uvc_stream_open_ctrl(devh, &strmh, ctrl);
    if (ret != UVC_SUCCESS)
    {
        return ret;
    }

    ret = winusb_uvc_stream_start(strmh, cb, user_ptr, flags);
    if (ret != UVC_SUCCESS)
    {
        winusb_uvc_stream_close(strmh);
        return ret;
    }

    return UVC_SUCCESS;
}

void winusb_stop_streaming(winusb_uvc_device *devh)
{
    winusb_uvc_stream_handle_t *strmh, *strmh_tmp;

    DL_FOREACH_SAFE(devh->streams, strmh, strmh_tmp) {
        winusb_uvc_stream_close(strmh);
    }
}


/** @internal
* @brief Find the descriptor for a specific frame configuration
* @param devh UVC device
* @param format_id Index of format class descriptor
* @param frame_id Index of frame descriptor
*/
uvc_frame_desc_t *winusb_uvc_find_frame_desc(winusb_uvc_device *devh,
    uint16_t format_id, uint16_t frame_id) {

    winusb_uvc_streaming_interface_t *stream_if;
    uvc_frame_desc_t *frame;

    DL_FOREACH(devh->deviceData.stream_ifs, stream_if) {
        frame = winusb_uvc_find_frame_desc_stream_if(stream_if, format_id, frame_id);
        if (frame)
            return frame;
    }

    return NULL;
}

uvc_error_t winusb_uvc_query_stream_ctrl(winusb_uvc_device *devh, uvc_stream_ctrl_t *ctrl, uint8_t probe, int req)
{
    uint8_t buf[48];
    size_t len;
    int err;

    memset(buf, 0, sizeof(buf));
    len = 34;

    /* prepare for a SET transfer */
    if (req == UVC_SET_CUR)
    {
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

    WINUSB_INTERFACE_HANDLE iface;
    if (devh->associateHandle != NULL)
    {
        iface = devh->associateHandle;
    }
    else
    {
        uint8_t interfaceNumber = ctrl->bInterfaceNumber;
        if (interfaceNumber < MAX_USB_INTERFACES)
        {
            // WinUsb_GetAssociatedInterface returns the associated interface (Video stream interface which is associated to Video control interface)
            // A value of 0 indicates the first associated interface (Video Stream 1), a value of 1 indicates the second associated interface (video stream 2)
            // WinUsbInterfaceNumber is the actual interface number taken from the USB config descriptor
            // A value of 0 indicates the first interface (Video Control Interface), A value of 1 indicates the first associated interface (Video Stream 1)
            // For this reason, when calling to WinUsb_GetAssociatedInterface, we must decrease 1 to receive the associated interface
            uint8_t winusbInterfaceNumber = devh->deviceData.interfaces->iface[interfaceNumber].winusbInterfaceNumber;
            WinUsb_GetAssociatedInterface(devh->winusbHandle, winusbInterfaceNumber - 1, &iface);
        }
        else
        {
            return UVC_ERROR_INVALID_PARAM;
        }

    }

    /* do the transfer */
    err = winusb_SendControl(
        iface,
        req == UVC_SET_CUR ? UVC_REQ_TYPE_INTERFACE_SET : UVC_REQ_TYPE_INTERFACE_GET,
        req,
        probe ? (UVC_VS_PROBE_CONTROL << 8) : (UVC_VS_COMMIT_CONTROL << 8),
        0, // When requestType is directed to an interface, WinUsb driver automatically passes the interface number in the low byte of index
        buf, len);

    if (err <= 0)
    {
        WinUsb_Free(iface);
        return (uvc_error_t)err;
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
        // 		else
        // 			ctrl->dwClockFrequency = devh->info->ctrl_if.dwClockFrequency;

        /* fix up block for cameras that fail to set dwMax* */
        if (ctrl->dwMaxVideoFrameSize == 0) {
            uvc_frame_desc_t *frame = winusb_uvc_find_frame_desc(devh, ctrl->bFormatIndex, ctrl->bFrameIndex);

            if (frame) {
                ctrl->dwMaxVideoFrameSize = frame->dwMaxVideoFrameBufferSize;
            }
        }
    }
    if (devh->associateHandle == NULL)
        WinUsb_Free(iface);
    return UVC_SUCCESS;
}

/** @brief Reconfigure stream with a new stream format.
* @ingroup streaming
*
* This may be executed whether or not the stream is running.
*
* @param[in] strmh Stream handle
* @param[in] ctrl Control block, processed using {winusb_uvc_probe_stream_ctrl} or
*             {uvc_get_stream_ctrl_format_size}
*/
uvc_error_t winusb_uvc_stream_ctrl(winusb_uvc_stream_handle_t *strmh, uvc_stream_ctrl_t *ctrl)
{
    uvc_error_t ret = UVC_SUCCESS;

    if (strmh->stream_if->bInterfaceNumber != ctrl->bInterfaceNumber)
    {
        return UVC_ERROR_INVALID_PARAM;
    }

    /* @todo Allow the stream to be modified without restarting the stream */
    if (strmh->running)
    {
        return UVC_ERROR_BUSY;
    }

    // Set streaming control block
    ret = winusb_uvc_query_stream_ctrl(strmh->devh, ctrl, 0, UVC_SET_CUR);
    if (ret != UVC_SUCCESS)
    {
        return ret;
    }

    strmh->cur_ctrl = *ctrl;
    return UVC_SUCCESS;
}

static winusb_uvc_stream_handle_t *winusb_uvc_get_stream_by_interface(winusb_uvc_device *devh, int interface_idx) {
    winusb_uvc_stream_handle_t *strmh;

    DL_FOREACH(devh->streams, strmh) {
        if (strmh->stream_if->bInterfaceNumber == interface_idx)
            return strmh;
    }

    return NULL;
}

uvc_error_t winusb_uvc_stream_open_ctrl(winusb_uvc_device *devh, winusb_uvc_stream_handle_t **strmhp, uvc_stream_ctrl_t *ctrl) {
    /* Chosen frame and format descriptors */
    winusb_uvc_stream_handle_t *strmh = NULL;
    winusb_uvc_streaming_interface_t *stream_if;
    uvc_error_t ret;

    if (winusb_uvc_get_stream_by_interface(devh, ctrl->bInterfaceNumber) != NULL) {
        ret = UVC_ERROR_BUSY; /* Stream is already opened */
        goto fail;
    }

    stream_if = winusb_uvc_get_stream_if(devh, ctrl->bInterfaceNumber);
    if (!stream_if) {
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }

    strmh = (winusb_uvc_stream_handle_t *)calloc(1, sizeof(*strmh));
    if (!strmh) {
        ret = UVC_ERROR_NO_MEM;
        goto fail;
    }
    memset(strmh, 0, sizeof(winusb_uvc_stream_handle_t));
    strmh->devh = devh;
    strmh->stream_if = stream_if;
    //strmh->frame.library_owns_data = 1;

    /*ret = uvc_claim_if(strmh->devh, strmh->stream_if->bInterfaceNumber);
    if (ret != UVC_SUCCESS)
    goto fail;*/

    ret = winusb_uvc_stream_ctrl(strmh, ctrl);
    if (ret != UVC_SUCCESS)
        goto fail;

    // Set up the streaming status and data space
    strmh->running = 0;
    /** @todo take only what we need */
    strmh->outbuf = (uint8_t *)malloc(LIBUVC_XFER_BUF_SIZE);
    strmh->holdbuf = (uint8_t *)malloc(LIBUVC_XFER_BUF_SIZE);

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
uvc_error_t winusb_uvc_probe_stream_ctrl(winusb_uvc_device *devh, uvc_stream_ctrl_t *ctrl)
{
    uvc_error_t ret = UVC_SUCCESS;

    uint8_t interfaceNumber = ctrl->bInterfaceNumber;
    if (interfaceNumber < MAX_USB_INTERFACES)
    {
        // WinUsb_GetAssociatedInterface returns the associated interface (Video stream interface which is associated to Video control interface)
        // A value of 0 indicates the first associated interface (Video Stream 1), a value of 1 indicates the second associated interface (video stream 2)
        // WinUsbInterfaceNumber is the actual interface number taken from the USB config descriptor
        // A value of 0 indicates the first interface (Video Control Interface), A value of 1 indicates the first associated interface (Video Stream 1)
        // For this reason, when calling to WinUsb_GetAssociatedInterface, we must decrease 1 to receive the associated interface
        uint8_t winusbInterfaceNumber = devh->deviceData.interfaces->iface[interfaceNumber].winusbInterfaceNumber;
        WinUsb_GetAssociatedInterface(devh->winusbHandle, winusbInterfaceNumber - 1, &devh->associateHandle);
    }
    else
    {
        return UVC_ERROR_INVALID_PARAM;
    }

    // Sending probe SET request - UVC_SET_CUR request in a probe/commit structure containing desired values for VS Format index, VS Frame index, and VS Frame Interval
    // UVC device will check the VS Format index, VS Frame index, and Frame interval properties to verify if possible and update the probe/commit structure if feasible
    ret = winusb_uvc_query_stream_ctrl(devh, ctrl, 1, UVC_SET_CUR);
    if (ret != UVC_SUCCESS)
    {
        return ret;
    }

    // Sending probe GET request - UVC_GET_CUR request to read the updated values from UVC device
    ret = winusb_uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_CUR);
    if (ret != UVC_SUCCESS)
    {
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
uvc_error_t winusb_get_stream_ctrl_format_size(
    winusb_uvc_device *devh,
    uvc_stream_ctrl_t *ctrl,
    uint32_t fourcc,
    int width, int height,
    int fps,
    int interface_idx
) {
    winusb_uvc_streaming_interface_t *stream_if;
    uvc_format_desc_t *format;
    uvc_error_t ret = UVC_SUCCESS;

    stream_if = winusb_uvc_get_stream_if(devh, interface_idx);
    if (stream_if == NULL)
    {
        return UVC_ERROR_INVALID_PARAM;
    }

    DL_FOREACH(stream_if->format_descs, format) {
        uvc_frame_desc_t *frame;

        if (SWAP_UINT32(fourcc) != *(const uint32_t *)format->guidFormat)
            continue;

        DL_FOREACH(format->frame_descs, frame) {
            if (frame->wWidth != width || frame->wHeight != height)
                continue;

            uint32_t *interval;

            if (frame->intervals) {
                for (interval = frame->intervals; *interval; ++interval) {
                    // allow a fps rate of zero to mean "accept first rate available"
                    if (10000000 / *interval == (unsigned int)fps || fps == 0) {

                        /* get the max values -- we need the interface number to be able
                        to do this */
                        ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
                        ret = winusb_uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);
                        if (ret != UVC_SUCCESS)
                        {
                            return ret;
                        }

                        ctrl->bmHint = (1 << 0); /* don't negotiate interval */
                        ctrl->bFormatIndex = format->bFormatIndex;
                        ctrl->bFrameIndex = frame->bFrameIndex;
                        ctrl->dwFrameInterval = *interval;

                        goto found;
                    }
                }
            }
            else {
                uint32_t interval_100ns = 10000000 / fps;
                uint32_t interval_offset = interval_100ns - frame->dwMinFrameInterval;

                if (interval_100ns >= frame->dwMinFrameInterval
                    && interval_100ns <= frame->dwMaxFrameInterval
                    && !(interval_offset
                        && (interval_offset % frame->dwFrameIntervalStep))) {

                    /* get the max values -- we need the interface number to be able
                    to do this */
                    ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
                    ret = winusb_uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);
                    if (ret != UVC_SUCCESS)
                    {
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
    return winusb_uvc_probe_stream_ctrl(devh, ctrl);
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
uvc_error_t winusb_get_stream_ctrl_format_size_all(
    winusb_uvc_device *devh,
    uvc_stream_ctrl_t *ctrl,
    uint32_t fourcc,
    int width, int height,
    int fps
) {
    winusb_uvc_streaming_interface_t *stream_if;
    uvc_format_desc_t *format;
    uvc_error_t ret = UVC_SUCCESS;

    DL_FOREACH(devh->deviceData.stream_ifs, stream_if) {

        DL_FOREACH(stream_if->format_descs, format) {
            uvc_frame_desc_t *frame;

            if (SWAP_UINT32(fourcc) != *(const uint32_t *)format->guidFormat)
                continue;

            DL_FOREACH(format->frame_descs, frame) {
                if (frame->wWidth != width || frame->wHeight != height)
                    continue;

                uint32_t *interval;

                if (frame->intervals) {
                    for (interval = frame->intervals; *interval; ++interval) {
                        // allow a fps rate of zero to mean "accept first rate available"
                        if (10000000 / *interval == (unsigned int)fps || fps == 0) {

                            /* get the max values -- we need the interface number to be able
                            to do this */
                            ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
                            ret = winusb_uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);
                            if (ret != UVC_SUCCESS)
                            {
                                return ret;
                            }

                            ctrl->bmHint = (1 << 0); /* don't negotiate interval */
                            ctrl->bFormatIndex = format->bFormatIndex;
                            ctrl->bFrameIndex = frame->bFrameIndex;
                            ctrl->dwFrameInterval = *interval;

                            goto found;
                        }
                    }
                }
                else {
                    uint32_t interval_100ns = 10000000 / fps;
                    uint32_t interval_offset = interval_100ns - frame->dwMinFrameInterval;

                    if (interval_100ns >= frame->dwMinFrameInterval
                        && interval_100ns <= frame->dwMaxFrameInterval
                        && !(interval_offset
                            && (interval_offset % frame->dwFrameIntervalStep))) {

                        /* get the max values -- we need the interface number to be able
                        to do this */
                        ctrl->bInterfaceNumber = stream_if->bInterfaceNumber;
                        ret = winusb_uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);
                        if (ret != UVC_SUCCESS)
                        {
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
    return winusb_uvc_probe_stream_ctrl(devh, ctrl);
}


bool wait_for_async_operation(WINUSB_INTERFACE_HANDLE interfaceHandle, 
    int ep, OVERLAPPED &hOvl, ULONG &lengthTransferred, USHORT timeout)
{
    if (GetOverlappedResult(interfaceHandle, &hOvl, &lengthTransferred, FALSE))
        return true;

    auto lastResult = GetLastError();
    if (lastResult == ERROR_IO_PENDING || lastResult == ERROR_IO_INCOMPLETE)
    {
        WaitForSingleObject(hOvl.hEvent, timeout);
        auto res = GetOverlappedResult(interfaceHandle, &hOvl, &lengthTransferred, FALSE);
        if (res != 1)
        {
            return false;
        }
    }
    else
    {
        lengthTransferred = 0;
        WinUsb_ResetPipe(interfaceHandle, ep);
        return false;
    }

    return true;
}

void poll_interrupts(WINUSB_INTERFACE_HANDLE handle, int ep, uint16_t timeout)
{
    static const unsigned short interrupt_buf_size = 0x400;
    uint8_t buffer[interrupt_buf_size];                         /* 64 byte transfer buffer  - dedicated channel*/
    ULONG num_bytes = 0;                                        /* Actual bytes transferred. */
    OVERLAPPED hOvl;
    safe_handle sh(CreateEvent(nullptr, false, false, nullptr));
    hOvl.hEvent = sh.GetHandle();
    int res = WinUsb_ReadPipe(handle, ep, buffer, interrupt_buf_size, &num_bytes, &hOvl);
    if (0 == res)
    {
        auto lastError = GetLastError();
        if (lastError == ERROR_IO_PENDING)
        {
            auto sts = wait_for_async_operation(handle, ep, hOvl, num_bytes, timeout);
            lastError = GetLastError();
            if (lastError == ERROR_OPERATION_ABORTED)
            {
                perror("receiving interrupt_ep bytes failed");
                fprintf(stderr, "Error receiving message.\n");
            }
            if (!sts)
                return;
        }
        else
        {
            WinUsb_ResetPipe(handle, ep);
            perror("receiving interrupt_ep bytes failed");
            fprintf(stderr, "Error receiving message.\n");
            return;
        }

        if (num_bytes == 0)
            return;

        // TODO: Complete XU set instead of using retries
    }
    else
    {
        // todo exception
        perror("receiving interrupt_ep bytes failed");
        fprintf(stderr, "Error receiving message.\n");
    }
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
int uvc_get_ctrl_len(winusb_uvc_device *devh, uint8_t unit, uint8_t ctrl) {
    unsigned char buf[2];

    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    int ret = winusb_SendControl(
        devh->winusbHandle,
        UVC_REQ_TYPE_INTERFACE_GET,
        UVC_GET_LEN,
        ctrl << 8,
        unit << 8 | devh->deviceData.ctrl_if.bInterfaceNumber,
        buf,
        2);

    if (ret < 0)
        return ret;
    else
        return (unsigned short)SW_TO_SHORT(buf);
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
int uvc_get_ctrl(winusb_uvc_device *devh, uint8_t unit, uint8_t ctrl, void *data, int len, enum uvc_req_code req_code) {
    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    return winusb_SendControl(
        devh->winusbHandle,
        UVC_REQ_TYPE_INTERFACE_GET, req_code,
        ctrl << 8,
        unit << 8 | devh->deviceData.ctrl_if.bInterfaceNumber,		// XXX saki
        static_cast<unsigned char*>(data),
        len);
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
int uvc_set_ctrl(winusb_uvc_device *devh, uint8_t unit, uint8_t ctrl, void *data, int len) {
    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    return winusb_SendControl(
        devh->winusbHandle,
        UVC_REQ_TYPE_INTERFACE_SET, UVC_SET_CUR,
        ctrl << 8,
        unit << 8 | devh->deviceData.ctrl_if.bInterfaceNumber,		// XXX saki
        static_cast<unsigned char*>(data),
        len);
}

uvc_error_t uvc_get_power_mode(winusb_uvc_device *devh, enum uvc_device_power_mode *mode, enum uvc_req_code req_code) {
    uint8_t mode_char;
    int ret;
    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    ret = winusb_SendControl(
        devh->winusbHandle,
        UVC_REQ_TYPE_INTERFACE_GET, req_code,
        UVC_VC_VIDEO_POWER_MODE_CONTROL << 8,
        devh->deviceData.ctrl_if.bInterfaceNumber,
        (unsigned char *)&mode_char,
        sizeof(mode_char));

    if (ret == 1) {
        *mode = static_cast<uvc_device_power_mode>(mode_char);
        return UVC_SUCCESS;
    }
    else {
        return static_cast<uvc_error_t>(ret);
    }
}

uvc_error_t uvc_set_power_mode(winusb_uvc_device *devh, enum uvc_device_power_mode mode) {
    uint8_t mode_char = mode;
    int ret;

    if (!devh)
        return UVC_ERROR_NO_DEVICE;

    ret = winusb_SendControl(
        devh->winusbHandle,
        UVC_REQ_TYPE_INTERFACE_SET, UVC_SET_CUR,
        UVC_VC_VIDEO_POWER_MODE_CONTROL << 8,
        devh->deviceData.ctrl_if.bInterfaceNumber,
        (unsigned char *)&mode_char,
        sizeof(mode_char));

    if (ret == 1)
        return UVC_SUCCESS;
    else
        return static_cast<uvc_error_t>(ret);
}

// Sending control packet using vendor-defined control transfer directed to WinUSB interface
// WinUSB Setup packet info is according to Universal Serial Bus Specification:
// RequestType: Bits[4..0] - Receipient: device (0) interface (1) endpoint (2) other (3), 
//              Bits[6..5] - Type: Standard (0) Class (1) Vendor Specific (2) Reserved (3)
//              Bit [7]    - Direction: Host-to-device (0), Device-to-host (1)
// Request: Vendor defined request number
// Value: Vendor defined Value number (0x0000-0xFFFF)
// Index: If RequestType is directed to the device, index field is available for any vendor use.
//        If RequestType is directed to an interface, the WinUSB driver passes the interface number in the low byte of index so only the high byte is available for vendor use.
//        If RequestType is directed to an endpoint, index lower byte must be the endpoint address.
// Length: Number of bytes to transfer (0x0000-0xFFFF)
int winusb_SendControl(WINUSB_INTERFACE_HANDLE ihandle, int requestType, int request, int value, int index, unsigned char *buffer, int buflen)
{
    WINUSB_SETUP_PACKET setupPacket;
    ULONG lengthOutput;

    setupPacket.RequestType = requestType;
    setupPacket.Request = request;
    setupPacket.Value = value;
    setupPacket.Index = index;
    setupPacket.Length = buflen;

    if (!WinUsb_ControlTransfer(ihandle, setupPacket, buffer, buflen, &lengthOutput, NULL))
    {
        return -1;
    }
    else
    {
        return lengthOutput;
    }

    return 0;
}

uvc_error_t winusb_find_devices(const std::string &uvc_interface, int vid, int pid, winusb_uvc_device ***devs, int& devs_count)
{
    GUID guid;
    std::wstring guidWStr(uvc_interface.begin(), uvc_interface.end());
    CLSIDFromString(guidWStr.c_str(), static_cast<LPCLSID>(&guid));

    HDEVINFO hDevInfo;
    SP_DEVICE_INTERFACE_DATA DevIntfData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
    SP_DEVINFO_DATA DevData;
    winusb_uvc_device **list_internal;
    int num_uvc_devices = 0;
    DWORD dwSize = 0;
    DWORD dwMemberIdx = 0;
    uvc_error_t ret = UVC_SUCCESS;

    list_internal = (winusb_uvc_device **)malloc(sizeof(*list_internal));
    *list_internal = NULL;

    // Return a handle to device information set with connected IVCAM device
    hDevInfo = SetupDiGetClassDevs(&guid, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        dwMemberIdx = 0;

        // Enumerates IVCAM device
        SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, dwMemberIdx, &DevIntfData);

        DevIntfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(2048);

        while (GetLastError() != ERROR_NO_MORE_ITEMS)
        {
            DevData.cbSize = sizeof(DevData);

            // Returns required buffer size for saving device interface details
            SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

            DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            // Returns details about connected device
            if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, DevIntfDetailData, dwSize, &dwSize, &DevData) == TRUE)
            {

                // Create a handle for I/O operations to the IVCAM device
                HANDLE Devicehandle = CreateFile(DevIntfDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
                if (Devicehandle != INVALID_HANDLE_VALUE)
                {
                    WINUSB_INTERFACE_HANDLE winusbHandle;

                    // Create WinUSB device handle for the IVCAM device
                    if (WinUsb_Initialize(Devicehandle, &winusbHandle) == TRUE)
                    {
                        USB_DEVICE_DESCRIPTOR deviceDescriptor;
                        ULONG returnLength;

                        // Returns IVCAM device descriptor which includes PID/VID info
                        if (!WinUsb_GetDescriptor(winusbHandle, USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, (PUCHAR)&deviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR), &returnLength))
                        {
                            printf("WinUsb_GetDescriptor failed - GetLastError = %d\n", GetLastError());
                            ret = UVC_ERROR_INVALID_PARAM;
                        }
                        else
                        {
                            if (deviceDescriptor.idVendor == vid && deviceDescriptor.idProduct == pid)
                            {
                                winusb_uvc_device *winUsbDevice = new winusb_uvc_device;
                                winUsbDevice->deviceHandle = NULL;
                                winUsbDevice->winusbHandle = NULL;
                                winUsbDevice->pid = pid;
                                winUsbDevice->vid = vid;
                                size_t devicePathLength = wcslen(DevIntfDetailData->DevicePath);
                                winUsbDevice->devPath = new WCHAR[devicePathLength + 1];
                                wcscpy_s(winUsbDevice->devPath, devicePathLength + 1, DevIntfDetailData->DevicePath);

                                num_uvc_devices++;
                                list_internal = (winusb_uvc_device **)realloc(list_internal, (num_uvc_devices + 1) * sizeof(*list_internal));
                                list_internal[num_uvc_devices - 1] = winUsbDevice;
                                list_internal[num_uvc_devices] = NULL;
                            }
                        }

                        WinUsb_Free(winusbHandle);
                    }
                    else
                    {
                        printf("WinUsb_Initialize failed - GetLastError = %d\n", GetLastError());
                        ret = UVC_ERROR_INVALID_PARAM;
                    }
                }
                else
                {
                    auto error = GetLastError();
                    printf("CreateFile failed - GetLastError = %d\n", GetLastError());
                    ret = UVC_ERROR_INVALID_PARAM;
                }

                if (Devicehandle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(Devicehandle);
                }
            }
            else
            {
                printf("SetupDiGetDeviceInterfaceDetail failed - GetLastError = %d\n", GetLastError());
                ret = UVC_ERROR_INVALID_PARAM;
            }

            // Continue looping on all connected IVCAM devices
            SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &guid, ++dwMemberIdx, &DevIntfData);
        }

        if (DevIntfDetailData != NULL)
        {
            free(DevIntfDetailData);
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);

        *devs = list_internal;
        devs_count = num_uvc_devices;
    }
    else
    {
        printf("SetupDiGetClassDevs failed - GetLastError = %d\n", GetLastError());
        ret = UVC_ERROR_INVALID_PARAM;
    }

    return ret;
}


// Return list of all connected IVCAM devices
uvc_error_t winusb_find_devices(winusb_uvc_device ***devs, int vid, int pid)
{
    // Intel(R) RealSense(TM) 415 Depth - MI 0
    // bInterfaceNumber 0 video control - endpoint 0x87 (FW->Host)
    // bInterfaceNumber 1 video stream - endpoint 0x82 (FW->Host)
    // bInterfaceNumber 2 video stream - endpoint 0x83 (FW->Host)
    //const GUID IVCAM_WIN_USB_DEVICE_GUID = { 0xe659c3ec, 0xbf3c, 0x48a5,{ 0x81, 0x92, 0x30, 0x73, 0xe8, 0x22, 0xd7, 0xcd } }; 

    // Intel(R) RealSense(TM) 415 RGB - MI 3
    // bInterfaceNumber 3 video control
    // bInterfaceNumber 4 video stream - endpoint 0x84 (FW->Host)
    const GUID IVCAM_WIN_USB_DEVICE_GUID = { 0x50537bc3, 0x2919, 0x452d,{ 0x88, 0xa9, 0xb1, 0x3b, 0xbf, 0x7d, 0x24, 0x59 } };

    HDEVINFO hDevInfo;
    SP_DEVICE_INTERFACE_DATA DevIntfData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA DevIntfDetailData;
    SP_DEVINFO_DATA DevData;
    winusb_uvc_device **list_internal;
    int num_uvc_devices = 0;
    DWORD dwSize = 0;
    DWORD dwMemberIdx = 0;
    uvc_error_t ret = UVC_SUCCESS;

    list_internal = (winusb_uvc_device **)malloc(sizeof(*list_internal));
    *list_internal = NULL;

    // Return a handle to device information set with connected IVCAM device
    hDevInfo = SetupDiGetClassDevs(&IVCAM_WIN_USB_DEVICE_GUID, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (hDevInfo != INVALID_HANDLE_VALUE)
    {
        DevIntfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
        dwMemberIdx = 0;

        // Enumerates IVCAM device
        SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &IVCAM_WIN_USB_DEVICE_GUID, dwMemberIdx, &DevIntfData);

        DevIntfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(2048);

        while (GetLastError() != ERROR_NO_MORE_ITEMS)
        {
            DevData.cbSize = sizeof(DevData);

            // Returns required buffer size for saving device interface details
            SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, NULL, 0, &dwSize, NULL);

            DevIntfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

            // Returns details about connected device
            if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &DevIntfData, DevIntfDetailData, dwSize, &dwSize, &DevData) == TRUE)
            {

                // Create a handle for I/O operations to the IVCAM device
                HANDLE Devicehandle = CreateFile(DevIntfDetailData->DevicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
                if (Devicehandle != INVALID_HANDLE_VALUE)
                {
                    WINUSB_INTERFACE_HANDLE winusbHandle;

                    // Create WinUSB device handle for the IVCAM device
                    if (WinUsb_Initialize(Devicehandle, &winusbHandle) == TRUE)
                    {
                        USB_DEVICE_DESCRIPTOR deviceDescriptor;
                        ULONG returnLength;

                        // Returns IVCAM device descriptor which includes PID/VID info
                        if (!WinUsb_GetDescriptor(winusbHandle, USB_DEVICE_DESCRIPTOR_TYPE, 0, 0, (PUCHAR)&deviceDescriptor, sizeof(USB_DEVICE_DESCRIPTOR), &returnLength))
                        {
                            printf("WinUsb_GetDescriptor failed - GetLastError = %d\n", GetLastError());
                            ret = UVC_ERROR_INVALID_PARAM;
                        }
                        else
                        {
                            if (deviceDescriptor.idVendor == vid && deviceDescriptor.idProduct == pid)
                            {
                                winusb_uvc_device *winUsbDevice = new winusb_uvc_device;
                                winUsbDevice->deviceHandle = NULL;
                                winUsbDevice->winusbHandle = NULL;
                                winUsbDevice->pid = pid;
                                winUsbDevice->vid = vid;
                                size_t devicePathLength = wcslen(DevIntfDetailData->DevicePath);
                                winUsbDevice->devPath = new WCHAR[devicePathLength + 1];
                                wcscpy_s(winUsbDevice->devPath, devicePathLength + 1, DevIntfDetailData->DevicePath);

                                num_uvc_devices++;
                                list_internal = (winusb_uvc_device **)realloc(list_internal, (num_uvc_devices + 1) * sizeof(*list_internal));

                                list_internal[num_uvc_devices - 1] = winUsbDevice;
                                list_internal[num_uvc_devices] = NULL;
                            }
                        }

                        WinUsb_Free(winusbHandle);
                    }
                    else
                    {
                        printf("WinUsb_Initialize failed - GetLastError = %d\n", GetLastError());
                        ret = UVC_ERROR_INVALID_PARAM;
                    }
                }
                else
                {
                    printf("CreateFile failed - GetLastError = %d\n", GetLastError());
                    ret = UVC_ERROR_INVALID_PARAM;
                }

                if (Devicehandle != INVALID_HANDLE_VALUE)
                {
                    CloseHandle(Devicehandle);
                }
            }
            else
            {
                printf("SetupDiGetDeviceInterfaceDetail failed - GetLastError = %d\n", GetLastError());
                ret = UVC_ERROR_INVALID_PARAM;
            }

            // Continue looping on all connected IVCAM devices
            SetupDiEnumDeviceInterfaces(hDevInfo, NULL, &IVCAM_WIN_USB_DEVICE_GUID, ++dwMemberIdx, &DevIntfData);
        }

        if (DevIntfDetailData != NULL)
        {
            free(DevIntfDetailData);
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);

        *devs = list_internal;
    }
    else
    {
        printf("SetupDiGetClassDevs failed - GetLastError = %d\n", GetLastError());
        ret = UVC_ERROR_INVALID_PARAM;
    }

    return ret;
}

// Iterate over all descriptors and parse all Interface and Endpoint descriptors
void winusb_uvc_parse_config_descriptors(USB_CONFIGURATION_DESCRIPTOR *cfgDesc, winusb_uvc_interfaces **interfaces)
{
    int length = cfgDesc->wTotalLength - sizeof(USB_CONFIGURATION_DESCRIPTOR);
    int curLocation = 0;
    int interfaceDescStart = -1;
    winusb_uvc_interfaces *descInterfaces = new winusb_uvc_interfaces;

    memset(descInterfaces, 0, sizeof(winusb_uvc_interfaces));
    winusb_uvc_interface *curInterface = NULL;
    USB_INTERFACE_DESCRIPTOR *curDescriptor = NULL;
    descInterfaces->numInterfaces = 0;

    PUCHAR descriptors = (PUCHAR)cfgDesc;
    descriptors += cfgDesc->bLength;

    // iterate over all descriptors
    do {
        int descLen = descriptors[curLocation];
        //printf("parse : length : %d type : %x first byte: %x\n", descLen, descriptors[curLocation + 1], descriptors[curLocation + 2]);
        if (descriptors[curLocation + 1] == USB_INTERFACE_DESCRIPTOR_TYPE || (curLocation + descLen) >= length)
        {
            if (interfaceDescStart != -1)
            {
                // Saving previous interface with all extra data afterwards (up to this interface)
                memcpy(&curInterface->desc, &descriptors[interfaceDescStart], sizeof(curInterface->desc));
                interfaceDescStart += sizeof(USB_INTERFACE_DESCRIPTOR);
                curInterface->extraLength = curLocation - interfaceDescStart;
                curInterface->extra = new UCHAR[curInterface->extraLength];
                memcpy(curInterface->extra, &descriptors[interfaceDescStart], curInterface->extraLength);
                curInterface->winusbInterfaceNumber = descInterfaces->numInterfaces;
                descInterfaces->numInterfaces++;
            }

            if ((curLocation + descLen) < length)
            {
                // Setting to new found interface descriptor pointer
                interfaceDescStart = curLocation;
                curDescriptor = (USB_INTERFACE_DESCRIPTOR *)&descriptors[interfaceDescStart];
                curInterface = &descInterfaces->iface[curDescriptor->bInterfaceNumber];
                curInterface->numEndpoints = 0;
            }
        }
        else if (descriptors[curLocation + 1] == USB_ENDPOINT_DESCRIPTOR_TYPE)
        {
            memcpy(&curInterface->endpoints[curInterface->numEndpoints], &descriptors[curLocation], descLen);
            curInterface->numEndpoints++;
        }

        curLocation += descLen;
        //printf("length = %d\n", curLocation);
    } while (curLocation < length);

    *interfaces = descInterfaces;
}

uvc_error_t winusb_open(winusb_uvc_device *device)
{
    USB_CONFIGURATION_DESCRIPTOR cfgDesc;
    winusb_uvc_interfaces* interfaces = NULL;
    UCHAR *descriptors = NULL;
    uvc_error_t ret = UVC_SUCCESS;
    ULONG returnLength = 0;

    device->associateHandle = NULL;
    device->streams = NULL;

    // Start by clearing deviceData, otherwise
    // winusb_uvc_free_device_info(&device->deviceData);
    // will do something not good
    memset(&device->deviceData, 0, sizeof(winusb_uvc_device_info_t));

    // Create a handle for I/O operations to the IVCAM device
    device->deviceHandle = CreateFile(device->devPath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_OVERLAPPED,
        NULL);

    if (device->deviceHandle == INVALID_HANDLE_VALUE)
    {
        printf("CreateFile failed - GetLastError = %d\n", GetLastError());
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }

    // Create WinUSB device handle for the IVCAM device
    if (!WinUsb_Initialize(device->deviceHandle, &device->winusbHandle))
    {
        printf("WinUsb_Initialize failed - GetLastError = %d\n", GetLastError());
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }

    // Returns IVCAM configuration descriptor
    if (!WinUsb_GetDescriptor(device->winusbHandle, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, (PUCHAR)&cfgDesc, sizeof(cfgDesc), &returnLength))
    {
        printf("WinUsb_GetDescriptor failed - GetLastError = %d\n", GetLastError());
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }

    descriptors = new UCHAR[cfgDesc.wTotalLength];

    // Returns IVCAM configuration descriptor - including all interface, endpoint, class-specific, and vendor-specific descriptors
    if (!WinUsb_GetDescriptor(device->winusbHandle, USB_CONFIGURATION_DESCRIPTOR_TYPE, 0, 0, descriptors, cfgDesc.wTotalLength, &returnLength))
    {
        printf("WinUsb_GetDescriptor failed - GetLastError = %d\n", GetLastError());
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }
    device->deviceData.config = cfgDesc;

    // Iterate over all descriptors and parse all Interface and Endpoint descriptors
    winusb_uvc_parse_config_descriptors((USB_CONFIGURATION_DESCRIPTOR *)descriptors, &interfaces);
    device->deviceData.interfaces = interfaces;

    if (descriptors)
    {
        delete descriptors;
        descriptors = NULL;
    }

    // Fill fields of uvc_device_info on device
    ret = winusb_uvc_scan_control(device, &device->deviceData);
    if (ret != UVC_SUCCESS)
    {
        printf("uvc_scan_control failed\n");
        goto fail;
    }

    return ret;

fail:

    if (descriptors)
    {
        delete descriptors;
    }

    if (device)
    {
        winusb_uvc_free_device_info(&device->deviceData);

        if (device->deviceHandle)
        {
            CloseHandle(device->deviceHandle);
            device->deviceHandle = NULL;
        }

        if (device->winusbHandle)
        {
            WinUsb_Free(device->winusbHandle);
            device->winusbHandle = NULL;
        }
    }

    return ret;
}


uvc_error_t winusb_close(winusb_uvc_device *device)
{
    uvc_error_t ret = UVC_SUCCESS;

    if (device != NULL)
    {
        winusb_uvc_free_device_info(&device->deviceData);
        memset(&device->deviceData, 0, sizeof(winusb_uvc_device_info_t));

        if (device->winusbHandle != NULL)
        {
            WinUsb_Free(device->winusbHandle);
            device->winusbHandle = NULL;
        }

        if (device->associateHandle != NULL)
        {
            WinUsb_Free(device->associateHandle);
            device->associateHandle = NULL;
        }

        if (device->deviceHandle != NULL)
        {
            CloseHandle(device->deviceHandle);
            device->deviceHandle = NULL;
        }

        device->streams = NULL;

        free(device->devPath);
        free(device);
    }
    else
    {
        printf("Error: device == NULL\n");
        ret = UVC_ERROR_NO_DEVICE;
    }

    return ret;
}

int winusb_init() {
    return 0;
}

void winusb_deinit() {

}

bool read_all_uvc_descriptors(winusb_uvc_device *device, PUCHAR buffer, ULONG bufferLength, PULONG lengthReturned) 
{
    UCHAR b[2048];
    ULONG returnLength;

    /*WinUsb_GetAssociatedInterface(device->deviceHande, 1
        , &iHandle);*/

    if (!WinUsb_GetDescriptor(device->deviceHandle,
        USB_CONFIGURATION_DESCRIPTOR_TYPE,
        0,
        0,
        b,
        2048,
        &returnLength))
    {
        printf("GetLastError = %d\n", GetLastError());
    }

    printf("success");

    return 0;
}

#endif
