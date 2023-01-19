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
#include "ds5/ds5-factory.h"
#include "l500/l500-factory.h"
#include "ds5/ds5-timestamp.h"
#include "backend.h"
#include "mock/recorder.h"
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
#include "software-device.h"
#include <librealsense2/h/rs_internal.h>
#include <realdds/topics/device-info/device-info-msg.h>
#include <realdds/topics/image/image-msg.h>
#include <rsutils/shared-ptr-singleton.h>
#include <fastdds/dds/domain/DomainParticipant.hpp>

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

#ifdef WITH_TRACKING
#include "tm2/tm-info.h"
#endif

template<unsigned... Is> struct seq{};
template<unsigned N, unsigned... Is>
struct gen_seq : gen_seq<N-1, N-1, Is...>{};
template<unsigned... Is>
struct gen_seq<0, Is...> : seq<Is...>{};

template<unsigned N1, unsigned... I1, unsigned N2, unsigned... I2>
constexpr std::array<char const, N1+N2-1> concat(char const (&a1)[N1], char const (&a2)[N2], seq<I1...>, seq<I2...>){
  return {{ a1[I1]..., a2[I2]... }};
}

template<unsigned N1, unsigned N2>
constexpr std::array<char const, N1+N2-1> concat(char const (&a1)[N1], char const (&a2)[N2]){
  return concat(a1, a2, gen_seq<N1-1>{}, gen_seq<N2>{});
}

