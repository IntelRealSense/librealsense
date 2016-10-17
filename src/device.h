// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "stream.h"
#include "archive.h"
#include <chrono>
#include <memory>
#include <vector>
#include "hw-monitor.h"

namespace rs{
    class device;
}

namespace rsimpl
{
    struct frame_timestamp_reader
    {
        virtual ~frame_timestamp_reader() = default;

        virtual bool validate_frame(const subdevice_mode & mode, const void * frame) const = 0;
        virtual double get_frame_timestamp(const subdevice_mode & mode, const void * frame) = 0;
        virtual unsigned long long get_frame_counter(const subdevice_mode & mode, const void * frame) const = 0;
    };

    class device;

    struct stream_profile
    {
        rs_stream stream;
        int width;
        int height;
        int fps;
        rs_format format;
    };

    struct output_type
    {
        rs_stream stream;
        rs_format format;
    };

    class streaming_lock
    {
    public:
        streaming_lock()
            : _callback(nullptr, [](rs_frame_callback*){}), 
              _archive(&max_publish_list_size), 
              _owner(nullptr)
        {
            
        }

        void set_owner(const rs_stream_lock* owner) { _owner = owner; }

        void play(frame_callback_ptr callback)
        {
            std::lock_guard<std::mutex> lock(_callback_mutex);
            _callback = std::move(callback);
        }

        void stop()
        {
            flush();
            std::lock_guard<std::mutex> lock(_callback_mutex);
            _callback.reset();
        }

        void release_frame(rs_frame_ref* frame)
        {
            _archive.release_frame_ref(frame);
        }

        rs_frame_ref* alloc_frame(rs_stream stream, frame_additional_data additional_data)
        {
            auto frame = _archive.alloc_frame(stream, additional_data, false);
            return _archive.track_frame(frame);
        }

        void invoke_callback(rs_frame_ref* frame_ref) const
        {
            if (frame_ref)
            {
                frame_ref->update_frame_callback_start_ts(std::chrono::high_resolution_clock::now());
                //frame_ref->log_callback_start(capture_start_time);
                //on_before_callback(streams[i], frame_ref, archive);
                _callback->on_frame(_owner, frame_ref);
            }
        }

        void flush()
        {
            _archive.flush();
        }

        virtual ~streaming_lock()
        {
            stop();
        }

    private:
        std::mutex _callback_mutex;
        frame_callback_ptr _callback;
        frame_archive _archive;
        std::atomic<uint32_t> max_publish_list_size;
        const rs_stream_lock* _owner;
    };

    class endpoint
    {
    public:
        virtual std::vector<stream_profile> get_stream_profiles() = 0;

        virtual std::shared_ptr<streaming_lock> configure(
            const std::vector<stream_profile>& request) = 0;

        virtual ~endpoint() = default;
    };

    class uvc_endpoint : public endpoint
    {
    public:
        uvc_endpoint(std::shared_ptr<uvc::uvc_device> uvc_device,
                     device* owner)
            : _device(std::move(uvc_device)), _owner(owner) {}

        std::vector<stream_profile> get_stream_profiles() override;

        std::shared_ptr<streaming_lock> configure(
            const std::vector<stream_profile>& request) override;

        const device& get_device() const { return *_owner; }

        void register_xu(uvc::extension_unit xu)
        {
            _xus.push_back(std::move(xu));
        }

        template<class T>
        auto invoke_powered(T action) 
            -> decltype(action(*static_cast<uvc::uvc_device*>(nullptr)))
        {
            power on(this);
            return action(*_device);
        }

        void stop_streaming();
    private:
        void acquire_power()
        {
            std::lock_guard<std::mutex> lock(_power_lock);
            if (!_user_count) 
            {
                _device->set_power_state(uvc::D0);
                for (auto& xu : _xus) _device->init_xu(xu);
            }
            _user_count++;
        }
        void release_power()
        {
            std::lock_guard<std::mutex> lock(_power_lock);
            _user_count--;
            if (!_user_count) _device->set_power_state(uvc::D3);
        }

        struct power
        {
            explicit power(uvc_endpoint* owner)
                : _owner(owner)
            {
                _owner->acquire_power();
            }

            ~power()
            {
                _owner->release_power();
            }
        private:
            uvc_endpoint* _owner;
        };

        class streaming_lock : public rsimpl::streaming_lock
        {
        public:
            explicit streaming_lock(uvc_endpoint* owner)
                : _owner(owner), _power(owner)
            {
            }

            const uvc_endpoint& get_endpoint() const { return *_owner; }
        private:
            uvc_endpoint* _owner;
            power _power;
        };

