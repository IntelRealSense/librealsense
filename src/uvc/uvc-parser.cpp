// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "uvc-parser.h"

//#define UVC_AE_MODE_D0_MANUAL   ( 1 << 0 )
//#define UVC_AE_MODE_D1_AUTO     ( 1 << 1 )
//#define UVC_AE_MODE_D2_SP       ( 1 << 2 )
//#define UVC_AE_MODE_D3_AP       ( 1 << 3 )
//
//#define SWAP_UINT32(x) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))

// Data structures for Backend-Frontend queue:
struct frame;
// We keep no more then 2 frames in between frontend and backend
typedef librealsense::small_heap<frame, 10> frames_archive;

namespace librealsense
{
    namespace platform
    {

        uvc_parser::uvc_parser(const rs_usb_device& usb_device, const uvc_device_info &info) :
                _usb_device(usb_device), _info(info)
        {
            scan_control(_info.mi);
        }

        uvc_parser::~uvc_parser()
        {

        }

        void uvc_parser::parse_video_stream_input_header(const std::vector<uint8_t>& block)
        {
             _endpoint_address = block[6] & 0x8f;
            _terminal_link = block[8];
        }

        void uvc_parser::parse_video_stream_format_uncompressed(const std::vector<uint8_t>& block, uvc_format_desc_t& format)
        {
            format.bDescriptorSubtype = (uvc_vs_desc_subtype) block[2];
            format.bFormatIndex = block[3];
            //format->bmCapabilities = block[4];
            //format->bmFlags = block[5];
            memcpy(format.guidFormat, &block[5], 16);
            format.bBitsPerPixel = block[21];
            format.bDefaultFrameIndex = block[22];
            format.bAspectRatioX = block[23];
            format.bAspectRatioY = block[24];
            format.bmInterlaceFlags = block[25];
            format.bCopyProtect = block[26];
        }

        void uvc_parser::parse_video_stream_format_mjpeg(const std::vector<uint8_t>& block, uvc_format_desc_t& format)
        {
            format.bDescriptorSubtype = (uvc_vs_desc_subtype) block[2];
            format.bFormatIndex = block[3];
            memcpy(format.fourccFormat, "MJPG", 4);
            format.bmFlags = block[5];
            format.bBitsPerPixel = 0;
            format.bDefaultFrameIndex = block[6];
            format.bAspectRatioX = block[7];
            format.bAspectRatioY = block[8];
            format.bmInterlaceFlags = block[9];
            format.bCopyProtect = block[10];
        }

        void uvc_parser::parse_video_stream_frame_format(const std::vector<uint8_t>& block, uvc_format_desc_t& format)
        {
            format.bDescriptorSubtype = (uvc_vs_desc_subtype) block[2];
            format.bFormatIndex = block[3];
            format.bNumFrameDescriptors = block[4];
            memcpy(format.guidFormat, &block[5], 16);
            format.bBitsPerPixel = block[21];
            format.bDefaultFrameIndex = block[22];
            format.bAspectRatioX = block[23];
            format.bAspectRatioY = block[24];
            format.bmInterlaceFlags = block[25];
            format.bCopyProtect = block[26];
            format.bVariableSize = block[27];
        }

        void uvc_parser::parse_video_stream_frame_uncompressed(const std::vector<uint8_t>& block, uvc_format_desc_t& format)
        {
            uvc_frame_desc_t frame{};

            frame.bDescriptorSubtype = (uvc_vs_desc_subtype) block[2];
            frame.bFrameIndex = block[3];
            frame.bmCapabilities = block[4];
            frame.wWidth = block[5] + (block[6] << 8);
            frame.wHeight = block[7] + (block[8] << 8);
            frame.dwMinBitRate = as<int>(block, 9);
            frame.dwMaxBitRate = as<int>(block, 13);
            frame.dwMaxVideoFrameBufferSize = as<int>(block, 17);
            frame.dwDefaultFrameInterval = as<int>(block, 21);
            frame.bFrameIntervalType = block[25];

            if (block[25] == 0) {
                frame.dwMinFrameInterval = as<int>(block, 26);
                frame.dwMaxFrameInterval = as<int>(block, 30);
                frame.dwFrameIntervalStep = as<int>(block, 34);
            } else {
                frame.intervals = std::vector<uint32_t>((block[25] + 1));

                for (int i = 0; i < block[25]; ++i)
                {
                    frame.intervals[i] = as<int>(block, 26 + (i * 4));
                }
//                frame.intervals[block[25]] = 0;
            }


            format.frame_descs.push_back(frame);
        }

