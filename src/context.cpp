// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "context.h"

#include "ds/d400/d400-info.h"
#include "ds/d500/d500-info.h"
#include "device.h"
#include "ds/ds-timestamp.h"
#include <media/ros/ros_reader.h>
#include "media/playback/playback-device-info.h"
#include "types.h"
#include "stream.h"
#include "environment.h"
#include "fw-update/fw-update-factory.h"
#include "proc/color-formats-converter.h"
#include "platform-camera.h"
#include <src/backend.h>
#include <src/platform/backend-device-group.h>
#include "software-device.h"


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
bool contains( librealsense::platform::backend_device_group const & first_data,
               librealsense::platform::backend_device_group const & second_data )
{
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

std::vector< std::shared_ptr< librealsense::device_info > >
subtract_sets( const std::vector< std::shared_ptr< librealsense::device_info > > & first,
               const std::vector< std::shared_ptr< librealsense::device_info > > & second )
{
    std::vector< std::shared_ptr< librealsense::device_info > > results;
    std::for_each(
        first.begin(),
        first.end(),
        [&]( std::shared_ptr< librealsense::device_info > const & data )
        {
            if( std::find_if(
                    second.begin(),
                    second.end(),
                    [&]( std::shared_ptr< librealsense::device_info > const & new_dev )
                    {
                        if( auto pnew = std::dynamic_pointer_cast< librealsense::platform::platform_device_info >( new_dev ) )
                            if( auto pold = std::dynamic_pointer_cast<librealsense::platform::platform_device_info >( data ) )
                                return contains( pold->get_group(), pnew->get_group() );
                        return data->is_same_as( new_dev );
                    } )
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


    context::context( json const & settings )
        : context()
    {
        _settings = settings;

        _backend = platform::create_backend();  // standard type

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

    static unsigned calc_mask( nlohmann::json const & settings, unsigned requested_mask, unsigned * p_device_mask = nullptr )
    {
        // The 'device-mask' in our settings can be used to disable certain device types from showing up in the context:
        unsigned device_mask = RS2_PRODUCT_LINE_ANY;
        unsigned mask = requested_mask;
        if( rsutils::json::get_ex( settings, "device-mask", &device_mask ) )
        {
            // The normal bits enable, so enable only those that are on
            mask &= device_mask & ~RS2_PRODUCT_LINE_SW_ONLY;
            // But the above turned off the SW-only bits, so turn them back on again
            if( (device_mask & RS2_PRODUCT_LINE_SW_ONLY) || (requested_mask & RS2_PRODUCT_LINE_SW_ONLY) )
                mask |= RS2_PRODUCT_LINE_SW_ONLY;
        }
        if( p_device_mask )
            *p_device_mask = device_mask;
        return mask;
    }

    std::vector<std::shared_ptr<device_info>> context::query_devices( int requested_mask ) const
    {
        unsigned device_mask;
        auto const mask = calc_mask( _settings, unsigned( requested_mask ), &device_mask );
        platform::backend_device_group devices;
        if( ! ( mask & RS2_PRODUCT_LINE_SW_ONLY ) )
        {
            devices.uvc_devices = _backend->query_uvc_devices();
            devices.usb_devices = _backend->query_usb_devices();
            devices.hid_devices = _backend->query_hid_devices();
        }
        auto const list = create_devices( devices, _playback_devices, mask );
        LOG_INFO( "Found " << list.size() << " RealSense devices (mask 0x" << std::hex << mask << "= " << requested_mask
                           << " requested & " << device_mask << " from device-mask in settings)" << std::dec );
        for( auto & item : list )
            LOG_INFO( "... " << item->get_address() );
        return list;
    }

    std::vector< std::shared_ptr< device_info > >
    context::create_devices( platform::backend_device_group devices,
                             const std::map< std::string, std::weak_ptr< device_info > > & playback_devices,
                             int requested_mask ) const
    {
        std::vector<std::shared_ptr<device_info>> list;
        unsigned mask = calc_mask( _settings, requested_mask );

        auto t = const_cast<context*>(this); // While generally a bad idea, we need to provide mutable reference to the devices
        // to allow them to modify context later on
        auto ctx = t->shared_from_this();

        if( mask & RS2_PRODUCT_LINE_D400 && ! ( mask & RS2_PRODUCT_LINE_SW_ONLY ) )
        {
            auto d400_devices = d400_info::pick_d400_devices(ctx, devices);
            std::copy(begin(d400_devices), end(d400_devices), std::back_inserter(list));
        }

        if( mask & RS2_PRODUCT_LINE_D500 && ! ( mask & RS2_PRODUCT_LINE_SW_ONLY ) )
        {
            auto d500_devices = d500_info::pick_d500_devices( ctx, devices );
            std::copy( begin( d500_devices ), end( d500_devices ), std::back_inserter( list ) );
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
                    else if( dev->device_info().product_line == "D500" )
                    {
                        if( ! ( mask & RS2_PRODUCT_LINE_D500 ) )
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
        if( ( ( mask & RS2_PRODUCT_LINE_D400 ) || ( mask & RS2_PRODUCT_LINE_D500 ) )
            && ! ( mask & RS2_PRODUCT_LINE_SW_ONLY ) )
        {
            auto recovery_devices = fw_update_info::pick_recovery_devices(ctx, devices.usb_devices, mask);
            std::copy(begin(recovery_devices), end(recovery_devices), std::back_inserter(list));
        }

        if( mask & RS2_PRODUCT_LINE_NON_INTEL && ! ( mask & RS2_PRODUCT_LINE_SW_ONLY ) )
        {
            auto uvc_devices = platform_camera_info::pick_uvc_devices(ctx, devices.uvc_devices);
            std::copy(begin(uvc_devices), end(uvc_devices), std::back_inserter(list));
        }

        for (auto&& item : playback_devices)
        {
            if (auto dev = item.second.lock())
                list.push_back(dev);
        }

        return list;
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
            catch( std::exception const & e )
            {
                LOG_ERROR( "Exception thrown from user callback handler: " << e.what() );
            }
            catch (...)
            {
                LOG_ERROR("Exception thrown from user callback handler");
            }
        }
    }

    void context::start_device_watcher()
    {
        _device_watcher->start(
            [this]( platform::backend_device_group old, platform::backend_device_group curr )
            {
                auto old_list = create_devices( old, {}, RS2_PRODUCT_LINE_ANY );
                auto new_list = create_devices( curr, {}, RS2_PRODUCT_LINE_ANY );

                if( librealsense::list_changed< std::shared_ptr< device_info > >(
                        old_list,
                        new_list,
                        []( std::shared_ptr< device_info > first, std::shared_ptr< device_info > second )
                        { return first->is_same_as( second ); } ) )
                {
                    std::vector< rs2_device_info > rs2_devices_info_removed;

                    auto devices_info_removed = subtract_sets( old_list, new_list );
                    for( size_t i = 0; i < devices_info_removed.size(); i++ )
                    {
                        rs2_devices_info_removed.push_back( { shared_from_this(), devices_info_removed[i] } );
                        LOG_DEBUG( "Device disconnected: " << devices_info_removed[i]->get_address() );
                    }

                    std::vector< rs2_device_info > rs2_devices_info_added;
                    auto devices_info_added = subtract_sets( new_list, old_list );
                    for( size_t i = 0; i < devices_info_added.size(); i++ )
                    {
                        rs2_devices_info_added.push_back( { shared_from_this(), devices_info_added[i] } );
                        LOG_DEBUG( "Device connected: " << devices_info_added[i]->get_address() );
                    }

                    invoke_devices_changed_callbacks( rs2_devices_info_removed, rs2_devices_info_added );
                }
            } );
    }

#ifdef BUILD_WITH_DDS
    void context::start_dds_device_watcher()
    {
        _dds_watcher->on_device_added( [this]( std::shared_ptr< realdds::dds_device > const & dev ) {
            dev->wait_until_ready();  // make sure handshake is complete

            std::vector<rs2_device_info> rs2_device_info_added;
            std::vector<rs2_device_info> rs2_device_info_removed;
            auto info = std::make_shared< dds_device_info >( shared_from_this(), dev );
            rs2_device_info_added.push_back( { shared_from_this(), info } );
            invoke_devices_changed_callbacks( rs2_device_info_removed, rs2_device_info_added );
        } );
        _dds_watcher->on_device_removed( [this]( std::shared_ptr< realdds::dds_device > const & dev ) {
            std::vector<rs2_device_info> rs2_device_info_added;
            std::vector<rs2_device_info> rs2_device_info_removed;
            auto info = std::make_shared< dds_device_info >( shared_from_this(), dev );
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
        auto dinfo = std::make_shared< playback_device_info >( shared_from_this(), file );
        _playback_devices[file] = dinfo;

        std::vector< rs2_device_info > rs2_device_info_added{ { shared_from_this(), dinfo } };
        std::vector< rs2_device_info > rs2_device_info_removed;
        invoke_devices_changed_callbacks( rs2_device_info_removed, rs2_device_info_added );

        return dinfo;
    }

    void context::add_software_device(std::shared_ptr<device_info> dev)
    {
        auto address = dev->get_address();

        auto it = _playback_devices.find(address);
        if (it != _playback_devices.end() && it->second.lock())
            throw librealsense::invalid_value_exception( "File \"" + address + "\" already loaded to context" );
        _playback_devices[address] = dev;

        std::vector< rs2_device_info > rs2_device_info_added{ { shared_from_this(), dev } };
        std::vector< rs2_device_info > rs2_device_info_removed;
        invoke_devices_changed_callbacks( rs2_device_info_removed, rs2_device_info_added );
    }

    void context::remove_device(const std::string& file)
    {
        auto it = _playback_devices.find(file);
        if(it == _playback_devices.end() )
            return;
        auto dev_info = it->second.lock();
        _playback_devices.erase(it);

        if( dev_info )
        {
            std::vector< rs2_device_info > rs2_device_info_added;
            std::vector< rs2_device_info > rs2_device_info_removed{ { shared_from_this(), dev_info } };
            invoke_devices_changed_callbacks( rs2_device_info_removed, rs2_device_info_added );
        }
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