        std::shared_ptr<uvc::uvc_device> _device;
        device* _owner;
        int _user_count = 0;
        std::mutex _power_lock;
        std::mutex _configure_lock;
        std::vector<uvc::stream_profile> _configuration;
        std::vector<uvc::extension_unit> _xus;
    };

    struct option_range
    {
        float min;
        float max;
        float step;
        float def;
    };

    class option
    {
    public:
        virtual void set(float value) = 0;
        virtual float query() const = 0;
        virtual option_range get_range() const = 0;
        virtual bool is_enabled() const = 0;

        virtual ~option() = default;
    };

    class uvc_pu_option : public option
    {
    public:
        void set(float value) override
        {
            _ep.invoke_powered(
                [this, value](uvc::uvc_device& dev)
                {
                    dev.set_pu(_id, static_cast<int>(value));
                });
        }
        float query() const override
        {
            return static_cast<float>(_ep.invoke_powered(
                [this](uvc::uvc_device& dev)
                {
                    return dev.get_pu(_id);
                }));
        }
        option_range get_range() const override
        {
            auto uvc_range = _ep.invoke_powered(
                [this](uvc::uvc_device& dev)
                {
                    return dev.get_pu_range(_id);
                });
            option_range result;
            result.min  = static_cast<float>(uvc_range.min);
            result.max  = static_cast<float>(uvc_range.max);
            result.def  = static_cast<float>(uvc_range.def);
            result.step = static_cast<float>(uvc_range.step);
            return result;
        }
        bool is_enabled() const override
        {
            return true;
        }

        uvc_pu_option(uvc_endpoint& ep, rs_option id)
            : _ep(ep), _id(id)
        {
        }

    private:
        uvc_endpoint& _ep;
        rs_option _id;
    };

    template<typename T>
    class uvc_xu_option : public option
    {
    public:
        void set(float value) override
        {
            _ep.invoke_powered(
                [this, value](uvc::uvc_device& dev)
                {
                    T t = static_cast<T>(value);
                    dev.set_xu(_xu, _id, reinterpret_cast<uint8_t*>(&t), sizeof(T));
                });
        }
        float query() const override
        {
            return static_cast<float>(_ep.invoke_powered(
                [this](uvc::uvc_device& dev)
                {
                    T t;
                    dev.get_xu(_xu, _id, reinterpret_cast<uint8_t*>(&t), sizeof(T));
                    return static_cast<float>(t);
                }));
        }
        option_range get_range() const override
        {
            auto uvc_range = _ep.invoke_powered(
                [this](uvc::uvc_device& dev)
                {
                    return dev.get_xu_range(_xu, _id);
                });
            option_range result;
            result.min = static_cast<float>(uvc_range.min);
            result.max = static_cast<float>(uvc_range.max);
            result.def = static_cast<float>(uvc_range.def);
            result.step = static_cast<float>(uvc_range.step);
            return result;
        }
        bool is_enabled() const override { return true; }

        uvc_xu_option(uvc_endpoint& ep, uvc::extension_unit xu, int id)
            : _ep(ep), _xu(xu), _id(id)
        {
        }

    private:
        uvc_endpoint& _ep;
        uvc::extension_unit _xu;
        int _id;
    };

    template<class T, class R, class W, class U>
    class struct_feild_option : public option
    {
    public:
        void set(float value) override
        {
            _struct_interface->set(_field, value);
        }
        float query() const override
        {
            return _struct_interface->get(_field);
        }
        option_range get_range() const override
        {
            return _range;
        }
        bool is_enabled() const override { return true; }

        explicit struct_feild_option(std::shared_ptr<struct_interface<T, R, W>> struct_interface,
                                     U T::* field, option_range range)
            : _struct_interface(struct_interface), _range(range), _field(field)
        {
        }

    private:
        std::shared_ptr<struct_interface<T, R, W>> _struct_interface;
        option_range _range;
        U T::* _field;
    };

    template<class T, class R, class W, class U>
    std::shared_ptr<struct_feild_option<T, R, W, U>> make_field_option(
        std::shared_ptr<struct_interface<T, R, W>> struct_interface,
        U T::* field, option_range range)
    {
        return std::make_shared<struct_feild_option<T, R, W, U>>
            (struct_interface, field, range);
    }

    class command_transfer_over_xu : public uvc::command_transfer
    {
    public:
        std::vector<uint8_t> send_receive(const std::vector<uint8_t>& data, int, bool require_response) override
        {
            return _uvc.invoke_powered([this, &data, require_response]
            (uvc::uvc_device& dev)
            {
                std::vector<uint8_t> result;
                std::lock_guard<uvc::uvc_device> lock(dev);
                dev.set_xu(_xu, _ctrl, data.data(), static_cast<int>(data.size()));
                if (require_response)
                {
                    result.resize(IVCAM_MONITOR_MAX_BUFFER_SIZE);
                    dev.get_xu(_xu, _ctrl, result.data(), static_cast<int>(result.size()));
                }
                return result;
            });
        }

