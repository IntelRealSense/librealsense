// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "core/frame-holder.h"
#include "core/roi.h"
#include "core/options-container.h"
#include "float3.h"

#include <rsutils/concurrency/concurrency.h>
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
        float eps = 0.01f;
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

    // Interface for target calculator
    class target_calculator_interface
    {
    public:
        virtual bool calculate(const uint8_t* img, float* target_dims, unsigned int target_dims_size) = 0;
        virtual ~target_calculator_interface() = default;
    };

    const int _roi_ws = 480;
    const int _roi_we = 800;
    const int _roi_hs = 240;
    const int _roi_he = 480;
    const int _patch_size = 20; // in pixels

    class rect_gaussian_dots_target_calculator : public target_calculator_interface
    {
    public:
        rect_gaussian_dots_target_calculator(int width, int height, int roi_start_x, int roi_start_y, int roi_width, int roi_height);
        virtual ~rect_gaussian_dots_target_calculator();
        bool calculate(const uint8_t* img, float* target_dims, unsigned int target_dims_size) override;

        rect_gaussian_dots_target_calculator(const rect_gaussian_dots_target_calculator&) = delete;
        rect_gaussian_dots_target_calculator& operator=(const rect_gaussian_dots_target_calculator&) = delete;

        rect_gaussian_dots_target_calculator(const rect_gaussian_dots_target_calculator&&) = delete;
        rect_gaussian_dots_target_calculator& operator=(const rect_gaussian_dots_target_calculator&&) = delete;

    protected:
        void normalize(const uint8_t* img);
        void calculate_ncc();

        bool find_corners();
        void refine_corners();
        bool validate_corners(const uint8_t* img);

        void calculate_rect_sides(float* rect_sides);

        void minimize_x(const double* p, int s, double* f, double& x);
        void minimize_y(const double* p, int s, double* f, double& y);
        double subpixel_agj(double* f, int s);

    protected:
        const int _tsize = 28; // template size
        const int _htsize = _tsize >> 1;
        const int _tsize2 = _tsize * _tsize;
        std::vector<double> _imgt;

        const std::vector<double> _template
        {
            -0.02870443, -0.02855973, -0.02855973, -0.02841493, -0.02827013, -0.02812543, -0.02783583, -0.02769113, -0.02725683, -0.02696723, -0.02667773, -0.02624343, -0.02609863, -0.02595393, -0.02580913, -0.02595393, -0.02609863, -0.02624343, -0.02667773, -0.02696723, -0.02725683, -0.02769113, -0.02783583, -0.02812543, -0.02827013, -0.02841493, -0.02855973, -0.02855973,
            -0.02855973, -0.02855973, -0.02841493, -0.02827013, -0.02798063, -0.02769113, -0.02740153, -0.02682253, -0.02624343, -0.02566433, -0.02508533, -0.02465103, -0.02421673, -0.02392713, -0.02378243, -0.02392713, -0.02421673, -0.02465103, -0.02508533, -0.02566433, -0.02624343, -0.02682253, -0.02740153, -0.02769113, -0.02798063, -0.02827013, -0.02841493, -0.02855973,
            -0.02855973, -0.02841493, -0.02827013, -0.02798063, -0.02754633, -0.02711203, -0.02638823, -0.02566433, -0.02479573, -0.02378243, -0.02276903, -0.02190043, -0.02117663, -0.02074233, -0.02059753, -0.02074233, -0.02117663, -0.02190043, -0.02276903, -0.02378243, -0.02479573, -0.02566433, -0.02638823, -0.02711203, -0.02754633, -0.02798063, -0.02827013, -0.02841493,
            -0.02841493, -0.02827013, -0.02798063, -0.02754633, -0.02696723, -0.02609863, -0.02508533, -0.02392713, -0.02247953, -0.02088703, -0.01929463, -0.01799173, -0.01683363, -0.01610973, -0.01582023, -0.01610973, -0.01683363, -0.01799173, -0.01929463, -0.02088703, -0.02247953, -0.02392713, -0.02508533, -0.02609863, -0.02696723, -0.02754633, -0.02798063, -0.02827013,
            -0.02827013, -0.02798063, -0.02754633, -0.02696723, -0.02609863, -0.02479573, -0.02320333, -0.02132133, -0.01914983, -0.01683363, -0.01451733, -0.01234583, -0.01060863, -0.00945043, -0.00916093, -0.00945043, -0.01060863, -0.01234583, -0.01451733, -0.01683363, -0.01914983, -0.02132133, -0.02320333, -0.02479573, -0.02609863, -0.02696723, -0.02754633, -0.02798063,
            -0.02812543, -0.02769113, -0.02711203, -0.02609863, -0.02479573, -0.02305853, -0.02074233, -0.01799173, -0.01480683, -0.01133243, -0.00785803, -0.00481793, -0.00221213, -0.00061973, -0.00004063, -0.00061973, -0.00221213, -0.00481793, -0.00785803, -0.01133243, -0.01480683, -0.01799173, -0.02074233, -0.02305853, -0.02479573, -0.02609863, -0.02711203, -0.02769113,
            -0.02783583, -0.02740153, -0.02638823, -0.02508533, -0.02320333, -0.02074233, -0.01755743, -0.01364873, -0.00916093, -0.00423883,  0.00053847,  0.00488147,  0.00850057,  0.01081687,  0.01154077,  0.01081687,  0.00850057,  0.00488147,  0.00053847, -0.00423883, -0.00916093, -0.01364873, -0.01755743, -0.02074233, -0.02320333, -0.02508533, -0.02638823, -0.02740153,
            -0.02769113, -0.02682253, -0.02566433, -0.02392713, -0.02132133, -0.01799173, -0.01364873, -0.00829233, -0.00221213,  0.00430237,  0.01081687,  0.01660757,  0.02138487,  0.02456977,  0.02558307,  0.02456977,  0.02138487,  0.01660757,  0.01081687,  0.00430237, -0.00221213, -0.00829233, -0.01364873, -0.01799173, -0.02132133, -0.02392713, -0.02566433, -0.02682253,
            -0.02725683, -0.02624343, -0.02479573, -0.02247953, -0.01914983, -0.01480683, -0.00916093, -0.00221213,  0.00560527,  0.01400177,  0.02239827,  0.03021567,  0.03629587,  0.04034937,  0.04179697,  0.04034937,  0.03629587,  0.03021567,  0.02239827,  0.01400177,  0.00560527, -0.00221213, -0.00916093, -0.01480683, -0.01914983, -0.02247953, -0.02479573, -0.02624343,
            -0.02696723, -0.02566433, -0.02378243, -0.02088703, -0.01683363, -0.01133243, -0.00423883,  0.00430237,  0.01400177,  0.02456977,  0.03499297,  0.04469237,  0.05236497,  0.05743187,  0.05916907,  0.05743187,  0.05236497,  0.04469237,  0.03499297,  0.02456977,  0.01400177,  0.00430237, -0.00423883, -0.01133243, -0.01683363, -0.02088703, -0.02378243, -0.02566433,
            -0.02667773, -0.02508533, -0.02276903, -0.01929463, -0.01451733, -0.00785803,  0.00053847,  0.01081687,  0.02239827,  0.03499297,  0.04758767,  0.05916907,  0.06828937,  0.07436957,  0.07639627,  0.07436957,  0.06828937,  0.05916907,  0.04758767,  0.03499297,  0.02239827,  0.01081687,  0.00053847, -0.00785803, -0.01451733, -0.01929463, -0.02276903, -0.02508533,
            -0.02624343, -0.02465103, -0.02190043, -0.01799173, -0.01234583, -0.00481793,  0.00488147,  0.01660757,  0.03021567,  0.04469237,  0.05916907,  0.07234287,  0.08291078,  0.08985968,  0.09217588,  0.08985968,  0.08291078,  0.07234287,  0.05916907,  0.04469237,  0.03021567,  0.01660757,  0.00488147, -0.00481793, -0.01234583, -0.01799173, -0.02190043, -0.02465103,
            -0.02609863, -0.02421673, -0.02117663, -0.01683363, -0.01060863, -0.00221213,  0.00850057,  0.02138487,  0.03629587,  0.05236497,  0.06828937,  0.08291078,  0.09463698,  0.10230958,  0.10491538,  0.10230958,  0.09463698,  0.08291078,  0.06828937,  0.05236497,  0.03629587,  0.02138487,  0.00850057, -0.00221213, -0.01060863, -0.01683363, -0.02117663, -0.02421673,
            -0.02595393, -0.02392713, -0.02074233, -0.01610973, -0.00945043, -0.00061973,  0.01081687,  0.02456977,  0.04034937,  0.05743187,  0.07436957,  0.08985968,  0.10230958,  0.11041658,  0.11316708,  0.11041658,  0.10230958,  0.08985968,  0.07436957,  0.05743187,  0.04034937,  0.02456977,  0.01081687, -0.00061973, -0.00945043, -0.01610973, -0.02074233, -0.02392713,
            -0.02580913, -0.02378243, -0.02059753, -0.01582023, -0.00916093, -0.00004063,  0.01154077,  0.02558307,  0.04179697,  0.05916907,  0.07639627,  0.09217588,  0.10491538,  0.11316708,  0.11606248,  0.11316708,  0.10491538,  0.09217588,  0.07639627,  0.05916907,  0.04179697,  0.02558307,  0.01154077, -0.00004063, -0.00916093, -0.01582023, -0.02059753, -0.02378243,
            -0.02595393, -0.02392713, -0.02074233, -0.01610973, -0.00945043, -0.00061973,  0.01081687,  0.02456977,  0.04034937,  0.05743187,  0.07436957,  0.08985968,  0.10230958,  0.11041658,  0.11316708,  0.11041658,  0.10230958,  0.08985968,  0.07436957,  0.05743187,  0.04034937,  0.02456977,  0.01081687, -0.00061973, -0.00945043, -0.01610973, -0.02074233, -0.02392713,
            -0.02609863, -0.02421673, -0.02117663, -0.01683363, -0.01060863, -0.00221213,  0.00850057,  0.02138487,  0.03629587,  0.05236497,  0.06828937,  0.08291078,  0.09463698,  0.10230958,  0.10491538,  0.10230958,  0.09463698,  0.08291078,  0.06828937,  0.05236497,  0.03629587,  0.02138487,  0.00850057, -0.00221213, -0.01060863, -0.01683363, -0.02117663, -0.02421673,
            -0.02624343, -0.02465103, -0.02190043, -0.01799173, -0.01234583, -0.00481793,  0.00488147,  0.01660757,  0.03021567,  0.04469237,  0.05916907,  0.07234287,  0.08291078,  0.08985968,  0.09217588,  0.08985968,  0.08291078,  0.07234287,  0.05916907,  0.04469237,  0.03021567,  0.01660757,  0.00488147, -0.00481793, -0.01234583, -0.01799173, -0.02190043, -0.02465103,
            -0.02667773, -0.02508533, -0.02276903, -0.01929463, -0.01451733, -0.00785803,  0.00053847,  0.01081687,  0.02239827,  0.03499297,  0.04758767,  0.05916907,  0.06828937,  0.07436957,  0.07639627,  0.07436957,  0.06828937,  0.05916907,  0.04758767,  0.03499297,  0.02239827,  0.01081687,  0.00053847, -0.00785803, -0.01451733, -0.01929463, -0.02276903, -0.02508533,
            -0.02696723, -0.02566433, -0.02378243, -0.02088703, -0.01683363, -0.01133243, -0.00423883,  0.00430237,  0.01400177,  0.02456977,  0.03499297,  0.04469237,  0.05236497,  0.05743187,  0.05916907,  0.05743187,  0.05236497,  0.04469237,  0.03499297,  0.02456977,  0.01400177,  0.00430237, -0.00423883, -0.01133243, -0.01683363, -0.02088703, -0.02378243, -0.02566433,
            -0.02725683, -0.02624343, -0.02479573, -0.02247953, -0.01914983, -0.01480683, -0.00916093, -0.00221213,  0.00560527,  0.01400177,  0.02239827,  0.03021567,  0.03629587,  0.04034937,  0.04179697,  0.04034937,  0.03629587,  0.03021567,  0.02239827,  0.01400177,  0.00560527, -0.00221213, -0.00916093, -0.01480683, -0.01914983, -0.02247953, -0.02479573, -0.02624343,
            -0.02769113, -0.02682253, -0.02566433, -0.02392713, -0.02132133, -0.01799173, -0.01364873, -0.00829233, -0.00221213,  0.00430237,  0.01081687,  0.01660757,  0.02138487,  0.02456977,  0.02558307,  0.02456977,  0.02138487,  0.01660757,  0.01081687,  0.00430237, -0.00221213, -0.00829233, -0.01364873, -0.01799173, -0.02132133, -0.02392713, -0.02566433, -0.02682253,
            -0.02783583, -0.02740153, -0.02638823, -0.02508533, -0.02320333, -0.02074233, -0.01755743, -0.01364873, -0.00916093, -0.00423883,  0.00053847,  0.00488147,  0.00850057,  0.01081687,  0.01154077,  0.01081687,  0.00850057,  0.00488147,  0.00053847, -0.00423883, -0.00916093, -0.01364873, -0.01755743, -0.02074233, -0.02320333, -0.02508533, -0.02638823, -0.02740153,
            -0.02812543, -0.02769113, -0.02711203, -0.02609863, -0.02479573, -0.02305853, -0.02074233, -0.01799173, -0.01480683, -0.01133243, -0.00785803, -0.00481793, -0.00221213, -0.00061973, -0.00004063, -0.00061973, -0.00221213, -0.00481793, -0.00785803, -0.01133243, -0.01480683, -0.01799173, -0.02074233, -0.02305853, -0.02479573, -0.02609863, -0.02711203, -0.02769113,
            -0.02827013, -0.02798063, -0.02754633, -0.02696723, -0.02609863, -0.02479573, -0.02320333, -0.02132133, -0.01914983, -0.01683363, -0.01451733, -0.01234583, -0.01060863, -0.00945043, -0.00916093, -0.00945043, -0.01060863, -0.01234583, -0.01451733, -0.01683363, -0.01914983, -0.02132133, -0.02320333, -0.02479573, -0.02609863, -0.02696723, -0.02754633, -0.02798063,
            -0.02841493, -0.02827013, -0.02798063, -0.02754633, -0.02696723, -0.02609863, -0.02508533, -0.02392713, -0.02247953, -0.02088703, -0.01929463, -0.01799173, -0.01683363, -0.01610973, -0.01582023, -0.01610973, -0.01683363, -0.01799173, -0.01929463, -0.02088703, -0.02247953, -0.02392713, -0.02508533, -0.02609863, -0.02696723, -0.02754633, -0.02798063, -0.02827013,
            -0.02855973, -0.02841493, -0.02827013, -0.02798063, -0.02754633, -0.02711203, -0.02638823, -0.02566433, -0.02479573, -0.02378243, -0.02276903, -0.02190043, -0.02117663, -0.02074233, -0.02059753, -0.02074233, -0.02117663, -0.02190043, -0.02276903, -0.02378243, -0.02479573, -0.02566433, -0.02638823, -0.02711203, -0.02754633, -0.02798063, -0.02827013, -0.02841493,
            -0.02855973, -0.02855973, -0.02841493, -0.02827013, -0.02798063, -0.02769113, -0.02740153, -0.02682253, -0.02624343, -0.02566433, -0.02508533, -0.02465103, -0.02421673, -0.02392713, -0.02378243, -0.02392713, -0.02421673, -0.02465103, -0.02508533, -0.02566433, -0.02624343, -0.02682253, -0.02740153, -0.02769113, -0.02798063, -0.02827013, -0.02841493, -0.02855973,
        };

        const double _thresh = 0.7; // used internally, range from 0 to 1 for normalized image ma
        std::vector<double> _buf;

        std::vector<double> _img;
        std::vector<double> _ncc;
        int _width = 0;
        int _height = 0;
        int _size = 0;

        int _wt = 0;
        int _ht = 0;
        int _hwidth;
        int _hheight;

        template <typename T>
        struct point
        {
            T x;
            T y;
        };

        point<double> _corners[4];
        point<int> _pts[4];

        int _roi_start_x;
        int _roi_start_y;
        int _full_width;
        int _full_height;
    };

    // Utility class for calculating the rectangle sides on the specific target
    class rect_calculator
    {
    public:
        rect_calculator(bool roi = false) : _roi(roi) {}
        virtual ~rect_calculator() {}

        // return 1 if target found, zero otherwise
        int extract_target_dims(const rs2_frame* frame_ref, float4& rect_sides);

    private:
        bool _roi = false;
    };
}
