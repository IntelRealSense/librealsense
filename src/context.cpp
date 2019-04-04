// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#ifdef _MSC_VER
#if (_MSC_VER <= 1800) // constexpr is not supported in MSVC2013
#error( "Librealsense requires MSVC2015 or later to build. Compilation will be aborted" )
#endif
#endif

#include <array>
#include <chrono>
#include "l500/l500-depth.h"
#include "ivcam/sr300.h"
#include "ds5/ds5-factory.h"
#include "l500/l500-factory.h"
#include "ds5/ds5-timestamp.h"
#include "backend.h"
#include "mock/recorder.h"
#include <media/ros/ros_reader.h>
#include "types.h"
#include "stream.h"
#include "environment.h"
#include "context.h"

#ifdef WITH_TRACKING
#include "tm2/tm-context.h"
#include "tm2/tm-info.h"
#endif

template<unsigned... Is> struct seq{};
template<unsigned N, unsigned... Is>
struct gen_seq : gen_seq<N-1, N-1, Is...>{};
template<unsigned... Is>
struct gen_seq<0, Is...> : seq<Is...>{};

template<unsigned N1, unsigned... I1, unsigned N2, unsigned... I2>
constexpr std::array<char const, N1+N2-1> concat(char const (&a1)[N1], char const (&a2)[N2], seq<I1...>, seq<I2...>){
  return {{ a1[I1]..., a2[I2]... }};
}

template<unsigned N1, unsigned N2>
constexpr std::array<char const, N1+N2-1> concat(char const (&a1)[N1], char const (&a2)[N2]){
  return concat(a1, a2, gen_seq<N1-1>{}, gen_seq<N2>{});
}

// The string is used to retrieve the version embedded into .so file on Linux
constexpr auto rs2_api_version = concat("VERSION: ",RS2_API_VERSION_STR);

template<>
bool contains(const std::shared_ptr<librealsense::device_info>& first,
              const std::shared_ptr<librealsense::device_info>& second)
{
    auto first_data = first->get_device_data();
    auto second_data = second->get_device_data();

    for (auto&& uvc : first_data.uvc_devices)
    {
        if (std::find(second_data.uvc_devices.begin(),
            second_data.uvc_devices.end(), uvc) ==
            second_data.uvc_devices.end())
            return false;
    }
    for (auto&& usb : first_data.usb_devices)
    {
        if (std::find(second_data.usb_devices.begin(),
            second_data.usb_devices.end(), usb) ==
            second_data.usb_devices.end())
            return false;
    }
    for (auto&& hid : first_data.hid_devices)
    {
        if (std::find(second_data.hid_devices.begin(),
            second_data.hid_devices.end(), hid) ==
            second_data.hid_devices.end())
            return false;
    }
    for (auto&& pd : first_data.playback_devices)
    {
        if (std::find(second_data.playback_devices.begin(),
            second_data.playback_devices.end(), pd) ==
            second_data.playback_devices.end())
            return false;
    }
    for (auto&& tm : first_data.tm2_devices)
    {
        if (std::find(second_data.tm2_devices.begin(),
            second_data.tm2_devices.end(), tm) ==
            second_data.tm2_devices.end())
            return false;
    }
    return true;
}

