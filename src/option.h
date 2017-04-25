// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "archive.h"
#include "hw-monitor.h"
#include "subdevice.h"

#include <chrono>
#include <memory>
#include <vector>

namespace rsimpl2
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
        virtual bool is_read_only() const { return false; }

        virtual const char* get_description() const = 0;
        virtual const char* get_value_description(float) const { return nullptr; }

        virtual ~option() = default;
    };

    class uvc_pu_option : public option
    {
    public:
        void set(float value) override;

        float query() const override;

        option_range get_range() const override;

        bool is_enabled() const override
        {
            return true;
        }

        uvc_pu_option(uvc_endpoint& ep, rs2_option id)
            : _ep(ep), _id(id)
        {
        }

        const char* get_description() const override;
    private:
        uvc_endpoint& _ep;
        rs2_option _id;
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
                    if (!dev.set_xu(_xu, _id, reinterpret_cast<uint8_t*>(&t), sizeof(T)))
                        throw invalid_value_exception(to_string() << "set_xu(id=" << std::to_string(_id) << ") failed!" << " Last Error: " << strerror(errno));
                });
        }

        float query() const override
        {
            return static_cast<float>(_ep.invoke_powered(
                [this](uvc::uvc_device& dev)
                {
                    T t;
                    if (!dev.get_xu(_xu, _id, reinterpret_cast<uint8_t*>(&t), sizeof(T)))
                        throw invalid_value_exception(to_string() << "get_xu(id=" << std::to_string(_id) << ") failed!" << " Last Error: " << strerror(errno));

                    return static_cast<float>(t);
                }));
        }

        option_range get_range() const override
        {
            auto uvc_range = _ep.invoke_powered(
                [this](uvc::uvc_device& dev)
                {
                    return dev.get_xu_range(_xu, _id, sizeof(T));
                });

            auto min = *(reinterpret_cast<int32_t*>(uvc_range.min.data()));
            auto max = *(reinterpret_cast<int32_t*>(uvc_range.max.data()));
            auto step = *(reinterpret_cast<int32_t*>(uvc_range.step.data()));
            auto def = *(reinterpret_cast<int32_t*>(uvc_range.def.data()));
            return option_range{static_cast<float>(min),
                                static_cast<float>(max),
                                static_cast<float>(step),
                                static_cast<float>(def)};
        }

        bool is_enabled() const override { return true; }

        uvc_xu_option(uvc_endpoint& ep, uvc::extension_unit xu, uint8_t id, std::string description)
            : _ep(ep), _xu(xu), _id(id), _desciption(std::move(description))
        {}

        const char* get_description() const override
        {
            return _desciption.c_str();
        }

    protected:
        uvc_endpoint&       _ep;
        uvc::extension_unit _xu;
        uint8_t             _id;
        std::string         _desciption;
    };

    inline std::string hexify(unsigned char n)
    {
        std::string res;

        do
        {
            res += "0123456789ABCDEF"[n % 16];
            n >>= 4;
        } while (n);

        reverse(res.begin(), res.end());

        if (res.size() == 1)
        {
            res.insert(0, "0");
        }

        return res;
    }

    template<class T, class R, class W, class U>
    class struct_field_option : public option
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

        explicit struct_field_option(std::shared_ptr<struct_interface<T, R, W>> struct_interface,
                                     U T::* field, const option_range& range)
            : _struct_interface(struct_interface), _range(range), _field(field)
        {
        }

        const char* get_description() const override
        {
            return nullptr;
        }

    private:
        std::shared_ptr<struct_interface<T, R, W>> _struct_interface;
        option_range _range;
        U T::* _field;
    };

    template<class T, class R, class W, class U>
    std::shared_ptr<struct_field_option<T, R, W, U>> make_field_option(
        std::shared_ptr<struct_interface<T, R, W>> struct_interface,
        U T::* field, const option_range& range)
    {
        return std::make_shared<struct_field_option<T, R, W, U>>
            (struct_interface, field, range);
    }

    class command_transfer_over_xu : public uvc::command_transfer
    {
    public:
        std::vector<uint8_t> send_receive(const std::vector<uint8_t>& data, int, bool require_response) override;

        command_transfer_over_xu(uvc_endpoint& uvc,
                             uvc::extension_unit xu, uint8_t ctrl)
            : _uvc(uvc), _xu(std::move(xu)), _ctrl(ctrl)
        {}

    private:
        uvc_endpoint&       _uvc;
        uvc::extension_unit _xu;
        uint8_t             _ctrl;
    };

    class polling_error_handler;

    class polling_errors_disable : public option
    {
    public:
        polling_errors_disable(polling_error_handler* handler)
            : _polling_error_handler(handler), _value(1)
        {}

        void set(float value);

        float query() const;

        option_range get_range() const;

        bool is_enabled() const;


        const char* get_description() const;

        const char* get_value_description(float value) const;

    private:
        polling_error_handler*          _polling_error_handler;
        float                           _value;
    };

    /** \brief auto_disabling_control class provided a control
    * that disable auto exposure whan changing the auto disabling control value */
   class auto_disabling_control : public option
   {
   public:
       const char* get_value_description(float val) const override
       {
           return _auto_disabling_control->get_value_description(val);
       }
       const char* get_description() const override
       {
            return _auto_disabling_control->get_description();
       }
       void set(float value) override
       {
          auto strong = _auto_exposure.lock();
          assert(strong);

          auto is_auto = strong->query();

          if(strong && is_auto)
          {
              LOG_DEBUG("Move auto exposure to manual mode in order set value to gain controll");
              strong->set(0);
          }
          _auto_disabling_control->set(value);
       }

       float query() const override
       {
           return _auto_disabling_control->query();
       }

       option_range get_range() const override
       {
           return _auto_disabling_control->get_range();
       }

       bool is_enabled() const override
       {
           return  _auto_disabling_control->is_enabled();
       }

       bool is_read_only() const override
       {
           return  _auto_disabling_control->is_read_only();
       }


       explicit auto_disabling_control(std::shared_ptr<option> auto_disabling,
                                       std::shared_ptr<option> auto_exposure)

           :_auto_disabling_control(auto_disabling), _auto_exposure(auto_exposure)
       {}

   private:
       std::shared_ptr<option> _auto_disabling_control;
       std::weak_ptr<option> _auto_exposure;
   };

   class readonly_option : public option
   {
   public:
       bool is_read_only() const override { return true; }

       void set(float) override
       {
           throw not_implemented_exception("This option is read-only!");
       }
   };


   class const_value_option : public readonly_option
   {
   public:
       const_value_option(std::string desc, float val)
            : _desc(std::move(desc)), _val(val) {}

       float query() const override { return _val; }
       option_range get_range() const override { return { _val, _val, 0, _val }; }
       bool is_enabled() const override { return true; }

       const char* get_description() const override { return _desc.c_str(); }

   private:
       float _val;
       std::string _desc;
   };
}
