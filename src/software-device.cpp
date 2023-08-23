// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include "software-device.h"
#include "stream.h"

#include <rsutils/string/from.h>
#include <rsutils/deferred.h>
#include <nlohmann/json.hpp>

using rsutils::deferred;


namespace librealsense
{
    software_device::software_device( std::shared_ptr< const device_info > const & dev_info )
        : device( dev_info, false )
    {
    }

    librealsense::software_device::~software_device()
    {
        if (_user_destruction_callback)
            _user_destruction_callback->on_destruction();
    }

    software_sensor& software_device::add_software_sensor(const std::string& name)
    {
        auto sensor = std::make_shared<software_sensor>(name, this);
        add_sensor(sensor);
        _software_sensors.push_back(sensor);

        return *sensor;
    }

    void software_device::register_extrinsic(const stream_interface& stream)
    {
        uint32_t max_idx = 0;
        std::set<uint32_t> bad_groups;
        for (auto & pair : _extrinsics) {
            if (pair.second.first > max_idx) max_idx = pair.second.first;
            if (bad_groups.count(pair.second.first)) continue; // already tried the group
            rs2_extrinsics ext;
            if (environment::get_instance().get_extrinsics_graph().try_fetch_extrinsics(stream, *pair.second.second, &ext)) {
                register_stream_to_extrinsic_group(stream, pair.second.first);
                return;
            }
        }
        register_stream_to_extrinsic_group(stream, max_idx+1);
    }

    void software_device::register_destruction_callback(software_device_destruction_callback_ptr callback)
    {
        _user_destruction_callback = std::move(callback);
    }

    software_sensor& software_device::get_software_sensor( size_t index)
    {
        if (index >= _software_sensors.size())
        {
            throw rs2::error("Requested index is out of range!");
        }
        return *_software_sensors[index];
    }
    
    void software_device::set_matcher_type(rs2_matchers matcher)
    {
        _matcher = matcher;
    }

    static std::shared_ptr< metadata_parser_map > create_software_metadata_parser_map()
    {
        auto md_parser_map = std::make_shared< metadata_parser_map >();
        for( int i = 0; i < static_cast< int >( rs2_frame_metadata_value::RS2_FRAME_METADATA_COUNT ); ++i )
        {
            auto key = static_cast< rs2_frame_metadata_value >( i );
            md_parser_map->emplace( key, std::make_shared< md_array_parser >( key ) );
        }
        return md_parser_map;
    }

    software_sensor::software_sensor( std::string const & name, software_device * owner )
        : sensor_base( name, owner, &_pbs )
        , _stereo_extension( [this]() { return stereo_extension( this ); } )
        , _depth_extension( [this]() { return depth_extension( this ); } )
        , _metadata_map{}  // to all 0's
    {
        // At this time (and therefore for backwards compatibility) no register_metadata is required for SW sensors,
        // and metadata persists between frames (!!!!!!!). All SW sensors support ALL metadata. We can therefore
        // also share their parsers:
        static auto software_metadata_parser_map = create_software_metadata_parser_map();
        _metadata_parsers = software_metadata_parser_map;
        _unique_id = unique_id::generate_id();
    }

    std::shared_ptr<matcher> software_device::create_matcher(const frame_holder& frame) const
    {
        std::vector<stream_interface*> profiles;

        for (auto&& s : _software_sensors)
            for (auto&& p : s->get_stream_profiles())
                profiles.push_back(p.get());

        return matcher_factory::create(_matcher, profiles);
    }

    std::shared_ptr<stream_profile_interface> software_sensor::add_video_stream(rs2_video_stream video_stream, bool is_default)
    {
        auto profile = std::make_shared<video_stream_profile>(
            platform::stream_profile{ (uint32_t)video_stream.width, (uint32_t)video_stream.height, (uint32_t)video_stream.fps, 0 });
        profile->set_dims(video_stream.width, video_stream.height);
        profile->set_format(video_stream.fmt);
        profile->set_framerate(video_stream.fps);
        profile->set_stream_index(video_stream.index);
        profile->set_stream_type(video_stream.type);
        profile->set_unique_id(video_stream.uid);
        profile->set_intrinsics([=]() {return video_stream.intrinsics; });
        if (is_default) profile->tag_profile(profile_tag::PROFILE_TAG_DEFAULT);
        _sw_profiles.push_back(profile);

        return profile;
    }