namespace librealsense
{
    context::context(backend_type type,
                     const char* filename,
                     const char* section,
                     rs2_recording_mode mode,
                     std::string min_api_version)
        : _devices_changed_callback(nullptr, [](rs2_devices_changed_callback*){})
    {
        LOG_DEBUG("Librealsense " << std::string(std::begin(rs2_api_version),std::end(rs2_api_version)));

        switch(type)
        {
        case backend_type::standard:
            _backend = platform::create_backend();
#if WITH_TRACKING
            _tm2_context = std::make_shared<tm2_context>(this);
            _tm2_context->on_device_changed += [this](std::shared_ptr<tm2_info> removed, std::shared_ptr<tm2_info> added)-> void
            {
                std::vector<rs2_device_info> rs2_devices_info_added;
                std::vector<rs2_device_info> rs2_devices_info_removed;
                if (removed)
                    rs2_devices_info_removed.push_back({ shared_from_this(), removed });
                if (added)
                    rs2_devices_info_added.push_back({ shared_from_this(), added });
                raise_devices_changed(rs2_devices_info_removed, rs2_devices_info_added);
            };
#endif
            break;
        case backend_type::record:
            _backend = std::make_shared<platform::record_backend>(platform::create_backend(), filename, section, mode);
            break;
        case backend_type::playback:
            _backend = std::make_shared<platform::playback_backend>(filename, section, min_api_version);
            break;
            // Strongly-typed enum. Default is redundant
        }

       environment::get_instance().set_time_service(_backend->create_time_service());

       _device_watcher = _backend->create_device_watcher();
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

    class platform_camera_sensor : public uvc_sensor
    {
    public:
        platform_camera_sensor(const std::shared_ptr<context>&,
            device* owner,
            std::shared_ptr<platform::uvc_device> uvc_device,
            std::unique_ptr<frame_timestamp_reader> timestamp_reader)
            : uvc_sensor("RGB Camera", uvc_device, move(timestamp_reader), owner),
              _default_stream(new stream(RS2_STREAM_COLOR))
        {
        }

        stream_profiles init_stream_profiles() override
        {
            auto lock = environment::get_instance().get_extrinsics_graph().lock();

            auto results = uvc_sensor::init_stream_profiles();

            for (auto p : results)
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
            auto color_ep = std::make_shared<platform_camera_sensor>(ctx, this,
                                                                     std::make_shared<platform::multi_pins_uvc_device>(devs),
                                                                     std::unique_ptr<ds5_timestamp_reader>(new ds5_timestamp_reader(environment::get_instance().get_time_service())));
            add_sensor(color_ep);

            register_info(RS2_CAMERA_INFO_NAME, "Platform Camera");
            std::string pid_str(to_string() << std::setfill('0') << std::setw(4) << std::hex << uvc_infos.front().pid);
            std::transform(pid_str.begin(), pid_str.end(), pid_str.begin(), ::toupper);

            register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, uvc_infos.front().unique_id);
            register_info(RS2_CAMERA_INFO_PHYSICAL_PORT, uvc_infos.front().device_path);
            register_info(RS2_CAMERA_INFO_PRODUCT_ID, pid_str);

            color_ep->register_pixel_format(pf_yuy2);
            color_ep->register_pixel_format(pf_yuyv);

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
        _device_watcher->stop(); //ensure that the device watcher will stop before the _devices_changed_callback will be deleted
    }

    std::vector<std::shared_ptr<device_info>> context::query_devices(int mask) const
    {

        platform::backend_device_group devices(_backend->query_uvc_devices(), _backend->query_usb_devices(), _backend->query_hid_devices());
#ifdef WITH_TRACKING
        if (_tm2_context) _tm2_context->create_manager();
#endif
        return create_devices(devices, _playback_devices, mask);
    }

    std::vector<std::shared_ptr<device_info>> context::create_devices(platform::backend_device_group devices,
                                                                      const std::map<std::string, std::weak_ptr<device_info>>& playback_devices,
                                                                      int mask) const
    {
        std::vector<std::shared_ptr<device_info>> list;

        auto t = const_cast<context*>(this); // While generally a bad idea, we need to provide mutable reference to the devices
        // to allow them to modify context later on
        auto ctx = t->shared_from_this();

        if (mask & RS2_PRODUCT_LINE_D400)
        {
            auto ds5_devices = ds5_info::pick_ds5_devices(ctx, devices);
            std::copy(begin(ds5_devices), end(ds5_devices), std::back_inserter(list));
        }

        auto l500_devices = l500_info::pick_l500_devices(ctx, devices.uvc_devices, devices.usb_devices);
        std::copy(begin(l500_devices), end(l500_devices), std::back_inserter(list));

        if (mask & RS2_PRODUCT_LINE_SR300)
        {
            auto sr300_devices = sr300_info::pick_sr300_devices(ctx, devices.uvc_devices, devices.usb_devices);
            std::copy(begin(sr300_devices), end(sr300_devices), std::back_inserter(list));
        }

#ifdef WITH_TRACKING
        if (_tm2_context)
        {
            auto tm2_devices = tm2_info::pick_tm2_devices(ctx, _tm2_context->get_manager(), _tm2_context->query_devices());
            std::copy(begin(tm2_devices), end(tm2_devices), std::back_inserter(list));
        }
#endif

        auto recovery_devices = recovery_info::pick_recovery_devices(ctx, devices.usb_devices);
        std::copy(begin(recovery_devices), end(recovery_devices), std::back_inserter(list));

        if (mask & RS2_PRODUCT_LINE_NON_INTEL)
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

            std::map<uint64_t, devices_changed_callback_ptr> devices_changed_callbacks;
            {
                std::lock_guard<std::mutex> lock(_devices_changed_callbacks_mtx);
                devices_changed_callbacks = _devices_changed_callbacks;
            }

            for (auto& kvp : devices_changed_callbacks)
            {
                try
                {
                    kvp.second->on_devices_changed(new rs2_device_list({ shared_from_this(), rs2_devices_info_removed }),
                                                   new rs2_device_list({ shared_from_this(), rs2_devices_info_added }));
                }
                catch (...)
                {
                    LOG_ERROR("Exception thrown from user callback handler");
                }
            }

            raise_devices_changed(rs2_devices_info_removed, rs2_devices_info_added);
        }
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
    uint64_t context::register_internal_device_callback(devices_changed_callback_ptr callback)
    {
        std::lock_guard<std::mutex> lock(_devices_changed_callbacks_mtx);
        auto callback_id = unique_id::generate_id();
        _devices_changed_callbacks.insert(std::make_pair(callback_id, std::move(callback)));

        return callback_id;
    }

    void context::unregister_internal_device_callback(uint64_t cb_id)
    {
        std::lock_guard<std::mutex> lock(_devices_changed_callbacks_mtx);
        _devices_changed_callbacks.erase(cb_id);
    }

    void context::set_devices_changed_callback(devices_changed_callback_ptr callback)
    {
        _device_watcher->stop();

        _devices_changed_callback = std::move(callback);
        _device_watcher->start([this](platform::backend_device_group old, platform::backend_device_group curr)
        {
            on_device_changed(old, curr, _playback_devices, _playback_devices);
        });
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
                if (hid.unique_id != "")
                {
                    std::stringstream(hid.vid) >> std::hex >> vid;
                    std::stringstream(hid.pid) >> std::hex >> pid;

                    if ((hid.unique_id == unique_id) || // Linux check
                        ((hid.unique_id == "*") && (hid.serial_number == device_serial))) // Windows check
                    {
                        hid_group.push_back(hid);
                    }
                }
            }
            results.push_back(std::make_pair(dev, hid_group));
        }
        return results;
    }

    std::shared_ptr<device_interface> context::add_device(const std::string& file)
    {
        auto it = _playback_devices.find(file);
        if (it != _playback_devices.end() && it->second.lock())
        {
            //Already exists
            throw librealsense::invalid_value_exception(to_string() << "File \"" << file << "\" already loaded to context");
        }
        auto playback_dev = std::make_shared<playback_device>(shared_from_this(), std::make_shared<ros_reader>(file, shared_from_this()));
        auto dinfo = std::make_shared<playback_device_info>(playback_dev);
        auto prev_playback_devices = _playback_devices;
        _playback_devices[file] = dinfo;
        on_device_changed({}, {}, prev_playback_devices, _playback_devices);
        return playback_dev;
    }
    
    void context::add_software_device(std::shared_ptr<device_info> dev)
    {
        auto file = dev->get_device_data().playback_devices.front().file_path;
        
        auto it = _playback_devices.find(file);
        if (it != _playback_devices.end() && it->second.lock())
        {
            //Already exists
            throw librealsense::invalid_value_exception(to_string() << "File \"" << file << "\" already loaded to context");
        }
        auto prev_playback_devices = _playback_devices;
        _playback_devices[file] = dev;
        on_device_changed({}, {}, prev_playback_devices, _playback_devices);
    }

    void context::remove_device(const std::string& file)
    {
        auto it = _playback_devices.find(file);
        if (!it->second.lock() || it == _playback_devices.end())
        {
            //Not found
            return;
        }
        auto prev_playback_devices =_playback_devices;
        _playback_devices.erase(it);
        on_device_changed({},{}, prev_playback_devices, _playback_devices);
    }

#if WITH_TRACKING
    void context::unload_tracking_module()
    {
        _tm2_context.reset();
    }
#endif

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
