// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#ifdef _MSC_VER
#if (_MSC_VER <= 1800) // constexpr is not supported in MSVC2013
#error( "Librealsense requires MSVC2015 or later to build. Compilation will be aborted" )
#endif
#endif

#include <array>
#include <chrono>
#include "ivcam/sr300.h"
#include "ds/d400/d400-factory.h"
#include "l500/l500-factory.h"
#include "ds/ds-timestamp.h"
#include "backend.h"
#include <media/ros/ros_reader.h>
#include "types.h"
#include "stream.h"
#include "environment.h"
#include "context.h"
#include "fw-update/fw-update-factory.h"
#ifdef BUILD_WITH_DDS
#include <realdds/dds-device-watcher.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-device.h>
#include <realdds/dds-stream.h>
#include <realdds/dds-stream-profile.h>
#include <realdds/dds-metadata-syncer.h>
#include "software-device.h"
#include <librealsense2/h/rs_internal.h>
#include <realdds/topics/device-info-msg.h>
#include <realdds/topics/image/image-msg.h>
#include <realdds/topics/flexible/flexible-msg.h>
#include <rsutils/shared-ptr-singleton.h>
#include <rsutils/os/executable-name.h>
#include <rsutils/deferred.h>
#include <fastdds/dds/domain/DomainParticipant.hpp>

// Processing blocks for DDSA SW sensors
#include <proc/decimation-filter.h>
#include <proc/disparity-transform.h>
#include <proc/hdr-merge.h>
#include <proc/hole-filling-filter.h>
#include <proc/sequence-id-filter.h>
#include <proc/spatial-filter.h>
#include <proc/temporal-filter.h>
#include <proc/threshold.h>

#include <rsutils/string/string-utilities.h>  // string_to_value

// We manage one participant and device-watcher per domain:
// Two contexts with the same domain-id will share the same participant and watcher, while a third context on a
// different domain will have its own.
//
struct dds_domain_context
{
    rsutils::shared_ptr_singleton< realdds::dds_participant > participant;
    rsutils::shared_ptr_singleton< realdds::dds_device_watcher > device_watcher;
};
//
// Domains are mapped by ID:
// Two contexts with the same participant name on different domain-ids are using two different participants!
//
static std::map< realdds::dds_domain_id, dds_domain_context > dds_domain_context_by_id;

#endif // BUILD_WITH_DDS

#include <rsutils/json.h>
using json = nlohmann::json;

namespace {

template< class T >
bool contains( const T & first, const T & second )
{
    return first == second;
}

template<>
bool contains( const std::shared_ptr< librealsense::device_info > & first,
               const std::shared_ptr< librealsense::device_info > & second )
{
    auto first_data = first->get_device_data();
    auto second_data = second->get_device_data();

    for( auto && uvc : first_data.uvc_devices )
    {
        if( std::find( second_data.uvc_devices.begin(), second_data.uvc_devices.end(), uvc )
            == second_data.uvc_devices.end() )
            return false;
    }
    for( auto && usb : first_data.usb_devices )
    {
        if( std::find( second_data.usb_devices.begin(), second_data.usb_devices.end(), usb )
            == second_data.usb_devices.end() )
            return false;
    }
    for( auto && hid : first_data.hid_devices )
    {
        if( std::find( second_data.hid_devices.begin(), second_data.hid_devices.end(), hid )
            == second_data.hid_devices.end() )
            return false;
    }
    for( auto && pd : first_data.playback_devices )
    {
        if( std::find( second_data.playback_devices.begin(), second_data.playback_devices.end(), pd )
            == second_data.playback_devices.end() )
            return false;
    }
    return true;
}

template< class T >
std::vector< std::shared_ptr< T > > subtract_sets( const std::vector< std::shared_ptr< T > > & first,
                                                   const std::vector< std::shared_ptr< T > > & second )
{
    std::vector< std::shared_ptr< T > > results;
    std::for_each( first.begin(), first.end(), [&]( std::shared_ptr< T > data ) {
        if( std::find_if( second.begin(),
                          second.end(),
                          [&]( std::shared_ptr< T > new_dev ) { return contains( data, new_dev ); } )
            == second.end() )
        {
            results.push_back( data );
        }
    } );
    return results;
}

}  // namespace

namespace librealsense
{
    const std::map<uint32_t, rs2_format> platform_color_fourcc_to_rs2_format = {
        {rs_fourcc('Y','U','Y','2'), RS2_FORMAT_YUYV},
        {rs_fourcc('Y','U','Y','V'), RS2_FORMAT_YUYV},
        {rs_fourcc('M','J','P','G'), RS2_FORMAT_MJPEG},
    };
    const std::map<uint32_t, rs2_stream> platform_color_fourcc_to_rs2_stream = {
        {rs_fourcc('Y','U','Y','2'), RS2_STREAM_COLOR},
        {rs_fourcc('Y','U','Y','V'), RS2_STREAM_COLOR},
        {rs_fourcc('M','J','P','G'), RS2_STREAM_COLOR},
    };


    context::context()
        : _devices_changed_callback( nullptr, []( rs2_devices_changed_callback* ) {} )
    {
        static bool version_logged = false;
        if( ! version_logged )
        {
            version_logged = true;
            LOG_DEBUG( "Librealsense VERSION: " << RS2_API_VERSION_STR );
        }
    }


    context::context( backend_type type )
        : context()
    {
        _backend = platform::create_backend();
#ifdef BUILD_WITH_DDS
        {
            realdds::dds_domain_id domain_id = 0;
            auto & domain = dds_domain_context_by_id[domain_id];
            _dds_participant = domain.participant.instance();
            if( ! _dds_participant->is_valid() )
                _dds_participant->init( domain_id, rsutils::os::executable_name() );
            _dds_watcher = domain.device_watcher.instance( _dds_participant );
        }
#endif //BUILD_WITH_DDS

        environment::get_instance().set_time_service(_backend->create_time_service());

        _device_watcher = _backend->create_device_watcher();
        assert(_device_watcher->is_stopped());
    }


    context::context( json const & settings )
        : context()
    {
        _backend = platform::create_backend();  // standard type

        environment::get_instance().set_time_service( _backend->create_time_service() );

        _device_watcher = _backend->create_device_watcher();
        assert( _device_watcher->is_stopped() );

#ifdef BUILD_WITH_DDS
        if( rsutils::json::get< bool >( settings, "dds-discovery", true ) )
        {
            realdds::dds_domain_id domain_id = rsutils::json::get< int >( settings, "dds-domain", 0 );
            std::string participant_name
                = rsutils::json::get< std::string >( settings, "dds-participant-name", rsutils::os::executable_name() );

            auto & domain = dds_domain_context_by_id[domain_id];
            _dds_participant = domain.participant.instance();
            if( ! _dds_participant->is_valid() )
            {
                _dds_participant->init( domain_id, participant_name );
            }
            else if( rsutils::json::has_value( settings, "dds-participant-name" )
                     && participant_name != _dds_participant->get()->get_qos().name().to_string() )
            {
                throw std::runtime_error(
                    rsutils::string::from()
                    << "A DDS participant '" << _dds_participant->get()->get_qos().name().to_string()
                    << "' already exists in domain " << domain_id << "; cannot create '" << participant_name << "'" );
            }
            _dds_watcher = domain.device_watcher.instance( _dds_participant );

            // The DDS device watcher should always be on
            if( _dds_watcher && _dds_watcher->is_stopped() )
            {
                start_dds_device_watcher( rsutils::json::get< size_t >( settings, "dds-message-timeout-ms", 5000 ) );
            }
        }
#endif //BUILD_WITH_DDS
    }


    context::context( char const * json_settings )
        : context( json_settings ? json::parse( json_settings ) : json() )
    {
    }