    std::shared_ptr<stream_profile_interface> software_sensor::add_motion_stream(rs2_motion_stream motion_stream, bool is_default)
    {
        auto profile = std::make_shared<motion_stream_profile>(
            platform::stream_profile{ 0, 0, (uint32_t)motion_stream.fps, 0 });
        profile->set_format(motion_stream.fmt);
        profile->set_framerate(motion_stream.fps);
        profile->set_stream_index(motion_stream.index);
        profile->set_stream_type(motion_stream.type);
        profile->set_unique_id(motion_stream.uid);
        profile->set_intrinsics([=]() {return motion_stream.intrinsics; });
        if (is_default) profile->tag_profile(profile_tag::PROFILE_TAG_DEFAULT);
        _sw_profiles.push_back(profile);

        return std::move(profile);
    }

    std::shared_ptr<stream_profile_interface> software_sensor::add_pose_stream(rs2_pose_stream pose_stream, bool is_default)
    {
        auto profile = std::make_shared<pose_stream_profile>(
            platform::stream_profile{ 0, 0, (uint32_t)pose_stream.fps, 0 });
        if (!profile)
            throw librealsense::invalid_value_exception("null pointer passed for argument \"profile\".");

        profile->set_format(pose_stream.fmt);
        profile->set_framerate(pose_stream.fps);
        profile->set_stream_index(pose_stream.index);
        profile->set_stream_type(pose_stream.type);
        profile->set_unique_id(pose_stream.uid);
        if (is_default) profile->tag_profile(profile_tag::PROFILE_TAG_DEFAULT);
        _sw_profiles.push_back(profile);

        return std::move(profile);
    }


    bool software_sensor::extend_to(rs2_extension extension_type, void ** ptr)
    {
        if (extension_type == RS2_EXTENSION_DEPTH_SENSOR)
        {
            if (supports_option(RS2_OPTION_DEPTH_UNITS))
            {
                *ptr = &(*_depth_extension);
                return true;
            }
        }
        else if (extension_type == RS2_EXTENSION_DEPTH_STEREO_SENSOR)
        {
            if (supports_option(RS2_OPTION_DEPTH_UNITS) && 
                supports_option(RS2_OPTION_STEREO_BASELINE))
            {
                *ptr = &(*_stereo_extension);
                return true;
            }
        }
        return false;
    }

    stream_profiles software_sensor::init_stream_profiles()
    {
        // By default the raw profiles we're given is what we output
        return _sw_profiles;
    }

    void software_sensor::open(const stream_profiles& requests)
    {
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("open(...) failed. Software device is streaming!");
        else if (_is_opened)
            throw wrong_api_call_sequence_exception("open(...) failed. Software device is already opened!");
        _is_opened = true;
        set_active_streams(requests);
    }

    void software_sensor::close()
    {
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("close() failed. Software device is streaming!");
        else if (!_is_opened)
            throw wrong_api_call_sequence_exception("close() failed. Software device was not opened!");
        _is_opened = false;
        set_active_streams({});
    }

    void software_sensor::start(frame_callback_ptr callback)
    {
        if (_is_streaming)
            throw wrong_api_call_sequence_exception("start_streaming(...) failed. Software device is already streaming!");
        else if (!_is_opened)
            throw wrong_api_call_sequence_exception("start_streaming(...) failed. Software device was not opened!");
        _source.get_published_size_option()->set(0);
        _source.init(_metadata_parsers);
        _source.set_sensor(this->shared_from_this());
        _source.set_callback(callback);
        _is_streaming = true;
        raise_on_before_streaming_changes(true);
    }

    void software_sensor::stop()
    {
        if (!_is_streaming)
            throw wrong_api_call_sequence_exception("stop_streaming() failed. Software device is not streaming!");

        _is_streaming = false;
        raise_on_before_streaming_changes(false);
        _source.flush();
        _source.reset();
    }


    void software_sensor::set_metadata( rs2_frame_metadata_value key, rs2_metadata_type value )
    {
        _metadata_map[key] = { true, value };
    }


    void software_sensor::erase_metadata( rs2_frame_metadata_value key )
    {
        _metadata_map[key].is_valid = false;
    }


    frame_interface * software_sensor::allocate_new_frame( rs2_extension extension,
                                                           stream_profile_interface * profile,
                                                           frame_additional_data && data )
    {
        auto frame = _source.alloc_frame( extension, 0, std::move( data ), false );
        if( ! frame )
        {
            LOG_WARNING( "Failed to allocate frame " << data.frame_number << " type " << extension );
        }
        else
        {
            frame->set_stream( std::dynamic_pointer_cast< stream_profile_interface >( profile->shared_from_this() ) );
        }
        return frame;
    }


