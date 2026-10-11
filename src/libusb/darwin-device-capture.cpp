// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2026 RealSense, Inc. All Rights Reserved.

#include "darwin-device-capture.h"
#include "darwin-capture-wait.h"

#if defined(__APPLE__)

#include "types.h"

#include <chrono>
#include <condition_variable>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <utility>

namespace librealsense
{
    namespace platform
    {
        namespace detail
        {
            void wait_for_capture_registry_change(
                std::condition_variable& changed,
                std::unique_lock<std::mutex>& lock,
                const std::function<bool()>& predicate,
                const std::string& key,
                std::chrono::steady_clock::duration timeout)
            {
                if(!changed.wait_for(lock, timeout, predicate))
                    throw std::runtime_error(
                        "timed out waiting for Darwin USB capture state for device " + key);
            }
        }

        constexpr std::chrono::milliseconds darwin_capture_settle_time{500};
        constexpr std::chrono::seconds darwin_capture_wait_timeout{5};

        class darwin_device_capture_state
        {
        public:
            darwin_device_capture_state(
                std::shared_ptr<usb_context> context,
                libusb_device* device,
                uint8_t interface_number)
                : _context(std::move(context)),
                  _handle(nullptr),
                  _interface_number(interface_number),
                  _captured(false)
            {
                auto status = libusb_open(device, &_handle);
                if(status != LIBUSB_SUCCESS)
                    throw std::runtime_error("failed to open USB device for Darwin capture");

                status = libusb_set_auto_detach_kernel_driver(_handle, false);
                if(status != LIBUSB_SUCCESS)
                {
                    libusb_close(_handle);
                    _handle = nullptr;
                    throw std::runtime_error("failed to disable automatic Darwin USB release");
                }

                status = libusb_detach_kernel_driver(_handle, _interface_number);

                if(status == LIBUSB_SUCCESS)
                {
                    _captured = true;
                    // Capture causes a device re-enumeration. Wait until UVC class
                    // requests can be serviced reliably before exposing the lease.
                    std::this_thread::sleep_for(darwin_capture_settle_time);
                }
                else if(status != LIBUSB_ERROR_NOT_FOUND)
                {
                    libusb_close(_handle);
                    _handle = nullptr;
                    throw std::runtime_error("failed to capture USB device on Darwin");
                }
            }

            ~darwin_device_capture_state()
            {
                release();
            }

            void release() noexcept
            {
                if(!_handle)
                    return;

                if(_captured)
                {
                    auto const status = libusb_attach_kernel_driver(_handle, _interface_number);
                    if(status != LIBUSB_SUCCESS)
                        LOG_WARNING("failed to release Darwin USB device capture: " << status);
                    _captured = false;
                }

                libusb_close(_handle);
                _handle = nullptr;
            }

        private:
            std::shared_ptr<usb_context> _context;
            libusb_device_handle* _handle;
            uint8_t _interface_number;
            bool _captured;
        };

        enum class capture_phase
        {
            creating,
            active,
            releasing
        };

        struct capture_registry_entry
        {
            capture_phase phase = capture_phase::creating;
            std::shared_ptr<darwin_device_capture_state> state;
            size_t leases = 0;
            std::condition_variable changed;
        };

        class darwin_device_capture_registry
        {
        public:
            std::mutex mutex;
            std::map<std::string, std::shared_ptr<capture_registry_entry>> entries;
        };

        namespace
        {

            struct capture_reservation
            {
                std::shared_ptr<capture_registry_entry> entry;
                std::shared_ptr<darwin_device_capture_state> state;
            };

            std::shared_ptr<darwin_device_capture_registry> get_capture_registry()
            {
                static auto registry = std::make_shared<darwin_device_capture_registry>();
                return registry;
            }

            std::string get_physical_device_key(libusb_device* device)
            {
                constexpr int max_usb_depth = 8;
                uint8_t ports[max_usb_depth] = {};
                auto const port_count = libusb_get_port_numbers(device, ports, max_usb_depth);
                if(port_count < 0)
                    throw std::runtime_error("failed to read USB port path for Darwin capture");

                libusb_device_descriptor descriptor{};
                auto const descriptor_status = libusb_get_device_descriptor(device, &descriptor);
                if(descriptor_status != LIBUSB_SUCCESS)
                    throw std::runtime_error("failed to read USB descriptor for Darwin capture");

                std::ostringstream key;
                key << static_cast<int>(libusb_get_bus_number(device)) << ':';
                for(int i = 0; i < port_count; ++i)
                    key << (i ? "." : "") << static_cast<int>(ports[i]);
                key << ':' << std::hex << descriptor.idVendor << ':' << descriptor.idProduct;
                return key.str();
            }

