// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#ifdef _MSC_VER
#if (_MSC_VER <= 1800) // constexpr is not supported in MSVC2013
#error( "Librealsense requires MSVC2015 or later to build. Compilation will be aborted" )
#endif
#endif

#include <array>
#include <chrono>
#include "ds/d400/d400-factory.h"
#include "device.h"
#include "ds/ds-timestamp.h"
#include "backend.h"
#include <media/ros/ros_reader.h>
#include "types.h"
#include "stream.h"
#include "environment.h"
#include "context.h"
#include "fw-update/fw-update-factory.h"
#include "proc/color-formats-converter.h"


#ifdef BUILD_WITH_DDS
#include "dds/rs-dds-device-info.h"

#include <realdds/dds-device-watcher.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-device.h>
#include <realdds/topics/device-info-msg.h>
#include <rsutils/shared-ptr-singleton.h>
#include <rsutils/os/executable-name.h>
#include <rsutils/string/slice.h>

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
                _dds_participant->init( domain_id, rsutils::os::executable_name(), {} );
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
        _settings = settings;

        _backend = platform::create_backend();  // standard type

        environment::get_instance().set_time_service( _backend->create_time_service() );

        _device_watcher = _backend->create_device_watcher();
        assert( _device_watcher->is_stopped() );

#ifdef BUILD_WITH_DDS
        nlohmann::json dds_settings
            = rsutils::json::get< nlohmann::json >( settings, std::string( "dds", 3 ), nlohmann::json::object() );
        if( dds_settings.is_object() )
        {
            realdds::dds_domain_id domain_id
                = rsutils::json::get< int >( dds_settings, std::string( "domain", 6 ), 0 );
            std::string participant_name = rsutils::json::get< std::string >( dds_settings,
                                                                              std::string( "participant", 11 ),
                                                                              rsutils::os::executable_name() );

            auto & domain = dds_domain_context_by_id[domain_id];
            _dds_participant = domain.participant.instance();
            if( ! _dds_participant->is_valid() )
            {
                _dds_participant->init( domain_id, participant_name, std::move( dds_settings ) );
            }
            else if( rsutils::json::has_value( dds_settings, std::string( "participant", 11 ) )
                     && participant_name != _dds_participant->name() )
            {
                throw std::runtime_error( rsutils::string::from() << "A DDS participant '" << _dds_participant->name()
                                                                  << "' already exists in domain " << domain_id
                                                                  << "; cannot create '" << participant_name << "'" );
            }
            _dds_watcher = domain.device_watcher.instance( _dds_participant );

            // The DDS device watcher should always be on
            if( _dds_watcher && _dds_watcher->is_stopped() )
            {
                start_dds_device_watcher();
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

#ifdef BUILD_WITH_DDS
        if( _dds_watcher )
            _dds_watcher->foreach_device(
                [&]( std::shared_ptr< realdds::dds_device > const & dev ) -> bool
                {
                    if( ! dev->is_ready() )
                    {
                        LOG_DEBUG( "device '" << dev->device_info().debug_name() << "' is not yet ready" );
                        return true;
                    }
                    if( dev->device_info().product_line == "D400" )
                    {
                        if( ! ( mask & RS2_PRODUCT_LINE_D400 ) )
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
        if (mask & RS2_PRODUCT_LINE_D400) 
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
    void context::start_dds_device_watcher()
    {
        _dds_watcher->on_device_added( [this]( std::shared_ptr< realdds::dds_device > const & dev ) {
            dev->wait_until_ready();  // make sure handshake is complete

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
