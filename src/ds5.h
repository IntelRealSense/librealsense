// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>

#include "device.h"
#include "context.h"
#include "backend.h"
#include "ds5-private.h"
#include "hw-monitor.h"
#include "image.h"
#include <mutex>
#include <chrono>
const double TIMESTAMP_TO_MILLISECONS = 0.001;

namespace rsimpl
{
    static const std::vector<std::uint16_t> rs4xx_sku_pid = { ds::RS400P_PID, ds::RS410A_PID, ds::RS420R_PID, ds::RS430C_PID, ds::RS450T_PID };

    class ds5_camera;

#pragma pack (push, 1)
    struct uvc_header
    {
        byte            length;
        byte            info;
        unsigned int    timestamp;
        byte            source_clock[6];
    };

    struct metadata_header
    {
        unsigned int    metaDataID;
        unsigned int    size;
    };


    struct metadata_capture_timing
    {
        metadata_header  metaDataIdHeader;
        unsigned int    version;
        unsigned int    flag;
        int    frameCounter;
        unsigned int    opticalTimestamp;   //In millisecond unit
        unsigned int    readoutTime;        //The readout time in millisecond second unit
        unsigned int    exposureTime;       //The exposure time in millisecond second unit
        unsigned int    frameInterval ;     //The frame interval in millisecond second unit
        unsigned int    pipeLatency;        //The latency between start of frame to frame ready in USB buffer
    };
#pragma pack(pop)

    struct metadata
    {
       uvc_header header;
       metadata_capture_timing md_capture_timing;
    };

    class ds5_timestamp_reader_from_metadata : public frame_timestamp_reader
    {
       std::unique_ptr<frame_timestamp_reader> _backup_timestamp_reader;
       static const int pins = 2;
       std::vector<std::atomic<bool>> _has_metadata;
       bool started;
       mutable std::recursive_mutex _mtx;

    public:
        ds5_timestamp_reader_from_metadata(std::unique_ptr<frame_timestamp_reader> backup_timestamp_reader)
            :_backup_timestamp_reader(std::move(backup_timestamp_reader)), _has_metadata(pins)
        {
            reset();
        }


        bool validate_frame(const request_mapping& mode, const void * frame) const override
        {
            return true;
        }

        bool has_metadata(const request_mapping& mode, const void * frame, unsigned int byte_received)
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
			if (byte_received <= mode.pf->get_image_size(mode.profile.width, mode.profile.height))
			{
				return false;
			}
            auto md = (metadata*)((byte*)frame +  mode.pf->get_image_size(mode.profile.width, mode.profile.height));

            for(auto i=0; i<sizeof(metadata); i++)
            {
                if(((byte*)md)[i] != 0)
                {
                    return true;
                }
            }
            return false;
        }

        double get_frame_timestamp(const request_mapping& mode, const void * frame, unsigned int byte_received) override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            auto pin_index = 0;
            if (mode.pf->fourcc == 0x5a313620) // Z16
                pin_index = 1;

            if(!_has_metadata[pin_index])
            {
               _has_metadata[pin_index] = has_metadata(mode, frame, byte_received);
            }