        void uvc_parser::parse_video_stream_frame_frame(const std::vector<uint8_t>& block, uvc_format_desc_t& format)
        {
            uvc_frame_desc_t frame{};

            frame.bDescriptorSubtype = (uvc_vs_desc_subtype) block[2];
            frame.bFrameIndex = block[3];
            frame.bmCapabilities = block[4];
            frame.wWidth = block[5] + (block[6] << 8);
            frame.wHeight = block[7] + (block[8] << 8);
            frame.dwMinBitRate = as<int>(block, 9);
            frame.dwMaxBitRate = as<int>(block, 13);
            frame.dwDefaultFrameInterval = as<int>(block, 17);
            frame.bFrameIntervalType = block[21];
            frame.dwBytesPerLine = as<int>(block, 22);

            if (block[21] == 0) {
                frame.dwMinFrameInterval = as<int>(block, 26);
                frame.dwMaxFrameInterval = as<int>(block, 30);
                frame.dwFrameIntervalStep = as<int>(block, 34);
            } else {
                frame.intervals = std::vector<uint32_t>((block[21] + 1));
                for (int i = 0; i < block[21]; ++i)
                {
                    frame.intervals[i] = as<int>(block, 26 + (i * 4));
                }
//                frame.intervals[block[21]] = 0;
            }
            format.frame_descs.push_back(frame);
        }

        void uvc_parser::parse_video_stream_format(int interface_number, int index)
        {
            auto descs = _usb_device->get_descriptors();
            auto block = descs[index].data;
            uvc_format_desc_t format{};
            auto descriptor_subtype = block[2];

            switch (descriptor_subtype) {
                case UVC_VS_FORMAT_UNCOMPRESSED:
                    parse_video_stream_format_uncompressed(block, format);
                    _formats[interface_number].push_back(format);
                    break;
                case UVC_VS_FORMAT_MJPEG:
                    parse_video_stream_format_mjpeg(block, format);
                    _formats[interface_number].push_back(format);
                    break;
                case UVC_VS_FORMAT_FRAME_BASED:
                    parse_video_stream_frame_format(block, format);
                    _formats[interface_number].push_back(format);
                    break;
                case UVC_VS_COLORFORMAT:
                    break;

                default:
                    break;
            }
        }

        void uvc_parser::parse_video_stream_frame(int interface_number, int index)
        {
            auto descs = _usb_device->get_descriptors();
            auto block = descs[index].data;
            auto descriptor_subtype = block[2];
            auto& format = _formats.at(interface_number).back();

            switch (descriptor_subtype) {
                case UVC_VS_FRAME_UNCOMPRESSED:
                case UVC_VS_FRAME_MJPEG:
                    parse_video_stream_frame_uncompressed(block, format);
                    break;
                case UVC_VS_FRAME_FRAME_BASED:
                    parse_video_stream_frame_frame(block, format);
                    break;

                default:
                    break;
            }
        }

        void uvc_parser::scan_streaming(int interface_number)
        {
            auto descs = _usb_device->get_descriptors();
            for(int i = 0; i < descs.size(); i++)
            {
                auto d = descs[i];
                if(d.data[1] != USB_DT_INTERFACE)
                    continue;
                if(d.data[2] != interface_number)
                    continue;

                for(i++ ; i < descs.size(); i++)
                {
                    d = descs[i];
                    if(d.data[1] == USB_DT_INTERFACE)
                        break;
                    auto descriptor_subtype = d.data[2];

                    switch (descriptor_subtype) {
                        case UVC_VS_INPUT_HEADER:
                            parse_video_stream_input_header(d.data);
                            break;
                        case UVC_VS_FORMAT_UNCOMPRESSED:
                        case UVC_VS_FORMAT_MJPEG:
                        case UVC_VS_FORMAT_FRAME_BASED:
                            parse_video_stream_format(interface_number, i);
                            break;
                        case UVC_VS_FRAME_UNCOMPRESSED:
                        case UVC_VS_FRAME_MJPEG:
                        case UVC_VS_FRAME_FRAME_BASED:
                            parse_video_stream_frame(interface_number, i);
                            break;
                        case UVC_VS_COLORFORMAT:
                            break;

                        default:
                            /** @todo handle JPEG and maybe still frames or even DV... */
                            break;
                    }

//                    parse_video_stream(interface_number, i);
                }
                break;
            }
        }

