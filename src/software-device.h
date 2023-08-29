// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.
#pragma once

#include "core/streaming.h"
#include "device.h"
#include "context.h"
#include <librealsense2/h/rs_internal.h>
#include <rsutils/lazy.h>

namespace librealsense
{
    class software_sensor;
    class software_device_info;
    class video_stream_profile_interface;

    class software_device : public device
    {
    public:
        software_device();
        software_device( std::shared_ptr< context > ctx );
        virtual ~software_device();

        software_sensor& add_software_sensor(const std::string& name);

        software_sensor& get_software_sensor( size_t index);

        void set_matcher_type(rs2_matchers matcher);
        
        std::shared_ptr<software_device_info> get_info();

        std::shared_ptr<matcher> create_matcher(const frame_holder& frame) const override;

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> markers;
            return markers;
        };
        void register_extrinsic(const stream_interface& stream);

        void register_destruction_callback(software_device_destruction_callback_ptr);

    protected:
        std::vector<std::shared_ptr<software_sensor>> _software_sensors;
        librealsense::software_device_destruction_callback_ptr _user_destruction_callback;
        rs2_matchers _matcher = RS2_MATCHER_DEFAULT;
        std::shared_ptr<software_device_info> _info;
    };
    
    class software_device_info : public device_info
    {
        std::weak_ptr<software_device> _dev;
    public:
        explicit software_device_info(std::shared_ptr<software_device> dev)
            : device_info(nullptr), _dev(dev)
        {
        }
        
        std::shared_ptr<device_interface> create_device() const override
        {
            return _dev.lock();
        }
        platform::backend_device_group get_device_data() const override
        {
            std::stringstream address;
            address << "software-device://";
            if (auto dev = _dev.lock())
            {
                auto ptr = dev.get();
                address << (unsigned long long)ptr;
            }
            return platform::backend_device_group({ platform::playback_device_info{ address.str() } });
        }
        
        std::shared_ptr<device_interface> create(std::shared_ptr<context>, bool) const override
        {
            return _dev.lock();
        }
    };

    class software_recommended_proccesing_blocks : public recommended_proccesing_blocks_interface
    {
    public:
        processing_blocks get_recommended_processing_blocks() const override {
            return _blocks;
        }
        ~software_recommended_proccesing_blocks() override {}

        void add_processing_block( std::shared_ptr< processing_block_interface > const & block );

    private:
        processing_blocks _blocks;
    };

    class software_sensor : public sensor_base, public extendable_interface
    {
    public:
        software_sensor( std::string const & name, software_device * owner );

        virtual std::shared_ptr<stream_profile_interface> add_video_stream(rs2_video_stream video_stream, bool is_default = false);
        virtual std::shared_ptr<stream_profile_interface> add_motion_stream(rs2_motion_stream motion_stream, bool is_default = false);
        virtual std::shared_ptr<stream_profile_interface> add_pose_stream(rs2_pose_stream pose_stream, bool is_default = false);

        bool extend_to(rs2_extension extension_type, void** ptr) override;

        stream_profiles init_stream_profiles() override;
        stream_profiles const & get_raw_stream_profiles() const override { return _sw_profiles; }

        void open(const stream_profiles& requests) override;
        void close() override;

        void start(frame_callback_ptr callback) override;
        void stop() override;

        void on_video_frame( rs2_software_video_frame const & );
        void on_motion_frame( rs2_software_motion_frame const & );
        void on_pose_frame( rs2_software_pose_frame const & );
        void on_notification( rs2_software_notification const & );
        void add_read_only_option(rs2_option option, float val);
        void update_read_only_option(rs2_option option, float val);
        void add_option(rs2_option option, option_range range, bool is_writable);
        void set_metadata( rs2_frame_metadata_value key, rs2_metadata_type value );
        void erase_metadata( rs2_frame_metadata_value key );

    protected:
        frame_interface * allocate_new_frame( rs2_extension, stream_profile_interface *, frame_additional_data && );
        frame_interface * allocate_new_video_frame( video_stream_profile_interface *, int stride, int bpp, frame_additional_data && );
        void invoke_new_frame( frame_holder &&, void const * pixels, std::function< void() > on_release );

        metadata_array _metadata_map;

        processing_blocks get_recommended_processing_blocks() const override
        {
            return _pbs.get_recommended_processing_blocks();
        }

        software_recommended_proccesing_blocks & get_software_recommended_proccesing_blocks() { return _pbs; }

        // We build profiles using add_video_stream(), etc., and feed those into init_stream_profiles() which could in
        // theory change them: so these are our "raw" profiles before initialization...
        stream_profiles _sw_profiles;

    private:
        friend class software_device;
        uint64_t _unique_id;

        class stereo_extension : public depth_stereo_sensor
        {
        public:
            stereo_extension(software_sensor* owner) : _owner(owner) {}

            float get_depth_scale() const override {
                return _owner->get_option(RS2_OPTION_DEPTH_UNITS).query();
            }

            float get_stereo_baseline_mm() const override {
                return _owner->get_option(RS2_OPTION_STEREO_BASELINE).query();
            }

            void create_snapshot(std::shared_ptr<depth_stereo_sensor>& snapshot) const override {}
            void enable_recording(std::function<void(const depth_stereo_sensor&)> recording_function) override {}

            void create_snapshot(std::shared_ptr<depth_sensor>& snapshot) const override {}
            void enable_recording(std::function<void(const depth_sensor&)> recording_function) override {}
        private:
            software_sensor* _owner;
        };

        class depth_extension : public depth_sensor
        {
        public:
            depth_extension(software_sensor* owner) : _owner(owner) {}

            float get_depth_scale() const override {
                return _owner->get_option(RS2_OPTION_DEPTH_UNITS).query();
            }

            void create_snapshot(std::shared_ptr<depth_sensor>& snapshot) const override {}
            void enable_recording(std::function<void(const depth_sensor&)> recording_function) override {}
        private:
            software_sensor* _owner;
        };

        rsutils::lazy< stereo_extension > _stereo_extension;
        rsutils::lazy< depth_extension > _depth_extension;

        software_recommended_proccesing_blocks _pbs;
    };
    MAP_EXTENSION(RS2_EXTENSION_SOFTWARE_SENSOR, software_sensor);
    MAP_EXTENSION(RS2_EXTENSION_SOFTWARE_DEVICE, software_device);
}
