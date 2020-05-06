// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <string>
#include <sstream>
#include <functional>

namespace rs2
{
    class config_value
    {
    public:
        template<class T>
        operator T()
        {
            std::stringstream ss;
            ss.str(_val);
            T res;
            ss >> res;
            return res;
        }

        // When converting config_value to string, we can't use >> operator since it reads until first whitespace rather than the whole string;
        // Therefore we use a different overload for strings
        operator std::string()
        {
            return _val;
        }

        config_value(std::string val) : _val(std::move(val)) {}

    private:
        std::string _val;
    };

    class config_file
    {
    public:
        config_file();
        config_file(std::string filename);

        void set_default(const char* key, const char* calculate);

        template<class T>
        void set_default(const char* key, T val)
        {
            std::stringstream ss;
            ss << val;
            set_default(key, ss.str().c_str());
        }

        bool operator==(const config_file& other) const;

        config_file& operator=(const config_file& other);

        void set(const char* key, const char* value);
        std::string get(const char* key, const char* def) const;

        config_value get(const char* key) const;

        template<class T>
        T get_or_default(const char* key, T def) const
        {
            if (contains(key)) return get(key);
            return def;
        }

        template<class T>
        void set(const char* key, T val)
        {
            std::stringstream ss;
            ss << val;
            set(key, ss.str().c_str());
        }

        bool contains(const char* key) const;
        
        void save(const char* filename);

        void reset();

        static config_file& instance();

    private:
        std::string get_default(const char* key, const char* def) const;

        void save();

        std::map<std::string, std::string> _values;
        std::map<std::string, std::string> _defaults;
        std::string _filename;
    };
}