        command_transfer_over_xu(uvc_endpoint& uvc, 
                             uvc::extension_unit xu, uint8_t ctrl)
            : _uvc(uvc), _xu(std::move(xu)), _ctrl(ctrl)
        {}

    private:
        uvc_endpoint&       _uvc;
        uvc::extension_unit _xu;
        uint8_t             _ctrl;
    };

    class device
    {
    public:
        device()
        {
            _endpoints.resize(RS_SUBDEVICE_COUNT);
            map_output(RS_FORMAT_YUYV, RS_STREAM_COLOR, "{32595559-0000-0010-8000-00AA00389B71}");
        }

        virtual ~device() = default;

        bool supports(rs_subdevice subdevice) const
        {
            return _endpoints[subdevice].get() != nullptr;
        }

        endpoint& get_endpoint(rs_subdevice sub) { return *_endpoints[sub]; }

        bool supports_guid(const std::string& guid) const
        {
            auto it = _guid_to_output.find(guid);
            return it != _guid_to_output.end();
        }

        output_type guid_to_format(const std::string& guid) const
        {
            auto it = _guid_to_output.find(guid);
            return it->second;
        }

        std::string format_to_guid(rs_stream stream, rs_format format) const
        {
            for (auto& kvp : _guid_to_output)
            {
                if (kvp.second.format == format && kvp.second.stream == stream)
                {
                    return kvp.first;
                }
            }
            throw std::runtime_error("Requested stream format is not supported by this device!");
        }

        option& get_option(rs_subdevice subdevice, rs_option id)
        {
            auto it = _options.find(std::make_pair(subdevice, id));
            if (it == _options.end())
            {
                throw std::runtime_error(to_string() 
                    << "Subdevice " << rs_subdevice_to_string(subdevice) 
                    << " does not support option " 
                    << rs_option_to_string(id) << "!");
            }
            return *it->second;
        }

        bool supports_option(rs_subdevice subdevice, rs_option id)
        {
            auto it = _options.find(std::make_pair(subdevice, id));
            if (it == _options.end()) return false;
            return it->second->is_enabled();
        }

        const std::string& get_info(rs_camera_info info) const
        {
            auto it = _static_info.camera_info.find(info);
            if (it == _static_info.camera_info.end())
                throw std::runtime_error("Selected camera info is not supported for this camera!");
            return it->second;
        }

        bool supports_info(rs_camera_info info) const
        {
            auto it = _static_info.camera_info.find(info);
            return it != _static_info.camera_info.end();
        }
    protected:
        void assign_endpoint(rs_subdevice subdevice,
            std::shared_ptr<endpoint> endpoint)
        {
            _endpoints[subdevice] = std::move(endpoint);
        }

        void map_output(rs_format format, rs_stream stream, const std::string& guid)
        {
            _guid_to_output[guid] = { stream, format };
        }

        uvc_endpoint& get_uvc_endpoint(rs_subdevice sub)
        {
            return static_cast<uvc_endpoint&>(*_endpoints[sub]);
        }

        void register_option(rs_option id, rs_subdevice subdevice, std::shared_ptr<option> option)
        {
            _options[std::make_pair(subdevice, id)] = std::move(option);
        }

        void register_pu(rs_subdevice subdevice, rs_option id)
        {
            register_option(id, subdevice, std::make_shared<uvc_pu_option>(get_uvc_endpoint(subdevice), id));
        }

        void register_device(std::string name, std::string fw_version, std::string serial, std::string location)
        {
            _static_info.camera_info[RS_CAMERA_INFO_DEVICE_NAME] = std::move(name);
            _static_info.camera_info[RS_CAMERA_INFO_CAMERA_FIRMWARE_VERSION] = std::move(fw_version);
            _static_info.camera_info[RS_CAMERA_INFO_DEVICE_SERIAL_NUMBER] = std::move(serial);
            _static_info.camera_info[RS_CAMERA_INFO_DEVICE_LOCATION] = std::move(location);
        }

        void set_pose(rs_subdevice subdevice, pose p)
        {
            _static_info.subdevice_poses[subdevice] = p;
        }
        void declare_capability(supported_capability cap)
        {
            _static_info.capabilities_vector.push_back(cap);
        }
        void set_depth_scale(float scale)
        {
            _static_info.nominal_depth_scale = scale;
        }

    private:
        std::vector<std::shared_ptr<endpoint>> _endpoints;
        std::map<std::pair<rs_subdevice, rs_option>, std::shared_ptr<option>> _options;
        std::map<std::string, output_type> _guid_to_output;
        static_device_info _static_info;
    };
}