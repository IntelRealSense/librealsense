// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <vector>

namespace rs2
{
    class stream_profile;
}

namespace librealsense {

    class color_map
    {
    public:
        color_map(std::map<float, float3> map, int steps = 4000) : _map(map)
        {
            initialize(steps);
        }

        color_map(const std::vector<float3>& values, int steps = 4000)
        {
            for (size_t i = 0; i < values.size(); i++)
            {
                _map[(float)i / (values.size() - 1)] = values[i];
            }
            initialize(steps);
        }

        color_map() {}

        inline float3 get(float value) const
        {
            if (_max == _min) return *_data;
            auto t = (value - _min) / (_max - _min);
            t = clamp_val(t, 0.f, 1.f);
            return _data[(int)(t * (_size - 1))];
        }

        float min_key() const { return _min; }
        float max_key() const { return _max; }

    private:
        inline float3 lerp(const float3& a, const float3& b, float t) const
        {
            return b * t + a * (1 - t);
        }

        float3 calc(float value) const
        {
            if (_map.size() == 0) return{ value, value, value };
            // if we have exactly this value in the map, just return it
            if (_map.find(value) != _map.end()) return _map.at(value);
            // if we are beyond the limits, return the first/last element
            if (value < _map.begin()->first)   return _map.begin()->second;
            if (value > _map.rbegin()->first)  return _map.rbegin()->second;

            auto lower = _map.lower_bound(value) == _map.begin() ? _map.begin() : --(_map.lower_bound(value));
            auto upper = _map.upper_bound(value);

            auto t = (value - lower->first) / (upper->first - lower->first);
            auto c1 = lower->second;
            auto c2 = upper->second;
            return lerp(c1, c2, t);
        }

        void initialize(int steps)
        {
            if (_map.size() == 0) return;

            _min = _map.begin()->first;
            _max = _map.rbegin()->first;

            _cache.resize(steps + 1);
            for (int i = 0; i <= steps; i++)
            {
                auto t = (float)i / steps;
                auto x = _min + t*(_max - _min);
                _cache[i] = calc(x);
            }

            // Save size and data to avoid STL checks penalties in DEBUG
            _size = _cache.size();
            _data = _cache.data();
        }

        std::map<float, float3> _map;
        std::vector<float3> _cache;
        float _min, _max;
        size_t _size; float3* _data;
    };

    class colorizer : public stream_filter_processing_block
    {
    public:
        colorizer();

    protected:
        rs2::frame process_frame(const rs2::frame_source& source, const rs2::frame& f) override;

    private:
        float _min, _max;
        bool _equalize;
        std::vector<color_map*> _maps;
        int _map_index = 0;
        int _preset = 0;
        rs2::stream_profile _target_stream_profile;
        rs2::stream_profile _source_stream_profile;
    };
}
