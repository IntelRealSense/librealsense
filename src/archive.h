// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "core/streaming.h"

#include <atomic>
#include <array>
#include <math.h>

namespace librealsense
{
    class archive_interface;
    class md_attribute_parser_base;
    class frame;
}

struct frame_additional_data
{
    rs2_time_t timestamp = 0;
    unsigned long long frame_number = 0;
    rs2_timestamp_domain timestamp_domain = RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK;
    rs2_time_t      system_time = 0;
    rs2_time_t      frame_callback_started = 0;
    uint32_t        metadata_size = 0;
    bool            fisheye_ae_mode = false;
    std::array<uint8_t,MAX_META_DATA_SIZE> metadata_blob;

    frame_additional_data() {};

    frame_additional_data(double in_timestamp, unsigned long long in_frame_number, double in_system_time, uint8_t md_size, const uint8_t* md_buf)
        : timestamp(in_timestamp),
          frame_number(in_frame_number),
          system_time(in_system_time),
          metadata_size(md_size)
    {
        // Copy up to 255 bytes to preserve metadata as raw data
        if (metadata_size)
            std::copy(md_buf,md_buf+ std::min(md_size,MAX_META_DATA_SIZE),metadata_blob.begin());
    }
};

namespace librealsense
{
    typedef std::map<rs2_frame_metadata_value, std::shared_ptr<md_attribute_parser_base>> metadata_parser_map;

    // Define a movable but explicitly noncopyable buffer type to hold our frame data
    class frame : public frame_interface
    {
    public:
        std::vector<byte> data;
        frame_additional_data additional_data;

        explicit frame() : ref_count(0), owner(nullptr), on_release() {}
        frame(const frame& r) = delete;
        frame(frame&& r)
            : ref_count(r.ref_count.exchange(0)),
              owner(r.owner), on_release()
        {
            *this = std::move(r);
        }

        frame& operator=(const frame& r) = delete;
        frame& operator=(frame&& r)
        {
            data = move(r.data);
            owner = r.owner;
            ref_count = r.ref_count.exchange(0);
            on_release = std::move(r.on_release);
            additional_data = std::move(r.additional_data);
            r.owner.reset();
            return *this;
        }

        virtual ~frame() { on_release.reset(); }
        rs2_metadata_type get_frame_metadata(const rs2_frame_metadata_value& frame_metadata) const override;
        bool supports_frame_metadata(const rs2_frame_metadata_value& frame_metadata) const override;
        const byte* get_frame_data() const override;
        rs2_time_t get_frame_timestamp() const override;
        rs2_timestamp_domain get_frame_timestamp_domain() const override;
        void set_timestamp(double new_ts) override { additional_data.timestamp = new_ts; }
        unsigned long long get_frame_number() const override;

        void set_timestamp_domain(rs2_timestamp_domain timestamp_domain) override
        {
            additional_data.timestamp_domain = timestamp_domain;
        }

        rs2_time_t get_frame_system_time() const override;

        std::shared_ptr<stream_profile_interface> get_stream() const override { return stream; }
        void set_stream(std::shared_ptr<stream_profile_interface> sp) override { stream = std::move(sp); }

        rs2_time_t get_frame_callback_start_time_point() const override;
        void update_frame_callback_start_ts(rs2_time_t ts) override;

        void acquire() override { ref_count.fetch_add(1); }
        void release() override;
        frame_interface* publish(std::shared_ptr<archive_interface> new_owner) override;
        void attach_continuation(frame_continuation&& continuation) override { on_release = std::move(continuation); }
        void disable_continuation() override { on_release.reset(); }

        archive_interface* get_owner() const override { return owner.get(); }

        std::shared_ptr<sensor_interface> get_sensor() const override;
        void set_sensor(std::shared_ptr<sensor_interface> s) override;


        void log_callback_start(rs2_time_t timestamp) override;
        void log_callback_end(rs2_time_t timestamp) const override;

        void mark_fixed() override { _fixed = true; }
        bool is_fixed() const override { return _fixed; }

    private:
        // TODO: check boost::intrusive_ptr or an alternative
        std::atomic<int> ref_count; // the reference count is on how many times this placeholder has been observed (not lifetime, not content)
        std::shared_ptr<archive_interface> owner; // pointer to the owner to be returned to by last observe
        std::weak_ptr<sensor_interface> sensor;
        frame_continuation on_release;
        bool _fixed = false;
        std::shared_ptr<stream_profile_interface> stream;
    };

    class points : public frame
    {
    public:
        float3* get_vertices();
        void export_to_ply(const std::string& fname, const frame_holder& texture);
        size_t get_vertex_count() const;
        float2* get_texture_coordinates();
    };

    MAP_EXTENSION(RS2_EXTENSION_POINTS, librealsense::points);

