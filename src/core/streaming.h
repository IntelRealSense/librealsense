// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "options.h"
#include "types.h"
#include "info.h"
#include <functional>

namespace librealsense
{
    namespace platform
    {
        class time_source;
    }

    class sensor_interface;
    class archive_interface;
    class device_interface;
    class processing_block_interface;

    class sensor_part
    {
    public:
        virtual std::shared_ptr<sensor_interface> get_sensor() const = 0;
        virtual void set_sensor(std::shared_ptr<sensor_interface> s) = 0;
        virtual ~sensor_part() = default;
    };

    class context;

    typedef enum profile_tag
    {
        PROFILE_TAG_ANY = 0,
        PROFILE_TAG_SUPERSET = 1, // to be included in enable_all
        PROFILE_TAG_DEFAULT = 2,  // to be included in default pipeline start
    } profile_tag;

    struct tagged_profile
    {
        rs2_stream stream;
        int stream_index;
        uint32_t width, height;
        rs2_format format;
        uint32_t fps;
        int tag;
    };

    class stream_interface : public std::enable_shared_from_this<stream_interface>
    {
    public:
        virtual ~stream_interface() = default;

        virtual int get_stream_index() const = 0;
        virtual void set_stream_index(int index) = 0;

        virtual int get_unique_id() const = 0;
        virtual void set_unique_id(int uid) = 0;

        virtual rs2_stream get_stream_type() const = 0;
        virtual void set_stream_type(rs2_stream stream) = 0;
    };

    class stream_profile_interface : public stream_interface, public recordable<stream_profile_interface>
    {
    public:
        virtual rs2_format get_format() const = 0;
        virtual void set_format(rs2_format format) = 0;

        virtual uint32_t get_framerate() const = 0;
        virtual void set_framerate(uint32_t val) = 0;

        virtual int get_tag() const = 0;
        virtual void tag_profile(int tag) = 0;

        virtual std::shared_ptr<stream_profile_interface> clone() const = 0;
        virtual rs2_stream_profile* get_c_wrapper() const = 0;
        virtual void set_c_wrapper(rs2_stream_profile* wrapper) = 0;
    };

    class frame_interface : public sensor_part
    {
    public:
        virtual rs2_metadata_type get_frame_metadata(const rs2_frame_metadata_value& frame_metadata) const = 0;
        virtual bool supports_frame_metadata(const rs2_frame_metadata_value& frame_metadata) const = 0;
        virtual int get_frame_data_size() const = 0;
        virtual const byte* get_frame_data() const = 0;
        virtual rs2_time_t get_frame_timestamp() const = 0;
        virtual rs2_timestamp_domain get_frame_timestamp_domain() const = 0;
        virtual void set_timestamp(double new_ts) = 0;
        virtual unsigned long long get_frame_number() const = 0;

        virtual void set_timestamp_domain(rs2_timestamp_domain timestamp_domain) = 0;
        virtual rs2_time_t get_frame_system_time() const = 0;
        virtual std::shared_ptr<stream_profile_interface> get_stream() const = 0;
        virtual void set_stream(std::shared_ptr<stream_profile_interface> sp) = 0;

        virtual rs2_time_t get_frame_callback_start_time_point() const = 0;
        virtual void update_frame_callback_start_ts(rs2_time_t ts) = 0;

        virtual void acquire() = 0;
        virtual void release() = 0;
        virtual frame_interface* publish(std::shared_ptr<archive_interface> new_owner) = 0;
        virtual void unpublish() = 0;
        virtual void attach_continuation(frame_continuation&& continuation) = 0;
        virtual void disable_continuation() = 0;

        virtual void log_callback_start(rs2_time_t timestamp) = 0;
        virtual void log_callback_end(rs2_time_t timestamp) const = 0;

        virtual archive_interface* get_owner() const = 0;

        virtual void mark_fixed() = 0;
        virtual bool is_fixed() const = 0;
        virtual void set_blocking(bool state) = 0;
        virtual bool is_blocking() const = 0;

        virtual void keep() = 0;

        virtual ~frame_interface() = default;
    };

    struct LRS_EXTENSION_API frame_holder
    {
        frame_interface* frame;

        frame_interface* operator->()
        {
            return frame;
        }

