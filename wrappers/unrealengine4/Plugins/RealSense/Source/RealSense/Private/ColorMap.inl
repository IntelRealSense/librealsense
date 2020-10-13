#pragma once

namespace rs2_utils {

template<typename T> inline T clamp_val(T val, const T& min, const T& max) { return std::min(std::max(val, min), max); }

struct float3 { float x, y, z; float & operator [] (int i) { return (&x)[i]; } };
inline float3 operator + (const float3 & a, const float3 & b) { return{ a.x + b.x, a.y + b.y, a.z + b.z }; }
inline float3 operator * (const float3 & a, float b) { return{ a.x*b, a.y*b, a.z*b }; }

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
		if (_map.find(value) != _map.end()) return _map.at(value);

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

		_size = _cache.size();
		_data = _cache.data();
	}

	std::map<float, float3> _map;
	std::vector<float3> _cache;
	float _min, _max;
	size_t _size; float3* _data;
};

static color_map jet{ {
	{ 0, 0, 255 },
	{ 0, 255, 255 },
	{ 255, 255, 0 },
	{ 255, 0, 0 },
	{ 50, 0, 0 },
	} };

static color_map classic{ {
	{ 30, 77, 203 },
	{ 25, 60, 192 },
	{ 45, 117, 220 },
	{ 204, 108, 191 },
	{ 196, 57, 178 },
	{ 198, 33, 24 },
	} };

static color_map grayscale{ {
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	} };

static color_map inv_grayscale{ {
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	} };

static color_map biomes{ {
	{ 0, 0, 204 },
	{ 204, 230, 255 },
	{ 255, 255, 153 },
	{ 170, 255, 128 },
	{ 0, 153, 0 },
	{ 230, 242, 255 },
	} };

static color_map cold{ {
	{ 230, 247, 255 },
	{ 0, 92, 230 },
	{ 0, 179, 179 },
	{ 0, 51, 153 },
	{ 0, 5, 15 }
	} };

static color_map warm{ {
	{ 255, 255, 230 },
	{ 255, 204, 0 },
	{ 255, 136, 77 },
	{ 255, 51, 0 },
	{ 128, 0, 0 },
	{ 10, 0, 0 }
	} };

static color_map quantized{ {
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	}, 6 };

static color_map pattern{ {
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	{ 255, 255, 255 },
	{ 0, 0, 0 },
	} };

static const color_map* colormap_presets[] = {
	&jet,
	&classic,
	&grayscale,
	&inv_grayscale,
	&biomes,
	&cold,
	&warm,
	&quantized,
	&pattern
};

} // namespace
