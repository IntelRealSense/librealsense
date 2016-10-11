// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS_HPP
#define LIBREALSENSE_RS_HPP

#include "rs.h"

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace rs
{
    class error : public std::runtime_error
    {
        std::string function, args;
    public:
        explicit error(rs_error * err) : std::runtime_error(rs_get_error_message(err))
        {
            function = (nullptr != rs_get_failed_function(err)) ? rs_get_failed_function(err) : std::string();
            args = (nullptr != rs_get_failed_args(err)) ? rs_get_failed_args(err) : std::string();
            rs_free_error(err);
        }
        const std::string & get_failed_function() const { return function; }
        const std::string & get_failed_args() const { return args; }
        static void handle(rs_error * e) { if (e) throw error(e); }
    };

    class context;
    class device;

    class device_info
    {
    public:

    private:
        friend context;
        explicit device_info(std::shared_ptr<rs_device_info> info) : _info(info) {}
        
        std::shared_ptr<rs_device_info> _info;
    };

    struct stream_profile
    {
        rs_stream stream;
        int width;
        int height;
        int fps;
        rs_format format;
    };

    class subdevice
    {
    public:
        std::vector<stream_profile> get_stream_profiles() const
        {
            std::vector<stream_profile> results;

            rs_error* e = nullptr;
            std::shared_ptr<rs_stream_profile_list> list(
                rs_get_supported_profiles(_dev, _index, &e),
                rs_delete_profiles_list);
            error::handle(e);

            auto size = rs_get_profile_list_size(list.get(), &e);
            error::handle(e);
            
            for (auto i = 0; i < size; i++)
            {
                stream_profile profile;
                rs_get_profile(list.get(), i, 
                    &profile.stream,
                    &profile.width,
                    &profile.height,
                    &profile.fps,
                    &profile.format,
                    &e);
                error::handle(e);
                results.push_back(profile);
            }

            return results;
        }

    private:
        friend device;
        explicit subdevice(rs_device* dev, rs_subdevice index) 
            : _dev(dev), _index(index) {}

        rs_device* _dev;
        rs_subdevice _index;
    };

    class device
    {
    public:
        subdevice& get_subdevice(rs_subdevice sub)
        {
            if (sub < _subdevices.size() && _subdevices[sub].get()) 
                return *_subdevices[sub];
            throw std::exception("Requested subdevice is not supported!");
        }

        const subdevice& get_subdevice(rs_subdevice sub) const
        {
            if (sub < _subdevices.size() && _subdevices[sub].get()) 
                return *_subdevices[sub];
            throw std::exception("Requested subdevice is not supported!");
        }

        bool supports(rs_subdevice sub) const
        {
            return sub < _subdevices.size() && _subdevices[sub].get();
        }

        const subdevice& color() const { return get_subdevice(RS_SUBDEVICE_COLOR); }
        subdevice& color() { return get_subdevice(RS_SUBDEVICE_COLOR); }

        const subdevice& depth() const { return get_subdevice(RS_SUBDEVICE_DEPTH); }
        subdevice& depth() { return get_subdevice(RS_SUBDEVICE_DEPTH); }


    private:
        friend context;
        explicit device(std::shared_ptr<rs_device> dev) : _dev(dev)
        {
            _subdevices.resize(RS_SUBDEVICE_COUNT);
            for (auto i = 0; i < RS_SUBDEVICE_COUNT; i++)
            {
                auto s = static_cast<rs_subdevice>(i);
                rs_error* e = nullptr;
                auto is_supported = rs_is_subdevice_supported(_dev.get(), s, &e);
                error::handle(e);
                if (is_supported) _subdevices[s].reset(new subdevice(_dev.get(), s));
                else  _subdevices[s] = nullptr;
            }
        }

        std::shared_ptr<rs_device> _dev;
        std::vector<std::shared_ptr<subdevice>> _subdevices;
    };

    class context
    {
    public:
        context()
        {
            rs_error* e = nullptr;
            _context = std::shared_ptr<rs_context>(
                rs_create_context(RS_API_VERSION, &e),
                rs_delete_context);
            error::handle(e);
        }

        std::vector<device_info> query_devices() const
        {
            rs_error* e = nullptr;
            std::shared_ptr<rs_device_info_list> list(
                rs_query_devices(_context.get(), &e),
                rs_delete_device_info_list);
            error::handle(e);

            auto size = rs_get_device_list_size(list.get(), &e);
            error::handle(e);

            std::vector<device_info> results;
            for (auto i = 0; i < size; i++)
            {
                std::shared_ptr<rs_device_info> info(
                    rs_get_device_info(list.get(), i, &e),
                    rs_delete_device_info);
                error::handle(e);

                device_info rs_info(info);
                results.push_back(rs_info);
            }

            return results;
        }

        device create(const device_info& info) const
        {
            rs_error* e = nullptr;
            std::shared_ptr<rs_device> dev(
                rs_create_device(_context.get(), info._info.get(), &e),
                rs_delete_device);
            error::handle(e);
            device result(dev);
            return result;
        }

    private:
        std::shared_ptr<rs_context> _context;
    };
};

#endif // LIBREALSENSE_RS2_HPP