        operator bool() const { return frame != nullptr; }

        operator frame_interface*() const { return frame; }

        frame_holder(frame_interface* f)
        {
            frame = f;
        }

        ~frame_holder();

        frame_holder(frame_holder&& other)
            : frame(other.frame)
        {
            other.frame = nullptr;
        }

        frame_holder() : frame(nullptr) {}


        frame_holder& operator=(frame_holder&& other);

        frame_holder clone() const;

        bool is_blocking() const { return frame->is_blocking(); };

    private:
        frame_holder& operator=(const frame_holder& other) = delete;
        frame_holder(const frame_holder& other);
    };

    using on_frame = std::function<void(frame_interface*)>;
    using stream_profiles = std::vector<std::shared_ptr<stream_profile_interface>>;
    using processing_blocks = std::vector<std::shared_ptr<processing_block_interface>>;

    inline std::ostream& operator << (std::ostream& os, const stream_profiles& profiles)
    {
        for (auto&& p : profiles)
        {
            os << rs2_format_to_string(p->get_format()) << " " << rs2_stream_to_string(p->get_stream_type()) << ", ";
        }
        return os;
    }

    class recommended_proccesing_blocks_interface
    {
    public:
        virtual processing_blocks get_recommended_processing_blocks() const = 0;
        virtual ~recommended_proccesing_blocks_interface() = default;
    };
    MAP_EXTENSION(RS2_EXTENSION_RECOMMENDED_FILTERS, librealsense::recommended_proccesing_blocks_interface);

    class recommended_proccesing_blocks_snapshot : public recommended_proccesing_blocks_interface, public extension_snapshot
    {
    public:
        recommended_proccesing_blocks_snapshot(const processing_blocks blocks)
            :_blocks(blocks) {}

         virtual processing_blocks get_recommended_processing_blocks() const override
        {
            return _blocks;
        }
         
        void update(std::shared_ptr<extension_snapshot> ext) override {}

        processing_blocks _blocks;
    };
   

    class recommended_proccesing_blocks_base : public virtual recommended_proccesing_blocks_interface, public virtual recordable<recommended_proccesing_blocks_interface>
    {
    public:
        recommended_proccesing_blocks_base(recommended_proccesing_blocks_interface* owner)
            :_owner(owner)
        {}
        
        virtual processing_blocks get_recommended_processing_blocks() const override { return _owner->get_recommended_processing_blocks(); };

        virtual void create_snapshot(std::shared_ptr<recommended_proccesing_blocks_interface>& snapshot) const override
        {
            snapshot = std::make_shared<recommended_proccesing_blocks_snapshot>(get_recommended_processing_blocks());
        }

        virtual void enable_recording(std::function<void(const recommended_proccesing_blocks_interface&)> recording_function)  override {}

    private:
        recommended_proccesing_blocks_interface* _owner;
    };

    class sensor_interface : public virtual info_interface, public virtual options_interface, public virtual recommended_proccesing_blocks_interface
    {
    public:
        virtual stream_profiles get_stream_profiles(int tag = profile_tag::PROFILE_TAG_ANY) const = 0;
        virtual stream_profiles get_active_streams() const = 0;
        virtual void open(const stream_profiles& requests) = 0;
        virtual void close() = 0;
        virtual notifications_callback_ptr get_notifications_callback() const = 0;

        virtual void register_notifications_callback(notifications_callback_ptr callback) = 0;
        virtual int register_before_streaming_changes_callback(std::function<void(bool)> callback) = 0;
        virtual void unregister_before_start_callback(int token) = 0;
        virtual void start(frame_callback_ptr callback) = 0;
        virtual void stop() = 0;
        virtual frame_callback_ptr get_frames_callback() const = 0;
        virtual void set_frames_callback(frame_callback_ptr cb) = 0;
        virtual bool is_streaming() const = 0;
        virtual device_interface& get_device() = 0;

        virtual ~sensor_interface() = default;
    };


    class matcher;

    class device_interface : public virtual info_interface, public std::enable_shared_from_this<device_interface>
    {
    public:
        virtual sensor_interface& get_sensor(size_t i) = 0;

        virtual const sensor_interface& get_sensor(size_t i) const = 0;

