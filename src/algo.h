// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#include "sensor.h"
#include "concurrency.h"
#include "types.h"

#include <stdint.h>
#include <vector>
#include <mutex>
#include <deque>
#include <cmath>
#include <memory>

namespace librealsense
{
    static const float ae_step_default_value = 0.5f;

    enum class auto_exposure_modes {
        static_auto_exposure = 0,
        auto_exposure_anti_flicker,
        auto_exposure_hybrid
    };

    class auto_exposure_state
    {
    public:
        auto_exposure_state() :
            is_auto_exposure(true),
            mode(auto_exposure_modes::auto_exposure_hybrid),
            rate(60),
            step(ae_step_default_value)
        {}

        bool get_enable_auto_exposure() const;
        auto_exposure_modes get_auto_exposure_mode() const;
        unsigned get_auto_exposure_antiflicker_rate() const;
        float get_auto_exposure_step() const;

        void set_enable_auto_exposure(bool value);
        void set_auto_exposure_mode(auto_exposure_modes value);
        void set_auto_exposure_antiflicker_rate(unsigned value);
        void set_auto_exposure_step(float value);

        static const unsigned      sample_rate = 1;
        static const unsigned      skip_frames = 2;

    private:
        bool                is_auto_exposure;
        auto_exposure_modes mode;
        unsigned            rate;
        float               step;
    };


    class auto_exposure_algorithm {
    public:
        void modify_exposure(float& exposure_value, bool& exp_modified, float& gain_value, bool& gain_modified); // exposure_value in milliseconds
        bool analyze_image(const frame_interface* image);
        auto_exposure_algorithm(const auto_exposure_state& auto_exposure_state);
        void update_options(const auto_exposure_state& options);
        void update_roi(const region_of_interest& ae_roi);

    private:
        struct histogram_metric { int under_exposure_count; int over_exposure_count; int shadow_limit; int highlight_limit; int lower_q; int upper_q; float main_mean; float main_std; };
        enum class rounding_mode_type { round, ceil, floor };

        inline void im_hist(const uint8_t* data, const region_of_interest& image_roi, const int rowStep, int h[]);
        void increase_exposure_target(float mult, float& target_exposure);
        void decrease_exposure_target(float mult, float& target_exposure);
        void increase_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain);
        void decrease_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain);
        void static_increase_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain);
        void static_decrease_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain);
        void anti_flicker_increase_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain);
        void anti_flicker_decrease_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain);
        void hybrid_increase_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain);
        void hybrid_decrease_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain);

    #if defined(_WINDOWS) || defined(WIN32) || defined(WIN64)
        inline float round(float x) { return std::round(x); }
    #else
        inline float round(float x) { return x < 0.0 ? std::ceil(x - 0.5f) : std::floor(x + 0.5f); }
    #endif

        float exposure_to_value(float exp_ms, rounding_mode_type rounding_mode);
        float gain_to_value(float gain, rounding_mode_type rounding_mode);
        template <typename T> inline T sqr(const T& x) { return (x*x); }
        void histogram_score(std::vector<int>& h, const int total_weight, histogram_metric& score);


        float minimal_exposure = 0.2f, maximal_exposure = 20.f, base_gain = 2.0f, gain_limit = 15.0f;
        float exposure = 10.0f, gain = 2.0f, target_exposure = 0.0f;
        uint8_t under_exposure_limit = 5, over_exposure_limit = 250; int under_exposure_noise_limit = 50, over_exposure_noise_limit = 50;
        int direction = 0, prev_direction = 0; float hysteresis = 0.075f;// 05;
        float eps = 0.01f, minimal_exposure_step = 0.01f;
        std::atomic<float> exposure_step;
        auto_exposure_state state; float flicker_cycle; bool anti_flicker_mode = true;
        region_of_interest roi{};
        bool is_roi_initialized = false;
        std::recursive_mutex state_mutex;
    };

    class auto_exposure_mechanism {
    public:
        auto_exposure_mechanism(option& gain_option, option& exposure_option, const auto_exposure_state& auto_exposure_state);
        ~auto_exposure_mechanism();
        void add_frame(frame_holder frame);
        void update_auto_exposure_state(const auto_exposure_state& auto_exposure_state);
        void update_auto_exposure_roi(const region_of_interest& roi);

        struct exposure_and_frame_counter {
            exposure_and_frame_counter()
                : exposure(0), frame_counter(0)
            {}

            exposure_and_frame_counter(double exposure, unsigned long long frame_counter)
                : exposure(exposure), frame_counter(frame_counter)
            {}

            double exposure;
            unsigned long long frame_counter;
        };

    private:
        static const int                          queue_size = 2;
        option&                                   _gain_option;
        option&                                   _exposure_option;
        auto_exposure_algorithm                   _auto_exposure_algo;
        std::shared_ptr<std::thread>              _exposure_thread;
        std::condition_variable                   _cv;
        std::atomic<bool>                         _keep_alive;
        single_consumer_queue<frame_holder>       _data_queue;
        std::mutex                                _queue_mtx;
        std::atomic<unsigned>                     _frames_counter;
        std::atomic<unsigned>                     _skip_frames;
    };

}
