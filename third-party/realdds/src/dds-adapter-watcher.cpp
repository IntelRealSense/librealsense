// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#include <realdds/dds-adapter-watcher.h>
#include <rsutils/shared-ptr-singleton.h>
#include <rsutils/signal.h>
#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/time/stopwatch.h>

#include <fastrtps/utils/IPFinder.h>
using eprosima::fastrtps::rtps::IPFinder;


namespace realdds {


using ip_set = dds_adapter_watcher::ip_set;


namespace detail {


class adapter_watcher_singleton
{
    std::shared_ptr< rsutils::os::adapter_watcher > _adapter_watcher;

    ip_set _ips;

    std::thread _th;
    rsutils::time::stopwatch _time_since_update;

    using public_signal
        = rsutils::public_signal< adapter_watcher_singleton, ip_set const & /*new*/, ip_set const & /*old*/ >;

public:
    public_signal callbacks;

    adapter_watcher_singleton()
        : _adapter_watcher( std::make_shared< rsutils::os::adapter_watcher >(
            [this]()
            {
                // Adapters have changed, but we won't see the effects for some time (several seconds)
                // But we don't want to wait here, so we start another thread
                _time_since_update.reset();
                if( ! _th.joinable() )
                {
                    _th = std::thread(
                        [this, weak = std::weak_ptr< rsutils::os::adapter_watcher >( _adapter_watcher )]
                        {
                            LOG_DEBUG( "waiting for IP changes" );
                            ip_set new_ips, old_ips;
                            while( auto strong = weak.lock() )
                            {
                                if( _time_since_update.get_elapsed() > std::chrono::seconds( 15 ) )
                                    break;
                                std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
                                new_ips.clear();
                                old_ips.clear();
                                update_ips( &new_ips, &old_ips );
                                if( new_ips.size() || old_ips.size() )
                                    callbacks.raise( new_ips, old_ips );
                            }
                            _th.detach();  // so it's not joinable
                            LOG_DEBUG( "done waiting for IP changes" );
                        } );
                }
            } ) )
    {
        LOG_DEBUG( "network adapter watcher singleton is up" );
        update_ips();
    }

    ~adapter_watcher_singleton()
    {
        _adapter_watcher.reset();  // signal the thread to finish
        if( _th.joinable() )
            _th.join();
    }

    void update_ips( ip_set * p_new_ips = nullptr, ip_set * p_old_ips = nullptr )
    {
        auto ips = dds_adapter_watcher::current_ips();

        auto first1 = ips.begin();
        auto const last1 = ips.end();
        auto first2 = _ips.begin();
        auto const last2 = _ips.end();

        while( first1 != last1 )
        {
            if( first2 == last2  || *first1 < *first2 )
            {
                // New IP
                LOG_DEBUG( "+adapter " << *first1 );
                if( p_new_ips )
                    p_new_ips->insert( *first1 );
                ++first1;
                continue;
            }

            if( *first2 < *first1 )
            {
                // Old IP
                LOG_DEBUG( "-adapter " << *first2 );
                if( p_old_ips )
                    p_old_ips->insert( *first2 );
                ++first2;
                continue;
            }

            ++first1;
            ++first2;
        }
        while( first2 != last2 )
        {
            // Old IP
            LOG_DEBUG( "-adapter " << *first2 );
            if( p_old_ips )
                p_old_ips->insert( *first2 );
            ++first2;
        }

        _ips = std::move( ips );
    }
};

static rsutils::shared_ptr_singleton< adapter_watcher_singleton > the_adapter_watcher;


}  // namespace detail


dds_adapter_watcher::dds_adapter_watcher( callback && cb )
    : _singleton( detail::the_adapter_watcher.instance() )  // keep it alive
    , _subscription( _singleton->callbacks.subscribe(
          [cb]( ip_set const & new_ips, ip_set const & old_ips ) { cb(); } ) )
{
    // As long as someone keeps a pointer to an adapter_watcher, the singleton will be kept alive and it will watch for
    // changes; as soon as all instances disappear, the singleton will disappear and the watch should stop.
}


/*static*/ ip_set dds_adapter_watcher::current_ips()
{
    std::vector< IPFinder::info_IP > local_interfaces;
    IPFinder::getIPs( &local_interfaces, false );
    ip_set ips;
    for( auto const & ip : local_interfaces )
    {
        if( ip.type != IPFinder::IP4 && ip.type != IPFinder::IP4_LOCAL )
            continue;
        ips.insert( ip.name );
    }
    return ips;
}


}  // namespace realdds
