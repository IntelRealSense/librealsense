// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "device.h"
#include <mutex>
#include <rsutils/os/os.h>

#define NOT_SUPPORTED(func_api)  func_api{throw std::runtime_error("function " #func_api " is not supported without BUILD_AUS flag on");}

namespace librealsense
{
    
    class aus_value
    {
    public:
        virtual long long get() = 0;
        virtual void set(long long value) = 0;
        virtual void increment() = 0;
        virtual void decrement() = 0;
        virtual void start() = 0;
        virtual void stop() = 0;
    };

    class aus_timer : public aus_value {
    
    public:
        aus_timer() : _is_running(false), _total(0), _start(0) { }

        long long get()
        {
            if (_is_running)
            {
                return get_current_time() - _start + _total;
            }
            else
            {
                return _total;
            }
        }
        
        void set(long long value) {
            return;
        }

        void increment()
        {
            return;
        }

        void decrement()
        {
            return;
        }

        void start()
        {
            _start = std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
            _is_running = true;
        }

        void stop()
        {
            if (!_is_running)
            {
                return;
            }
            _is_running = false;
            long long current_time = get_current_time();
            _total += (current_time - _start);
            _start = current_time;
        }



    private:
        long long _start;
        long long _total;
        bool _is_running;


        time_t get_current_time() 
        {
            return std::chrono::system_clock::to_time_t(
                std::chrono::system_clock::now());
        }

    };

    class aus_counter : public aus_value
    {
    public:
        aus_counter(long long value = 0) : _counter(value), _total(0) { }
        
        long long get()
        {
            return _counter;
        }

        void set(long long value) {
            _counter = value;
        }

        void increment()
        {
            _counter++;
        }

        void decrement()
        {
            _counter--;
        }

        void start() { return; }

        void stop() { return; }


    private:
        long long _counter;
        long long _total;
    };

    class aus_data
    {

    public:
        aus_data()
        {
            _start_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            _librealsense_version = RS2_API_VERSION_STR;
            _os_name = rsutils::get_os_name();
            _platform_name = rsutils::get_platform_name();

        }

        void set(std::string key, long long value)
        {
            std::lock_guard<std::mutex> lock(_m);
            if (_mp.find(key) != _mp.end())
            {
                _mp[key]->set(value);
            }
            else
            {
                _mp[key] = std::make_shared<aus_counter>(value);
            }

        }

        void increment(std::string key)
        {
            std::lock_guard<std::mutex> lock(_m);
            if (_mp.find(key) != _mp.end())
            {
                _mp[key]->increment();
            }
            else
            {
                _mp[key] = std::make_shared<aus_counter>(1);
            }
        }

        void decrement(std::string key)
        {
            std::lock_guard<std::mutex> lock(_m);
            assert_key_exists(key);
            if (_mp[key]->get()>0)
            {
                _mp[key]->decrement();
            }
        }

        long long get(std::string key)
        {
            std::lock_guard<std::mutex> lock(_m);
            assert_key_exists(key);
            return _mp[key]->get();
        }

        void start(std::string key)
        {
            std::lock_guard<std::mutex> lock(_m);
            if (_mp.find(key) == _mp.end())
            {
 
                _mp.insert(std::make_pair(key, std::make_shared<aus_timer>()));
                _mp[key]->start();
            }
            else
            {
                _mp[key]->start();
            }
        }

        void stop(std::string key)
        {
            std::lock_guard<std::mutex> lock(_m);
            assert_key_exists(key);
            _mp[key]->stop();
        }

        std::vector<std::string> get_counters_list()
        {
            std::lock_guard<std::mutex> lock(_m);
            std::vector<std::string> result;
            for (const auto& pair : _mp) {
                result.push_back(pair.first);
            }
            return result;
        }

        bool device_exist(std::string serial)
        {
            if ( _mp_devices_manager.find( serial ) == _mp_devices_manager.end())
            {
                return false;
            }
            return true;
        }

        void insert_device_to_device_manager(std::string serial, std::string name)
        {
            increment( librealsense::aus_build_system_counter_name( "CONNECTED_DEVICES" ));
            increment( librealsense::aus_build_system_counter_name( "CONNECTED_DEVICES", name ) );
            _mp_devices_manager[serial]=name;
        }

        void on_device_changed(std::shared_ptr<device_interface> device)
        {
            if (device->supports_info(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER) && device->supports_info(RS2_CAMERA_INFO_NAME))
            {
                std::string serial = device->get_info(RS2_CAMERA_INFO_ASIC_SERIAL_NUMBER);
                std::string name = device->get_info(RS2_CAMERA_INFO_NAME);
                if (!device_exist(serial))
                {
                    insert_device_to_device_manager(serial, name);
                }
            }
        } 

    private:
        std::unordered_map<std::string, std::shared_ptr<aus_value>>_mp;
        std::unordered_map<std::string, std::string > _mp_devices_manager;
        std::mutex _m;

        long long _start_time;

        std::string _librealsense_version;
        std::string _os_name;
        std::string _platform_name;

        void assert_key_exists(std::string key)
        {
            if (_mp.find(key) == _mp.end())
            {
                throw std::runtime_error("\"" + key + "\" does not exist");
            }
        }

    }; // end of class aus_data


}
