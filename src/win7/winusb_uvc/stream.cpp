#include "windows.h"
#include "winusb_uvc.h"
#include "utlist.h"
#include "backend.h"

struct format_table_entry {
    enum uvc_frame_format format;
    uint8_t abstract_fmt;
    uint8_t guid[16];
    int children_count;
    enum uvc_frame_format *children;
};

struct format_table_entry *_get_format_entry(enum uvc_frame_format format) {
#define ABS_FMT(_fmt, _num, ...) \
    case _fmt: { \
    static enum uvc_frame_format _fmt##_children[] = __VA_ARGS__; \
    static struct format_table_entry _fmt##_entry = { \
      _fmt, 0, {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, _num, _fmt##_children }; \
    return &_fmt##_entry; }

#define FMT(_fmt, ...) \
    case _fmt: { \
    static struct format_table_entry _fmt##_entry = { \
      _fmt, 0, __VA_ARGS__, 0, NULL }; \
    return &_fmt##_entry; }

    switch (format) {
        /* Define new formats here */
        ABS_FMT(UVC_FRAME_FORMAT_ANY, 2,
        { UVC_FRAME_FORMAT_UNCOMPRESSED, UVC_FRAME_FORMAT_COMPRESSED })

        ABS_FMT(UVC_FRAME_FORMAT_UNCOMPRESSED, 3,
        { UVC_FRAME_FORMAT_YUYV, UVC_FRAME_FORMAT_UYVY, UVC_FRAME_FORMAT_GRAY8 })
        FMT(UVC_FRAME_FORMAT_YUYV,
        { 'Y', 'U', 'Y', '2', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 })
        FMT(UVC_FRAME_FORMAT_UYVY,
        { 'U', 'Y', 'V', 'Y', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 })
        FMT(UVC_FRAME_FORMAT_GRAY8,
        { 'Y', '8', '0', '0', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 })
        FMT(UVC_FRAME_FORMAT_BY8,
        { 'B', 'Y', '8', ' ', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 })

        ABS_FMT(UVC_FRAME_FORMAT_COMPRESSED, 1,
        { UVC_FRAME_FORMAT_MJPEG })
        FMT(UVC_FRAME_FORMAT_MJPEG,
        { 'M', 'J', 'P', 'G' })

    default:
        return NULL;
    }
#undef ABS_FMT
#undef FMT
}


static uvc_streaming_interface_t *_uvc_get_stream_if(winusb_uvc_device *devh, int interface_idx);

uvc_frame_desc_t *uvc_find_frame_desc(winusb_uvc_device *devh,
    uint16_t format_id, uint16_t frame_id);

uvc_error_t uvc_query_stream_ctrl(winusb_uvc_device *devh, uvc_stream_ctrl_t *ctrl, uint8_t probe, int req)
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
            uint8_t winusbInterfaceNumber = devh->deviceData.interfaces->interfaces[interfaceNumber].winusbInterfaceNumber;
            WinUsb_GetAssociatedInterface(devh->winusbHandle, winusbInterfaceNumber-1, &iface);
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
        buf, len );

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
            uvc_frame_desc_t *frame = uvc_find_frame_desc(devh, ctrl->bFormatIndex, ctrl->bFrameIndex);

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
* @param[in] ctrl Control block, processed using {uvc_probe_stream_ctrl} or
*             {uvc_get_stream_ctrl_format_size}
*/
uvc_error_t uvc_stream_ctrl(uvc_stream_handle_t *strmh, uvc_stream_ctrl_t *ctrl) 
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
    ret = uvc_query_stream_ctrl(strmh->devh, ctrl, 0, UVC_SET_CUR);
    if (ret != UVC_SUCCESS)
    {
        return ret;
    }

    strmh->cur_ctrl = *ctrl;
    return UVC_SUCCESS;
}

