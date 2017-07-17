// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#ifdef _MSC_VER
#if (_MSC_VER <= 1800) // constexpr is not supported in MSVC2013
#error( "Librealsense requires MSVC2015 or later to build. Compilation will be aborted" )
#endif
#endif

#include <array>

#include "ivcam/sr300.h"
#include "ds5/ds5-factory.h"
#include "ds5/ds5-timestamp.h"
#include "backend.h"
#include "mock/recorder.h"
#include <chrono>
#include "types.h"
#include "context.h"

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

namespace librealsense
{
    context::context(backend_type type,
                     const char* filename,
                     const char* section,
                     rs2_recording_mode mode)
        : _devices_changed_callback(nullptr , [](rs2_devices_changed_callback*){})
    {

        LOG_DEBUG("Librealsense " << std::string(std::begin(rs2_api_version),std::end(rs2_api_version)));

        switch(type)
        {
        case backend_type::standard:
            _backend = platform::create_backend();
            break;
        case backend_type::record:
            _backend = std::make_shared<platform::record_backend>(platform::create_backend(), filename, section, mode);
            break;
        case backend_type::playback:
            _backend = std::make_shared<platform::playback_backend>(filename, section);

            break;
        default: throw invalid_value_exception(to_string() << "Undefined backend type " << static_cast<int>(type));
        }

       _ts = _backend->create_time_service();

       _device_watcher = _backend->create_device_watcher();
    }

    
    class recovery_info : public device_info
    {
    public:
        std::shared_ptr<device_interface> create(const platform::backend& /*backend*/) const override
        {
            throw unrecoverable_exception(RECOVERY_MESSAGE,
                RS2_EXCEPTION_TYPE_DEVICE_IN_RECOVERY_MODE);
        }

        static bool is_recovery_pid(uint16_t pid)
        {
            return pid == 0x0ADB || pid == 0x0AB3;
        }

        static std::vector<std::shared_ptr<device_info>> pick_recovery_devices(
            const std::shared_ptr<platform::backend>& backend,
            const std::vector<platform::usb_device_info>& usb_devices)
        {
            std::vector<std::shared_ptr<device_info>> list;
            for (auto&& usb : usb_devices)
            {
                if (is_recovery_pid(usb.pid))
                {
                    list.push_back(std::make_shared<recovery_info>(backend, usb));
                }
            }
            return list;
        }

        explicit recovery_info(std::shared_ptr<platform::backend> backend, platform::usb_device_info dfu)
            : device_info(backend), _dfu(std::move(dfu)) {}

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
        std::shared_ptr<device_interface> create(const platform::backend& /*backend*/) const override;

        static std::vector<std::shared_ptr<device_info>> pick_uvc_devices(
            const std::shared_ptr<platform::backend>& backend,
            const std::vector<platform::uvc_device_info>& uvc_devices)
        {
            std::vector<std::shared_ptr<device_info>> list;
            for (auto&& uvc : uvc_devices)
            {
                list.push_back(std::make_shared<platform_camera_info>(backend, uvc));
            }
            return list;
        }

        explicit platform_camera_info(std::shared_ptr<platform::backend> backend,
                                      platform::uvc_device_info uvc)
            : device_info(backend), _uvc(std::move(uvc)) {}

        platform::backend_device_group get_device_data() const override
        {
            return platform::backend_device_group();
        }

    private:
        platform::uvc_device_info _uvc;
    };

    class platform_camera : public device
    {
    public:
        platform_camera(std::shared_ptr<platform::uvc_device> uvc, std::shared_ptr<platform::time_service> ts)
        {
            auto color_ep = std::make_shared<uvc_sensor>("RGB Camera", uvc, std::unique_ptr<ds5_timestamp_reader>(new ds5_timestamp_reader(ts)), ts, this);
            add_sensor(color_ep);

            register_info(RS2_CAMERA_INFO_NAME, "Platform Camera");

            color_ep->register_pixel_format(pf_yuy2);
            color_ep->register_pixel_format(pf_yuyv);

            color_ep->register_pu(RS2_OPTION_BACKLIGHT_COMPENSATION);
            color_ep->register_pu(RS2_OPTION_BRIGHTNESS);
            color_ep->register_pu(RS2_OPTION_CONTRAST);
            color_ep->register_pu(RS2_OPTION_EXPOSURE);
            color_ep->register_pu(RS2_OPTION_GAMMA);
            color_ep->register_pu(RS2_OPTION_HUE);
            color_ep->register_pu(RS2_OPTION_SATURATION);
            color_ep->register_pu(RS2_OPTION_SHARPNESS);
            color_ep->register_pu(RS2_OPTION_WHITE_BALANCE);
            color_ep->register_pu(RS2_OPTION_ENABLE_AUTO_EXPOSURE);
            color_ep->register_pu(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);

            color_ep->set_pose(lazy<pose>([](){pose p = {{ { 1,0,0 },{ 0,1,0 },{ 0,0,1 } },{ 0,0,0 }}; return p; }));
        }

