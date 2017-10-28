// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "archive.h"
#include "hw-monitor.h"
#include "sensor.h"
#include "core/streaming.h"

#include <chrono>
#include <memory>
#include <vector>
#include <cmath>

namespace librealsense
{
    class readonly_option : public option
    {
    public:
        bool is_read_only() const override { return true; }

        void set(float) override
        {
            throw not_implemented_exception("This option is read-only!");
        }

        void enable_recording(std::function<void(const option &)> record_action) override
        {
            //empty
        }
    };

    class const_value_option : public readonly_option, public extension_snapshot
    {
    public:
        const_value_option(std::string desc, float val)
            : _val(lazy<float>([val]() {return val; })), _desc(std::move(desc)) {}

        const_value_option(std::string desc, lazy<float> val)
            : _val(std::move(val)), _desc(std::move(desc)) {}

        float query() const override { return *_val; }
        option_range get_range() const override { return { *_val, *_val, 0, *_val }; }
        bool is_enabled() const override { return true; }

        const char* get_description() const override { return _desc.c_str(); }

        void update(std::shared_ptr<extension_snapshot> ext) override
        {
            if (auto opt = As<option>(ext))
            {
                auto new_val = opt->query();
                _val = lazy<float>([new_val]() {return new_val; });
                _desc = opt->get_description();
            }
        }
    private:
        lazy<float> _val;
        std::string _desc;
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

        uvc_pu_option(uvc_sensor& ep, rs2_option id)
            : _ep(ep), _id(id)
        {
        }

        uvc_pu_option(uvc_sensor& ep, rs2_option id, const std::map<float, std::string>& description_per_value)
            : _ep(ep), _id(id), _description_per_value(description_per_value)
        {
        }

        const char* get_description() const override;

        const char* get_value_description(float val) const override
        {
            if (_description_per_value.find(val) != _description_per_value.end())
                return _description_per_value.at(val).c_str();
            return nullptr;
        }
        void enable_recording(std::function<void(const option &)> record_action) override
        {
            _record = record_action;
        }
    private:
        uvc_sensor& _ep;
        rs2_option _id;
        const std::map<float, std::string> _description_per_value;
        std::function<void(const option &)> _record = [](const option &) {};
    };

    template<typename T>
    class uvc_xu_option : public option
    {
    public:
        void set(float value) override
        {
            _ep.invoke_powered(
                [this, value](platform::uvc_device& dev)
                {
                    T t = static_cast<T>(value);
                    if (!dev.set_xu(_xu, _id, reinterpret_cast<uint8_t*>(&t), sizeof(T)))
                        throw invalid_value_exception(to_string() << "set_xu(id=" << std::to_string(_id) << ") failed!" << " Last Error: " << strerror(errno));
                    _recording_function(*this);
                });
        }

        float query() const override
        {
            return static_cast<float>(_ep.invoke_powered(
                [this](platform::uvc_device& dev)
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
                [this](platform::uvc_device& dev)
                {
                    return dev.get_xu_range(_xu, _id, sizeof(T));
                });
            
            if (uvc_range.min.size() < sizeof(int32_t)) return option_range{0,0,1,0};

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

        uvc_xu_option(uvc_sensor& ep, platform::extension_unit xu, uint8_t id, std::string description)
            : _ep(ep), _xu(xu), _id(id), _desciption(std::move(description))
        {}

        const char* get_description() const override
        {
            return _desciption.c_str();
        }
        void enable_recording(std::function<void(const option &)> record_action) override
        {
            _recording_function = record_action;
        }
    protected:
        uvc_sensor&       _ep;
        platform::extension_unit _xu;
        uint8_t             _id;
        std::string         _desciption;
        std::function<void(const option&)> _recording_function = [](const option&) {};
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
            _recording_function(*this);
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

        void enable_recording(std::function<void(const option &)> record_action) override
        {
            _recording_function = record_action;
        }
    private:
        std::shared_ptr<struct_interface<T, R, W>> _struct_interface;
        option_range _range;
        U T::* _field;
        std::function<void(const option&)> _recording_function = [](const option&) {};
    };

    template<class T, class R, class W, class U>
    std::shared_ptr<struct_field_option<T, R, W, U>> make_field_option(
        std::shared_ptr<struct_interface<T, R, W>> struct_interface,
        U T::* field, const option_range& range)
    {
        return std::make_shared<struct_field_option<T, R, W, U>>
            (struct_interface, field, range);
    }

    class command_transfer_over_xu : public platform::command_transfer
    {
    public:
        std::vector<uint8_t> send_receive(const std::vector<uint8_t>& data, int, bool require_response) override;

        command_transfer_over_xu(uvc_sensor& uvc,
                                 platform::extension_unit xu, uint8_t ctrl)
            : _uvc(uvc), _xu(std::move(xu)), _ctrl(ctrl)
        {}

    private:
        uvc_sensor&       _uvc;
        platform::extension_unit _xu;
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
        void enable_recording(std::function<void(const option &)> record_action) override
        {
            _recording_function = record_action;
        }
    private:
        polling_error_handler*          _polling_error_handler;
        float                           _value;
        std::function<void(const option&)> _recording_function = [](const option&) {};
    };

    /** \brief auto_disabling_control class provided a control
    * that disable auto-control when changing the auto disabling control value */
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

          auto move_to_manual = false;
          auto val = strong->query();

          if (std::find(_move_to_manual_values.begin(),
                        _move_to_manual_values.end(), val) != _move_to_manual_values.end())
          {
              move_to_manual = true;
          }

          if (strong && move_to_manual)
          {
              LOG_DEBUG("Move option to manual mode in order to set a value");
              strong->set(_manual_value);
          }
          _auto_disabling_control->set(value);
          _recording_function(*this);
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
                                       std::shared_ptr<option> auto_exposure,
                                       std::vector<float> move_to_manual_values = {1.f},
                                       float manual_value = 0.f)

           : _auto_disabling_control(auto_disabling), _auto_exposure(auto_exposure),
             _move_to_manual_values(move_to_manual_values), _manual_value(manual_value)
       {}
       void enable_recording(std::function<void(const option &)> record_action) override
       {
           _recording_function = record_action;
       }
   private:
       std::shared_ptr<option> _auto_disabling_control;
       std::weak_ptr<option>   _auto_exposure;
       std::vector<float>      _move_to_manual_values;
       float                   _manual_value;
       std::function<void(const option&)> _recording_function = [](const option&) {};
   };

   class option_base : public option
   {
   public:
       option_base(const option_range& opt_range)
           : _opt_range(opt_range)
       {}

       bool is_valid(float value) const
       {
           if (!std::isnormal(_opt_range.step))
               throw invalid_value_exception(to_string() << "is_valid(...) failed! step is not properly defined. (" << _opt_range.step << ")");

           if ((value < _opt_range.min) || (value > _opt_range.max))
               return false;

           auto n = (value - _opt_range.min)/_opt_range.step;
           return (fabs(fmod(n, 1)) < std::numeric_limits<float>::min());
       }

       option_range get_range() const override
       {
           return _opt_range;
       }
       virtual void enable_recording(std::function<void(const option&)> recording_action) override
       {
           _recording_function = recording_action;
       }
    protected:
       const option_range _opt_range;
       std::function<void(const option&)> _recording_function = [](const option&) {};

   };
}