    class recovery_info : public device_info
    {
    public:
        std::shared_ptr<device_interface> create(std::shared_ptr<context>, bool) const override
        {
            throw unrecoverable_exception(RECOVERY_MESSAGE,
                RS2_EXCEPTION_TYPE_DEVICE_IN_RECOVERY_MODE);
        }

        static bool is_recovery_pid(uint16_t pid)
        {
            return pid == 0x0ADB || pid == 0x0AB3;
        }

        static std::vector<std::shared_ptr<device_info>> pick_recovery_devices(
            std::shared_ptr<context> ctx,
            const std::vector<platform::usb_device_info>& usb_devices)
        {
            std::vector<std::shared_ptr<device_info>> list;
            for (auto&& usb : usb_devices)
            {
                if (is_recovery_pid(usb.pid))
                {
                    list.push_back(std::make_shared<recovery_info>(ctx, usb));
                }
            }
            return list;
        }

        explicit recovery_info(std::shared_ptr<context> ctx, platform::usb_device_info dfu)
            : device_info(ctx), _dfu(std::move(dfu)) {}

        platform::backend_device_group get_device_data()const override
        {
            return platform::backend_device_group({ _dfu });
        }

    private:
        platform::usb_device_info _dfu;
        const char* RECOVERY_MESSAGE = "Selected RealSense device is in recovery mode!\nEither perform a firmware update or reconnect the camera to fall-back to last working firmware if available!";
    };

    class platform_camera_info : public device_info
    {
    public:
        std::shared_ptr<device_interface> create(std::shared_ptr<context>, bool) const override;

        static std::vector<std::shared_ptr<device_info>> pick_uvc_devices(
            const std::shared_ptr<context>& ctx,
            const std::vector<platform::uvc_device_info>& uvc_devices)
        {
            std::vector<std::shared_ptr<device_info>> list;
            auto groups = group_devices_by_unique_id(uvc_devices);

            for (auto&& g : groups)
            {
                if (g.front().vid != VID_INTEL_CAMERA)
                    list.push_back(std::make_shared<platform_camera_info>(ctx, g));
            }
            return list;
        }

        explicit platform_camera_info(std::shared_ptr<context> ctx,
                                      std::vector<platform::uvc_device_info> uvcs)
            : device_info(ctx), _uvcs(std::move(uvcs)) {}

        platform::backend_device_group get_device_data() const override
        {
            return platform::backend_device_group();
        }

    private:
        std::vector<platform::uvc_device_info> _uvcs;
    };

    class platform_camera_sensor : public synthetic_sensor
    {
    public:
        platform_camera_sensor(device* owner,
            std::shared_ptr<uvc_sensor> uvc_sensor)
            : synthetic_sensor("RGB Camera", uvc_sensor, owner,platform_color_fourcc_to_rs2_format,platform_color_fourcc_to_rs2_stream),
              _default_stream(new stream(RS2_STREAM_COLOR))
        {
        }

        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();

            auto results = synthetic_sensor::init_stream_profiles();

            for (auto&& p : results)
            {
                // Register stream types
                assign_stream(_default_stream, p);
                environment::get_instance().get_extrinsics_graph().register_same_extrinsics(*_default_stream, *p);
            }

            return results;
        }


    private:
        std::shared_ptr<stream_interface> _default_stream;
    };

    class platform_camera : public device
    {
    public:
        platform_camera(const std::shared_ptr<context>& ctx,
                        const std::vector<platform::uvc_device_info>& uvc_infos,
                        const platform::backend_device_group& group,
                        bool register_device_notifications)
            : device(ctx, group, register_device_notifications)
        {
            std::vector<std::shared_ptr<platform::uvc_device>> devs;
            for (auto&& info : uvc_infos)
                devs.push_back(ctx->get_backend().create_uvc_device(info));

            std::unique_ptr<frame_timestamp_reader> host_timestamp_reader_backup(new ds_timestamp_reader(environment::get_instance().get_time_service()));
            auto raw_color_ep = std::make_shared<uvc_sensor>("Raw RGB Camera",
                std::make_shared<platform::multi_pins_uvc_device>(devs),
                std::unique_ptr<frame_timestamp_reader>(new ds_timestamp_reader_from_metadata(std::move(host_timestamp_reader_backup))),
                this);
            auto color_ep = std::make_shared<platform_camera_sensor>(this, raw_color_ep);
            add_sensor(color_ep);

            register_info(RS2_CAMERA_INFO_NAME, "Platform Camera");
            std::string pid_str( rsutils::string::from()
                                 << std::setfill( '0' ) << std::setw( 4 ) << std::hex << uvc_infos.front().pid );
            std::transform(pid_str.begin(), pid_str.end(), pid_str.begin(), ::toupper);

            using namespace platform;
            auto usb_mode = raw_color_ep->get_usb_specification();
            std::string usb_type_str("USB");
            if (usb_spec_names.count(usb_mode) && (usb_undefined != usb_mode))
                usb_type_str = usb_spec_names.at(usb_mode);

            register_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR, usb_type_str);
            register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, uvc_infos.front().unique_id);
            register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, uvc_infos.front().device_path);
            register_info(RS2_CAMERA_INFO_PRODUCT_ID, pid_str);

            color_ep->register_processing_block(processing_block_factory::create_pbf_vector<yuy2_converter>(RS2_FORMAT_YUYV, map_supported_color_formats(RS2_FORMAT_YUYV), RS2_STREAM_COLOR));
            color_ep->register_processing_block({ {RS2_FORMAT_MJPEG} }, { {RS2_FORMAT_RGB8, RS2_STREAM_COLOR} }, []() { return std::make_shared<mjpeg_converter>(RS2_FORMAT_RGB8); });
            color_ep->register_processing_block(processing_block_factory::create_id_pbf(RS2_FORMAT_MJPEG, RS2_STREAM_COLOR));

            // Timestamps are given in units set by device which may vary among the OEM vendors.
            // For consistent (msec) measurements use "time of arrival" metadata attribute
            color_ep->register_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP, make_uvc_header_parser(&platform::uvc_header::timestamp));

            color_ep->try_register_pu(RS2_OPTION_BACKLIGHT_COMPENSATION);
            color_ep->try_register_pu(RS2_OPTION_BRIGHTNESS);
            color_ep->try_register_pu(RS2_OPTION_CONTRAST);
            color_ep->try_register_pu(RS2_OPTION_EXPOSURE);
            color_ep->try_register_pu(RS2_OPTION_GAMMA);
            color_ep->try_register_pu(RS2_OPTION_HUE);
            color_ep->try_register_pu(RS2_OPTION_SATURATION);
            color_ep->try_register_pu(RS2_OPTION_SHARPNESS);
            color_ep->try_register_pu(RS2_OPTION_WHITE_BALANCE);
            color_ep->try_register_pu(RS2_OPTION_ENABLE_AUTO_EXPOSURE);
            color_ep->try_register_pu(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);
        }

        virtual rs2_intrinsics get_intrinsics(unsigned int, const stream_profile&) const
        {
            return rs2_intrinsics {};
        }

        std::vector<tagged_profile> get_profiles_tags() const override
        {
            std::vector<tagged_profile> markers;
            markers.push_back({ RS2_STREAM_COLOR, -1, 640, 480, RS2_FORMAT_RGB8, 30, profile_tag::PROFILE_TAG_SUPERSET | profile_tag::PROFILE_TAG_DEFAULT });
            return markers;
        }
    };

    std::shared_ptr<device_interface> platform_camera_info::create(std::shared_ptr<context> ctx,
                                                                   bool register_device_notifications) const
    {
        return std::make_shared<platform_camera>(ctx, _uvcs, this->get_device_data(), register_device_notifications);
    }

