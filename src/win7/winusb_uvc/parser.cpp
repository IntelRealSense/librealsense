#include "windows.h"
#include "winusb_internal.h"
#include "parser.h"
#include "utlist.h"

uvc_error_t uvc_parse_vc(
    winusb_uvc_device *dev,
    uvc_device_info_t *info,
    const unsigned char *block, size_t block_size);

uvc_error_t uvc_scan_streaming(winusb_uvc_device *dev,
    uvc_device_info_t *info,
    int interface_idx);

uvc_error_t uvc_parse_vs(
    winusb_uvc_device *dev,
    uvc_device_info_t *info,
    uvc_streaming_interface_t *stream_if,
    const unsigned char *block, size_t block_size);

// Iterate over all descriptors and parse all Interface and Endpoint descriptors
void ParseConfigDescriptors(USB_CONFIGURATION_DESCRIPTOR *cfgDesc, WINUSB_INTERFACES **interfaces) 
{
    int length = cfgDesc->wTotalLength - sizeof(USB_CONFIGURATION_DESCRIPTOR);
    int curLocation = 0;
    int interfaceDescStart = -1;
    WINUSB_INTERFACES *descInterfaces = new WINUSB_INTERFACES;

    memset(descInterfaces, 0, sizeof(WINUSB_INTERFACES));
    WINUSB_INTERFACE *curInterface = NULL;
    USB_INTERFACE_DESCRIPTOR *curDescriptor = NULL;
    descInterfaces->numInterfaces = 0;

    PUCHAR descriptors = (PUCHAR)cfgDesc;
    descriptors += cfgDesc->bLength;

    // iterate over all descriptors
    do {
        int descLen = descriptors[curLocation];
        //printf("parse : length : %d type : %x first byte: %x\n", descLen, descriptors[curLocation + 1], descriptors[curLocation + 2]);
        if (descriptors[curLocation+1] == USB_INTERFACE_DESCRIPTOR_TYPE || (curLocation + descLen) >= length)
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
                curInterface = &descInterfaces->interfaces[curDescriptor->bInterfaceNumber];
                curInterface->numEndpoints = 0;
            }
        }
        else if (descriptors[curLocation+1] == USB_ENDPOINT_DESCRIPTOR_TYPE) 
        {
            memcpy(&curInterface->endpoints[curInterface->numEndpoints], &descriptors[curLocation], descLen);
            curInterface->numEndpoints++;
        }

        curLocation += descLen;
        //printf("length = %d\n", curLocation);
    } while (curLocation < length);

    *interfaces = descInterfaces;
}

void FreeWinusbInterfaces(WINUSB_INTERFACES *winusbInterfaces) {
    for (int i = 0; i < 10; i++) {
        if (winusbInterfaces->interfaces[i].extra != NULL) {
            delete winusbInterfaces->interfaces[i].extra;
            winusbInterfaces->interfaces[i].extra = NULL;
        }
    }

    delete winusbInterfaces;
}

void FreeStreamInterfaces(uvc_streaming_interface_t * streamInterfaces) 
{
    uvc_streaming_interface_t *stream_if = NULL;
    uvc_streaming_interface_t *stream_if_tmp = NULL;

    DL_FOREACH_SAFE(streamInterfaces, stream_if, stream_if_tmp)
    {
        uvc_format_desc_t *format = NULL;
        uvc_format_desc_t *format_tmp = NULL;

        DL_FOREACH_SAFE(stream_if->format_descs, format, format_tmp)
        {
            uvc_frame_desc_t *frame = NULL;
            uvc_frame_desc_t *frame_tmp = NULL;

            DL_FOREACH_SAFE(format->frame_descs, frame, frame_tmp)
            {
                delete frame;
            }

            delete format;
        }

        delete stream_if;
    }
}