        void uvc_parser::parse_video_control_header(const std::vector<uint8_t>& block)
        {
            _bcd_uvc = as<uint16_t>(block, 3);

            switch (_bcd_uvc) {
                case 0x0100:
                case 0x010a:
                    _clock_frequency = as<int>(block, 7);
                    break;
                case 0x0110:
                case 0x0150:
                    _clock_frequency = 0;
                    break;
                default:
                    return throw std::runtime_error("parse_video_control_header failed to parse bcdUVC");
            }

            for (int j = 12; j < block.size(); ++j) {
                auto intf = block[j];
                scan_streaming(intf);
            }
        }

        void uvc_parser::parse_video_control_input_terminal(const std::vector<uint8_t>& block)
        {
            /* only supporting camera-type input terminals */
            if (as<uint16_t>(block, 4) != UVC_ITT_CAMERA) {
                return;
            }

            _input_terminal.bTerminalID = block[3];
            _input_terminal.wTerminalType = (uvc_it_type) as<uint16_t>(block, 4);
            _input_terminal.wObjectiveFocalLengthMin = as<uint16_t>(block, 8);
            _input_terminal.wObjectiveFocalLengthMax = as<uint16_t>(block, 10);
            _input_terminal.wOcularFocalLength = as<uint16_t>(block, 12);

            for (size_t i = 14 + block[14]; i >= 15; --i)
                _input_terminal.bmControls = block[i] + (_input_terminal.bmControls << 8);
        }

        void uvc_parser::parse_video_control_selector_unit(const std::vector<uint8_t>& block)
        {
            _selector_unit.bUnitID = block[3];
        }

        void uvc_parser::parse_video_control_processing_unit(const std::vector<uint8_t>& block)
        {
            _processing_unit.bUnitID = block[3];
            _processing_unit.bSourceID = block[4];

            for (size_t i = 7 + block[7]; i >= 8; --i)
                _processing_unit.bmControls = block[i] + (_processing_unit.bmControls << 8);
        }

        void uvc_parser::parse_video_control_extension_unit(const std::vector<uint8_t>& block)
        {
            const uint8_t *start_of_controls;
            int size_of_controls, num_in_pins;

            _extension_unit.bUnitID = block[3];
            memcpy(_extension_unit.guidExtensionCode, &block[4], 16);

            num_in_pins = block[21];
            size_of_controls = block[22 + num_in_pins];
            start_of_controls = &block[23 + num_in_pins];

            for (int i = size_of_controls - 1; i >= 0; --i)
                _extension_unit.bmControls = start_of_controls[i] + (_extension_unit.bmControls << 8);
        }

        void uvc_parser::parse_video_control(const std::vector<uint8_t>& block)
        {
            auto descriptor_subtype = block[2];

            if (block[1] != 36)
                return;

            switch (descriptor_subtype) {
                case UVC_VC_HEADER:
                    parse_video_control_header(block);
                    break;
                case UVC_VC_INPUT_TERMINAL:
                    parse_video_control_input_terminal(block);
                    break;
                case UVC_VC_OUTPUT_TERMINAL:
                    break;
                case UVC_VC_SELECTOR_UNIT:
                    parse_video_control_selector_unit(block);
                    break;
                case UVC_VC_PROCESSING_UNIT:
                    parse_video_control_processing_unit(block);
                    break;
                case UVC_VC_EXTENSION_UNIT:
                    parse_video_control_extension_unit(block);
                    break;
                default:
                    break;
            }
        }

        void get_block_range(const std::vector<usb_descriptor>& descs, int mi, int& begin, int& end)
        {
            begin = -1;
            end = -1;

            for (int i = 0; i < descs.size(); i++)
            {
                auto d = descs[i];
                if(d.data[1] != USB_DT_INTERFACE)
                    continue;
                if (d.data[2] == mi)
                {
                    begin = i;
                    continue;
                }
                if (begin != -1)
                {
                    end = i;
                    return;
                }
            }
        }

        void uvc_parser::scan_control(int interface_number)
        {
            auto descs = _usb_device->get_descriptors();

            int begin, end;
            get_block_range(descs, _info.mi, begin, end);
            for(int i = begin; i < end; i++)
            {
                auto d = descs[i];
                parse_video_control(d.data);
            }
        }
    }
}