#ifdef BUILD_WITH_DDS
    struct sid_index
    {
        int sid;    // Stream ID; assigned based on the stream TYPE, really
        int index;  // Used to distinguish similar streams like IR L / R, 0 otherwise

        sid_index( int sid_, int index_ )
            : sid( sid_ )
            , index( index_ )
        {
        }

        sid_index() = default;
        sid_index( sid_index const& ) = default;
        sid_index( sid_index&& ) = default;

        std::string to_string() const { return '(' + std::to_string( sid ) + '.' + std::to_string( index ) + ')'; }

        inline bool operator<( sid_index const & r ) const
        {
            return this->sid < r.sid || this->sid == r.sid && this->index < r.index;
        }
    };

    //A facade for a realdds::dds_option exposing librealsense interface
    class rs2_dds_option : public option_base
    {
    public:
        typedef std::function< void( const std::string & name, float value ) > set_option_callback;
        typedef std::function< float( const std::string & name ) > query_option_callback;

        rs2_dds_option( const std::shared_ptr< realdds::dds_option > & dds_opt,
                        set_option_callback set_opt_cb,
                        query_option_callback query_opt_cb )
            : option_base( { dds_opt->get_range().min, dds_opt->get_range().max,
                             dds_opt->get_range().step, dds_opt->get_range().default_value } )
            , _dds_opt( dds_opt )
            , _set_opt_cb( set_opt_cb )
            , _query_opt_cb( query_opt_cb )
        {
        }

        void set( float value ) override
        {
            if( ! _set_opt_cb )
                throw std::runtime_error( "Set option callback is not set for option " + _dds_opt->get_name() );

            _set_opt_cb( _dds_opt->get_name(), value );
        }

        float query() const override
        {
            if( !_query_opt_cb )
                throw std::runtime_error( "Query option callback is not set for option " + _dds_opt->get_name() );

            return _query_opt_cb( _dds_opt->get_name() );
        }

        bool is_enabled() const override { return true; };
        const char * get_description() const override { return _dds_opt->get_description().c_str(); };

    protected:
        std::shared_ptr< realdds::dds_option > _dds_opt;

        set_option_callback _set_opt_cb;
        query_option_callback _query_opt_cb;
    };


    class dds_sensor_proxy : public software_sensor
    {
        std::shared_ptr< realdds::dds_device > const _dev;
        std::string const _name;
        bool const _md_enabled;

        typedef realdds::dds_metadata_syncer syncer_type;
        static void frame_releaser( syncer_type::frame_type * f ) { static_cast< frame * >( f )->release(); }

        std::map< sid_index, std::shared_ptr< realdds::dds_stream > > _streams;
        std::map< std::string, syncer_type > _stream_name_to_syncer;

    public:
        dds_sensor_proxy( std::string const & sensor_name,
                          software_device * owner,
                          std::shared_ptr< realdds::dds_device > const & dev )
            : software_sensor( sensor_name, owner )
            , _dev( dev )
            , _name( sensor_name )
            , _md_enabled( dev->supports_metadata() )
        {
        }

        void add_dds_stream( sid_index sidx, std::shared_ptr< realdds::dds_stream > const & stream )
        {
            auto & s = _streams[sidx];
            if( s )
            {
                LOG_ERROR( "stream at " << sidx.to_string() << " already exists for sensor '" << get_name() << "'" );
                return;
            }
            s = stream;
        }

        std::shared_ptr< realdds::dds_video_stream_profile >
        find_profile( sid_index sidx, realdds::dds_video_stream_profile const & profile ) const
        {
            auto it = _streams.find( sidx );
            if( it == _streams.end() )
            {
                LOG_ERROR( "Invalid stream index " << sidx.to_string() << " in rs2 profile for sensor '" << get_name() << "'" );
            }
            else
            {
                auto& stream = it->second;
                for( auto & sp : stream->profiles() )
                {
                    auto vsp = std::static_pointer_cast< realdds::dds_video_stream_profile >( sp );
                    if( profile.width() == vsp->width() && profile.height() == vsp->height()
                        && profile.format() == vsp->format() && profile.frequency() == vsp->frequency() )
                    {
                        return vsp;
                    }
                }
            }
            return std::shared_ptr< realdds::dds_video_stream_profile >();
        }

        std::shared_ptr< realdds::dds_motion_stream_profile >
        find_profile( sid_index sidx, realdds::dds_motion_stream_profile const & profile ) const
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
                    if( profile.format() == msp->format() && profile.frequency() == msp->frequency() )
                    {
                        return msp;
                    }
                }
            }
            return std::shared_ptr< realdds::dds_motion_stream_profile >();
        }

        void open( const stream_profiles & profiles ) override
        {
            realdds::dds_stream_profiles realdds_profiles;
            for( size_t i = 0; i < profiles.size(); ++i )
            {
                auto & sp = profiles[i];
                sid_index sidx( sp->get_unique_id(), sp->get_stream_index() );
                if( Is< video_stream_profile >( sp ) )
                {
                    const auto && vsp = As< video_stream_profile >( profiles[i] );
                    auto video_profile
                        = find_profile( sidx,
                                        realdds::dds_video_stream_profile(
                                            sp->get_framerate(),
                                            realdds::dds_stream_format::from_rs2( sp->get_format() ),
                                            vsp->get_width(),
                                            vsp->get_height() ) );
                    if( video_profile )
                        realdds_profiles.push_back( video_profile );
                    else
                        LOG_ERROR( "no profile found in stream for rs2 profile " << vsp );
                }
                else if( Is< motion_stream_profile >( sp ) )
                {
                    auto motion_profile
                        = find_profile( sidx,
                                        realdds::dds_motion_stream_profile(
                                            profiles[i]->get_framerate(),
                                            realdds::dds_stream_format::from_rs2( sp->get_format() ) ) );
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

            if ( profiles.size() > 0 )
            {
                _dev->open( realdds_profiles );
            }

            software_sensor::open( profiles );
        }

        void handle_video_data( realdds::topics::device::image && dds_frame,
                                const std::shared_ptr< stream_profile_interface > & profile,
                                syncer_type & syncer )
        {
            frame_additional_data data;  // with NO metadata by default!
            data.timestamp; // from metadata
            data.timestamp_domain; // from metadata
            data.depth_units; // from metadata
            if( ! rsutils::string::string_to_value( dds_frame.frame_id, data.frame_number ))
                data.frame_number = 0;

            auto vid_profile = dynamic_cast< video_stream_profile_interface * >( profile.get() );
            if( ! vid_profile )
                throw invalid_value_exception( "non-video profile provided to on_video_frame" );

            auto stride = static_cast< int >( dds_frame.height > 0 ? dds_frame.raw_data.size() / dds_frame.height
                                                                   : dds_frame.raw_data.size() );
            auto bpp = dds_frame.width > 0 ? stride / dds_frame.width : stride;
            auto new_frame_interface
                = allocate_new_video_frame( vid_profile, stride, bpp, std::move( data ) );
            if( ! new_frame_interface )
                return;

            auto new_frame = static_cast< frame * >( new_frame_interface );
            new_frame->data = std::move( dds_frame.raw_data );

            if( _md_enabled )
            {
                syncer.enqueue_frame( data.frame_number,  // for now, we use this as the synced-to ID
                                      syncer.hold( new_frame ) );
            }
            else
            {
                invoke_new_frame( new_frame,
                                  nullptr,    // pixels are already inside new_frame->data
                                  nullptr );  // so no deleter is necessary
            }
        }

        void add_frame_metadata( frame * const f, json && dds_md )
        {
            json const & md_header = dds_md["header"];
            json const & md = dds_md["metadata"];

            // Always expected metadata
            f->additional_data.timestamp = rsutils::json::get< rs2_time_t >( md_header, "timestamp" );
            f->additional_data.timestamp_domain = rsutils::json::get< rs2_timestamp_domain >( md_header, "timestamp-domain" );

            // Expected metadata for all depth images
            if( rsutils::json::has( md_header, "depth-units" ) )
                f->additional_data.depth_units = rsutils::json::get< float >( md_header, "depth-units" );

            // Other metadata fields. Metadata fields that are present but unknown by librealsense will be ignored.
            auto & metadata = reinterpret_cast< metadata_array & >( f->additional_data.metadata_blob );
            for( size_t i = 0; i < static_cast< size_t >( RS2_FRAME_METADATA_COUNT ); ++i )
            {
                auto key = static_cast< rs2_frame_metadata_value >( i );
                const char * keystr = rs2_frame_metadata_to_string( key );
                try
                {
                    metadata[key] = { true, rsutils::json::get< rs2_metadata_type >( md, keystr ) };
                }
                catch( std::runtime_error const & )
                {
                    // The metadata key doesn't exist or the value isn't the right type... we ignore it!
                    // (all metadata is not there when we create the frame, so no need to erase)
                }
            }
        }

        void handle_motion_data( realdds::topics::device::image && dds_frame,
                                 const std::shared_ptr< stream_profile_interface > & profile,
                                 syncer_type & syncer )
        {
            frame_additional_data data;  // with NO metadata by default!
            data.timestamp;              // from metadata
            data.timestamp_domain;       // from metadata
            data.depth_units;            // from metadata
            if( ! rsutils::string::string_to_value( dds_frame.frame_id, data.frame_number ) )
                data.frame_number = 0;

            auto new_frame_interface
                = allocate_new_frame( RS2_EXTENSION_MOTION_FRAME, profile.get(), std::move( data ) );
            if( ! new_frame_interface )
                return;

            auto new_frame = static_cast< frame * >( new_frame_interface );
            new_frame->data = std::move( dds_frame.raw_data );

            if( _md_enabled )
            {
                syncer.enqueue_frame( data.frame_number,  // for now, we use this as the synced-to ID
                                      syncer.hold( new_frame ));
            }
            else
            {
                invoke_new_frame( new_frame,
                                  nullptr,    // pixels are already inside new_frame->data
                                  nullptr );  // so no deleter is necessary
            }
        }

        void handle_new_metadata( std::string const & stream_name, json && dds_md )
        {
            if( ! _md_enabled )
                return;

            auto it = _stream_name_to_syncer.find( stream_name );
            if( it != _stream_name_to_syncer.end() )
                it->second.enqueue_metadata(
                    std::stoull( rsutils::json::get< std::string >( dds_md["header"], "frame-id" ) ),
                    std::move( dds_md ) );
            else
                throw std::runtime_error( "Stream '" + stream_name
                                          + "' received metadata for unsupported frame type" );
        }

        void start( frame_callback_ptr callback ) override
        {
            for ( auto & profile : sensor_base::get_active_streams() )
            {
                auto & dds_stream = _streams[sid_index( profile->get_unique_id(), profile->get_stream_index() )];
                // Opening it will start streaming on the server side automatically
                dds_stream->open( "rt/" + _dev->device_info().topic_root + '_' + dds_stream->name(), _dev->subscriber());
                _stream_name_to_syncer[dds_stream->name()]
                    .on_frame_release( frame_releaser )
                    .on_frame_ready(
                        [this]( syncer_type::frame_holder && fh, json && md )
                        {
                            if( ! md.empty() )
                                add_frame_metadata( static_cast< frame * >( fh.get() ), std::move( md ) );
                            // else the frame should already have empty metadata!
                            invoke_new_frame( static_cast< frame * >( fh.release() ), nullptr, nullptr );
                        } );

                if( Is< realdds::dds_video_stream >( dds_stream ) )
                {
                    As< realdds::dds_video_stream >( dds_stream )->on_data_available(
                        [profile, this, dds_stream]( realdds::topics::device::image && dds_frame ) {
                            handle_video_data( std::move( dds_frame ), profile, _stream_name_to_syncer[dds_stream->name()] );
                        } );
                }
                else if( Is< realdds::dds_motion_stream >( dds_stream ) )
                {
                    As< realdds::dds_motion_stream >( dds_stream )->on_data_available(
                        [profile, this, dds_stream]( realdds::topics::device::image && dds_frame ) {
                            handle_motion_data( std::move( dds_frame ), profile, _stream_name_to_syncer[dds_stream->name()] );
                        } );
                }
                else
                    throw std::runtime_error( "Unsupported stream type" );

                dds_stream->start_streaming();
            }

            software_sensor::start( callback );
        }

        void stop()
        {
            _stream_name_to_syncer.clear();

            for( auto & profile : sensor_base::get_active_streams() )
            {
                auto & dds_stream = _streams[sid_index( profile->get_unique_id(), profile->get_stream_index() )];
                dds_stream->stop_streaming();
                dds_stream->close();
            }

            software_sensor::stop();
        }

        void add_option( std::shared_ptr< realdds::dds_option > option )
        {
            //Convert name to rs2_option type
            rs2_option option_id = RS2_OPTION_COUNT;
            for( size_t i = 0; i < static_cast< size_t >( RS2_OPTION_COUNT ); i++ )
            {
                if( option->get_name().compare( get_string( static_cast< rs2_option >( i ) ) ) == 0 )
                {
                    option_id = static_cast< rs2_option >( i );
                    break;
                }
            }

            if( option_id == RS2_OPTION_COUNT )
                throw librealsense::invalid_value_exception( "Option " + option->get_name() + " type not found" );

            auto opt = std::make_shared< rs2_dds_option >( option,
                [&]( const std::string & name, float value ) { set_option( name, value ); },
                [&]( const std::string & name ) -> float { return query_option( name ); } );
            register_option( option_id, opt );
        }

        void set_option( const std::string & name, float value ) const
        {
            // Sensor is setting the option for all supporting streams (with same value)
            for( auto & stream : _streams )
            {
                for( auto & dds_opt : stream.second->options() )
                {
                    if( dds_opt->get_name().compare( name ) == 0 )
                    {
                        _dev->set_option_value( dds_opt, value );
                        break;
                    }
                }
            }
        }

        float query_option( const std::string & name ) const
        {
            for( auto & stream : _streams )
            {
                for( auto & dds_opt : stream.second->options() )
                {
                    if( dds_opt->get_name().compare( name ) == 0 )
                    {
                        //Assumes value is same for all relevant streams in the sensor, values are always set together
                        return _dev->query_option_value( dds_opt );
                    }
                }
            }

            throw std::runtime_error( "Could not find a stream that supports option " + name );
        }

        void add_processing_block( std::string filter_name )
        {
            auto & current_filters = get_software_recommended_proccesing_blocks();
            
            if( processing_block_exists( current_filters.get_recommended_processing_blocks(), filter_name ) )
                return; // Already created by another stream of this sensor

            create_processing_block( filter_name );
        }

        bool processing_block_exists( processing_blocks const & blocks, std::string const & block_name ) const
        {
            for( auto & block : blocks )
                if( block_name.compare( block->get_info( RS2_CAMERA_INFO_NAME ) ) == 0 )
                    return true;

            return false;
        }

        void create_processing_block( std::string & filter_name )
        {
            auto & current_filters = get_software_recommended_proccesing_blocks();

            if( filter_name.compare( "Decimation Filter" ) == 0 )
                // sensor.cpp sets format option based on sensor type, but the filter does not use it and selects the
                // appropriate decimation algorithm based on processed frame profile format.
                current_filters.add_processing_block( std::make_shared< decimation_filter >() );
            else if( filter_name.compare( "HDR Merge" ) == 0 )
                current_filters.add_processing_block( std::make_shared< hdr_merge >() );
            else if( filter_name.compare( "Filter By Sequence id" ) == 0 )
                current_filters.add_processing_block( std::make_shared< sequence_id_filter >() );
            else if( filter_name.compare( "Threshold Filter" ) == 0 )
                current_filters.add_processing_block( std::make_shared< threshold >() );
            else if( filter_name.compare( "Depth to Disparity" ) == 0 )
                current_filters.add_processing_block( std::make_shared< disparity_transform >( true ) );
            else if( filter_name.compare( "Disparity to Depth" ) == 0 )
                current_filters.add_processing_block( std::make_shared< disparity_transform >( false ) );
            else if( filter_name.compare( "Spatial Filter" ) == 0 )
                current_filters.add_processing_block( std::make_shared< spatial_filter >() );
            else if( filter_name.compare( "Temporal Filter" ) == 0 )
                current_filters.add_processing_block( std::make_shared< temporal_filter >() );
            else if( filter_name.compare( "Hole Filling Filter" ) == 0 )
                current_filters.add_processing_block( std::make_shared< hole_filling_filter >() );
            else
                throw std::runtime_error( "Unsupported processing block '" + filter_name + "' received" );
        }

        const std::string & get_name() const { return _name; }
    };

    // For cases when checking if this is< color_sensor > (like realsense-viewer::subdevice_model)
    class dds_color_sensor_proxy : public dds_sensor_proxy, public color_sensor
    {
    public:
        dds_color_sensor_proxy( std::string const & sensor_name,
                                software_device * owner,
                                std::shared_ptr< realdds::dds_device > const & dev )
            : dds_sensor_proxy( sensor_name, owner, dev )
        {
        }
    };

    // For cases when checking if this is< depth_sensor > (like realsense-viewer::subdevice_model)
    class dds_depth_sensor_proxy : public dds_sensor_proxy, public depth_sensor
    {
    public:
        dds_depth_sensor_proxy( std::string const & sensor_name,
                                software_device * owner,
                                std::shared_ptr< realdds::dds_device > const & dev )
            : dds_sensor_proxy( sensor_name, owner, dev )
        {
        }

        // Needed by abstract interfaces
        float get_depth_scale() const override { return get_option( RS2_OPTION_DEPTH_UNITS ).query(); }

        void create_snapshot( std::shared_ptr<depth_sensor> & snapshot ) const override
        {
            snapshot = std::make_shared<depth_sensor_snapshot>( get_depth_scale() );
        }

        void enable_recording( std::function<void( const depth_sensor & )> recording_function ) override
        {
            //does not change over time
        }
    };

    // This is the rs2 device; it proxies to an actual DDS device that does all the actual
    // work. For example:
    //     auto dev_list = ctx.query_devices();
    //     auto dev1 = dev_list[0];
    //     auto dev2 = dev_list[0];
    // dev1 and dev2 are two different rs2 devices, but they both go to the same DDS source!
    //
    class dds_device_proxy : public software_device
    {
        std::shared_ptr< realdds::dds_device > _dds_dev;
        std::map< std::string, std::vector< std::shared_ptr< stream_profile_interface > > > _stream_name_to_profiles;
        std::map< std::string, std::shared_ptr< librealsense::stream > > _stream_name_to_librs_stream;
        std::map< std::string, std::shared_ptr< dds_sensor_proxy > > _stream_name_to_owning_sensor;

        static rs2_stream to_rs2_stream_type( std::string const & type_string )
        {
            static const std::map< std::string, rs2_stream > type_to_rs2 = {
                { "depth", RS2_STREAM_DEPTH },
                { "color", RS2_STREAM_COLOR },
                { "ir", RS2_STREAM_INFRARED },
                { "fisheye", RS2_STREAM_FISHEYE },
                { "gyro", RS2_STREAM_GYRO },
                { "accel", RS2_STREAM_ACCEL },
                { "gpio", RS2_STREAM_GPIO },
                { "pose", RS2_STREAM_POSE },
                { "confidence", RS2_STREAM_CONFIDENCE },
            };
            auto it = type_to_rs2.find( type_string );
            if( it == type_to_rs2.end() )
            {
                LOG_ERROR( "Unknown stream type '" << type_string << "'" );
                return RS2_STREAM_ANY;
            }
            return it->second;
        }

        static rs2_video_stream
        to_rs2_video_stream( rs2_stream const stream_type,
                             sid_index const & sidx,
                             std::shared_ptr< realdds::dds_video_stream_profile > const & profile,
                             const std::set< realdds::video_intrinsics > & intrinsics )
        {
            rs2_video_stream prof = {};
            prof.type = stream_type;
            prof.index = sidx.index;
            prof.uid = sidx.sid;
            prof.width = profile->width();
            prof.height = profile->height();
            prof.fps = profile->frequency();
            prof.fmt = static_cast< rs2_format >( profile->format().to_rs2() );
            
            // Handle intrinsics
            auto intr = std::find_if( intrinsics.begin(),
                                      intrinsics.end(),
                                      [profile]( const realdds::video_intrinsics & intr )
                                      { return profile->width() == intr.width && profile->height() == intr.height; } );
            if( intr != intrinsics.end() ) //Some profiles don't have intrinsics
            {
                prof.intrinsics.width = intr->width;
                prof.intrinsics.height = intr->height;
                prof.intrinsics.ppx = intr->principal_point_x;
                prof.intrinsics.ppy = intr->principal_point_y;
                prof.intrinsics.fx = intr->focal_lenght_x;
                prof.intrinsics.fy = intr->focal_lenght_y;
                prof.intrinsics.model = static_cast< rs2_distortion >( intr->distortion_model );
                memcpy( prof.intrinsics.coeffs, intr->distortion_coeffs.data(), sizeof( prof.intrinsics.coeffs ) );
            }

            return prof;
        }

        static rs2_motion_stream
        to_rs2_motion_stream( rs2_stream const stream_type,
                              sid_index const & sidx,
                              std::shared_ptr< realdds::dds_motion_stream_profile > const & profile,
                              const realdds::motion_intrinsics & intrinsics )
        {
            rs2_motion_stream prof;
            prof.type = stream_type;
            prof.index = sidx.index;
            prof.uid = sidx.sid;
            prof.fps = profile->frequency();
            prof.fmt = static_cast< rs2_format >( profile->format().to_rs2() );

            memcpy( prof.intrinsics.data, intrinsics.data.data(), sizeof( prof.intrinsics.data ) );
            memcpy( prof.intrinsics.noise_variances, intrinsics.noise_variances.data(), sizeof( prof.intrinsics.noise_variances ) );
            memcpy( prof.intrinsics.bias_variances, intrinsics.bias_variances.data(), sizeof( prof.intrinsics.bias_variances ) );

            return prof;
        }

        static rs2_extrinsics to_rs2_extrinsics( const std::shared_ptr< realdds::extrinsics > & dds_extrinsics )
        {
            rs2_extrinsics rs2_extr;

            memcpy( rs2_extr.rotation, dds_extrinsics->rotation.data(), sizeof( rs2_extr.rotation ) );
            memcpy( rs2_extr.translation , dds_extrinsics->translation.data(), sizeof( rs2_extr.translation ) );

            return rs2_extr;
        }

    public:
        dds_device_proxy( std::shared_ptr< context > ctx, std::shared_ptr< realdds::dds_device > const & dev )
            : software_device( ctx )
            , _dds_dev( dev )
        {
            LOG_DEBUG( "=====> dds-device-proxy " << this << " created on top of dds-device " << _dds_dev.get() );
            register_info( RS2_CAMERA_INFO_NAME, dev->device_info().name );
            register_info( RS2_CAMERA_INFO_SERIAL_NUMBER, dev->device_info().serial );
            register_info( RS2_CAMERA_INFO_PRODUCT_LINE, dev->device_info().product_line );
            register_info( RS2_CAMERA_INFO_PRODUCT_ID, dev->device_info().product_id );
            register_info( RS2_CAMERA_INFO_PHYSICAL_PORT, dev->device_info().topic_root );
            register_info( RS2_CAMERA_INFO_CAMERA_LOCKED, dev->device_info().locked ? "YES" : "NO" );

            //Assumes dds_device initialization finished
            struct sensor_info
            {
                std::shared_ptr< dds_sensor_proxy > proxy;
                int sensor_index = 0;
            };
            std::map< std::string, sensor_info > sensor_name_to_info;
            // We assign (sid,index) based on the stream type:
            // LibRS assigns streams names based on the type followed by an index if it's not 0.
            // I.e., sid is based on the type, and index is 0 unless there's more than 1 in which case it's 1-based.
            int counts[RS2_STREAM_COUNT] = { 0 };
            // Count how many streams per type
            _dds_dev->foreach_stream( [&]( std::shared_ptr< realdds::dds_stream > const & stream ) {
                ++counts[to_rs2_stream_type( stream->type_string() )];
            } );
            // Anything that's more than 1 stream starts the count at 1, otherwise 0
            for( int & count : counts )
                count = ( count > 1 ) ? 1 : 0;
            // Now we can finally assign (sid,index):
            _dds_dev->foreach_stream( [&]( std::shared_ptr< realdds::dds_stream > const & stream ) {
                auto & sensor_info = sensor_name_to_info[stream->sensor_name()];
                if( ! sensor_info.proxy )
                {
                    // This is a new sensor we haven't seen yet
                    sensor_info.proxy = create_sensor( stream->sensor_name() );
                    sensor_info.sensor_index = add_sensor( sensor_info.proxy );
                    assert( sensor_info.sensor_index == _software_sensors.size() );
                    _software_sensors.push_back( sensor_info.proxy );
                }
                auto stream_type = to_rs2_stream_type( stream->type_string() );
                sid_index sidx( stream_type, counts[stream_type]++ );
                _stream_name_to_librs_stream[stream->name()] = std::make_shared< librealsense::stream >( stream_type, sidx.index );
                sensor_info.proxy->add_dds_stream( sidx, stream );
                _stream_name_to_owning_sensor[stream->name()] = sensor_info.proxy;
                LOG_DEBUG( sidx.to_string() << " " << stream->sensor_name() << " : " << stream->name() );

                software_sensor & sensor = get_software_sensor( sensor_info.sensor_index );
                auto video_stream = std::dynamic_pointer_cast< realdds::dds_video_stream >( stream );
                auto motion_stream = std::dynamic_pointer_cast< realdds::dds_motion_stream >( stream );
                auto & profiles = stream->profiles();
                auto const & default_profile = profiles[stream->default_profile_index()];
                for( auto & profile : profiles )
                {
                    if( video_stream )
                    {
                        auto added_stream_profile = sensor.add_video_stream(
                            to_rs2_video_stream(
                                stream_type,
                                sidx,
                                std::static_pointer_cast< realdds::dds_video_stream_profile >( profile ),
                                video_stream->get_intrinsics() ),
                            profile == default_profile );
                        _stream_name_to_profiles[stream->name()].push_back( added_stream_profile ); // for extrinsics
                    }
                    else if( motion_stream )
                    {
                        auto added_stream_profile = sensor.add_motion_stream(
                            to_rs2_motion_stream(
                                stream_type,
                                sidx,
                                std::static_pointer_cast< realdds::dds_motion_stream_profile >( profile ),
                                motion_stream->get_intrinsics() ),
                            profile == default_profile );
                        _stream_name_to_profiles[stream->name()].push_back( added_stream_profile ); // for extrinsics
                    }
                }

                auto & options = stream->options();
                for( auto & option : options )
                {
                    sensor_info.proxy->add_option( option );
                }

                auto & recommended_filters = stream->recommended_filters();
                for( auto & filter_name : recommended_filters )
                {
                    sensor_info.proxy->add_processing_block( filter_name );
                }
            } );  // End foreach_stream lambda


            if( _dds_dev->supports_metadata() )
            {
                _dds_dev->on_metadata_available(
                    [this]( json && dds_md )
                    {
                        std::string stream_name = rsutils::json::get< std::string >( dds_md, "stream-name" );
                        auto it = _stream_name_to_owning_sensor.find( stream_name );
                        if( it != _stream_name_to_owning_sensor.end() )
                            it->second->handle_new_metadata( stream_name, std::move( dds_md ) );
                    } );
            }

            // According to extrinsics_graph (in environment.h) we need 3 steps
            // 1. Register streams with extrinsics between them
            for( auto & from_stream : _stream_name_to_librs_stream )
            {
                for( auto & to_stream : _stream_name_to_librs_stream )
                {
                    if( from_stream.first != to_stream.first )
                    {
                        const auto & dds_extr = _dds_dev->get_extrinsics( from_stream.first, to_stream.first );
                        if( dds_extr )
                        {
                            rs2_extrinsics extr = to_rs2_extrinsics( dds_extr );
                            environment::get_instance().get_extrinsics_graph().register_extrinsics( *from_stream.second,
                                                                                                    *to_stream.second,
                                                                                                    extr );
                        }
                    }
                }
            }
            // 2. Register all profiles
            for( auto & it : _stream_name_to_profiles )
            {
                for( auto profile : it.second )
                {
                    environment::get_instance().get_extrinsics_graph().register_profile( *profile );
                }
            }
            // 3. Link profile to it's stream
            for( auto & it : _stream_name_to_librs_stream )
            {
                for( auto & profile : _stream_name_to_profiles[it.first] )
                {
                    environment::get_instance().get_extrinsics_graph().register_same_extrinsics( *it.second, *profile );
                }
            }
            // TODO - need to register extrinsics group in dev?
        } //End dds_device_proxy constructor

        std::shared_ptr< dds_sensor_proxy> create_sensor( const std::string & sensor_name )
        {
            if( sensor_name.compare( "RGB Camera" ) == 0 )
                return std::make_shared< dds_color_sensor_proxy>( sensor_name, this, _dds_dev );
            else if( sensor_name.compare( "Stereo Module" ) == 0 )
                return std::make_shared< dds_depth_sensor_proxy>( sensor_name, this, _dds_dev );

            return std::make_shared< dds_sensor_proxy >( sensor_name, this, _dds_dev );
        }
    };

    class dds_device_info : public device_info
    {
        std::shared_ptr< realdds::dds_device > _dev;

    public:
        dds_device_info( std::shared_ptr< context > const & ctx, std::shared_ptr< realdds::dds_device > const & dev )
            : device_info( ctx )
            , _dev( dev )
        {
        }

        std::shared_ptr< device_interface > create( std::shared_ptr< context > ctx,
                                                    bool register_device_notifications ) const override
        {
            return std::make_shared< dds_device_proxy >( ctx, _dev );
        }

        platform::backend_device_group get_device_data() const override
        {
            return platform::backend_device_group{ { platform::playback_device_info{ _dev->device_info().topic_root } } };
        }
    };
#endif //BUILD_WITH_DDS

    context::~context()
    {
        //ensure that the device watchers will stop before the _devices_changed_callback will be deleted

        if ( _device_watcher )
            _device_watcher->stop(); 
#ifdef BUILD_WITH_DDS
        if( _dds_watcher )
            _dds_watcher->stop();
#endif //BUILD_WITH_DDS
    }

    std::vector<std::shared_ptr<device_info>> context::query_devices(int mask) const
    {
        platform::backend_device_group devices;
        if( ! ( mask & RS2_PRODUCT_LINE_SW_ONLY ) )
        {
            devices.uvc_devices = _backend->query_uvc_devices();
            devices.usb_devices = _backend->query_usb_devices();
            devices.hid_devices = _backend->query_hid_devices();
        }
        return create_devices( devices, _playback_devices, mask );
    }

    std::vector< std::shared_ptr< device_info > >
    context::create_devices( platform::backend_device_group devices,
                             const std::map< std::string, std::weak_ptr< device_info > > & playback_devices,
                             int mask ) const
    {
        std::vector<std::shared_ptr<device_info>> list;

        auto t = const_cast<context*>(this); // While generally a bad idea, we need to provide mutable reference to the devices
        // to allow them to modify context later on
        auto ctx = t->shared_from_this();

        if( mask & RS2_PRODUCT_LINE_D400 )
        {
            auto d400_devices = d400_info::pick_d400_devices(ctx, devices);
            std::copy(begin(d400_devices), end(d400_devices), std::back_inserter(list));
        }

        if( mask & RS2_PRODUCT_LINE_L500 )
        {
            auto l500_devices = l500_info::pick_l500_devices(ctx, devices);
            std::copy(begin(l500_devices), end(l500_devices), std::back_inserter(list));
        }

        if( mask & RS2_PRODUCT_LINE_SR300 )
        {
            auto sr300_devices = sr300_info::pick_sr300_devices(ctx, devices.uvc_devices, devices.usb_devices);
            std::copy(begin(sr300_devices), end(sr300_devices), std::back_inserter(list));
        }

#ifdef BUILD_WITH_DDS
        if( _dds_watcher )
            _dds_watcher->foreach_device(
                [&]( std::shared_ptr< realdds::dds_device > const & dev ) -> bool
                {
                    if( ! dev->is_running() )
                    {
                        LOG_DEBUG( "device '" << dev->device_info().topic_root << "' is not yet running" );
                        return true;
                    }
                    if( dev->device_info().product_line == "D400" )
                    {
                        if( ! ( mask & RS2_PRODUCT_LINE_D400 ) )
                            return true;
                    }
                    else if( dev->device_info().product_line == "L500" )
                    {
                        if( ! ( mask & RS2_PRODUCT_LINE_L500 ) )
                            return true;
                    }
                    else if( dev->device_info().product_line == "SR300" )
                    {
                        if( ! ( mask & RS2_PRODUCT_LINE_SR300 ) )
                            return true;
                    }
                    else if( ! ( mask & RS2_PRODUCT_LINE_NON_INTEL ) )
                    {
                        return true;
                    }

                    std::shared_ptr< device_info > info = std::make_shared< dds_device_info >( ctx, dev );
                    list.push_back( info );
                    return true;
                } );
#endif //BUILD_WITH_DDS

        // Supported recovery devices
        if( mask & RS2_PRODUCT_LINE_D400 || mask & RS2_PRODUCT_LINE_SR300 || mask & RS2_PRODUCT_LINE_L500 ) 
        {
            auto recovery_devices = fw_update_info::pick_recovery_devices(ctx, devices.usb_devices, mask);
            std::copy(begin(recovery_devices), end(recovery_devices), std::back_inserter(list));
        }

        if( mask & RS2_PRODUCT_LINE_NON_INTEL )
        {
            auto uvc_devices = platform_camera_info::pick_uvc_devices(ctx, devices.uvc_devices);
            std::copy(begin(uvc_devices), end(uvc_devices), std::back_inserter(list));
        }

        for (auto&& item : playback_devices)
        {
            if (auto dev = item.second.lock())
                list.push_back(dev);
        }

        if (list.size())
            LOG_INFO( "Found " << list.size() << " RealSense devices (mask 0x" << std::hex << mask << ")" << std::dec );
        return list;
    }


    void context::on_device_changed(platform::backend_device_group old,
                                    platform::backend_device_group curr,
                                    const std::map<std::string, std::weak_ptr<device_info>>& old_playback_devices,
                                    const std::map<std::string, std::weak_ptr<device_info>>& new_playback_devices)
    {
        auto old_list = create_devices(old, old_playback_devices, RS2_PRODUCT_LINE_ANY);
        auto new_list = create_devices(curr, new_playback_devices, RS2_PRODUCT_LINE_ANY);

        if (librealsense::list_changed<std::shared_ptr<device_info>>(old_list, new_list, [](std::shared_ptr<device_info> first, std::shared_ptr<device_info> second) {return *first == *second; }))
        {
            std::vector<rs2_device_info> rs2_devices_info_added;
            std::vector<rs2_device_info> rs2_devices_info_removed;

            auto devices_info_removed = subtract_sets(old_list, new_list);

            for (size_t i = 0; i < devices_info_removed.size(); i++)
            {
                rs2_devices_info_removed.push_back({ shared_from_this(), devices_info_removed[i] });
                LOG_DEBUG("\nDevice disconnected:\n\n" << std::string(devices_info_removed[i]->get_device_data()));
            }

            auto devices_info_added = subtract_sets(new_list, old_list);
            for (size_t i = 0; i < devices_info_added.size(); i++)
            {
                rs2_devices_info_added.push_back({ shared_from_this(), devices_info_added[i] });
                LOG_DEBUG("\nDevice connected:\n\n" << std::string(devices_info_added[i]->get_device_data()));
            }

            invoke_devices_changed_callbacks( rs2_devices_info_removed, rs2_devices_info_added );
        }
    }

    void context::invoke_devices_changed_callbacks( std::vector<rs2_device_info> & rs2_devices_info_removed,
                                                    std::vector<rs2_device_info> & rs2_devices_info_added )
    {
        std::map<uint64_t, devices_changed_callback_ptr> devices_changed_callbacks;
        {
            std::lock_guard<std::mutex> lock( _devices_changed_callbacks_mtx );
            devices_changed_callbacks = _devices_changed_callbacks;
        }

        for( auto & kvp : devices_changed_callbacks )
        {
            try
            {
                kvp.second->on_devices_changed( new rs2_device_list( { shared_from_this(), rs2_devices_info_removed } ),
                                                new rs2_device_list( { shared_from_this(), rs2_devices_info_added } ) );
            }
            catch( ... )
            {
                LOG_ERROR( "Exception thrown from user callback handler" );
            }
        }

        raise_devices_changed( rs2_devices_info_removed, rs2_devices_info_added );
    }

    void context::raise_devices_changed(const std::vector<rs2_device_info>& removed, const std::vector<rs2_device_info>& added)
    {
        if (_devices_changed_callback)
        {
            try
            {
                _devices_changed_callback->on_devices_changed(new rs2_device_list({ shared_from_this(), removed }),
                    new rs2_device_list({ shared_from_this(), added }));
            }
            catch (...)
            {
                LOG_ERROR("Exception thrown from user callback handler");
            }
        }
    }

    void context::start_device_watcher()
    {
        _device_watcher->start([this](platform::backend_device_group old, platform::backend_device_group curr)
        {
            on_device_changed(old, curr, _playback_devices, _playback_devices);
        });
    }

#ifdef BUILD_WITH_DDS
    void context::start_dds_device_watcher( size_t message_timeout_ms )
    {
        _dds_watcher->on_device_added( [this, message_timeout_ms]( std::shared_ptr< realdds::dds_device > const & dev ) {
            dev->run( message_timeout_ms );

            std::vector<rs2_device_info> rs2_device_info_added;
            std::vector<rs2_device_info> rs2_device_info_removed;
            std::shared_ptr< device_info > info = std::make_shared< dds_device_info >( shared_from_this(), dev );
            rs2_device_info_added.push_back( { shared_from_this(), info } );
            invoke_devices_changed_callbacks( rs2_device_info_removed, rs2_device_info_added );
        } );
        _dds_watcher->on_device_removed( [this]( std::shared_ptr< realdds::dds_device > const & dev ) {
            std::vector<rs2_device_info> rs2_device_info_added;
            std::vector<rs2_device_info> rs2_device_info_removed;
            std::shared_ptr< device_info > info = std::make_shared< dds_device_info >( shared_from_this(), dev );
            rs2_device_info_removed.push_back( { shared_from_this(), info } );
            invoke_devices_changed_callbacks( rs2_device_info_removed, rs2_device_info_added );
        } );
        _dds_watcher->start();
    }
#endif //BUILD_WITH_DDS

    uint64_t context::register_internal_device_callback(devices_changed_callback_ptr callback)
    {
        std::lock_guard<std::mutex> lock(_devices_changed_callbacks_mtx);
        auto callback_id = unique_id::generate_id();
        _devices_changed_callbacks.insert(std::make_pair(callback_id, std::move(callback)));

        if (_device_watcher->is_stopped())
        {
            start_device_watcher();
        }
        return callback_id;
    }

    void context::unregister_internal_device_callback(uint64_t cb_id)
    {
        std::lock_guard<std::mutex> lock(_devices_changed_callbacks_mtx);
        _devices_changed_callbacks.erase(cb_id);

        if (_devices_changed_callback == nullptr && _devices_changed_callbacks.size() == 0) // There are no register callbacks any more _device_watcher can be stopped
        {
            _device_watcher->stop();
        }
    }

    void context::set_devices_changed_callback(devices_changed_callback_ptr callback)
    {
        std::lock_guard<std::mutex> lock(_devices_changed_callbacks_mtx);
        _devices_changed_callback = std::move(callback);

        if (_device_watcher->is_stopped())
        {
            start_device_watcher();
        }
    }

    std::vector<platform::uvc_device_info> filter_by_product(const std::vector<platform::uvc_device_info>& devices, const std::set<uint16_t>& pid_list)
    {
        std::vector<platform::uvc_device_info> result;
        for (auto&& info : devices)
        {
            if (pid_list.count(info.pid))
                result.push_back(info);
        }
        return result;
    }

    // TODO: Make template
    std::vector<platform::usb_device_info> filter_by_product(const std::vector<platform::usb_device_info>& devices, const std::set<uint16_t>& pid_list)
    {
        std::vector<platform::usb_device_info> result;
        for (auto&& info : devices)
        {
            if (pid_list.count(info.pid))
                result.push_back(info);
        }
        return result;
    }

    std::vector<std::pair<std::vector<platform::uvc_device_info>, std::vector<platform::hid_device_info>>> group_devices_and_hids_by_unique_id(
        const std::vector<std::vector<platform::uvc_device_info>>& devices,
        const std::vector<platform::hid_device_info>& hids)
    {
        std::vector<std::pair<std::vector<platform::uvc_device_info>, std::vector<platform::hid_device_info>>> results;
        uint16_t vid;
        uint16_t pid;

        for (auto&& dev : devices)
        {
            std::vector<platform::hid_device_info> hid_group;
            auto unique_id = dev.front().unique_id;
            auto device_serial = dev.front().serial;

            for (auto&& hid : hids)
            {
                if( ! hid.unique_id.empty() )
                {
                    std::stringstream(hid.vid) >> std::hex >> vid;
                    std::stringstream(hid.pid) >> std::hex >> pid;

                    if (hid.unique_id == unique_id)
                    {
                        hid_group.push_back(hid);
                    }
                }
            }
            results.push_back(std::make_pair(dev, hid_group));
        }
        return results;
    }

    std::shared_ptr<playback_device_info> context::add_device(const std::string& file)
    {
        auto it = _playback_devices.find(file);
        if (it != _playback_devices.end() && it->second.lock())
        {
            //Already exists
            throw librealsense::invalid_value_exception( rsutils::string::from()
                                                         << "File \"" << file << "\" already loaded to context" );
        }
        auto playback_dev = std::make_shared<playback_device>(shared_from_this(), std::make_shared<ros_reader>(file, shared_from_this()));
        auto dinfo = std::make_shared<playback_device_info>(playback_dev);
        auto prev_playback_devices = _playback_devices;
        _playback_devices[file] = dinfo;
        on_device_changed({}, {}, prev_playback_devices, _playback_devices);
        return dinfo;
    }

    void context::add_software_device(std::shared_ptr<device_info> dev)
    {
        auto file = dev->get_device_data().playback_devices.front().file_path;

        auto it = _playback_devices.find(file);
        if (it != _playback_devices.end() && it->second.lock())
        {
            //Already exists
            throw librealsense::invalid_value_exception( rsutils::string::from() << "File \"" << file << "\" already loaded to context");
        }
        auto prev_playback_devices = _playback_devices;
        _playback_devices[file] = dev;
        on_device_changed({}, {}, prev_playback_devices, _playback_devices);
    }

    void context::remove_device(const std::string& file)
    {
        auto it = _playback_devices.find(file);
        if(it == _playback_devices.end() || !it->second.lock())
        {
            //Not found
            return;
        }
        auto prev_playback_devices =_playback_devices;
        _playback_devices.erase(it);
        on_device_changed({},{}, prev_playback_devices, _playback_devices);
    }

    std::vector<std::vector<platform::uvc_device_info>> group_devices_by_unique_id(const std::vector<platform::uvc_device_info>& devices)
    {
        std::map<std::string, std::vector<platform::uvc_device_info>> map;
        for (auto&& info : devices)
        {
            map[info.unique_id].push_back(info);
        }
        std::vector<std::vector<platform::uvc_device_info>> result;
        for (auto&& kvp : map)
        {
            result.push_back(kvp.second);
        }
        return result;
    }

    // TODO: Sergey
    // Make template
    void trim_device_list(std::vector<platform::usb_device_info>& devices, const std::vector<platform::usb_device_info>& chosen)
    {
        if (chosen.empty())
            return;

        auto was_chosen = [&chosen](const platform::usb_device_info& info)
        {
            return find(chosen.begin(), chosen.end(), info) != chosen.end();
        };
        devices.erase(std::remove_if(devices.begin(), devices.end(), was_chosen), devices.end());
    }

    void trim_device_list(std::vector<platform::uvc_device_info>& devices, const std::vector<platform::uvc_device_info>& chosen)
    {
        if (chosen.empty())
            return;

        auto was_chosen = [&chosen](const platform::uvc_device_info& info)
        {
            return find(chosen.begin(), chosen.end(), info) != chosen.end();
        };
        devices.erase(std::remove_if(devices.begin(), devices.end(), was_chosen), devices.end());
    }

    bool mi_present(const std::vector<platform::uvc_device_info>& devices, uint32_t mi)
    {
        for (auto&& info : devices)
        {
            if (info.mi == mi)
                return true;
        }
        return false;
    }

    platform::uvc_device_info get_mi(const std::vector<platform::uvc_device_info>& devices, uint32_t mi)
    {
        for (auto&& info : devices)
        {
            if (info.mi == mi)
                return info;
        }
        throw invalid_value_exception("Interface not found!");
    }

    std::vector<platform::uvc_device_info> filter_by_mi(const std::vector<platform::uvc_device_info>& devices, uint32_t mi)
    {
        std::vector<platform::uvc_device_info> results;
        for (auto&& info : devices)
        {
            if (info.mi == mi)
                results.push_back(info);
        }
        return results;
    }
}

using namespace librealsense;