            void remove_registry_entry(
                const std::shared_ptr<darwin_device_capture_registry>& registry,
                const std::string& key,
                const std::shared_ptr<capture_registry_entry>& entry) noexcept
            {
                {
                    std::lock_guard<std::mutex> lock(registry->mutex);
                    auto const existing = registry->entries.find(key);
                    if(existing != registry->entries.end() && existing->second == entry)
                        registry->entries.erase(existing);
                }
                entry->changed.notify_all();
            }

            void release_capture(
                const std::shared_ptr<darwin_device_capture_registry>& registry,
                const std::string& key,
                const std::shared_ptr<darwin_device_capture_state>& state) noexcept
            {
                std::shared_ptr<capture_registry_entry> entry;
                {
                    std::lock_guard<std::mutex> lock(registry->mutex);
                    auto const existing = registry->entries.find(key);
                    if(existing == registry->entries.end()
                        || existing->second->state != state
                        || !existing->second->leases)
                        return;

                    entry = existing->second;
                    --entry->leases;
                    if(entry->leases)
                        return;

                    entry->phase = capture_phase::releasing;
                }

                state->release();
                remove_registry_entry(registry, key, entry);
            }

            capture_reservation reserve_capture(
                const std::shared_ptr<darwin_device_capture_registry>& registry,
                const std::string& key)
            {
                for(;;)
                {
                    std::unique_lock<std::mutex> lock(registry->mutex);
                    auto const existing = registry->entries.find(key);
                    if(existing == registry->entries.end())
                    {
                        auto entry = std::make_shared<capture_registry_entry>();
                        registry->entries.emplace(key, entry);
                        return { entry, nullptr };
                    }

                    auto const entry = existing->second;
                    if(entry->phase == capture_phase::active)
                    {
                        ++entry->leases;
                        return { entry, entry->state };
                    }

                    detail::wait_for_capture_registry_change(
                        entry->changed,
                        lock,
                        [&]() {
                        auto const current = registry->entries.find(key);
                        return current == registry->entries.end()
                            || current->second != entry
                            || entry->phase == capture_phase::active;
                        },
                        key,
                        darwin_capture_wait_timeout);
                }
            }

            std::shared_ptr<darwin_device_capture_state> activate_capture(
                const std::shared_ptr<darwin_device_capture_registry>& registry,
                const std::string& key,
                const std::shared_ptr<capture_registry_entry>& entry,
                const std::shared_ptr<usb_context>& context,
                libusb_device* device,
                uint8_t interface_number)
            {
                std::shared_ptr<darwin_device_capture_state> state;
                try
                {
                    state = std::make_shared<darwin_device_capture_state>(
                        context, device, interface_number);
                }
                catch(...)
                {
                    remove_registry_entry(registry, key, entry);
                    throw;
                }

                {
                    std::lock_guard<std::mutex> lock(registry->mutex);
                    entry->state = state;
                    entry->leases = 1;
                    entry->phase = capture_phase::active;
                }
                entry->changed.notify_all();
                return state;
            }
        }

        std::shared_ptr<darwin_device_capture> darwin_device_capture::acquire(
            const std::shared_ptr<usb_context>& context,
            libusb_device* device,
            uint8_t interface_number)
        {
            auto const registry = get_capture_registry();
            auto const key = get_physical_device_key(device);
            auto const reservation = reserve_capture(registry, key);
            auto state = reservation.state;
            if(!state)
                state = activate_capture(
                    registry, key, reservation.entry, context, device, interface_number);

            try
            {
                return std::shared_ptr<darwin_device_capture>(
                    new darwin_device_capture(registry, key, state));
            }
            catch(...)
            {
                release_capture(registry, key, state);
                throw;
            }
        }

        darwin_device_capture::darwin_device_capture(
            std::shared_ptr<darwin_device_capture_registry> registry,
            std::string key,
            std::shared_ptr<darwin_device_capture_state> state)
            : _registry(std::move(registry)), _key(std::move(key)), _state(std::move(state))
        {
        }

        darwin_device_capture::~darwin_device_capture()
        {
            release_capture(_registry, _key, _state);
        }
    }
}

#endif
