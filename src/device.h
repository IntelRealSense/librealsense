// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_DEVICE_H
#define LIBREALSENSE_DEVICE_H

#include "backend.h"
#include "stream.h"
#include <chrono>
#include <memory>
#include <vector>

struct rs_device_info
{
    virtual rs_device* create(const rsimpl::uvc::backend& backend) const = 0;
    virtual rs_device_info* clone() const = 0;

    virtual ~rs_device_info() = default;
};

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
}

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


namespace rsimpl
{
    class endpoint
    {
    public:
        virtual std::vector<stream_profile> get_stream_profiles() const = 0;
        virtual ~endpoint() = default;
    };

    class uvc_endpoint : public endpoint
    {
    public:
        uvc_endpoint(std::shared_ptr<uvc::uvc_device> device,
                     rs_device* owner)
            : _device(std::move(device)), _owner(owner) {}

        std::vector<stream_profile> get_stream_profiles() const override;

    private:
        std::shared_ptr<uvc::uvc_device> _device;
        rs_device* _owner;
    };
}

struct rs_stream_profile_list
{
    std::vector<stream_profile> list;
};

struct rs_device
{
    rs_device()
    {
        _endpoints.resize(RS_SUBDEVICE_COUNT);
    }

    virtual ~rs_device() = default;

    virtual bool supports(rs_subdevice subdevice) const
    {
        return _endpoints[subdevice].get() != nullptr;
    }

    const rsimpl::endpoint& get_endpoint(rs_subdevice sub) const { return *_endpoints[sub]; }
    rsimpl::endpoint& get_endpoint(rs_subdevice sub) { return *_endpoints[sub]; }

    virtual bool supports_guid(const std::string& guid) const
    {
        auto it = _guid_to_output.find(guid);
        return it != _guid_to_output.end();
    }

    virtual output_type guid_to_format(const std::string& guid) const
    {
        auto it = _guid_to_output.find(guid);
        return it->second;
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
    std::map<std::string, output_type> _guid_to_output;
};

#endif