    class composite_frame : public frame
    {
    public:
        composite_frame() : frame() {}

        frame_interface* get_frame(int i) const
        {
            auto frames = get_frames();
            return frames[i];
        }

        frame_interface** get_frames() const { return (frame_interface**)data.data(); }

        const frame_interface* first() const
        {
            return get_frame(0);
        }
        frame_interface* first()
        {
            return get_frame(0);
        }

        size_t get_embedded_frames_count() const { return data.size() / sizeof(rs2_frame*); }

        // In the next section we make the composite frame "look and feel" like the first of its children
        rs2_metadata_type get_frame_metadata(const rs2_frame_metadata_value& frame_metadata) const override
        {
            return first()->get_frame_metadata(frame_metadata);
        }
        bool supports_frame_metadata(const rs2_frame_metadata_value& frame_metadata) const override
        {
            return first()->supports_frame_metadata(frame_metadata);
        }
        const byte* get_frame_data() const override
        {
            return first()->get_frame_data();
        }
        rs2_time_t get_frame_timestamp() const override
        {
            return first()->get_frame_timestamp();
        }
        rs2_timestamp_domain get_frame_timestamp_domain() const override
        {
            return first()->get_frame_timestamp_domain();
        }
        unsigned long long get_frame_number() const override
        {
            if (first())
                return first()->get_frame_number();
            else
                return frame::get_frame_number();
        }
        rs2_time_t get_frame_system_time() const override
        {
            return first()->get_frame_system_time();
        }
        std::shared_ptr<sensor_interface> get_sensor() const override
        {
            return first()->get_sensor();
        }
    };

    MAP_EXTENSION(RS2_EXTENSION_COMPOSITE_FRAME, librealsense::composite_frame);

    class video_frame : public frame
    {
    public:
        video_frame()
            : frame(), _width(0), _height(0), _bpp(0), _stride(0)
        {}

        int get_width() const { return _width; }
        int get_height() const { return _height; }
        int get_stride() const { return _stride; }
        int get_bpp() const { return _bpp; }

        void assign(int width, int height, int stride, int bpp)
        {
            _width = width;
            _height = height;
            _stride = stride;
            _bpp = bpp;
        }

    private:
        int _width, _height, _bpp, _stride;
    };

    MAP_EXTENSION(RS2_EXTENSION_VIDEO_FRAME, librealsense::video_frame);

    class depth_frame : public video_frame
    {
    public:
        depth_frame() : video_frame()
        {
        }

        float get_distance(int x, int y) const
        {
            if (_original)
                return((depth_frame*)_original.frame)->get_distance(x, y);

            uint64_t pixel = 0;
            switch (get_bpp()/8) // bits per pixel
            {
            case 1: pixel = get_frame_data()[y*get_width() + x];                                    break;
            case 2: pixel = reinterpret_cast<const uint16_t*>(get_frame_data())[y*get_width() + x]; break;
            case 4: pixel = reinterpret_cast<const uint32_t*>(get_frame_data())[y*get_width() + x]; break;
            case 8: pixel = reinterpret_cast<const uint64_t*>(get_frame_data())[y*get_width() + x]; break;
            default: throw std::runtime_error(to_string() << "Unrecognized depth format " << int(get_bpp() / 8) << " bytes per pixel");
            }

            return pixel * get_units();
        }

        float get_units() const { return query_units(this->get_sensor()); }

        const frame_interface* get_original_depth() const
        {
            auto res = _original.frame;
            auto df = dynamic_cast<depth_frame*>(res);
            if (df)
            {
                auto prev = df->get_original_depth();
                if (prev) return prev;
            }
            return res;
        }

        void set_original(frame_holder h)
        {
            _original = std::move(h);
            attach_continuation(frame_continuation([this]() {
                if (_original)
                {
                    _original = {};
                }
            }, get_frame_data()));
        }

