// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "device-watcher-libusb.h"

#include "libusb.h"
#include "context-libusb.h"

using namespace librealsense;
using namespace librealsense::platform;

device_watcher_libusb::device_watcher_libusb(const platform::backend* backend_ref) : _backend(backend_ref)
{
    LOG_DEBUG("Making a libusb device watcher");
    _usb_context = get_usb_context();
    _prev_group = {_backend->query_uvc_devices(),
                   _backend->query_usb_devices(),
                   _backend->query_hid_devices() };
}

void device_watcher_libusb::update_devices()
{
    LOG_DEBUG("Updating devices");
    platform::backend_device_group current_group = {_backend->query_uvc_devices(),
                                                    _backend->query_usb_devices(),
                                                    _backend->query_hid_devices() };
    std::lock_guard<std::mutex> lk(_mutex);
    _callback(_prev_group, current_group);
    _prev_group = current_group;
}

void device_watcher_libusb::watch_thread()
{
    int completed = 0;
    struct timeval watch_tv;
    watch_tv.tv_sec = 0;
    watch_tv.tv_usec = 10000;
    while (!completed && !_watch_thread_stop) {
        libusb_handle_events_timeout_completed(_usb_context->get(), &watch_tv, &completed);
        if(update_next) {
            update_devices();
            update_next = false;
        }
    }
}

void device_watcher_libusb::start(librealsense::platform::device_changed_callback callback)
{
    std::lock_guard<std::mutex> lk(_mutex);
    _callback = std::move(callback);

    bool hotplug_started = false;
    update_next = false;
    if(_callback && libusb_has_capability(LIBUSB_CAP_HAS_HOTPLUG)) {

        int rc = libusb_hotplug_register_callback(_usb_context->get(),
                                        libusb_hotplug_event(LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                                        LIBUSB_HOTPLUG_NO_FLAGS,
                                        LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                                        [](libusb_context *, libusb_device * d, libusb_hotplug_event event, void * user_data) -> int {
                                            std::string event_str = "libusb device ";
                                            if(event == LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED) event_str += "arrived";
                                            else if(event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT) event_str += "left";

                                            int addr = libusb_get_device_address(d);
                                            
                                            libusb_device_descriptor desc;
                                            int rc = libusb_get_device_descriptor(d, &desc);
                                            if(rc)
                                                LOG_ERROR("Error getting device descriptor " << libusb_error_name(rc));
                                            else
                                                LOG_INFO(event_str << " address: " << addr << " VID: " << hexify(desc.idVendor) << " PID: " << hexify(desc.idProduct));

                                            // No querying or talking to devices in hotplug callback https://github.com/libusb/libusb/issues/408
                                            ((device_watcher_libusb *)user_data)->update_next = true;
                                            return 0; // Continue processing hotplug events
                                        },
                                        this,
                                        &callback_handle);

        if(rc != LIBUSB_SUCCESS)
            LOG_WARNING("Error registering hotplug " << libusb_error_name(rc));
        else
            hotplug_started = true;

        _watch_thread_stop = false;
        _watch_thread = std::thread(&device_watcher_libusb::watch_thread, this);
    }

    if(!hotplug_started) {
        LOG_WARNING("Failed to start USB hotplug, falling back to polling");
        _fallback_polling = std::make_shared<polling_device_watcher>(_backend);
        _fallback_polling->start(callback);
    }
}

void device_watcher_libusb::stop()
{
    LOG_DEBUG("Stop device watcher");
    std::lock_guard<std::mutex> lk(_mutex);
    if(_fallback_polling) {
        _fallback_polling->stop();
        _fallback_polling.reset();
        return;
    }

    if(_callback) {
        libusb_hotplug_deregister_callback(_usb_context->get(), callback_handle);
        _watch_thread_stop = true;
        _watch_thread.join();
    }
    _callback = nullptr;
}
