// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"

namespace librealsense
{
    
    class aus_value
    {
    public:
        virtual long get() = 0;
        virtual void set(long value) = 0;
        virtual void increment() = 0;
        virtual void start() = 0;
        virtual void stop() = 0;
    };

    class aus_timer : public aus_value {
    
    public:
        aus_timer() : _is_running(false), _total(0), _start(0) { }

        long get()
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
        
        void set(long value) {
            return;
        }

        void increment()
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
            long current_time = get_current_time();
            _total += (current_time - _start);
            _start = current_time;
        }



    private:
        long _start;
        long _total;
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
        aus_counter(long value = 0) : _counter(value), _total(0) { }
        
        long get()
        {
            return _counter;
        }

        void set(long value) {
            _counter = value;
        }

        void increment()
        {
            _counter++;
        }

        void start() { return; }

        void stop() { return; }


    private:
        long _counter;
        long _total; 
    };

  

#ifdef BUILD_AUS

    class aus_data
    {

    public:
        aus_data()
        {
            _start_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
            _librealsense_version = RS2_API_VERSION_STR;
        }

        void set(std::string key, long value)
        {
            if (_mp.find(key) != _mp.end())
            {
                _mp[key]->set(value);
            }
            else
            {//_mp[key] = std::make_unique<aus_counter>(new aus_counter(value));
                _mp[key] = new aus_counter(value);
            }

        }

        void increment(std::string key)
        {
            if (_mp.find(key) != _mp.end())
            {
                _mp[key]->increment();
            }
            else
            {
               // _mp[key] = std::make_unique<aus_counter>(new aus_counter(1));
                _mp[key] = new aus_counter(1);
            }
        }

        long get(std::string key)
        {
            assert_key_exists(key);
            return _mp[key]->get();
        }

        void start(std::string key)
        {
            if (_mp.find(key) == _mp.end())
            {
                _mp.insert(std::make_pair(key, new aus_timer()));
                _mp[key]->start();
            }
            else
            {
                _mp[key]->start();
            }
        }

        void stop(std::string key)
        {
            assert_key_exists(key);
            _mp[key]->stop();
        }

        std::vector<std::string> get_counters_list()
        {
            std::vector<std::string> result;
            for (const auto& pair : _mp) {
                result.push_back(pair.first);
            }
            return result;
        }


    private:
        std::unordered_map<std::string, aus_value*> _mp;

        long _start_time;

        std::string _librealsense_version;

        void assert_key_exists(std::string key)
        {
            if (_mp.find(key) == _mp.end())
            {
                throw std::runtime_error("\"" + key + "\" does not exist");
            }
        }

    }; // end of class aus_data

#else
    class aus_data
    {
    public:

        void set(std::string key, long value)
        {
            throw std::runtime_error("set is not supported without BUILD_AUS");
        }

        void increment(std::string key)
        {
            throw std::runtime_error("increase is not supported without BUILD_AUS");
        }

        long get(std::string key)
        {
            throw std::runtime_error("get is not supported without BUILD_AUS");
        }
        
        void start(std::string key)
        {
            throw std::runtime_error("start_timer is not supported without BUILD_AUS");
        }

        void stop(std::string key)
        {
            throw std::runtime_error("stop_timer is not supported without BUILD_AUS");
        }

        std::vector<std::string> get_counters_names()
        {
            throw std::runtime_error("get_counters_names is not supported without BUILD_AUS");
        }

    }; // end of class aus_data

#endif
}
