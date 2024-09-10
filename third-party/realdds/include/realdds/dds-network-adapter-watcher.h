// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/os/network-adapter-watcher.h>
#include <set>
#include <string>


namespace realdds {


namespace detail {
class network_adapter_watcher_singleton;
}


// Watch for changes to network adapter IPs
// 
// Unlike rsutils::os::network_adapter_watcher, we only call the callbacks when actual changes to IPs are made, which
// can sometimes be seconds after adapter-change notifications are sent
// 
// All you have to do is create a watcher and keep a pointer to it to get notifications
//
class dds_network_adapter_watcher
{
    std::shared_ptr< detail::network_adapter_watcher_singleton > _singleton;
    rsutils::subscription _subscription;

public:
    using callback = std::function< void() >;

    dds_network_adapter_watcher( callback && );

    using ip_set = std::set< std::string >;
    static ip_set current_ips();
};


}  // namespace realdds