// The string is used to retrieve the version embedded into .so file on Linux
constexpr auto rs2_api_version = concat("VERSION: ",RS2_API_VERSION_STR);

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
            LOG_DEBUG( "Librealsense " << std::string( std::begin( rs2_api_version ), std::end( rs2_api_version ) ) );
        }
    }


    context::context(backend_type type,
                     const char* filename,
                     const char* section,
                     rs2_recording_mode mode,
                     std::string min_api_version)
        : context()
    {
        switch(type)
        {
        case backend_type::standard:
            _backend = platform::create_backend();
#ifdef BUILD_WITH_DDS
            {
                realdds::dds_domain_id domain_id = 0;
                auto & domain = dds_domain_context_by_id[domain_id];
                _dds_participant = domain.participant.instance();
                if( ! _dds_participant->is_valid() )
                    _dds_participant->init( domain_id, "librealsense" );
                _dds_watcher = domain.device_watcher.instance( _dds_participant );
            }
#endif //BUILD_WITH_DDS
            break;
        case backend_type::record:
            _backend = std::make_shared<platform::record_backend>(platform::create_backend(), filename, section, mode);
            break;
        case backend_type::playback:
            _backend = std::make_shared<platform::playback_backend>(filename, section, min_api_version);
            break;
            // Strongly-typed enum. Default is redundant
        }

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
            std::string participant_name = rsutils::json::get< std::string >( settings, "dds-participant-name", "librealsense" );

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

            // When building with DDS allowed, we want the DDS device watcher always on.
            // Not only when it has device change callback.
            // The initialize process is longer then USB. (getting all info from the device)
            
            if( _dds_watcher && _dds_watcher->is_stopped() )
            {
                start_dds_device_watcher();
            }
            //_dds_backend = ...; TODO
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

            std::unique_ptr<frame_timestamp_reader> host_timestamp_reader_backup(new ds5_timestamp_reader(environment::get_instance().get_time_service()));
            auto raw_color_ep = std::make_shared<uvc_sensor>("Raw RGB Camera",
                std::make_shared<platform::multi_pins_uvc_device>(devs),
                std::unique_ptr<frame_timestamp_reader>(new ds5_timestamp_reader_from_metadata(std::move(host_timestamp_reader_backup))),
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


    class dds_sensor_proxy : public software_sensor
    {
    public:
        dds_sensor_proxy( std::string const & sensor_name,
                          software_device * owner,
                          std::shared_ptr< realdds::dds_device > const & dev )
            : software_sensor( sensor_name, owner )
            , _dev( dev )
            , _name( sensor_name )
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

        void start( frame_callback_ptr callback ) override
        {
            //For each stream in sensor_base::get_active_streams() set callback function that will call on_video_frame
            for ( auto & profile : sensor_base::get_active_streams() )
            {
                //stream is the dds_stream matching the librealsense active stream
                auto & stream = _streams[sid_index( profile->get_unique_id(), profile->get_stream_index() )];
                stream->start_streaming( [p = profile, this]( realdds::topics::device::image && dds_frame ) {
                    rs2_stream_profile prof = { p.get() };
                    rs2_software_video_frame rs2_frame;
                    
                    //Copying from dds into LibRS space, same as copy from USB backend.
                    //TODO - use memory pool or some other frame allocator
                    rs2_frame.pixels = new uint8_t[dds_frame.size];
                    if ( !rs2_frame.pixels )
                        throw std::runtime_error( "Could not allocate memory for new frame" );
                    memcpy( rs2_frame.pixels, dds_frame.raw_data.data(), dds_frame.size );
                    
                    rs2_frame.deleter = []( void * ptr) { delete[] ptr; };
                    rs2_frame.stride = dds_frame.size / dds_frame.height;
                    rs2_frame.bpp = rs2_frame.stride / dds_frame.width;
                    rs2_frame.timestamp = frame_counter * 1000.0 / p->get_framerate(); // TODO - timestamp from dds
                    rs2_frame.domain = RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK; // TODO - timestamp domain from options?
                    rs2_frame.frame_number = frame_counter++; // TODO - frame_number from dds
                    rs2_frame.profile = &prof;
                    rs2_frame.depth_units = 0.001f; //TODO - depth unit from dds, if needed
                    on_video_frame( rs2_frame );
                } );
            }

            software_sensor::start( callback );
        }

        void stop()
        {
            for ( auto & profile : sensor_base::get_active_streams() )
            {
                //stream is the dds_stream matching the librealsense active stream
                auto & stream = _streams[sid_index( profile->get_unique_id(), profile->get_stream_index() )];
                stream->stop_streaming();
            }

            software_sensor::stop();
        }

        void close() override
        {
            realdds::dds_streams streams_to_close;
            for( auto & profile : sensor_base::get_active_streams() )
            {
                streams_to_close.push_back(
                    _streams[sid_index( profile->get_unique_id(), profile->get_stream_index() )] );
            }
            _dev->close( streams_to_close );
            software_sensor::close();
        }

        //float get_option( rs2_option option ) const override;
        //void set_option( rs2_option option, float value ) const override;

        const std::string & get_name() const { return _name; }

    private:
        std::shared_ptr< realdds::dds_device > const & _dev;
        std::string _name;
        std::map< sid_index, std::shared_ptr< realdds::dds_stream > > _streams;

        int frame_counter = 0;
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
                             std::shared_ptr< realdds::dds_video_stream_profile > const & profile )
        {
            rs2_video_stream prof;
            prof.type = stream_type;
            prof.index = sidx.index;
            prof.uid = sidx.sid;
            prof.width = profile->width();
            prof.height = profile->height();
            prof.fps = profile->frequency();
            prof.fmt = static_cast< rs2_format >( profile->format().to_rs2() );
            // TODO - add intrinsics

            return prof;
        }

        static rs2_motion_stream
        to_rs2_motion_stream( rs2_stream const stream_type,
                              sid_index const & sidx,
                              std::shared_ptr< realdds::dds_motion_stream_profile > const & profile )
        {
            rs2_motion_stream prof;
            prof.type = stream_type;
            prof.index = sidx.index;
            prof.uid = sidx.sid;
            prof.fps = profile->frequency();
            prof.fmt = static_cast< rs2_format >( profile->format().to_rs2() );

            return prof;
        }

    public:
        dds_device_proxy( std::shared_ptr< context > ctx, std::shared_ptr< realdds::dds_device > const & dev )
            : software_device( ctx )
            , _dds_dev( dev )
        {
            LOG_DEBUG( "=====> dds-device-proxy " << this << " created on top of dds-device " << _dds_dev.get() );
            auto & info = dev->device_info();
            register_info( RS2_CAMERA_INFO_NAME, info.name );
            register_info( RS2_CAMERA_INFO_PRODUCT_LINE, info.product_line );
            register_info( RS2_CAMERA_INFO_SERIAL_NUMBER, info.serial );
            register_info( RS2_CAMERA_INFO_CAMERA_LOCKED, info.locked ? "YES" : "NO" );
            register_info( RS2_CAMERA_INFO_PHYSICAL_PORT, info.topic_root );

            //Assumes dds_device initialization finished
            struct sensor_info
            {
                std::shared_ptr< dds_sensor_proxy > proxy;
                int sensor_index;
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
                auto & info = sensor_name_to_info[stream->sensor_name()];
                if( ! info.proxy )
                {
                    // This is a new sensor we haven't seen yet
                    info.proxy = std::make_shared< dds_sensor_proxy >( stream->sensor_name(), this, _dds_dev );
                    info.sensor_index = add_sensor( info.proxy );
                    assert( info.sensor_index == _software_sensors.size() );
                    _software_sensors.push_back( info.proxy );
                }
                auto stream_type = to_rs2_stream_type( stream->type_string() );
                sid_index sidx( stream_type, counts[stream_type]++ );
                info.proxy->add_dds_stream( sidx, stream );
                LOG_DEBUG( sidx.to_string() << " " << stream->sensor_name() << " : " << stream->name() );

                software_sensor & sensor = get_software_sensor( info.sensor_index );
                auto video_stream = std::dynamic_pointer_cast< realdds::dds_video_stream >( stream );
                auto motion_stream = std::dynamic_pointer_cast< realdds::dds_motion_stream >( stream );
                auto & profiles = stream->profiles();
                auto const & default_profile = profiles[stream->default_profile_index()];
                for( auto & profile : profiles )
                {
                    if( video_stream )
                    {
                        sensor.add_video_stream(
                            to_rs2_video_stream(
                                stream_type,
                                sidx,
                                std::static_pointer_cast< realdds::dds_video_stream_profile >( profile ) ),
                            profile == default_profile );
                    }
                    else if( motion_stream )
                    {
                        sensor.add_motion_stream(
                            to_rs2_motion_stream(
                                stream_type,
                                sidx,
                                std::static_pointer_cast< realdds::dds_motion_stream_profile >( profile ) ),
                            profile == default_profile );
                    }
                }
            } );  // End foreach_stream lambda
        } //End dds_device_proxy constructor
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
            return platform::backend_device_group{};
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
            auto ds5_devices = ds5_info::pick_ds5_devices(ctx, devices);
            std::copy(begin(ds5_devices), end(ds5_devices), std::back_inserter(list));

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
            _dds_watcher->foreach_device( [&]( std::shared_ptr< realdds::dds_device > const & dev ) -> bool {
                //if( mask & RS2_PRODUCT_LINE_D400 )
                    //if( dev.product_line == "D400" )
                    {
                        std::shared_ptr< device_info > info = std::make_shared< dds_device_info >( ctx, dev );
                        list.push_back( info );
                    }
                return true;
            } );
#endif //BUILD_WITH_DDS

#ifdef WITH_TRACKING
        if (mask & RS2_PRODUCT_LINE_T200)
        {
            auto tm2_devices = tm2_info::pick_tm2_devices(ctx, devices.usb_devices);
            std::copy(begin(tm2_devices), end(tm2_devices), std::back_inserter(list));
        }
#endif //WITH_TRACKING
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
            LOG_INFO( "Found " << list.size() << " RealSense devices (mask 0x" << std::hex << mask << ")" );
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

            std::map<uint64_t, devices_changed_callback_ptr> devices_changed_callbacks;
            {
                std::lock_guard<std::mutex> lock(_devices_changed_callbacks_mtx);
                devices_changed_callbacks = _devices_changed_callbacks;
            }

            for (auto& kvp : devices_changed_callbacks)
            {
                try
                {
                    kvp.second->on_devices_changed(new rs2_device_list({ shared_from_this(), rs2_devices_info_removed }),
                                                   new rs2_device_list({ shared_from_this(), rs2_devices_info_added }));
                }
                catch (...)
                {
                    LOG_ERROR("Exception thrown from user callback handler");
                }
            }

            raise_devices_changed(rs2_devices_info_removed, rs2_devices_info_added);
        }
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
    void context::start_dds_device_watcher()
    {
        // TODO Here we should add DDS devices to the `on_device_changed`parameters
        // on_device_changed(old, curr, _playback_devices, _playback_devices);
        _dds_watcher->on_device_added( [this]( std::shared_ptr< realdds::dds_device > const & dev ) { dev->run(); } );
        _dds_watcher->on_device_removed( [this]( std::shared_ptr< realdds::dds_device > const & dev ) {} );
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
#ifdef BUILD_WITH_DDS
            if( _dds_watcher )
                _dds_watcher->stop();
#endif //BUILD_WITH_DDS
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
        return std::move(dinfo);
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

#if WITH_TRACKING
    void context::unload_tracking_module()
    {
    }
#endif

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