uvc_error_t uvc_scan_control(winusb_uvc_device *dev, uvc_device_info_t *info) {
    USB_INTERFACE_DESCRIPTOR *if_desc;
    uvc_error_t parse_ret, ret;
    int interface_idx;
    const unsigned char *buffer;
    size_t buffer_left, block_size;

    ret = UVC_SUCCESS;
    if_desc = NULL;

    for (interface_idx = 0; interface_idx < 10; ++interface_idx) {
        if_desc = &info->interfaces->interfaces[interface_idx].desc;

        if (if_desc->bInterfaceClass == 14 && if_desc->bInterfaceSubClass == 1) // Video, Control
            break;

        if_desc = NULL;
    }

    if (if_desc == NULL) {
        return UVC_ERROR_INVALID_DEVICE;
    }

    info->ctrl_if.bInterfaceNumber = interface_idx;
    if (if_desc->bNumEndpoints != 0) {
        info->ctrl_if.bEndpointAddress = info->interfaces->interfaces[interface_idx].endpoints[0].bEndpointAddress;
    }

    buffer = info->interfaces->interfaces[interface_idx].extra;
    buffer_left = info->interfaces->interfaces[interface_idx].extraLength;

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
* @brief Parse a VideoControl header.
* @ingroup device
*/
uvc_error_t uvc_parse_vc_header(winusb_uvc_device *dev,
    uvc_device_info_t *info,
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
        scan_ret = uvc_scan_streaming(dev, info, block[i]);
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
    uvc_device_info_t *info,
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
    uvc_device_info_t *info,
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
    uvc_device_info_t *info,
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
    uvc_device_info_t *info,
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
    uvc_device_info_t *info,
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

/** @internal
* Process a VideoStreaming interface
* @ingroup device
*/
uvc_error_t uvc_scan_streaming(winusb_uvc_device *dev,
    uvc_device_info_t *info,
    int interface_idx) {
    const struct WINUSB_INTERFACE *if_desc;
    const unsigned char *buffer;
    int buffer_left, block_size;
    uvc_error_t ret, parse_ret;
    uvc_streaming_interface_t *stream_if;


    ret = UVC_SUCCESS;

    if_desc = &(info->interfaces->interfaces[interface_idx]);
    buffer = if_desc->extra;
    buffer_left = if_desc->extraLength - sizeof(USB_INTERFACE_DESCRIPTOR);

    stream_if = (uvc_streaming_interface_t *)calloc(1, sizeof(*stream_if));
    memset(stream_if, 0, sizeof(uvc_streaming_interface_t));
    stream_if->parent = info;
    stream_if->bInterfaceNumber = if_desc->desc.bInterfaceNumber;
    DL_APPEND(info->stream_ifs, stream_if);

    while (buffer_left >= 3) {
        block_size = buffer[0];
        parse_ret = uvc_parse_vs(dev, info, stream_if, buffer, block_size);

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
uvc_error_t uvc_parse_vs_input_header(uvc_streaming_interface_t *stream_if,
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
uvc_error_t uvc_parse_vs_format_uncompressed(uvc_streaming_interface_t *stream_if,
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
uvc_error_t uvc_parse_vs_frame_format(uvc_streaming_interface_t *stream_if,
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
uvc_error_t uvc_parse_vs_format_mjpeg(uvc_streaming_interface_t *stream_if,
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
uvc_error_t uvc_parse_vs_frame_frame(uvc_streaming_interface_t *stream_if,
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
        frame->intervals = (uint32_t *)malloc((block[21] + 1) *sizeof(uint32_t));
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
uvc_error_t uvc_parse_vs_frame_uncompressed(uvc_streaming_interface_t *stream_if,
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
uvc_error_t uvc_parse_vs(
    winusb_uvc_device *dev,
    uvc_device_info_t *info,
    uvc_streaming_interface_t *stream_if,
    const unsigned char *block, size_t block_size) {
    uvc_error_t ret;
    int descriptor_subtype;

    ret = UVC_SUCCESS;
    descriptor_subtype = block[2];

    switch (descriptor_subtype) {
    case UVC_VS_INPUT_HEADER:
        ret = uvc_parse_vs_input_header(stream_if, block, block_size);
        break;
    case UVC_VS_FORMAT_UNCOMPRESSED:
        ret = uvc_parse_vs_format_uncompressed(stream_if, block, block_size);
        break;
    case UVC_VS_FORMAT_MJPEG:
        ret = uvc_parse_vs_format_mjpeg(stream_if, block, block_size);
        break;
    case UVC_VS_FRAME_UNCOMPRESSED:
    case UVC_VS_FRAME_MJPEG:
        ret = uvc_parse_vs_frame_uncompressed(stream_if, block, block_size);
        break;
    case UVC_VS_FORMAT_FRAME_BASED:
        ret = uvc_parse_vs_frame_format(stream_if, block, block_size);
        break;
    case UVC_VS_FRAME_FRAME_BASED:
        ret = uvc_parse_vs_frame_frame(stream_if, block, block_size);
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
