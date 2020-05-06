// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include "ds5-private.h"
#include "ds5-timestamp.h"

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>

#include "device.h"
#include "context.h"
#include "image.h"

namespace librealsense
{
    ds5_timestamp_reader_from_metadata::ds5_timestamp_reader_from_metadata(std::unique_ptr<frame_timestamp_reader> backup_timestamp_reader)
        :_backup_timestamp_reader(std::move(backup_timestamp_reader)), _has_metadata(pins), one_time_note(false)
    {
        reset();
    }

    bool ds5_timestamp_reader_from_metadata::has_metadata(const std::shared_ptr<frame_interface>& frame)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        auto f = std::dynamic_pointer_cast<librealsense::frame>(frame);
        if (!f)
        {
            LOG_ERROR("Frame is not valid. Failed to downcast to librealsense::frame.");
            return false;
        }
        auto md = f->additional_data.metadata_blob;
        auto mds = f->additional_data.metadata_size;

        for(uint32_t i = 0; i < mds; i++)
        {
            if(md[i] != 0)
            {
                return true;
            }
        }
        return false;
    }

    rs2_time_t ds5_timestamp_reader_from_metadata::get_frame_timestamp(const std::shared_ptr<frame_interface>& frame)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        auto f = std::dynamic_pointer_cast<librealsense::frame>(frame);
        if (!f)
        {
            LOG_ERROR("Frame is not valid. Failed to downcast to librealsense::frame.");
            return 0;
        }
        size_t pin_index = 0;

        if (frame->get_stream()->get_format() == RS2_FORMAT_Z16)
            pin_index = 1;

        _has_metadata[pin_index] = has_metadata(frame);

        auto md = (librealsense::metadata_intel_basic*)(f->additional_data.metadata_blob.data());
        if(_has_metadata[pin_index] && md)
        {
            return (double)(md->header.timestamp)*TIMESTAMP_USEC_TO_MSEC;
        }
        else
        {
            if (!one_time_note)
            {
                LOG_WARNING("UVC metadata payloads not available. Please refer to the installation chapter for details.");
                one_time_note = true;
            }
            return _backup_timestamp_reader->get_frame_timestamp(frame);
        }
    }

    unsigned long long ds5_timestamp_reader_from_metadata::get_frame_counter(const std::shared_ptr<frame_interface>& frame) const
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);

        auto f = std::dynamic_pointer_cast<librealsense::frame>(frame);
        if (!f)
        {
            LOG_ERROR("Frame is not valid. Failed to downcast to librealsense::frame.");
            return 0;
        }
        size_t pin_index = 0;

        if (frame->get_stream()->get_format() == RS2_FORMAT_Z16)
            pin_index = 1;

        if(_has_metadata[pin_index] && f->additional_data.metadata_size > platform::uvc_header_size)
        {
            auto md = (librealsense::metadata_intel_basic*)(f->additional_data.metadata_blob.data());
            if (md->capture_valid())
                return md->payload.frame_counter;
        }

        return _backup_timestamp_reader->get_frame_counter(frame);
    }

    void ds5_timestamp_reader_from_metadata::reset()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        one_time_note = false;
        for (auto i = 0; i < pins; ++i)
        {
            _has_metadata[i] = false;
        }
    }

    rs2_timestamp_domain ds5_timestamp_reader_from_metadata::get_frame_timestamp_domain(const std::shared_ptr<frame_interface>& frame) const
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        auto pin_index = 0;
        if (frame->get_stream()->get_format() == RS2_FORMAT_Z16)
            pin_index = 1;

        return _has_metadata[pin_index] ? RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK :
                                          _backup_timestamp_reader->get_frame_timestamp_domain(frame);
    }

    ds5_timestamp_reader::ds5_timestamp_reader(std::shared_ptr<platform::time_service> ts)
        : counter(pins), _ts(ts)
    {
        reset();
    }

    void ds5_timestamp_reader::reset()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        for (auto i = 0; i < pins; ++i)
        {
            counter[i] = 0;
        }
    }

    rs2_time_t ds5_timestamp_reader::get_frame_timestamp(const std::shared_ptr<frame_interface>& frame)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        return _ts->get_time();
    }

    unsigned long long ds5_timestamp_reader::get_frame_counter(const std::shared_ptr<frame_interface>& frame) const
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        auto pin_index = 0;
        if (frame->get_stream()->get_format() == RS2_FORMAT_Z16)
            pin_index = 1;

        return ++counter[pin_index];
    }

    rs2_timestamp_domain ds5_timestamp_reader::get_frame_timestamp_domain(const std::shared_ptr<frame_interface>& frame) const
    {
        return RS2_TIMESTAMP_DOMAIN_SYSTEM_TIME;
    }

    ds5_custom_hid_timestamp_reader::ds5_custom_hid_timestamp_reader()
    {
        counter.resize(sensors);
        reset();
    }

    void ds5_custom_hid_timestamp_reader::reset()
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        for (auto i = 0; i < sensors; ++i)
        {
            counter[i] = 0;
        }
    }

    rs2_time_t ds5_custom_hid_timestamp_reader::get_frame_timestamp(const std::shared_ptr<frame_interface>& frame)
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        static const uint8_t timestamp_offset = 17;
        auto f = std::dynamic_pointer_cast<librealsense::frame>(frame);
        if (!f)
        {
            LOG_ERROR("Frame is not valid. Failed to downcast to librealsense::frame.");
            return 0;
        }

        // The timewstamp shall be trimmed back to 32 bit to allow HID/UVC intra-stream sync
        // See ds5_iio_hid_timestamp_reader description
        auto timestamp = *((uint32_t*)((const uint8_t*)f->get_frame_data() + timestamp_offset));
        // TODO - verify units with custom report
        return static_cast<rs2_time_t>(timestamp) * TIMESTAMP_USEC_TO_MSEC;
    }

    bool ds5_custom_hid_timestamp_reader::has_metadata(const std::shared_ptr<frame_interface>& frame) const
    {
        return true;
    }

    unsigned long long ds5_custom_hid_timestamp_reader::get_frame_counter(const std::shared_ptr<frame_interface>& frame) const
    {
        std::lock_guard<std::recursive_mutex> lock(_mtx);
        return ++counter.front();
    }

    rs2_timestamp_domain ds5_custom_hid_timestamp_reader::get_frame_timestamp_domain(const std::shared_ptr<frame_interface>& frame) const
    {
        return RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK;
    }
}
