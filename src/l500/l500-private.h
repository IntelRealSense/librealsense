// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "types.h"
#include "option.h"

namespace librealsense
{
    const uint16_t L500_PID = 0x0b0d;
    const uint16_t L515_PID = 0x0b3d;

    namespace ivcam2
    {
        // L500 depth XU identifiers
        const uint8_t L500_HWMONITOR = 1;
        const uint8_t L500_DEPTH_LASER_POWER = 2;
        const uint8_t L500_ERROR_REPORTING = 3;

        const platform::extension_unit depth_xu = { 0, 3, 2,
        { 0xC9606CCB, 0x594C, 0x4D25,{ 0xaf, 0x47, 0xcc, 0xc4, 0x96, 0x43, 0x59, 0x95 } } };

        enum fw_cmd : uint8_t
        {
            HWReset = 0x20,
            GVD = 0x10,
            GLD = 0x0f,
            DPT_INTRINSICS_GET = 0x5A,
            MRD = 0x01,
            TEMPERATURES_GET = 0x6A
        };

        enum gvd_fields
        {
            fw_version_offset = 12,
            module_serial_offset = 56,
            module_serial_size = 8
        };

        static const std::map<std::uint16_t, std::string> rs500_sku_names = {
            { L500_PID,        "Intel RealSense L500"},
            { L515_PID,        "Intel RealSense L515"},
        };

        enum l500_notifications_types
        {
            success = 0,
            depth_not_available,
            overflow_infrared,
            overflow_depth,
            overflow_confidence,
            depth_stream_hard_error,
            depth_stream_soft_error,
            temp_warning,
            temp_critical,
            DFU_error
        };

        // Elaborate FW XU report.
        const std::map< uint8_t, std::string> l500_fw_error_report = {
            { success,                      "Success" },
            { depth_not_available,          "Fatal error occur and device is unable \nto run depth stream" },
            { overflow_infrared,            "Overflow occur on infrared stream" },
            { overflow_depth,               "Overflow occur on depth stream" },
            { overflow_confidence,          "Overflow occur on confidence stream" },
            { depth_stream_hard_error,      "Stream stoped. \nNon recoverable. power reset may help" },
            { depth_stream_soft_error,      "Error that may be overcome in few sec. \nStream stoped. May be recoverable" },
            { temp_warning,                 "Warning, temperature close to critical" },
            { temp_critical,                "Critical temperature reached" },
            { DFU_error,                    "DFU error" },
        };

        bool try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
                                         const platform::uvc_device_info& info, platform::usb_device_info& result);


        class l500_temperature_options : public readonly_option
        {
        public:
            float query() const override;

            option_range get_range() const override { return option_range{ 0, 100, 0, 0 }; };

            bool is_enabled() const override { return true; }

            const char* get_description() const override { return ""; }

            explicit l500_temperature_options(hw_monitor* hw_monitor, rs2_option opt);

        private:
            rs2_option _option;
            hw_monitor* _hw_monitor;
        };

        class l500_timestamp_reader : public frame_timestamp_reader
        {
            static const int pins = 3;
            mutable std::vector<int64_t> counter;
            std::shared_ptr<platform::time_service> _ts;
            mutable std::recursive_mutex _mtx;
        public:
            l500_timestamp_reader(std::shared_ptr<platform::time_service> ts)
                : counter(pins), _ts(ts)
            {
                reset();
            }

            void reset() override
            {
                std::lock_guard<std::recursive_mutex> lock(_mtx);
                for (auto i = 0; i < pins; ++i)
                {
                    counter[i] = 0;
                }
            }

            rs2_time_t get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo) override
            {
                std::lock_guard<std::recursive_mutex> lock(_mtx);
                return _ts->get_time();
            }

            unsigned long long get_frame_counter(const request_mapping & mode, const platform::frame_object& fo) const override
            {
                std::lock_guard<std::recursive_mutex> lock(_mtx);
                auto pin_index = 0;
                if (mode.pf->fourcc == 0x5a313620) // Z16
                    pin_index = 1;
                else if (mode.pf->fourcc == 0x43202020) // Confidence
                    pin_index = 2;

                return ++counter[pin_index];
            }

            rs2_timestamp_domain get_frame_timestamp_domain(const request_mapping & mode, const platform::frame_object& fo) const override
            {
                return RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME;
            }
        };

        class l500_timestamp_reader_from_metadata : public frame_timestamp_reader
        {
            std::unique_ptr<l500_timestamp_reader> _backup_timestamp_reader;
            bool one_time_note;
            mutable std::recursive_mutex _mtx;
            arithmetic_wraparound<uint32_t, uint64_t > ts_wrap;

        protected:

            bool has_metadata_ts(const platform::frame_object& fo) const
            {
                // Metadata support for a specific stream is immutable
                const bool has_md_ts = [&] { std::lock_guard<std::recursive_mutex> lock(_mtx);
                return ((fo.metadata != nullptr) && (fo.metadata_size >= platform::uvc_header_size) && ((byte*)fo.metadata)[0] >= platform::uvc_header_size);
                }();

                return has_md_ts;
            }

            bool has_metadata_fc(const platform::frame_object& fo) const
            {
                // Metadata support for a specific stream is immutable
                const bool has_md_frame_counter = [&] { std::lock_guard<std::recursive_mutex> lock(_mtx);
                return ((fo.metadata != nullptr) && (fo.metadata_size > platform::uvc_header_size) && ((byte*)fo.metadata)[0] > platform::uvc_header_size);
                }();

                return has_md_frame_counter;
            }

        public:
            l500_timestamp_reader_from_metadata(std::shared_ptr<platform::time_service> ts) :_backup_timestamp_reader(nullptr), one_time_note(false)
            {
                _backup_timestamp_reader = std::unique_ptr<l500_timestamp_reader>(new l500_timestamp_reader(ts));
                reset();
            }

            rs2_time_t get_frame_timestamp(const request_mapping& mode, const platform::frame_object& fo) override;

            unsigned long long get_frame_counter(const request_mapping & mode, const platform::frame_object& fo) const override;

            void reset() override;

            rs2_timestamp_domain get_frame_timestamp_domain(const request_mapping & mode, const platform::frame_object& fo) const override;
        };
    } // librealsense::ivcam2
} // namespace librealsense
