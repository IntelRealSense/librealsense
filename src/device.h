// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "stream.h"
#include "archive.h"
#include <chrono>
#include <memory>
#include <vector>

namespace rsimpl
{
    

    // This class is used to buffer up several writes to a structure-valued XU control, and send the entire structure all at once
    // Additionally, it will ensure that any fields not set in a given struct will retain their original values
    template<class T, class R, class W> struct struct_interface
    {
        T struct_;
        R reader;
        W writer;
        bool active;

        struct_interface(R r, W w) : reader(r), writer(w), active(false) {}

        void activate() { if (!active) { struct_ = reader(); active = true; } }
        template<class U> double get(U T::* field) { activate(); return static_cast<double>(struct_.*field); }
        template<class U, class V> void set(U T::* field, V value) { activate(); struct_.*field = static_cast<U>(value); }
        void commit() { if (active) writer(struct_); }
    };

    template<class T, class R, class W> struct_interface<T, R, W> make_struct_interface(R r, W w) { return{ r,w }; }

    template <typename T>
    class wraparound_mechanism
    {
    public:
        wraparound_mechanism(T min_value, T max_value)
            : max_number(max_value - min_value + 1), last_number(min_value), num_of_wraparounds(0)
        {}

        T fix(T number)
        {
            if ((number + (num_of_wraparounds*max_number)) < last_number)
                ++num_of_wraparounds;


            number += (num_of_wraparounds*max_number);
            last_number = number;
            return number;
        }

    private:
        T max_number;
        T last_number;
        unsigned long long num_of_wraparounds;
    };

    struct frame_timestamp_reader
    {
        virtual ~frame_timestamp_reader() = default;

        virtual bool validate_frame(const subdevice_mode & mode, const void * frame) const = 0;
        virtual double get_frame_timestamp(const subdevice_mode & mode, const void * frame) = 0;
        virtual unsigned long long get_frame_counter(const subdevice_mode & mode, const void * frame) const = 0;
    };


    namespace motion_module
    {
        struct motion_module_parser;
    }
    struct cam_mode { int2 dims; std::vector<int> fps; };

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
        uvc_endpoint(std::shared_ptr<uvc::uvc_device> device,
                     rs_device* owner)
            : _device(std::move(device)), _owner(owner) {}

        std::vector<stream_profile> get_stream_profiles() override;

        std::shared_ptr<rsimpl::streaming_lock> configure(
            const std::vector<stream_profile>& request) override;

        const rs_device& get_device() const { return *_owner; }

        void register_xu(uvc::extension_unit xu)
        {
            _xus.push_back(std::move(xu));
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
        rs_device* _owner;
        int _user_count = 0;
        std::mutex _power_lock;
        std::mutex _configure_lock;
        std::vector<uvc::stream_profile> _configuration;
        std::vector<uvc::extension_unit> _xus;
    };
}

struct rs_device_info
{
    virtual rs_device* create(const rsimpl::uvc::backend& backend) const = 0;
    virtual rs_device_info* clone() const = 0;

    virtual ~rs_device_info() = default;
};

struct rs_stream_profile_list
{
    std::vector<rsimpl::stream_profile> list;
};

struct rs_stream_lock
{
    std::shared_ptr<rsimpl::streaming_lock> lock;
};

struct rs_device
{
    rs_device()
    {
        _endpoints.resize(RS_SUBDEVICE_COUNT);
        map_output(RS_FORMAT_YUYV, RS_STREAM_COLOR, "{32595559-0000-0010-8000-00AA00389B71}");
    }

    virtual ~rs_device() = default;

    bool supports(rs_subdevice subdevice) const
    {
        return _endpoints[subdevice].get() != nullptr;
    }

    rsimpl::endpoint& get_endpoint(rs_subdevice sub) { return *_endpoints[sub]; }

    bool supports_guid(const std::string& guid) const
    {
        auto it = _guid_to_output.find(guid);
        return it != _guid_to_output.end();
    }

    rsimpl::output_type guid_to_format(const std::string& guid) const
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

protected:
    void assign_endpoint(rs_subdevice subdevice,
                         std::shared_ptr<rsimpl::endpoint> endpoint)
    {
        _endpoints[subdevice] = std::move(endpoint);
    }

    void map_output(rs_format format, rs_stream stream, const std::string& guid)
    {
        _guid_to_output[guid] = { stream, format };
    }
private:
    std::vector<std::shared_ptr<rsimpl::endpoint>> _endpoints;
    std::map<std::string, rsimpl::output_type> _guid_to_output;
};