    frame_interface * software_sensor::allocate_new_video_frame( video_stream_profile_interface * profile,
                                                                 int stride,
                                                                 int bpp,
                                                                 frame_additional_data && data )
    {
        auto frame = allocate_new_frame( profile->get_stream_type() == RS2_STREAM_DEPTH ? RS2_EXTENSION_DEPTH_FRAME
                                                                                        : RS2_EXTENSION_VIDEO_FRAME,
                                         profile,
                                         std::move( data ) );
        if( frame )
        {
            auto vid_frame = dynamic_cast< video_frame * >( frame );
            vid_frame->assign( profile->get_width(), profile->get_height(), stride, bpp * 8 );
            auto sd = dynamic_cast< software_device * >( _owner );
            sd->register_extrinsic( *profile );
        }
        return frame;
    }


    void software_sensor::invoke_new_frame( frame_holder && frame,
                                            void const * pixels,
                                            std::function< void() > on_release )
    {
        // The frame pixels/data are stored in the continuation object!
        if( pixels )
            frame->attach_continuation( frame_continuation( on_release, pixels ) );
        _source.invoke_callback( std::move( frame ) );
    }


    void software_sensor::on_video_frame( rs2_software_video_frame const & software_frame )
    {
        deferred on_release( [deleter = software_frame.deleter, data = software_frame.pixels]() { deleter( data ); } );
        
        stream_profile_interface * profile = software_frame.profile->profile;
        auto vid_profile = dynamic_cast< video_stream_profile_interface * >( profile );
        if( ! vid_profile )
            throw invalid_value_exception( "Non-video profile provided to on_video_frame" );

        if( ! _is_streaming )
            return;

        frame_additional_data data( _metadata_map );
        data.timestamp = software_frame.timestamp;
        data.timestamp_domain = software_frame.domain;
        data.frame_number = software_frame.frame_number;
        data.depth_units = software_frame.depth_units;

        auto frame
            = allocate_new_video_frame( vid_profile, software_frame.stride, software_frame.bpp, std::move( data ) );
        if( frame )
            invoke_new_frame( frame, software_frame.pixels, on_release.detach() );
    }

    void software_sensor::on_motion_frame( rs2_software_motion_frame const & software_frame )
    {
        deferred on_release( [deleter = software_frame.deleter, data = software_frame.data]() { deleter( data ); } );
        if( ! _is_streaming )
            return;

        frame_additional_data data( _metadata_map );
        data.timestamp = software_frame.timestamp;
        data.timestamp_domain = software_frame.domain;
        data.frame_number = software_frame.frame_number;

        auto frame
            = allocate_new_frame( RS2_EXTENSION_MOTION_FRAME, software_frame.profile->profile, std::move( data ) );
        if( frame )
            invoke_new_frame( frame, software_frame.data, on_release.detach() );
    }

    void software_sensor::on_pose_frame( rs2_software_pose_frame const & software_frame )
    {
        deferred on_release( [deleter = software_frame.deleter, data = software_frame.data]() { deleter( data ); } );
        if( ! _is_streaming )
            return;

        frame_additional_data data( _metadata_map );
        data.timestamp = software_frame.timestamp;
        data.timestamp_domain = software_frame.domain;
        data.frame_number = software_frame.frame_number;

        auto frame = allocate_new_frame( RS2_EXTENSION_POSE_FRAME, software_frame.profile->profile, std::move( data ) );
        if( frame )
            invoke_new_frame( frame, software_frame.data, on_release.detach() );
    }

    void software_sensor::on_notification( rs2_software_notification const & notif )
    {
        notification n{ notif.category, notif.type, notif.severity, notif.description };
        n.serialized_data = notif.serialized_data;
        _notifications_processor->raise_notification(n);
    }

    void software_sensor::add_read_only_option(rs2_option option, float val)
    {
        register_option( option,
                         std::make_shared< const_value_option >( "bypass sensor read only option",
                                                                 rsutils::lazy< float >( [=]() { return val; } ) ) );
    }

    void software_sensor::update_read_only_option(rs2_option option, float val)
    {
        if (auto opt = dynamic_cast<readonly_float_option*>(&get_option(option)))
            opt->update(val);
        else
            throw invalid_value_exception( rsutils::string::from() << "option " << get_string( option )
                                                                   << " is not read-only or is deprecated type" );
    }

    void software_sensor::add_option(rs2_option option, option_range range, bool is_writable)
    {
        register_option(option, (is_writable? std::make_shared<float_option>(range) :
                                              std::make_shared<readonly_float_option>(range)));
    }

    void software_recommended_proccesing_blocks::add_processing_block( std::shared_ptr< processing_block_interface > const & block )
    {
        if( ! block )
            throw invalid_value_exception( "trying to add an empty software processing block" );

        _blocks.push_back( block );
    }

} // namespace librealsense