            if(_has_metadata[pin_index])
            {
                auto md = (metadata*)((byte*)frame +  mode.pf->get_image_size(mode.profile.width, mode.profile.height));
                return (double)(md->header.timestamp)*TIMESTAMP_TO_MILLISECONS;
            }
            else
            {
                if (!started)
                {
                    LOG_WARNING("UVC timestamp not found! please apply UVC metadata patch.");
                    started = true;
                }
                return _backup_timestamp_reader->get_frame_timestamp(mode, frame, byte_received);
            }
        }

        unsigned long long get_frame_counter(const request_mapping & mode, const void * frame, unsigned int byte_received) const override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            auto pin_index = 0;
            if (mode.pf->fourcc == 0x5a313620) // Z16
                pin_index = 1;

            if(_has_metadata[pin_index])
            {
                auto md = (metadata*)((byte*)frame +  mode.pf->get_image_size(mode.profile.width, mode.profile.height));
                return md->md_capture_timing.frameCounter;
            }
            else
            {
                return _backup_timestamp_reader->get_frame_counter(mode, frame, byte_received);
            }
        }

        void reset() override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            started = false;
            for (auto i = 0; i < pins; ++i)
            {
                _has_metadata[i] = false;
            }
        }

        rs_timestamp_domain get_frame_timestamp_domain(const request_mapping& mode) const override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            auto pin_index = 0;
            if (mode.pf->fourcc == 0x5a313620) // Z16
                pin_index = 1;

            return _has_metadata[pin_index] ? RS_TIMESTAMP_DOMAIN_HARDWARE_CLOCK :
                                              _backup_timestamp_reader->get_frame_timestamp_domain(mode);
        }
    };

    class ds5_timestamp_reader : public frame_timestamp_reader
    {
        static const int pins = 2;
        std::vector<int64_t> total;
        std::vector<int> last_timestamp;
        mutable std::vector<int64_t> counter;
        std::vector<std::chrono::time_point<std::chrono::high_resolution_clock>> start_time;
        mutable std::recursive_mutex _mtx;
    public:
        ds5_timestamp_reader()
            : total(pins),
              last_timestamp(pins), counter(pins), start_time(pins)
        {
            reset();
        }

        void reset() override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            for (auto i = 0; i < pins; ++i)
            {
                total[i] = 0;
                last_timestamp[i] = 0;
                counter[i] = 0;
            }
        }

        bool validate_frame(const request_mapping& mode, const void * frame) const override
        {
            return true;
        }

        double get_frame_timestamp(const request_mapping& mode, const void * frame, unsigned int byte_received) override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            auto ts = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
            return static_cast<double>(ts.time_since_epoch().count());
        }

        unsigned long long get_frame_counter(const request_mapping & mode, const void * /*frame*/, unsigned int byte_received) const override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            auto pin_index = 0;
            if (mode.pf->fourcc == 0x5a313620) // Z16
                pin_index = 1;

            return ++counter[pin_index];
        }

        rs_timestamp_domain get_frame_timestamp_domain(const request_mapping& mode) const override
        {
            return RS_TIMESTAMP_DOMAIN_SYSTEM_TIME;
        }
    };

    class ds5_hid_timestamp_reader : public frame_timestamp_reader
    {
        static const int sensors = 2;
        bool started;
        std::vector<int64_t> total;
        std::vector<int> last_timestamp;
        mutable std::vector<int64_t> counter;
        mutable std::recursive_mutex _mtx;
        const unsigned hid_data_size = 14;      // RS4xx HID Data:: 3 Words for axes + 8-byte Timestamp
        const double timestamp_to_ms = 0.001;
    public:
        ds5_hid_timestamp_reader()
        {
            total.resize(sensors);
            last_timestamp.resize(sensors);
            counter.resize(sensors);
            reset();
        }

        void reset() override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            started = false;
            for (auto i = 0; i < sensors; ++i)
            {
                total[i] = 0;
                last_timestamp[i] = 0;
                counter[i] = 0;
            }
        }

        bool validate_frame(const request_mapping& mode, const void * frame) const override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            // Validate that at least one byte of the image is nonzero
            for (const uint8_t * it = (const uint8_t *)frame, *end = it + mode.pf->get_image_size(mode.profile.width, mode.profile.height); it != end; ++it)
            {
                if (*it)
                {
                    return true;
                }
            }

            return false;
        }

        double get_frame_timestamp(const request_mapping& mode, const void * frame, unsigned int byte_received) override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            auto frame_size = mode.profile.width * mode.profile.height;
            static const unsigned timestamp_offset = 6;
            if (frame_size == hid_data_size)
            {
                auto timestamp = *((uint64_t*)((const uint8_t*)frame + timestamp_offset));
                return static_cast<double>(timestamp) * timestamp_to_ms;
            }

            if (!started)
            {
                LOG_WARNING("HID timestamp not found! please apply HID patch.");
                started = true;
            }
            auto ts = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now());
            return static_cast<double>(ts.time_since_epoch().count());
        }

        unsigned long long get_frame_counter(const request_mapping & mode, const void * /*frame*/, unsigned int byte_received) const override
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            int index = 0;
            if (mode.pf->fourcc == 'GYRO')
                index = 1;

            return ++counter[index];
        }

        rs_timestamp_domain get_frame_timestamp_domain(const request_mapping& mode) const
        {
            std::lock_guard<std::recursive_mutex> lock(_mtx);
            auto frame_size = mode.profile.width * mode.profile.height;
            if (frame_size == hid_data_size)
            {
                return RS_TIMESTAMP_DOMAIN_HARDWARE_CLOCK;
            }
            return RS_TIMESTAMP_DOMAIN_SYSTEM_TIME;
        }
    };

    class ds5_info : public device_info
    {
    public:
        std::shared_ptr<device> create(const uvc::backend& backend) const override;

        uint8_t get_subdevice_count() const override
        {
            auto depth_pid = _depth.front().pid;
            switch(depth_pid)
            {
            case ds::RS400P_PID:
            case ds::RS410A_PID:
            case ds::RS420R_PID:
            case ds::RS430C_PID: return 1;
            case ds::RS450T_PID: return 3;
            default:
                throw not_implemented_exception(to_string() <<
                    "get_subdevice_count is not implemented for DS5 device of type " <<
                    depth_pid);
            }
        }

        ds5_info(std::shared_ptr<uvc::backend> backend,
                 std::vector<uvc::uvc_device_info> depth,
                 uvc::usb_device_info hwm,
                 std::vector<uvc::hid_device_info> hid)
            : device_info(std::move(backend)), _hwm(std::move(hwm)),
              _depth(std::move(depth)), _hid(std::move(hid)) {}

        static std::vector<std::shared_ptr<device_info>> pick_ds5_devices(
                std::shared_ptr<uvc::backend> backend,
                std::vector<uvc::uvc_device_info>& uvc,
                std::vector<uvc::usb_device_info>& usb,
                std::vector<uvc::hid_device_info>& hid);

    private:
        std::vector<uvc::uvc_device_info> _depth;
        uvc::usb_device_info _hwm;
        std::vector<uvc::hid_device_info> _hid;
    };

    class ds5_camera final : public device
    {
    public:
        class emitter_option : public uvc_xu_option<uint8_t>
        {
        public:
            const char* get_value_description(float val) const override
            {
                switch (static_cast<int>(val))
                {
                    case 0:
                    {
                        return "Off";
                    }
                    case 1:
                    {
                        return "On";
                    }
                    case 2:
                    {
                        return "Auto";
                    }
                    default:
                        throw invalid_value_exception("value not found");
                }
            }

            explicit emitter_option(uvc_endpoint& ep)
                : uvc_xu_option(ep, ds::depth_xu, ds::DS5_DEPTH_EMITTER_ENABLED,
                                "Power of the DS5 projector, 0 meaning projector off, 1 meaning projector off, 2 meaning projector in auto mode")
            {}
        };

        std::shared_ptr<hid_endpoint> create_hid_device(const uvc::backend& backend,
                                                        const std::vector<uvc::hid_device_info>& all_hid_infos);

        std::shared_ptr<uvc_endpoint> create_depth_device(const uvc::backend& backend,
                                                          const std::vector<uvc::uvc_device_info>& all_device_infos);

        uvc_endpoint& get_depth_endpoint()
        {
            return static_cast<uvc_endpoint&>(get_endpoint(_depth_device_idx));
        }

        ds5_camera(const uvc::backend& backend,
            const std::vector<uvc::uvc_device_info>& dev_info,
            const uvc::usb_device_info& hwm_device,
            const std::vector<uvc::hid_device_info>& hid_info);

        std::vector<uint8_t> send_receive_raw_data(const std::vector<uint8_t>& input) override;
        virtual rs_intrinsics get_intrinsics(unsigned int subdevice, stream_profile profile) const override;

    private:
        bool is_camera_in_advanced_mode() const;

        const uint8_t _depth_device_idx;
        std::shared_ptr<hw_monitor> _hw_monitor;


        lazy<std::vector<uint8_t>> _coefficients_table_raw;

        std::vector<uint8_t> get_raw_calibration_table(ds::calibration_table_id table_id) const;

    };
}
