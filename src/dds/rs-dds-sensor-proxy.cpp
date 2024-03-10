// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include "rs-dds-sensor-proxy.h"
#include "rs-dds-option.h"

#include <src/software-device.h>

#include <realdds/dds-device.h>
#include <realdds/dds-time.h>

#include <realdds/topics/device-info-msg.h>
#include <realdds/topics/image-msg.h>
#include <realdds/topics/imu-msg.h>
#include <realdds/topics/dds-topic-names.h>

#include <src/core/options-registry.h>
#include <src/core/frame-callback.h>
#include <src/stream.h>

#include <src/proc/color-formats-converter.h>

#include <rsutils/json.h>
using rsutils::json;


namespace librealsense {


dds_sensor_proxy::dds_sensor_proxy( std::string const & sensor_name,
                                    software_device * owner,
                                    std::shared_ptr< realdds::dds_device > const & dev )
    : software_sensor( sensor_name, owner )
    , _dev( dev )
    , _name( sensor_name )
    , _md_enabled( dev->supports_metadata() )
{
    rsutils::json const & settings = owner->get_context()->get_settings();
    if( auto interval_j = settings.nested( std::string( "options-update-interval", 23 ) ) )
    {
        auto interval = interval_j.get< uint32_t >();  // NOTE: can throw!
        _options_watcher.set_update_interval( std::chrono::milliseconds( interval ) );
    }
}


void dds_sensor_proxy::add_dds_stream( sid_index sidx, std::shared_ptr< realdds::dds_stream > const & stream )
{
    auto & s = _streams[sidx];
    if( s )
    {
        LOG_ERROR( "stream at " << sidx.to_string() << " already exists for sensor '" << get_name() << "'" );
        return;
    }
    s = stream;
}


std::shared_ptr< stream_profile_interface > dds_sensor_proxy::add_video_stream( rs2_video_stream video_stream,
                                                                                bool is_default )
{
    auto profile = std::make_shared< video_stream_profile >();
    profile->set_dims( video_stream.width, video_stream.height );
    profile->set_format( video_stream.fmt );
    profile->set_framerate( video_stream.fps );
    profile->set_stream_index( video_stream.index );
    profile->set_stream_type( video_stream.type );
    profile->set_unique_id( video_stream.uid );
    profile->set_intrinsics( [=]() {  //
        return video_stream.intrinsics;
    } );
    if( is_default )
        profile->tag_profile( profile_tag::PROFILE_TAG_DEFAULT );
    _sw_profiles.push_back( profile );

    return profile;
}


std::shared_ptr< stream_profile_interface > dds_sensor_proxy::add_motion_stream( rs2_motion_stream motion_stream,
                                                                                 bool is_default )
{
    auto profile = std::make_shared< motion_stream_profile >();
    profile->set_format( motion_stream.fmt );
    profile->set_framerate( motion_stream.fps );
    profile->set_stream_index( motion_stream.index );
    profile->set_stream_type( motion_stream.type );
    profile->set_unique_id( motion_stream.uid );
    profile->set_intrinsics( [=]() { return motion_stream.intrinsics; } );
    if( is_default )
        profile->tag_profile( profile_tag::PROFILE_TAG_DEFAULT );
    _sw_profiles.push_back( profile );

    return profile;
}


stream_profiles dds_sensor_proxy::init_stream_profiles()
{
    auto profiles = get_raw_stream_profiles();

    auto format = get_format_conversion();
    if( format_conversion::raw == format )
    {
        // NOTE: this is not meant for actual streaming at this time -- actual behavior of the
        // formats_converter has not been implemented!
    }
    else
    {
        register_basic_converters();
        if( format_conversion::basic == format )
            _formats_converter.drop_non_basic_formats();
        profiles = _formats_converter.get_all_possible_profiles( profiles );
    }

    sort_profiles( profiles );
    return profiles;
}


void dds_sensor_proxy::register_basic_converters()
{
    // Color
    _formats_converter.register_converter( processing_block_factory::create_id_pbf( RS2_FORMAT_RGB8, RS2_STREAM_COLOR ) );
    _formats_converter.register_converter( processing_block_factory::create_id_pbf( RS2_FORMAT_RGBA8, RS2_STREAM_COLOR ) );
    _formats_converter.register_converter( processing_block_factory::create_id_pbf( RS2_FORMAT_BGR8, RS2_STREAM_COLOR ) );
    _formats_converter.register_converter( processing_block_factory::create_id_pbf( RS2_FORMAT_BGRA8, RS2_STREAM_COLOR ) );

    _formats_converter.register_converters(
        processing_block_factory::create_pbf_vector< uyvy_converter >(
            RS2_FORMAT_UYVY,
            { RS2_FORMAT_UYVY, RS2_FORMAT_YUYV, RS2_FORMAT_RGB8, RS2_FORMAT_Y8, RS2_FORMAT_RGBA8, RS2_FORMAT_BGR8, RS2_FORMAT_BGRA8 },
            RS2_STREAM_COLOR ) );
    _formats_converter.register_converters(
        processing_block_factory::create_pbf_vector< yuy2_converter >(
            RS2_FORMAT_YUYV,
            { RS2_FORMAT_YUYV, RS2_FORMAT_RGB8, RS2_FORMAT_Y8, RS2_FORMAT_RGBA8, RS2_FORMAT_BGR8, RS2_FORMAT_BGRA8 },
            RS2_STREAM_COLOR ) );

    // Depth
    _formats_converter.register_converter(
        processing_block_factory::create_id_pbf( RS2_FORMAT_Z16, RS2_STREAM_DEPTH ) );

    // Infrared (converter source needs type to be handled properly by formats_converter)
    _formats_converter.register_converter(
                          { { { RS2_FORMAT_Y8, RS2_STREAM_INFRARED } },
                            { { RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 0 },
                              { RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 1 },
                              { RS2_FORMAT_Y8, RS2_STREAM_INFRARED, 2 } },
                            []() { return std::make_shared< identity_processing_block >(); } } );
    _formats_converter.register_converter(
                          { { { RS2_FORMAT_Y16, RS2_STREAM_INFRARED } },
                            { { RS2_FORMAT_Y16, RS2_STREAM_INFRARED, 1 },
                              { RS2_FORMAT_Y16, RS2_STREAM_INFRARED, 2 } },
                            []() { return std::make_shared< identity_processing_block >(); } } );

    // Motion
    _formats_converter.register_converter( processing_block_factory::create_id_pbf( RS2_FORMAT_COMBINED_MOTION, RS2_STREAM_MOTION ) );

    // Confidence
    _formats_converter.register_converter( processing_block_factory::create_id_pbf( RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE ) );
}


rsutils::subscription dds_sensor_proxy::register_options_changed_callback( options_watcher::callback && cb )
{
    return _options_watcher.subscribe( std::move( cb ) );
}


std::shared_ptr< realdds::dds_video_stream_profile >
dds_sensor_proxy::find_profile( sid_index sidx, realdds::dds_video_stream_profile const & profile ) const
{
    auto it = _streams.find( sidx );
    if( it == _streams.end() )
    {
        LOG_ERROR( "Invalid stream index " << sidx.to_string() << " in rs2 profile for sensor '" << get_name() << "'" );
    }
    else
    {
        auto & stream = it->second;
        for( auto & sp : stream->profiles() )
        {
            auto vsp = std::static_pointer_cast< realdds::dds_video_stream_profile >( sp );
            if( profile.width() == vsp->width() && profile.height() == vsp->height()
                && profile.encoding() == vsp->encoding() && profile.frequency() == vsp->frequency() )
            {
                return vsp;
            }
        }
    }
    return std::shared_ptr< realdds::dds_video_stream_profile >();
}


std::shared_ptr< realdds::dds_motion_stream_profile >
dds_sensor_proxy::find_profile( sid_index sidx, realdds::dds_motion_stream_profile const & profile ) const
{
    auto it = _streams.find( sidx );
    if( it == _streams.end() )
    {
        LOG_ERROR( "Invalid stream index " << sidx.to_string() << " in rs2 profile for sensor '" << get_name() << "'" );
    }
    else
    {
        auto & stream = it->second;
        for( auto & sp : stream->profiles() )
        {
            auto msp = std::static_pointer_cast< realdds::dds_motion_stream_profile >( sp );
            if( profile.frequency() == msp->frequency() )
            {
                return msp;
            }
        }
    }
    return std::shared_ptr< realdds::dds_motion_stream_profile >();
}


void dds_sensor_proxy::open( const stream_profiles & profiles )
{
    _formats_converter.prepare_to_convert( profiles );

    const auto & source_profiles = _formats_converter.get_active_source_profiles();
    // TODO - register processing block options?

    realdds::dds_stream_profiles realdds_profiles;
    for( size_t i = 0; i < source_profiles.size(); ++i )
    {
        auto & sp = source_profiles[i];
        sid_index sidx( sp->get_unique_id(), sp->get_stream_index() );
        if( Is< video_stream_profile >( sp ) )
        {
            const auto && vsp = As< video_stream_profile >( source_profiles[i] );
            auto video_profile = find_profile(
                sidx,
                realdds::dds_video_stream_profile( sp->get_framerate(),
                                                   realdds::dds_video_encoding::from_rs2( sp->get_format() ),
                                                   vsp->get_width(),
                                                   vsp->get_height() ) );
            if( video_profile )
                realdds_profiles.push_back( video_profile );
            else
                LOG_ERROR( "no profile found in stream for rs2 profile " << vsp );
        }
        else if( Is< motion_stream_profile >( sp ) )
        {
            auto motion_profile = find_profile(
                sidx,
                realdds::dds_motion_stream_profile( source_profiles[i]->get_framerate() ) );
            if( motion_profile )
                realdds_profiles.push_back( motion_profile );
            else
                LOG_ERROR( "no profile found in stream for rs2 profile " << sp );
        }
        else
        {
            LOG_ERROR( "unknown stream profile type for rs2 profile " << sp );
        }
    }

    if( source_profiles.size() > 0 )
    {
        _dev->open( realdds_profiles );
    }

    software_sensor::open( source_profiles );
}


void dds_sensor_proxy::handle_video_data( realdds::topics::image_msg && dds_frame,
                                          const std::shared_ptr< stream_profile_interface > & profile,
                                          streaming_impl & streaming )
{
    frame_additional_data data;  // with NO metadata by default!
    data.timestamp               // in ms
        = static_cast< rs2_time_t >( realdds::time_to_double( dds_frame.timestamp ) * 1e3 );
    data.timestamp_domain;  // from metadata, or leave default (hardware domain)
    data.depth_units;       // from metadata
    data.frame_number;      // filled in only once metadata is known

    auto vid_profile = dynamic_cast< video_stream_profile_interface * >( profile.get() );
    if( ! vid_profile )
        throw invalid_value_exception( "non-video profile provided to on_video_frame" );

    auto stride = static_cast< int >( dds_frame.height > 0 ? dds_frame.raw_data.size() / dds_frame.height
                                                           : dds_frame.raw_data.size() );
    auto bpp = dds_frame.width > 0 ? stride / dds_frame.width : stride;
    auto new_frame_interface = allocate_new_video_frame( vid_profile, stride, bpp, std::move( data ) );
    if( ! new_frame_interface )
        return;

    auto new_frame = static_cast< frame * >( new_frame_interface );
    new_frame->data = std::move( dds_frame.raw_data );

    if( _md_enabled )
    {
        streaming.syncer.enqueue_frame( dds_frame.timestamp.to_ns(), streaming.syncer.hold( new_frame ) );
    }
    else
    {
        add_no_metadata( new_frame, streaming );
        invoke_new_frame( new_frame,
                          nullptr,    // pixels are already inside new_frame->data
                          nullptr );  // so no deleter is necessary
    }
}


void dds_sensor_proxy::handle_motion_data( realdds::topics::imu_msg && imu,
                                           const std::shared_ptr< stream_profile_interface > & profile,
                                           streaming_impl & streaming )
{
    frame_additional_data data;  // with NO metadata by default!
    data.timestamp               // in ms
        = static_cast< rs2_time_t >( realdds::time_to_double( imu.timestamp() ) * 1e3 );
    data.timestamp_domain;  // leave default (hardware domain)
    data.last_frame_number = streaming.last_frame_number.fetch_add( 1 );
    data.frame_number = data.last_frame_number + 1;

    auto new_frame_interface = allocate_new_frame( RS2_EXTENSION_MOTION_FRAME, profile.get(), std::move( data ) );
    if( ! new_frame_interface )
        return;

    auto new_frame = static_cast< frame * >( new_frame_interface );
    new_frame->data.resize( sizeof( rs2_combined_motion ) );
    rs2_combined_motion * m = reinterpret_cast< rs2_combined_motion * >( new_frame->data.data() );
    m->orientation.x = imu.imu_data().orientation().x();
    m->orientation.y = imu.imu_data().orientation().y();
    m->orientation.z = imu.imu_data().orientation().z();
    m->orientation.w = imu.imu_data().orientation().w();
    m->angular_velocity.x = imu.gyro_data().x();  // should be in rad/sec
    m->angular_velocity.y = imu.gyro_data().y();
    m->angular_velocity.z = imu.gyro_data().z();
    m->linear_acceleration.x = imu.accel_data().x();  // should be in m/s^2
    m->linear_acceleration.y = imu.accel_data().y();
    m->linear_acceleration.z = imu.accel_data().z();

    // No metadata for motion streams, therefore no syncer
    invoke_new_frame( new_frame,
                      nullptr,    // pixels are already inside new_frame->data
                      nullptr );  // so no deleter is necessary
}


void dds_sensor_proxy::handle_new_metadata( std::string const & stream_name,
                                            std::shared_ptr< const json > const & dds_md )
{
    if( ! _md_enabled )
        return;

    auto it = _streaming_by_name.find( stream_name );
    if( it != _streaming_by_name.end() )
    {
        if( auto timestamp = dds_md->nested( realdds::topics::metadata::key::header,
                                             realdds::topics::metadata::header::key::timestamp ) )
            it->second.syncer.enqueue_metadata( timestamp.get< realdds::dds_nsec >(), dds_md );
        else
            throw std::runtime_error( "missing metadata header/timestamp" );
    }
    // else we're not streaming -- must be another client that's subscribed
}


void dds_sensor_proxy::add_no_metadata( frame * const f, streaming_impl & streaming )
{
    // Without MD, we have no way of knowing the frame-number - we assume it's one higher than the last
    f->additional_data.last_frame_number = streaming.last_frame_number.fetch_add( 1 );
    f->additional_data.frame_number = f->additional_data.last_frame_number + 1;

    // the frame should already have empty metadata, so no need to do anything else
}


void dds_sensor_proxy::add_frame_metadata( frame * const f,
                                           json const & dds_md,
                                           streaming_impl & streaming )
{
    auto md_header = dds_md.nested( realdds::topics::metadata::key::header );
    auto md = dds_md.nested( realdds::topics::metadata::key::metadata );

    // A frame number is "optional". If the server supplies it, we try to use it for the simple fact that,
    // otherwise, we have no way of detecting drops without some advanced heuristic tracking the FPS and
    // timestamps. If not supplied, we use an increasing counter.
    // Note that if we have no metadata, we have no frame-numbers! So we need a way of generating them
    if( md_header.nested( realdds::topics::metadata::header::key::frame_number )
            .get_ex( f->additional_data.frame_number ) )
    {
        f->additional_data.last_frame_number = streaming.last_frame_number.exchange( f->additional_data.frame_number );
        if( f->additional_data.frame_number != f->additional_data.last_frame_number + 1
            && f->additional_data.last_frame_number )
        {
            LOG_DEBUG( "frame drop? expecting " << f->additional_data.last_frame_number + 1 << "; got "
                                                << f->additional_data.frame_number );
        }
    }
    else
    {
        f->additional_data.last_frame_number = streaming.last_frame_number.fetch_add( 1 );
        f->additional_data.frame_number = f->additional_data.last_frame_number + 1;
    }

    // Timestamp is already set in the frame - must be communicated in the metadata, but only for syncing
    // purposes, so we ignore here. The domain is optional, and really only rs-dds-adapter communicates it
    // because the source is librealsense...
    f->additional_data.timestamp;
    md_header.nested( realdds::topics::metadata::header::key::timestamp_domain )
        .get_ex( f->additional_data.timestamp_domain );

    if( ! md.empty() )
    {
        // Other metadata fields. Metadata fields that are present but unknown by librealsense will be ignored.
        auto & metadata = reinterpret_cast< metadata_array & >( f->additional_data.metadata_blob );
        for( size_t i = 0; i < static_cast< size_t >( RS2_FRAME_METADATA_COUNT ); ++i )
        {
            auto key = static_cast< rs2_frame_metadata_value >( i );
            std::string const & keystr = librealsense::get_string( key );
            try
            {
                if( auto value_j = md.nested( keystr, &json::is_number_integer ) )
                    metadata[key] = { true, value_j.get< rs2_metadata_type >() };
            }
            catch( json::exception const & )
            {
                // The metadata key doesn't exist or the value isn't the right type... we ignore it!
                // (all metadata is not there when we create the frame, so no need to erase)
            }
        }
    }
}


void dds_sensor_proxy::start( rs2_frame_callback_sptr callback )
{
    for( auto & profile : sensor_base::get_active_streams() )
    {
        auto streamit = _streams.find( sid_index( profile->get_unique_id(), profile->get_stream_index() ) );
        if( streamit == _streams.end() )
        {
            LOG_ERROR( "Profile (" << profile->get_unique_id() << "," << profile->get_stream_index() << ") not found in streams!");
            continue;
        }
        auto const & dds_stream = streamit->second;
        // Opening it will start streaming on the server side automatically
        dds_stream->open( "rt/" + _dev->device_info().topic_root() + '_' + dds_stream->name(), _dev->subscriber() );
        auto & streaming = _streaming_by_name[dds_stream->name()];
        streaming.syncer.on_frame_release( frame_releaser );
        streaming.syncer.on_frame_ready(
            [this, &streaming]( syncer_type::frame_holder && fh, std::shared_ptr< const json > const & md )
            {
                if( _is_streaming ) // stop was not called
                {
                    if( ! md )
                        add_no_metadata( static_cast< frame * >( fh.get() ), streaming );
                    else
                        add_frame_metadata( static_cast< frame * >( fh.get() ), *md, streaming );
                    invoke_new_frame( static_cast< frame * >( fh.release() ), nullptr, nullptr );
                }
            } );

        if( auto dds_video_stream = std::dynamic_pointer_cast< realdds::dds_video_stream >( dds_stream ) )
        {
            dds_video_stream->on_data_available(
                [profile, this, &streaming]( realdds::topics::image_msg && dds_frame )
                {
                    if( _is_streaming )
                        handle_video_data( std::move( dds_frame ), profile, streaming );
                } );
        }
        else if( auto dds_motion_stream = std::dynamic_pointer_cast< realdds::dds_motion_stream >( dds_stream ) )
        {
            dds_motion_stream->on_data_available(
                [profile, this, &streaming]( realdds::topics::imu_msg && imu )
                {
                    if( _is_streaming )
                        handle_motion_data( std::move( imu ), profile, streaming );
                } );
        }
        else
            throw std::runtime_error( "Unsupported stream type" );

        dds_stream->start_streaming();
    }

    _formats_converter.set_frames_callback( callback );
    const auto && process_cb = make_frame_callback( [&, this]( frame_holder f ) {
        _formats_converter.convert_frame( f );
    } );

    software_sensor::start( process_cb );
}


void dds_sensor_proxy::stop()
{
    for( auto & profile : sensor_base::get_active_streams() )
    {
        auto streamit = _streams.find( sid_index( profile->get_unique_id(), profile->get_stream_index() ) );
        if( streamit == _streams.end() )
        {
            LOG_ERROR( "Profile (" << profile->get_unique_id() << "," << profile->get_stream_index() << ") not found in streams!" );
            continue;
        }
        auto const & dds_stream = streamit->second;

        dds_stream->stop_streaming();
        dds_stream->close();

        _streaming_by_name[dds_stream->name()].syncer.on_frame_ready( nullptr );

        if( auto dds_video_stream = std::dynamic_pointer_cast< realdds::dds_video_stream >( dds_stream ) )
        {
            dds_video_stream->on_data_available( nullptr );
        }
        else if( auto dds_motion_stream = std::dynamic_pointer_cast< realdds::dds_motion_stream >( dds_stream ) )
        {
            dds_motion_stream->on_data_available( nullptr );
        }
        else
            throw std::runtime_error( "Unsupported stream type" );
    }

    // Resets frame source. Nullify streams on_data_available before calling stop.
    software_sensor::stop();

    // Must be done after dds_stream->stop_streaming or we will need to add validity checks to on_data_available,
    // and after software_sensor::stop cause to make sure _is_streaming is false
    _streaming_by_name.clear();
}


void dds_sensor_proxy::add_option( std::shared_ptr< realdds::dds_option > option )
{
    bool const ok_if_there = true;
    auto option_id = options_registry::register_option_by_name( option->get_name(), ok_if_there );

    if( ! is_valid( option_id ) )
    {
        LOG_ERROR( "Option '" << option->get_name() << "' not found" );
        throw librealsense::invalid_value_exception( "Option '" + option->get_name() + "' not found" );
    }

    if( get_option_handler( option_id ) )
        throw std::runtime_error( "option '" + option->get_name() + "' already exists in sensor" );

    LOG_DEBUG( "... option -> " << option->get_name() );
    auto opt = std::make_shared< rs_dds_option >(
        option,
        [=]( json value )
        {
            // Send the new value to the remote device; the local value gets cached automatically as part of the reply
            _dev->set_option_value( option, std::move( value ) );
        },
        [=]() -> json
        {
            // We don't have to constantly query the option: we expect to get new values automatically, so can return
            // the "last-known" value:
            return option->get_value();
            // If the value is null, we shouldn't get here (is_enabled() should return false) from the user but it's
            // still possible from internal mechanisms (like the options-watcher).
            // If we query for the actual current value:
            //    return _dev->query_option_value( option );
            // Then we may have get a null even when is_enabled() returned true!
        } );
    register_option( option_id, opt );
    _options_watcher.register_option( option_id, opt );
}


static bool processing_block_exists( processing_blocks const & blocks, std::string const & block_name )
{
    for( auto & block : blocks )
        if( block_name.compare( block->get_info( RS2_CAMERA_INFO_NAME ) ) == 0 )
            return true;

    return false;
}


void dds_sensor_proxy::add_processing_block( std::string const & filter_name )
{
    if( processing_block_exists( get_recommended_processing_blocks(), filter_name ) )
        return;  // Already created by another stream of this sensor

    try
    {
        auto ppb = get_device().get_context()->create_pp_block( filter_name, {} );
        if( ! ppb )
            LOG_WARNING( "Unsupported processing block '" + filter_name + "' received" );
        else
            super::add_processing_block( ppb );
    }
    catch( std::exception const & e )
    {
        // Bad settings, error in configuration, etc.
        LOG_ERROR( "Failed to create processing block '" << filter_name << "': " << e.what() );
    }
}


}  // namespace librealsense
