// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

//#define _WINSOCKAPI_    // stops windows.h including winsock.h

#include <rsutils/os/adapter-watcher.h>
#include <rsutils/shared-ptr-singleton.h>
#include <rsutils/signal.h>
#include <rsutils/easylogging/easyloggingpp.h>

#ifdef WIN32

#include <windows.h>
//#include <winsock2.h>
#include <iphlpapi.h>
#pragma comment( lib, "iphlpapi.lib" )

#else
#endif


namespace rsutils {
namespace os {
namespace detail {


class adapter_watcher_singleton
{
    using public_signal = rsutils::public_signal< adapter_watcher_singleton >;

#ifdef WIN32
    HANDLE _done;
    std::thread _th;
#else
#endif

public:
    public_signal callbacks;

    adapter_watcher_singleton()
#ifdef WIN32
        : _done( CreateEvent(
            nullptr /*security attrs*/, TRUE /*manual reset*/, FALSE /*init unset*/, nullptr /*no name*/ ) )
        , _th(
              [this]()
              {
                  // We're going to wait on the following handles:
                  HANDLE handles[2];
                  DWORD const n_handles = sizeof( handles ) / sizeof( handles[0] );
                  handles[0] = _done;
                  // https://learn.microsoft.com/en-us/windows/win32/api/iphlpapi/nf-iphlpapi-notifyaddrchange
                  OVERLAPPED overlap = {0};
                  //overlap.hEvent = WSACreateEvent();
                  overlap.hEvent = CreateEvent( nullptr /*security attrs*/,
                                                FALSE /*auto reset*/,
                                                FALSE /*init unset*/,
                                                nullptr /*no name*/ );
                  handles[1] = overlap.hEvent;

                  LOG_DEBUG( "starting network adapter watcher" );

                  while( true )
                  {
                      HANDLE h_file = NULL;
                      NotifyAddrChange( &h_file, &overlap );
                      // If multiple objects become signaled, the function returns the index of the first handle in the
                      // array whose object was signaled:
                      if( WAIT_OBJECT_0
                          == WaitForMultipleObjects( n_handles, handles, FALSE /*wait-for-all*/, INFINITE ) )
                          break;  // we're done
                      LOG_DEBUG( "network adapter changes detected!" );
                      //WSAResetEvent( handles[1] );
                      callbacks.raise();
                  }

                  LOG_DEBUG( "exiting network adapter watcher" );

                  CancelIPChangeNotify( &overlap );
              } )
#else
        // https://manpages.ubuntu.com/manpages/oracular/en/man7/netlink.7.html
        // https://stackoverflow.com/questions/27008067/how-to-get-notified-about-network-interface-changes-with-netlist-and-rtmgrp-link
        On Linux, it is done opening and reading on a special kind of socket.Documentation here.Some nice examples here.
#endif
    {
        LOG_DEBUG( "network adapter watcher singleton is up" );
    }

    ~adapter_watcher_singleton()
    {
#ifdef WIN32
        if( _th.joinable() )
        {
            SetEvent( _done );
            _th.join();
        }
        CloseHandle( _done );
#else
#endif
    }
};

static rsutils::shared_ptr_singleton< detail::adapter_watcher_singleton > the_adapter_watcher;


}  // namespace detail


adapter_watcher::adapter_watcher( callback && cb )
    : _singleton( detail::the_adapter_watcher.instance() )  // keep it alive
    , _subscription( _singleton->callbacks.subscribe( std::move( cb ) ) )
{
    // As long as someone keeps a pointer to an adapter_watcher, the singleton will be kept alive and it will watch for
    // changes; as soon as all instances disappear, the singleton will disappear and the watch should stop.
}


}  // namespace os
}  // namespace rsutils
