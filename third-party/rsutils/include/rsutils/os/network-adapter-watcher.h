// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/subscription.h>


namespace rsutils {
namespace os {
namespace detail {
class network_adapter_watcher_singleton;
}


// Watch for OS changes to network adapters
// 
// We don't get any parameters; just that something changed.
// 
// To use, just create a watcher and keep a pointer to it to get notifications. Single callback per instance.
//
class network_adapter_watcher
{
    std::shared_ptr< detail::network_adapter_watcher_singleton > _singleton;
    subscription _subscription;

public:
    using callback = std::function< void() >;

    network_adapter_watcher( callback && );
};


}  // namespace os
}  // namespace rsutils
