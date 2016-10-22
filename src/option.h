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
#include "subdevice.h"

namespace rsimpl
{
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
}