        virtual size_t get_sensors_count() const = 0;

        virtual void hardware_reset() = 0;

        virtual std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const = 0;

        virtual std::shared_ptr<context> get_context() const = 0;

        virtual platform::backend_device_group get_device_data() const = 0;

        virtual std::pair<uint32_t, rs2_extrinsics> get_extrinsics(const stream_interface& stream) const = 0;

        virtual bool is_valid() const = 0;

        virtual ~device_interface() = default;

        virtual std::vector<tagged_profile> get_profiles_tags() const = 0;

        virtual void tag_profiles(stream_profiles profiles) const = 0;

        virtual bool compress_while_record() const = 0;

        virtual bool contradicts(const stream_profile_interface* a, const std::vector<stream_profile>& others) const = 0;
    };

    class depth_stereo_sensor;

    class color_sensor : public recordable<color_sensor>
    {
    public:
        virtual ~color_sensor() = default;

        void create_snapshot(std::shared_ptr<color_sensor>& snapshot) const override;
        void enable_recording(std::function<void(const color_sensor&)> recording_function) override {};
    };

    MAP_EXTENSION(RS2_EXTENSION_COLOR_SENSOR, librealsense::color_sensor);

    class color_sensor_snapshot : public virtual color_sensor, public extension_snapshot
    {
    public:
        color_sensor_snapshot() {}

        void update(std::shared_ptr<extension_snapshot> ext) override
        {
        }

        void create_snapshot(std::shared_ptr<color_sensor>& snapshot) const  override
        {
            snapshot = std::make_shared<color_sensor_snapshot>(*this);
        }
        void enable_recording(std::function<void(const color_sensor&)> recording_function) override
        {
            //empty
        }
    };


    class depth_sensor : public recordable<depth_sensor>
    {
    public:
        virtual float get_depth_scale() const = 0;
        virtual ~depth_sensor() = default;
    };

    MAP_EXTENSION(RS2_EXTENSION_DEPTH_SENSOR, librealsense::depth_sensor);

    class depth_sensor_snapshot : public virtual depth_sensor, public extension_snapshot
    {
    public:
        depth_sensor_snapshot(float depth_units) : m_depth_units(depth_units) {}
        float get_depth_scale() const override
        {
            return m_depth_units;
        }

        void update(std::shared_ptr<extension_snapshot> ext) override
        {
            if (auto api = As<depth_sensor>(ext))
            {
                m_depth_units = api->get_depth_scale();
            }
        }
        void create_snapshot(std::shared_ptr<depth_sensor>& snapshot) const  override
        {
            snapshot = std::make_shared<depth_sensor_snapshot>(*this);
        }
        void enable_recording(std::function<void(const depth_sensor&)> recording_function) override
        {
            //empty
        }

    protected:
        float m_depth_units;
    };

    class depth_stereo_sensor : public virtual depth_sensor, public recordable<depth_stereo_sensor>
    {
    public:
        virtual float get_stereo_baseline_mm() const = 0;
    };

    MAP_EXTENSION(RS2_EXTENSION_DEPTH_STEREO_SENSOR, librealsense::depth_stereo_sensor);

    class depth_stereo_sensor_snapshot : public depth_stereo_sensor, public depth_sensor_snapshot
    {
    public:
        depth_stereo_sensor_snapshot(float depth_units, float stereo_bl_mm) :
            depth_sensor_snapshot(depth_units),
            m_stereo_baseline_mm(stereo_bl_mm) {}

        float get_stereo_baseline_mm() const override
        {
            return m_stereo_baseline_mm;
        }

        void update(std::shared_ptr<extension_snapshot> ext) override
        {
            depth_sensor_snapshot::update(ext);

            if (auto api = As<depth_stereo_sensor>(ext))
            {
                m_stereo_baseline_mm = api->get_stereo_baseline_mm();
            }
        }

        void create_snapshot(std::shared_ptr<depth_stereo_sensor>& snapshot) const override
        {
            snapshot = std::make_shared<depth_stereo_sensor_snapshot>(*this);
        }

        void enable_recording(std::function<void(const depth_stereo_sensor&)> recording_function) override
        {
            //empty
        }

    private:
        float m_stereo_baseline_mm;
    };
}
