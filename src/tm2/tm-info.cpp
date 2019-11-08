// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>
#include <thread>

#include "tm-info.h"
#include "tm-device.h"
#include "libusb.h"
#include "common/fw/target.h"

#ifdef WIN32
static const bool use_hotplug = false;
#else
static const bool use_hotplug = true;
#endif

namespace librealsense
{
    tm2_info::tm2_info(std::shared_ptr<context> ctx, void * _device_ptr)
        : device_info(ctx), device_ptr(_device_ptr)
    {
        LOG_DEBUG("tm2_info constructed, libusb_ref_device for " << this);
        libusb_ref_device((libusb_device *)device_ptr);
    }

    tm2_info::~tm2_info()
    {
        LOG_DEBUG("tm2_info destroyed for " << this);
        libusb_unref_device((libusb_device *)device_ptr);
    }

    std::shared_ptr<device_interface> tm2_info::create(std::shared_ptr<context> ctx,
        bool register_device_notifications) const
    {
        LOG_DEBUG("tm2_info::create " << this);
        return std::make_shared<tm2_device>(_ctx, get_device_data());
    }

    platform::backend_device_group tm2_info::get_device_data() const
    {
        LOG_DEBUG("tm2_info::get_device_data " << this);
        auto bdg = platform::backend_device_group();
        bdg.tm2_devices.push_back({ device_ptr });
        return bdg;
    }

    void tm2_info::boot_tm2_devices(std::shared_ptr<tm2_context> tm2_context)
    {
        LOG_DEBUG("tm2_info::boot_devices");
        libusb_device **dev_list;
        ssize_t n_devices = libusb_get_device_list(tm2_context::get(), &dev_list);
        if (n_devices < 0)  LOG_ERROR("Failed to get device list");

        LOG_DEBUG("Found " << n_devices << " devices");

        for (ssize_t i = 0; i < n_devices; i++) {
            libusb_device_descriptor desc;
            if (libusb_get_device_descriptor(dev_list[i], &desc) < 0) {
                LOG_ERROR("Failed to get device descriptor for device " << i << "... ignoring");
                continue;
            }
            std::stringstream s;
            s << std::hex << "vid: " << desc.idVendor << " pid: " << desc.idProduct;
            LOG_DEBUG("Deciding if we should boot " << s.str());
            // Not yet booted
            if(desc.idVendor == 0x03E7 && desc.idProduct == 0x2150) {
                LOG_INFO("Found a T265, booting it");

                libusb_device_handle *handle;
                if (libusb_open(dev_list[i], &handle) != 0)
                    LOG_ERROR("T265 boot: couldn't open device");

                if (libusb_claim_interface(handle, 0) != 0) {
                    libusb_close(handle);
                    LOG_ERROR("T265 boot: couldn't claim interface");
                }

                int completed = false;

                // Before booting the firmware, prepare to receive notification when the firmware has booted
                struct vph { uint16_t vendor_id, product_id; libusb_hotplug_callback_handle handle; } known[] = {
                    { 0x8087, 0x0AF3 }, // T260
                    { 0x8087, 0x0B37 }, // T261/T265
                };
                if (use_hotplug) {
                    for (auto &d : known)
                        libusb_hotplug_register_callback(tm2_context::get(),
                                                         LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED,
                                                         LIBUSB_HOTPLUG_NO_FLAGS/* LIBUSB_HOTPLUG_ENUMERATE*/,
                                                         d.vendor_id, d.product_id,
                                                         LIBUSB_HOTPLUG_MATCH_ANY,
                                                         [](libusb_context *ctx, libusb_device *device, libusb_hotplug_event event, void *completed_) -> int {
                                                             *(int*)completed_ = true;
                                                             return 0;
                                                         },
                                                         &completed,
                                                         &d.handle);
                }

                // transfer the firmware data
                int size;
                auto target_hex = fw_get_target(size);

                if(!target_hex)
                    LOG_ERROR("librealsense failed to get T265 FW resource");

                int bytes_written;
                if (!target_hex || libusb_bulk_transfer(handle, 0x1, const_cast<unsigned char*>(target_hex), size, &bytes_written, 10000) != 0 || bytes_written != size) {
                    libusb_release_interface(handle, 0);
                    libusb_close(handle);
                    if (use_hotplug) {
                        for (auto &d : known)
                            libusb_hotplug_deregister_callback(tm2_context::get(), d.handle);
                    }
                    LOG_ERROR("T265 boot: transfer failed");
                    continue;
                }
                LOG_INFO("Wrote " << bytes_written << "/" << size << " firmware bytes");

                libusb_release_interface(handle, 0);
                libusb_close(handle);

                if (use_hotplug) {
                    LOG_INFO("Waiting for firmware to show up");
                    // wait for the firmware to show up
                    auto boot_delay = std::chrono::seconds(2);
                    auto secs = std::chrono::duration_cast<std::chrono::seconds>(boot_delay);
                    struct timeval boot_tv;
                    boot_tv.tv_sec = secs.count();
                    boot_tv.tv_usec = std::chrono::duration_cast<std::chrono::microseconds>(boot_delay - secs).count();
                    while (!completed)
                        libusb_handle_events_timeout_completed(tm2_context::get(), &boot_tv, &completed);

                    // after either a timeout or success return
                    for (auto &d : known)
                        libusb_hotplug_deregister_callback(tm2_context::get(), d.handle);
                }

                if (completed)
                    LOG_INFO("T265 booted");
            }
        }
        libusb_free_device_list(dev_list, 1);
    }

    std::vector<std::shared_ptr<device_info>> tm2_info::pick_tm2_devices(
        std::shared_ptr<context> ctx,
        std::shared_ptr<tm2_context> tm2_context)
    {
        LOG_DEBUG("tm2::pick_tm2_devices");

        // Boot any unbooted devices
        boot_tm2_devices(tm2_context);

        // Enumerate devices and add tm2_info's for any T265s
        std::vector<std::shared_ptr<device_info>> results;

        libusb_device **dev_list;
        ssize_t n_devices = libusb_get_device_list(tm2_context::get(), &dev_list);
        if (n_devices < 0)
            LOG_ERROR("Failed to get device list");

        LOG_DEBUG("Pick from " << n_devices << " devices");
        for (ssize_t i = 0; i < n_devices; i++) {
            libusb_device_descriptor desc;
            if (libusb_get_device_descriptor(dev_list[i], &desc) < 0) {
                LOG_ERROR("Failed to get device descriptor for device " << i << "...ignoring");
                continue;
            }
            std::stringstream s;
            s << std::hex << "vid: " << desc.idVendor << " pid: " << desc.idProduct;
            LOG_DEBUG("Checking device " << s.str());
            if(desc.idVendor == 0x8087 && desc.idProduct == 0x0B37) {
                LOG_INFO("Found a booted T265, opening it");

                bool good = false;
                libusb_device_handle * handle;
                int e = 1;
                int inc;
                for(inc = 0; inc < 20; inc++) {
                    e = libusb_open(dev_list[i], &handle);
                    if(!e) break;
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                }
                LOG_INFO("Waited " << inc * 100 << " milliseconds to obtain device");
                if(!e) {
                    if (libusb_claim_interface(handle, 0) == LIBUSB_SUCCESS) {
                        libusb_release_interface(handle, 0);
                        good = true;
                    }
                    libusb_close(handle);
                }
                else
                    LOG_ERROR("Error opening T265 device");

                if(good) {
                    results.push_back(std::make_shared<tm2_info>(ctx, dev_list[i]));
                }
            }
        }


        libusb_free_device_list(dev_list, 1);

        return results;
    }
}
