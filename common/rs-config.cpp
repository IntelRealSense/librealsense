#include "rs-config.h"

#include "../third-party/json.hpp"

#include "model-views.h"

#include <fstream>

#include "os.h"

using json = nlohmann::json;

using namespace rs2;

void config_file::set(const char* key, const char* value)
{
    _values[key] = value;
    save();
}

void config_file::set_default(const char* key, const char* calculate)
{
    _defaults[key] = calculate;
}

void config_file::reset()
{
    _values.clear();
    save();
}

std::string config_file::get(const char* key, const char* def) const
{
    auto it = _values.find(key);
    if (it != _values.end())
    {
        return it->second;
    }
    return get_default(key, def);
}

bool config_file::contains(const char* key) const
{
    auto it = _values.find(key);
    return it != _values.end();
}

std::string config_file::get_default(const char* key, const char* def) const
{
    auto it = _defaults.find(key);
    if (it == _defaults.end()) return def;
    return it->second;
}

config_value config_file::get(const char* key) const
{
    if (!contains(key)) return config_value(get_default(key, ""));
    return config_value(get(key, ""));
}

void config_file::save(const char* filename)
{
    json j;
    for(auto kvp : _values)
    {
        j[kvp.first] = kvp.second; 
    }
    std::string s = j.dump(2);
    try
    {
        std::ofstream out(filename);
        out << s;
        out.close();
    }
    catch (...)
    {
    }
}

config_file& config_file::instance()
{
    static config_file inst(get_folder_path(rs2::special_folder::app_data) 
                            + std::string("realsense-config.json"));
    return inst;
}

config_file::config_file(std::string filename)
    : _filename(std::move(filename)), _values()
{
    try
    {

        std::ifstream t(_filename);
        if (!t.good()) return;
        std::string str((std::istreambuf_iterator<char>(t)),
                 std::istreambuf_iterator<char>());
        auto j = json::parse(str);
        for (json::iterator it = j.begin(); it != j.end(); ++it) 
        {
            _values[it.key()] = it.value().get<std::string>();
        }
    }
    catch(...)
    {

    }
}

void config_file::save()
{
    save(_filename.c_str());
}

config_file::config_file()
    : _filename(""), _values()
{
}

config_file& config_file::operator=(const config_file& other)
{
    if (this != &other)
    {
        _values = other._values;
        _defaults = other._defaults;
        save();
    }
    return *this;
}

bool config_file::operator==(const config_file& other) const
{
    return _values == other._values;
}