    protected:
        static float query_units(const std::shared_ptr<sensor_interface>& sensor)
        {
            if (sensor != nullptr)
            {
                try
                {
                    auto depth_sensor = As<librealsense::depth_sensor>(sensor);
                    if(depth_sensor != nullptr)
                    {
                        return depth_sensor->get_depth_scale();
                    }
                    else
                    {
                        //For playback sensors
                        auto extendable = As<librealsense::extendable_interface>(sensor);
                        if (extendable && extendable->extend_to(TypeToExtension<librealsense::depth_sensor>::value, (void**)(&depth_sensor)))
                        {
                            return depth_sensor->get_depth_scale();
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR("Failed to query depth units from sensor. " << e.what());
                }
                catch (...)
                {
                    LOG_ERROR("Failed to query depth units from sensor");
                }
            }
            else
            {
                LOG_WARNING("sensor was nullptr");
            }

            return 0;
        }

        frame_holder _original;
    };

    MAP_EXTENSION(RS2_EXTENSION_DEPTH_FRAME, librealsense::depth_frame);

    // Disparity frame provides an alternative representation of the depth data for stereo-based depth sensors
    // In addition for the depth frame API it allows to query the stereoscopic baseline required to transform depth to disparity and vice versa
    class disparity_frame : public depth_frame
    {
    public:
        disparity_frame() : depth_frame()
        {
        }

        // TODO Refactor to framemetadata
        float get_stereo_baseline(void) const { return query_stereo_baseline(this->get_sensor()); }

    protected:

        static float query_stereo_baseline(const std::shared_ptr<sensor_interface>& sensor)
        {
            if (sensor != nullptr)
            {
                try
                {
                    auto stereo_sensor = As<librealsense::depth_stereo_sensor>(sensor);
                    if (stereo_sensor != nullptr)
                    {
                        return stereo_sensor->get_stereo_baseline_mm();
                    }
                    else
                    {
                        //For playback sensors
                        auto extendable = As<librealsense::extendable_interface>(sensor);
                        if (extendable && extendable->extend_to(TypeToExtension<librealsense::depth_stereo_sensor>::value, (void**)(&stereo_sensor)))
                        {
                            return stereo_sensor->get_stereo_baseline_mm();
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR("Failed to query stereo baseline from sensor. " << e.what());
                }
                catch (...)
                {
                    LOG_ERROR("Failed to query stereo baseline from sensor");
                }
            }
            else
            {
                LOG_WARNING("sensor was nullptr");
            }

            return 0;
        }
    };

    MAP_EXTENSION(RS2_EXTENSION_DISPARITY_FRAME, librealsense::disparity_frame);

    class motion_frame : public frame
    {
    public:
        motion_frame() : frame()
        {}
    };

    MAP_EXTENSION(RS2_EXTENSION_MOTION_FRAME, librealsense::motion_frame);

    class pose_frame : public frame
    {
    public:
        // pose frame data buffer is pose info struct
        struct pose_info
        {
            float3   translation;          /**< X, Y, Z values of translation, in meters (relative to initial position)                                    */
            float3   velocity;             /**< X, Y, Z values of velocity, in meter/sec                                                                   */
            float3   acceleration;         /**< X, Y, Z values of acceleration, in meter/sec^2                                                             */
            float4   rotation;             /**< Qi, Qj, Qk, Qr components of rotation as represented in quaternion rotation (relative to initial position) */
            float3   angular_velocity;     /**< X, Y, Z values of angular velocity, in radians/sec                                                         */
            float3   angular_acceleration; /**< X, Y, Z values of angular acceleration, in radians/sec^2                                                   */
            uint32_t tracker_confidence;   /**< pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High                                     */
            uint32_t mapper_confidence;    /**< pose data confidence 0x0 - Failed, 0x1 - Low, 0x2 - Medium, 0x3 - High                                     */
        };

        pose_frame() : frame() {}

        float3   get_translation()          const { return reinterpret_cast<const pose_info*>(data.data())->translation; }
        float3   get_velocity()             const { return reinterpret_cast<const pose_info*>(data.data())->velocity; }
        float3   get_acceleration()         const { return reinterpret_cast<const pose_info*>(data.data())->acceleration; }
        float4   get_rotation()             const { return reinterpret_cast<const pose_info*>(data.data())->rotation; }
        float3   get_angular_velocity()     const { return reinterpret_cast<const pose_info*>(data.data())->angular_velocity; }
        float3   get_angular_acceleration() const { return reinterpret_cast<const pose_info*>(data.data())->angular_acceleration; }
        uint32_t get_tracker_confidence()   const { return reinterpret_cast<const pose_info*>(data.data())->tracker_confidence; }
        uint32_t get_mapper_confidence()    const { return reinterpret_cast<const pose_info*>(data.data())->mapper_confidence; }
    };

    MAP_EXTENSION(RS2_EXTENSION_POSE_FRAME, librealsense::pose_frame);


    class archive_interface : public sensor_part
    {
    public:
        virtual callback_invocation_holder begin_callback() = 0;

        virtual frame_interface* alloc_and_track(const size_t size, const frame_additional_data& additional_data, bool requires_memory) = 0;

        virtual std::shared_ptr<metadata_parser_map> get_md_parsers() const = 0;

        virtual void flush() = 0;

        virtual frame_interface* publish_frame(frame_interface* frame) = 0;
        virtual void unpublish_frame(frame_interface* frame) = 0;

        virtual ~archive_interface() = default;

    };

    std::shared_ptr<archive_interface> make_archive(rs2_extension type,
                                                    std::atomic<uint32_t>* in_max_frame_queue_size,
                                                    std::shared_ptr<platform::time_service> ts,
                                                    std::shared_ptr<metadata_parser_map> parsers);
}
