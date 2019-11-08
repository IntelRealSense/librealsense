// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include <memory>
#include <vector>
#include <thread>
#include <atomic>
#include "context.h"
#include "libusb.h"

namespace librealsense
{
    class tm2_info;

    class tm2_context
    {
    public:
        tm2_context(context* ctx) : _ctx(ctx) { _is_disposed = false;}
        ~tm2_context() { _is_disposed = true; };
        static libusb_context *get() {
            if (m_ctx == nullptr) {
                LOG_DEBUG("tm2_context libusb_init");
                libusb_init(&m_ctx);
#if LIBUSB_API_VERSION >= 0x01000106
                libusb_set_option(m_ctx, LIBUSB_OPTION_LOG_LEVEL, LIBUSB_LOG_LEVEL_NONE);
#else
                libusb_set_debug(m_ctx, LIBUSB_LOG_LEVEL_NONE);
#endif
            }
            return m_ctx;
        }
        signal<tm2_context, std::shared_ptr<tm2_info>, std::shared_ptr<tm2_info>> on_device_changed;
    private:
        static libusb_context *m_ctx;
        context* _ctx;
        std::atomic_bool _is_disposed;
    };
}