/** @internal
* @brief Find the descriptor for a specific frame configuration
* @param stream_if Stream interface
* @param format_id Index of format class descriptor
* @param frame_id Index of frame descriptor
*/
static uvc_frame_desc_t *_uvc_find_frame_desc_stream_if(uvc_streaming_interface_t *stream_if,
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

uvc_frame_desc_t *uvc_find_frame_desc_stream(uvc_stream_handle_t *strmh,
    uint16_t format_id, uint16_t frame_id) {
    return _uvc_find_frame_desc_stream_if(strmh->stream_if, format_id, frame_id);
}

/** @internal
* @brief Find the descriptor for a specific frame configuration
* @param devh UVC device
* @param format_id Index of format class descriptor
* @param frame_id Index of frame descriptor
*/
uvc_frame_desc_t *uvc_find_frame_desc(winusb_uvc_device *devh,
    uint16_t format_id, uint16_t frame_id) {

    uvc_streaming_interface_t *stream_if;
    uvc_frame_desc_t *frame;

    DL_FOREACH(devh->deviceData.stream_ifs, stream_if) {
        frame = _uvc_find_frame_desc_stream_if(stream_if, format_id, frame_id);
        if (frame)
            return frame;
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

    uvc_streaming_interface_t *stream_if = NULL;
    uvc_format_t *prev_format = NULL;
    uvc_format_desc_t *format;

    stream_if = _uvc_get_stream_if(devh, interface_idx);
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

    uvc_streaming_interface_t *stream_if = NULL;
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

static uvc_stream_handle_t *_uvc_get_stream_by_interface(winusb_uvc_device *devh, int interface_idx) {
    uvc_stream_handle_t *strmh;

    DL_FOREACH(devh->streams, strmh) {
        if (strmh->stream_if->bInterfaceNumber == interface_idx)
            return strmh;
    }

    return NULL;
}


// Probe (Set and Get) streaming control block
uvc_error_t uvc_probe_stream_ctrl(winusb_uvc_device *devh, uvc_stream_ctrl_t *ctrl) 
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
        uint8_t winusbInterfaceNumber = devh->deviceData.interfaces->interfaces[interfaceNumber].winusbInterfaceNumber;
        WinUsb_GetAssociatedInterface(devh->winusbHandle, winusbInterfaceNumber - 1, &devh->associateHandle);
    }
    else
    {
        return UVC_ERROR_INVALID_PARAM;
    }

    // Sending probe SET request - UVC_SET_CUR request in a probe/commit structure containing desired values for VS Format index, VS Frame index, and VS Frame Interval
    // UVC device will check the VS Format index, VS Frame index, and Frame interval properties to verify if possible and update the probe/commit structure if feasible
    ret = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_SET_CUR);
    if (ret != UVC_SUCCESS)
    {
        return ret;
    }

    // Sending probe GET request - UVC_GET_CUR request to read the updated values from UVC device
    ret = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_CUR);
    if (ret != UVC_SUCCESS)
    {
        return ret;
    }

    /** @todo make sure that worked */
    return UVC_SUCCESS;
}

/* Return only Video stream interfaces */
static uvc_streaming_interface_t *_uvc_get_stream_if(winusb_uvc_device *devh, int interface_idx) {
    uvc_streaming_interface_t *stream_if;

    DL_FOREACH(devh->deviceData.stream_ifs, stream_if) {
        if (stream_if->bInterfaceNumber == interface_idx)
            return stream_if;
    }

    return NULL;
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
    uvc_streaming_interface_t *stream_if;
    uvc_format_desc_t *format;
    uvc_error_t ret = UVC_SUCCESS;

    stream_if = _uvc_get_stream_if(devh, interface_idx);
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
                        ret = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);
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
                    ret = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);
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
    return uvc_probe_stream_ctrl(devh, ctrl);
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
    uvc_streaming_interface_t *stream_if;
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
                            ret = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);
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
                        ret = uvc_query_stream_ctrl(devh, ctrl, 1, UVC_GET_MAX);
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
    return uvc_probe_stream_ctrl(devh, ctrl);
}