        virtual rs2_intrinsics get_intrinsics(unsigned int subdevice, const stream_profile& profile) const 
        {
            return rs2_intrinsics {};
        }
    };

    std::shared_ptr<device_interface> platform_camera_info::create(const platform::backend& backend) const
    {
        return std::make_shared<platform_camera>(backend.create_uvc_device(_uvc), backend.create_time_service());
    }

    context::~context()
    {
        _device_watcher->stop(); //ensure that the device watcher will stop before the _devices_changed_callback will be deleted
    }

    std::vector<std::shared_ptr<device_info>> context::query_devices() const
    {

        platform::backend_device_group devices(_backend->query_uvc_devices(), _backend->query_usb_devices(), _backend->query_hid_devices());

        return create_devices(devices);
    }

    std::vector<std::shared_ptr<device_info>> context::create_devices(platform::backend_device_group devices) const
    {
        std::vector<std::shared_ptr<device_info>> list;

        auto ds5_devices = ds5_info::pick_ds5_devices(_backend, devices);
        std::copy(begin(ds5_devices), end(ds5_devices), std::back_inserter(list));

        auto sr300_devices = sr300_info::pick_sr300_devices(_backend, devices.uvc_devices, devices.usb_devices);
        std::copy(begin(sr300_devices), end(sr300_devices), std::back_inserter(list));

        auto recovery_devices = recovery_info::pick_recovery_devices(_backend, devices.usb_devices);
        std::copy(begin(recovery_devices), end(recovery_devices), std::back_inserter(list));

        auto uvc_devices = platform_camera_info::pick_uvc_devices(_backend, devices.uvc_devices);
        std::copy(begin(uvc_devices), end(uvc_devices), std::back_inserter(list));

        return list;
    }


    void context::register_same_extrinsics(const stream_interface& from, const stream_interface& to)
    {
        auto id = std::make_shared<lazy<rs2_extrinsics>>([]()
        {
            return identity_matrix();
        });
        register_extrinsics(from, to, id);
    }

    void context::register_extrinsics(const stream_interface& from, const stream_interface& to, std::weak_ptr<lazy<rs2_extrinsics>> extr)
    {
        std::lock_guard<std::mutex> lock(_streams_mutex);

        // First, trim any dead stream, to make sure we are not keep gaining memory
        cleanup_extrinsics();

        // Second, register new extrinsics
        auto from_idx = find_stream_profile(from);
        // If this is a new index, add it to the map preemptively,
        // This way find on to will be able to return another new index
        if (_extrinsics.find(from_idx) == _extrinsics.end())
            _extrinsics.insert({from_idx, {}});

        auto to_idx = find_stream_profile(to);

        _extrinsics[from_idx][to_idx] = extr;
        _extrinsics[to_idx][from_idx] = std::make_shared<lazy<rs2_extrinsics>>([extr]()
        {
            auto sp = extr.lock();
            if (sp)
            {
                return inverse(sp->operator*());
            }
            // This most definetly not supposed to ever happen
            throw std::runtime_error("Could not calculate inverse extrinsics because the forward extrinsics are no longer available!");
        });
    }

    bool context::try_fetch_extrinsics(const stream_interface& from, const stream_interface& to, rs2_extrinsics* extr)
    {
        std::lock_guard<std::mutex> lock(_streams_mutex);
        cleanup_extrinsics();
        auto from_idx = find_stream_profile(from);
        auto to_idx = find_stream_profile(to);

        if (from_idx == to_idx)
        {
            *extr = identity_matrix();
            return true;
        }

        std::vector<int> visited;
        return try_fetch_extrinsics(from_idx, to_idx, visited, extr);
    }

    void context::on_device_changed(platform::backend_device_group old, platform::backend_device_group curr)
    {
        auto old_list = create_devices(old);
        auto new_list = create_devices(curr);

        if (librealsense::list_changed<std::shared_ptr<device_info>>(old_list, new_list, [](std::shared_ptr<device_info> first, std::shared_ptr<device_info> second) {return *first == *second; }))
        {

            std::vector<rs2_device_info> rs2_devices_info_added;
            std::vector<rs2_device_info> rs2_devices_info_removed;

            auto devices_info_removed = subtract_sets(old_list, new_list);

            for (auto i=0; i<devices_info_removed.size(); i++)
            {
                rs2_devices_info_removed.push_back({ shared_from_this(), devices_info_removed[i] });
                LOG_DEBUG("\nDevice disconnected:\n\n" << std::string(devices_info_removed[i]->get_device_data()));
            }

            auto devices_info_added = subtract_sets(new_list, old_list);
            for (auto i = 0; i<devices_info_added.size(); i++)
            {
                rs2_devices_info_added.push_back({ shared_from_this(), devices_info_added[i] });
                LOG_DEBUG("\nDevice sconnected:\n\n" << std::string(devices_info_added[i]->get_device_data()));
            }

            _devices_changed_callback->on_devices_changed(  new rs2_device_list({ shared_from_this(), rs2_devices_info_removed }),
                                                            new rs2_device_list({ shared_from_this(), rs2_devices_info_added }));
        }
    }

