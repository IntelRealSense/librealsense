// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "uvc-types.h"
#include "../types.h"

#include "stdio.h"
#include "stdlib.h"
#include <cstring>
#include <string>
#include <chrono>
#include <thread>

typedef void(uvc_frame_callback_t)(struct librealsense::platform::frame_object *frame, void *user_ptr);

namespace librealsense
{
    namespace platform
    {
        class uvc_parser
        {
        public:
            uvc_parser(const rs_usb_device& usb_device, const uvc_device_info &info);
            virtual ~uvc_parser();

            uvc_processing_unit_t get_processing_unit() { return _processing_unit; }
            uvc_input_terminal_t get_input_terminal() { return _input_terminal; }
            uint16_t get_bcd_uvc() { return _bcd_uvc; }
            int get_clock_frequency() { return _clock_frequency; }
            const std::map<int,std::vector<uvc_format_desc_t>>& get_formats() { return _formats; }

        private:
            void scan_control(int interface_number);
            void scan_streaming(int interface_number);

            void parse_video_control(const std::vector<uint8_t>& block);
            void parse_video_control_header(const std::vector<uint8_t>& block);
            void parse_video_control_input_terminal(const std::vector<uint8_t>& block);
            void parse_video_control_selector_unit(const std::vector<uint8_t>& block);
            void parse_video_control_processing_unit(const std::vector<uint8_t>& block);
            void parse_video_control_extension_unit(const std::vector<uint8_t>& block);

            void parse_video_stream_format(int interface_number, int index);
            void parse_video_stream_frame(int interface_number, int index);

            void parse_video_stream_input_header(const std::vector<uint8_t>& block);

            void parse_video_stream_frame_format(const std::vector<uint8_t>& block, uvc_format_desc_t& format);
            void parse_video_stream_format_uncompressed(const std::vector<uint8_t>& block, uvc_format_desc_t& format);
            void parse_video_stream_format_mjpeg(const std::vector<uint8_t>& block, uvc_format_desc_t& format);

            void parse_video_stream_frame_uncompressed(const std::vector<uint8_t>& block, uvc_format_desc_t& format);
            void parse_video_stream_frame_frame(const std::vector<uint8_t>& block, uvc_format_desc_t& format);


            const uvc_device_info                   _info;

            rs_usb_device                           _usb_device = nullptr;

            // uvc internal
            uint16_t                                _bcd_uvc;
            int                                     _clock_frequency;
            uint8_t                                 _endpoint_address;
            uint8_t                                 _terminal_link;
            uvc_input_terminal_t                    _input_terminal;
            uvc_selector_unit_t                     _selector_unit;
            uvc_processing_unit_t                   _processing_unit;
            uvc_extension_unit_t                    _extension_unit;

            std::map<int,std::vector<uvc_format_desc_t>>    _formats;
        };
    }
}