uvc_error_t uvc_stream_open_ctrl(winusb_uvc_device *devh, uvc_stream_handle_t **strmhp, uvc_stream_ctrl_t *ctrl) {
    /* Chosen frame and format descriptors */
    uvc_stream_handle_t *strmh = NULL;
    uvc_streaming_interface_t *stream_if;
    uvc_error_t ret;

    if (_uvc_get_stream_by_interface(devh, ctrl->bInterfaceNumber) != NULL) {
        ret = UVC_ERROR_BUSY; /* Stream is already opened */
        goto fail;
    }

    stream_if = _uvc_get_stream_if(devh, ctrl->bInterfaceNumber);
    if (!stream_if) {
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }

    strmh = (uvc_stream_handle_t *)calloc(1, sizeof(*strmh));
    if (!strmh) {
        ret = UVC_ERROR_NO_MEM;
        goto fail;
    }
    memset(strmh, 0, sizeof(uvc_stream_handle_t));
    strmh->devh = devh;
    strmh->stream_if = stream_if;
    //strmh->frame.library_owns_data = 1;

    /*ret = uvc_claim_if(strmh->devh, strmh->stream_if->bInterfaceNumber);
    if (ret != UVC_SUCCESS)
        goto fail;*/

    ret = uvc_stream_ctrl(strmh, ctrl);
    if (ret != UVC_SUCCESS)
        goto fail;

    // Set up the streaming status and data space
    strmh->running = 0;
    /** @todo take only what we need */
    strmh->outbuf = (uint8_t *)malloc(LIBUVC_XFER_BUF_SIZE);
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

static enum uvc_frame_format uvc_frame_format_for_guid(uint8_t guid[16]) {
    struct format_table_entry *format;
    int fmt;

    for ( fmt = 0; fmt < (uvc_frame_format) UVC_FRAME_FORMAT_COUNT;  ++fmt) {
        format = _get_format_entry((uvc_frame_format)fmt);
        if (!format || format->abstract_fmt)
            continue;
        if (!memcmp(format->guid, guid, 16))
            return format->format;
    }

    return UVC_FRAME_FORMAT_UNKNOWN;
}

void _uvc_swap_buffers(uvc_stream_handle_t *strmh) {
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

void _uvc_process_payload(uvc_stream_handle_t *strmh, uint8_t *payload, size_t payload_len) {
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
        printf("bogus packet: actual_len=%zd, header_len=%zd\n", payload_len, header_len);
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
            printf("complete buffer : length %d\n", strmh->got_bytes);
            _uvc_swap_buffers(strmh);

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

    if (data_len > 0) {
        memcpy(strmh->outbuf + strmh->got_bytes, payload + header_len, data_len);
        strmh->got_bytes += data_len;

        if (header_info & (1 << 1)) {
            /* The EOF bit is set, so publish the complete frame */
            _uvc_swap_buffers(strmh);
            printf("Complete Frame %d (%d bytes): header info = %08X\n", strmh->seq, payload_len, payload[1]);
        }
    }

    librealsense::platform::frame_object fo{data_len, header_len, payload + header_len , payload };
    strmh->user_cb(&fo, strmh->user_ptr);
}

void stream_thread(uvc_stream_context *strctx)
{
    PUCHAR buffer = (PUCHAR)malloc(strctx->maxPayloadTransferSize);
    memset(buffer, 0, sizeof(strctx->maxPayloadTransferSize));

    do {

        DWORD lengthTransfered;
        if (!WinUsb_ReadPipe(strctx->stream->devh->associateHandle,
            strctx->endpoint,
            buffer,
            strctx->maxPayloadTransferSize,
            &lengthTransfered,
            NULL)) {
            printf("error : %d\n" + GetLastError());
            return;
        }

        //printf("success : %d\n", lengthTransfered);
        _uvc_process_payload(strctx->stream, buffer, lengthTransfered);
    } while (strctx->stream->running);

    free(buffer);
    free(strctx);
};

uvc_error_t uvc_stream_start(
    uvc_stream_handle_t *strmh,
    winusb_uvc_frame_callback_t *cb,
    void *user_ptr,
    uint8_t flags
    ) {
    /* USB interface we'll be using */
    WINUSB_INTERFACE *iface;
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

    frame_desc = uvc_find_frame_desc_stream(strmh, ctrl->bFormatIndex, ctrl->bFrameIndex);
    if (!frame_desc) {
        ret = UVC_ERROR_INVALID_PARAM;
        goto fail;
    }
    format_desc = frame_desc->parent;

    /*strmh->frame_format = uvc_frame_format_for_guid(format_desc->guidFormat);
    if (strmh->frame_format == UVC_FRAME_FORMAT_UNKNOWN) {
        ret = UVC_ERROR_NOT_SUPPORTED;
        goto fail;
    }*/

    // Get the interface that provides the chosen format and frame configuration
    interface_id = strmh->stream_if->bInterfaceNumber;
    iface = &strmh->devh->deviceData.interfaces->interfaces[interface_id];
    
    uvc_stream_context *streamctx = new uvc_stream_context;

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

uvc_error_t uvc_stream_stop(uvc_stream_handle_t *strmh) {
    if (!strmh->running)
        return UVC_ERROR_INVALID_PARAM;

    strmh->running = 0;
    strmh->cb_thread.join();

    return UVC_SUCCESS;
}

void uvc_stream_close(uvc_stream_handle_t *strmh) 
{
    if (strmh->running)
    {
        uvc_stream_stop(strmh);
    }

    free(strmh->outbuf);
    free(strmh->holdbuf);

    DL_DELETE(strmh->devh->streams, strmh);

    free(strmh->user_ptr);
    free(strmh);
}

uvc_error_t winusb_start_streaming(winusb_uvc_device *devh, uvc_stream_ctrl_t *ctrl, winusb_uvc_frame_callback_t *cb, void *user_ptr, uint8_t flags)
{
    uvc_error_t ret;
    uvc_stream_handle_t *strmh;

    ret = uvc_stream_open_ctrl(devh, &strmh, ctrl);
    if (ret != UVC_SUCCESS)
    {
        return ret;
    }

    ret = uvc_stream_start(strmh, cb, user_ptr, flags);
    if (ret != UVC_SUCCESS) 
    {
        uvc_stream_close(strmh);
        return ret;
    }

    return UVC_SUCCESS;
}

void winusb_stop_streaming(winusb_uvc_device *devh)
{
    uvc_stream_handle_t *strmh, *strmh_tmp;

    DL_FOREACH_SAFE(devh->streams, strmh, strmh_tmp) {
        uvc_stream_close(strmh);
    }
}