    int context::find_stream_profile(const stream_interface& p) const
    {
        auto sp = p.shared_from_this();
        auto max = 0;
        for (auto&& kvp : _streams)
        {
            max = std::max(max, kvp.first);
            if (kvp.second.lock().get() == sp.get()) return kvp.first;
        }
        return max + 1;
    }

    std::shared_ptr<lazy<rs2_extrinsics>> context::fetch_edge(int from, int to)
    {
        auto it = _extrinsics.find(from);
        if (it != _extrinsics.end())
        {
            auto it2 = it->second.find(to);
            if (it2 != it->second.end())
            {
                return it2->second.lock();
            }
        }

        return nullptr;
    }

    bool context::try_fetch_extrinsics(int from, int to, std::vector<int>& visited, rs2_extrinsics* extr)
    {
        auto it = _extrinsics.find(from);
        if (it != _extrinsics.end())
        {
            auto back_edge = fetch_edge(to, from);
            auto fwd_edge = fetch_edge(from, to);

            // Make sure both parts of the edge are still available
            if (fwd_edge.get() && back_edge.get())
            {
                *extr = fwd_edge->operator*(); // Evaluate the expression
                return true;
            }
            else
            {
                visited.push_back(from);
                for (auto&& kvp : it->second)
                {
                    auto new_from = kvp.first;
                    auto way = kvp.second;

                    // Lock down the edge in both directions to ensure we can evaluate the extrinsics
                    back_edge = fetch_edge(new_from, from);
                    fwd_edge = fetch_edge(from, new_from);

                    if (back_edge.get() && fwd_edge.get() &&
                        try_fetch_extrinsics(new_from, to, visited, extr))
                    {
                        auto pose = to_pose(fwd_edge->operator*()) * to_pose(*extr);
                        *extr = from_pose(pose);
                        return true;
                    }
                }
            }
        } // If there are no extrinsics from from, there are none to it, so it is completely isolated
        return false;
    }

    void context::cleanup_extrinsics()
    {
        auto counter = 0;
        auto dead_counter = 0;
        for (auto&& kvp : _streams)
        {
            if (!kvp.second.lock())
            {
                auto dead_id = kvp.first;
                for (auto&& edge : _extrinsics[dead_id])
                {
                    // First, delete any extrinsics going into the stream
                    _extrinsics[edge.first].erase(dead_id);
                    counter += 2;
                }
                // Then delete all extrinsics going out of this stream
                _extrinsics.erase(dead_id);
                dead_counter++;
            }
        }
        if (dead_counter)
        LOG_INFO("Found " << dead_counter << " unreachable streams, " << counter << " extrinsics deleted");
    }

    double context::get_time()
    {
        return _ts->get_time();
    }

    void context::set_devices_changed_callback(devices_changed_callback_ptr callback)
    {
        _device_watcher->stop();

        _devices_changed_callback = std::move(callback);
        _device_watcher->start([this](platform::backend_device_group old, platform::backend_device_group curr)
        {
            on_device_changed(old, curr);
        });
    }

        static std::vector<platform::uvc_device_info> filter_by_product(const std::vector<platform::uvc_device_info>& devices, const std::set<uint16_t>& pid_list)
    {
        std::vector<platform::uvc_device_info> result;
        for (auto&& info : devices)
        {
            if (pid_list.count(info.pid))
                result.push_back(info);
        }
        return result;
    }

    std::vector<std::pair<std::vector<platform::uvc_device_info>, std::vector<platform::hid_device_info>>> group_devices_and_hids_by_unique_id(const std::vector<std::vector<platform::uvc_device_info>>& devices,
        const std::vector<platform::hid_device_info>& hids)
    {
        std::vector<std::pair<std::vector<platform::uvc_device_info>, std::vector<platform::hid_device_info>>> results;
        for (auto&& dev : devices)
        {
            std::vector<platform::hid_device_info> hid_group;
            auto unique_id = dev.front().unique_id;
            for (auto&& hid : hids)
            {
                if (hid.unique_id == unique_id || hid.unique_id == "*")
                    hid_group.push_back(hid);
            }
            results.push_back(std::make_pair(dev, hid_group));
        }
        return results;
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
