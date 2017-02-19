// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "algo.h"
#include "option.h"

using namespace rsimpl;


bool auto_exposure_state::get_enable_auto_exposure() const
{
    return is_auto_exposure;
}

auto_exposure_modes auto_exposure_state::get_auto_exposure_mode() const
{
    return mode;
}

unsigned auto_exposure_state::get_auto_exposure_antiflicker_rate() const
{
    return rate;
}

void auto_exposure_state::set_enable_auto_exposure(bool value)
{
    is_auto_exposure = value;
}

void auto_exposure_state::set_auto_exposure_mode(auto_exposure_modes value)
{
    mode = value;
}

void auto_exposure_state::set_auto_exposure_antiflicker_rate(unsigned value)
{
    rate = value;
}


auto_exposure_mechanism::auto_exposure_mechanism(std::shared_ptr<uvc_endpoint> dev, auto_exposure_state auto_exposure_state)
    : _device(dev), _auto_exposure_algo(auto_exposure_state),
      _keep_alive(true), _frames_counter(0),
      _skip_frames(auto_exposure_state.skip_frames), _data_queue(queue_size)
{
    _exposure_thread = std::make_shared<std::thread>([this]() {
        while (_keep_alive)
        {
            std::unique_lock<std::mutex> lk(_queue_mtx);
            _cv.wait(lk, [&] {return (_data_queue.size() || !_keep_alive); });
            if (!_keep_alive)
                return;


            rs_frame* frame_ref = nullptr;
            auto frame_sts = try_pop_front_data(&frame_ref);
            lk.unlock();

            double values[2] = {};
            try {
                if (frame_ref->get()->supports_frame_metadata(RS_FRAME_METADATA_ACTUAL_EXPOSURE))
                {
                    auto gain = _device->get_option(RS_OPTION_GAIN).query();
                    values[0] = frame_ref->get()->get_frame_metadata(RS_FRAME_METADATA_ACTUAL_EXPOSURE);
                    values[1] = gain;
                }
                else
                {
                    values[0] = _device->get_option(RS_OPTION_EXPOSURE).query();
                    values[1] = _device->get_option(RS_OPTION_GAIN).query();
                }

                values[0] /= 10.; // Fisheye exposure value by extension control is in units of 10 mSec
            }
            catch (...) {};

            if (frame_sts)
            {
                auto exposure_value = static_cast<float>(values[0]);
                auto gain_value = static_cast<float>(2. + (values[1] - 15.) / 8.);

                bool sts = _auto_exposure_algo.analyze_image(frame_ref);
                if (sts)
                {
                    bool modify_exposure, modify_gain;
                    _auto_exposure_algo.modify_exposure(exposure_value, modify_exposure, gain_value, modify_gain);

                    if (modify_exposure)
                    {
                        auto value = exposure_value * 10.;
                        if (value < 1)
                            value = 1;

                        auto& opt = _device->get_option(RS_OPTION_EXPOSURE);
                        opt.set(value);
                    }

                    if (modify_gain)
                    {
                        auto value =  (gain_value - 2.) * 8. + 15. ;
                        auto& opt = _device->get_option(RS_OPTION_GAIN);
                        opt.set(value);
                    }
                }
            }
            frame_ref->get()->get_owner()->release_frame_ref(frame_ref);
        }
    });
}

auto_exposure_mechanism::~auto_exposure_mechanism()
{
    {
        std::lock_guard<std::mutex> lk(_queue_mtx);
        _keep_alive = false;
    }
    _cv.notify_one();
    _exposure_thread->join();
}

void auto_exposure_mechanism::update_auto_exposure_state(auto_exposure_state& auto_exposure_state)
{
    std::lock_guard<std::mutex> lk(_queue_mtx);
    _skip_frames = auto_exposure_state.skip_frames;
    _auto_exposure_algo.update_options(auto_exposure_state);
}

void auto_exposure_mechanism::add_frame(rs_frame* frame)
{
    if (!_keep_alive || (_skip_frames && (_frames_counter++) != _skip_frames))
    {
        frame->get()->get_owner()->release_frame_ref(frame);
        return;
    }

    _frames_counter = 0;

    {
        std::lock_guard<std::mutex> lk(_queue_mtx);
        if (_data_queue.size() > 1)
        {
            _data_queue.dequeue();
        }

        push_back_data(frame);
    }
    _cv.notify_one();
}

void auto_exposure_mechanism::push_back_data(rs_frame* data)
{
    frame_holder fh;
    fh.frame = data;
    _data_queue.enqueue(std::move(fh));
}

bool auto_exposure_mechanism::try_pop_front_data(rs_frame** data)
{
    if (!_data_queue.size())
        return false;

    auto fh = _data_queue.dequeue();
    rs_frame* result = nullptr;
    std::swap(result, fh.frame);
    *data = result;

    return true;
}

auto_exposure_algorithm::auto_exposure_algorithm(auto_exposure_state auto_exposure_state)
{
    update_options(auto_exposure_state);
}

void auto_exposure_algorithm::modify_exposure(float& exposure_value, bool& exp_modified, float& gain_value, bool& gain_modified)
{
    float total_exposure = exposure * gain;
    LOG_DEBUG("TotalExposure " << total_exposure << ", target_exposure " << target_exposure);
    if (fabs(target_exposure - total_exposure) > eps)
    {
        rounding_mode_type RoundingMode;

        if (target_exposure > total_exposure)
        {
            float target_exposure0 = total_exposure * (1.0f + exposure_step);

            target_exposure0 = std::min(target_exposure0, target_exposure);
            increase_exposure_gain(target_exposure0, target_exposure0, exposure, gain);
            RoundingMode = rounding_mode_type::ceil;
            LOG_DEBUG(" ModifyExposure: IncreaseExposureGain: ");
            LOG_DEBUG(" target_exposure0 " << target_exposure0);
        }
        else
        {
            float target_exposure0 = total_exposure / (1.0f + exposure_step);

            target_exposure0 = std::max(target_exposure0, target_exposure);
            decrease_exposure_gain(target_exposure0, target_exposure0, exposure, gain);
            RoundingMode = rounding_mode_type::floor;
            LOG_DEBUG(" ModifyExposure: DecreaseExposureGain: ");
            LOG_DEBUG(" target_exposure0 " << target_exposure0);
        }
        LOG_DEBUG(" exposure " << exposure << ", gain " << gain);
        if (exposure_value != exposure)
        {
            exp_modified = true;
            exposure_value = exposure;
            exposure_value = exposure_to_value(exposure_value, RoundingMode);
            LOG_DEBUG("output exposure by algo = " << exposure_value);
        }
        if (gain_value != gain)
        {
            gain_modified = true;
            gain_value = gain;
            LOG_DEBUG("GainModified: gain = " << gain);
            gain_value = gain_to_value(gain_value, RoundingMode);
            LOG_DEBUG(" rounded to: " << gain);
        }
    }
}

bool auto_exposure_algorithm::analyze_image(const rs_frame* image)
{
    int cols = image->get()->get_width();
    int rows = image->get()->get_height();

    const int number_of_pixels = cols * rows; //VGA
    if (number_of_pixels == 0)  return false;   // empty image

    std::vector<int> H(256);
    int total_weight = number_of_pixels;

    im_hist((uint8_t*)image->get()->get_frame_data(), cols, rows, image->get()->get_bpp() / 8 * cols, &H[0]);

    histogram_metric score = {};
    histogram_score(H, total_weight, score);
    // int EffectiveDynamicRange = (score.highlight_limit - score.shadow_limit);
    ///
    float s1 = (score.main_mean - 128.0f) / 255.0f;
    float s2 = 0;

    s2 = (score.over_exposure_count - score.under_exposure_count) / (float)total_weight;

    float s = -0.3f * (s1 + 5.0f * s2);
    LOG_DEBUG(" AnalyzeImage Score: " << s);

    if (s > 0)
    {
        direction = +1;
        increase_exposure_target(s, target_exposure);
    }
    else
    {
        LOG_DEBUG(" AnalyzeImage: DecreaseExposure");
        direction = -1;
        decrease_exposure_target(s, target_exposure);
    }

    if (fabs(1.0f - (exposure * gain) / target_exposure) < hysteresis)
    {
        LOG_DEBUG(" AnalyzeImage: Don't Modify (Hysteresis): " << target_exposure << " " << exposure * gain);
        return false;
    }

    prev_direction = direction;
    LOG_DEBUG(" AnalyzeImage: Modify");
    return true;
}

void auto_exposure_algorithm::update_options(const auto_exposure_state& options)
{
    std::lock_guard<std::recursive_mutex> lock(state_mutex);

    state = options;
    flicker_cycle = 1000.0f / (state.get_auto_exposure_antiflicker_rate() * 2.0f);
}

void auto_exposure_algorithm::im_hist(const uint8_t* data, const int width, const int height, const int rowStep, int h[])
{
    std::lock_guard<std::recursive_mutex> lock(state_mutex);

    for (int i = 0; i < 256; ++i) h[i] = 0;
    const uint8_t* rowData = data;
    for (int i = 0; i < height; ++i, rowData += rowStep) for (int j = 0; j < width; j+=state.sample_rate) ++h[rowData[j]];
}

void auto_exposure_algorithm::increase_exposure_target(float mult, float& target_exposure)
{
    target_exposure = std::min((exposure * gain) * (1.0f + mult), maximal_exposure * gain_limit);
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void auto_exposure_algorithm::decrease_exposure_target(float mult, float& target_exposure)
{
    target_exposure = std::max((exposure * gain) * (1.0f + mult), minimal_exposure * base_gain);
}

void auto_exposure_algorithm::increase_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain)
{
    std::lock_guard<std::recursive_mutex> lock(state_mutex);

    switch ((int)(state.get_auto_exposure_mode()))
    {
    case int(auto_exposure_modes::static_auto_exposure):          static_increase_exposure_gain(target_exposure, target_exposure0, exposure, gain); break;
    case int(auto_exposure_modes::auto_exposure_anti_flicker):    anti_flicker_increase_exposure_gain(target_exposure, target_exposure0, exposure, gain); break;
    case int(auto_exposure_modes::auto_exposure_hybrid):          hybrid_increase_exposure_gain(target_exposure, target_exposure0, exposure, gain); break;
    }
}
void auto_exposure_algorithm::decrease_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain)
{
    std::lock_guard<std::recursive_mutex> lock(state_mutex);

    switch ((int)(state.get_auto_exposure_mode()))
    {
    case int(auto_exposure_modes::static_auto_exposure):          static_decrease_exposure_gain(target_exposure, target_exposure0, exposure, gain); break;
    case int(auto_exposure_modes::auto_exposure_anti_flicker):    anti_flicker_decrease_exposure_gain(target_exposure, target_exposure0, exposure, gain); break;
    case int(auto_exposure_modes::auto_exposure_hybrid):          hybrid_decrease_exposure_gain(target_exposure, target_exposure0, exposure, gain); break;
    }
}
void auto_exposure_algorithm::static_increase_exposure_gain(const float& /*target_exposure*/, const float& target_exposure0, float& exposure, float& gain)
{
    exposure = std::max(minimal_exposure, std::min(target_exposure0 / base_gain, maximal_exposure));
    gain = std::min(gain_limit, std::max(target_exposure0 / exposure, base_gain));
}
void auto_exposure_algorithm::static_decrease_exposure_gain(const float& /*target_exposure*/, const float& target_exposure0, float& exposure, float& gain)
{
    exposure = std::max(minimal_exposure, std::min(target_exposure0 / base_gain, maximal_exposure));
    gain = std::min(gain_limit, std::max(target_exposure0 / exposure, base_gain));
}
void auto_exposure_algorithm::anti_flicker_increase_exposure_gain(const float& target_exposure, const float& /*target_exposure0*/, float& exposure, float& gain)
{
    std::vector< std::tuple<float, float, float> > exposure_gain_score;

    for (int i = 1; i < 4; ++i)
    {
        if (i * flicker_cycle >= maximal_exposure)
        {
            continue;
        }
        float exposure1 = std::max(std::min(i * flicker_cycle, maximal_exposure), flicker_cycle);
        float gain1 = base_gain;

        if ((exposure1 * gain1) != target_exposure)
        {
            gain1 = std::min(std::max(target_exposure / exposure1, base_gain), gain_limit);
        }
        float score1 = fabs(target_exposure - exposure1 * gain1);
        exposure_gain_score.push_back(std::tuple<float, float, float>(score1, exposure1, gain1));
    }

    std::sort(exposure_gain_score.begin(), exposure_gain_score.end());

    exposure = std::get<1>(exposure_gain_score.front());
    gain = std::get<2>(exposure_gain_score.front());
}
void auto_exposure_algorithm::anti_flicker_decrease_exposure_gain(const float& target_exposure, const float& /*target_exposure0*/, float& exposure, float& gain)
{
    std::vector< std::tuple<float, float, float> > exposure_gain_score;

    for (int i = 1; i < 4; ++i)
    {
        if (i * flicker_cycle >= maximal_exposure)
        {
            continue;
        }
        float exposure1 = std::max(std::min(i * flicker_cycle, maximal_exposure), flicker_cycle);
        float gain1 = base_gain;
        if ((exposure1 * gain1) != target_exposure)
        {
            gain1 = std::min(std::max(target_exposure / exposure1, base_gain), gain_limit);
        }
        float score1 = fabs(target_exposure - exposure1 * gain1);
        exposure_gain_score.push_back(std::tuple<float, float, float>(score1, exposure1, gain1));
    }

    std::sort(exposure_gain_score.begin(), exposure_gain_score.end());

    exposure = std::get<1>(exposure_gain_score.front());
    gain = std::get<2>(exposure_gain_score.front());
}
void auto_exposure_algorithm::hybrid_increase_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain)
{
    if (anti_flicker_mode)
    {
        anti_flicker_increase_exposure_gain(target_exposure, target_exposure0, exposure, gain);
    }
    else
    {
        static_increase_exposure_gain(target_exposure, target_exposure0, exposure, gain);
        LOG_DEBUG("HybridAutoExposure::IncreaseExposureGain: " << exposure * gain << " " << flicker_cycle * base_gain << " " << base_gain);
        if (target_exposure > 0.99 * flicker_cycle * base_gain)
        {
            anti_flicker_mode = true;
            anti_flicker_increase_exposure_gain(target_exposure, target_exposure0, exposure, gain);
            LOG_DEBUG("anti_flicker_mode = true");
        }
    }
}
void auto_exposure_algorithm::hybrid_decrease_exposure_gain(const float& target_exposure, const float& target_exposure0, float& exposure, float& gain)
{
    if (anti_flicker_mode)
    {
        LOG_DEBUG("HybridAutoExposure::DecreaseExposureGain: " << exposure << " " << flicker_cycle << " " << gain << " " << base_gain);
        if ((target_exposure) <= 0.99 * (flicker_cycle * base_gain))
        {
            anti_flicker_mode = false;
            static_decrease_exposure_gain(target_exposure, target_exposure0, exposure, gain);
            LOG_DEBUG("anti_flicker_mode = false");
        }
        else
        {
            anti_flicker_decrease_exposure_gain(target_exposure, target_exposure0, exposure, gain);
        }
    }
    else
    {
        static_decrease_exposure_gain(target_exposure, target_exposure0, exposure, gain);
    }
}

float auto_exposure_algorithm::exposure_to_value(float exp_ms, rounding_mode_type rounding_mode)
{
    const float line_period_us = 19.33333333f;

    float ExposureTimeLine = (exp_ms * 1000.0f / line_period_us);
    if (rounding_mode == rounding_mode_type::ceil) ExposureTimeLine = std::ceil(ExposureTimeLine);
    else if (rounding_mode == rounding_mode_type::floor) ExposureTimeLine = std::floor(ExposureTimeLine);
    else ExposureTimeLine = round(ExposureTimeLine);
    return ((float)ExposureTimeLine * line_period_us) / 1000.0f;
}

float auto_exposure_algorithm::gain_to_value(float gain, rounding_mode_type rounding_mode)
{

    if (gain < base_gain) { return base_gain; }
    else if (gain > 16.0f) { return 16.0f; }
    else {
        if (rounding_mode == rounding_mode_type::ceil) return std::ceil(gain * 8.0f) / 8.0f;
        else if (rounding_mode == rounding_mode_type::floor) return std::floor(gain * 8.0f) / 8.0f;
        else return round(gain * 8.0f) / 8.0f;
    }
}

template <typename T> inline T sqr(const T& x) { return (x*x); }
void auto_exposure_algorithm::histogram_score(std::vector<int>& h, const int total_weight, histogram_metric& score)
{
    score.under_exposure_count = 0;
    score.over_exposure_count = 0;

    for (size_t i = 0; i <= under_exposure_limit; ++i)
    {
        score.under_exposure_count += h[i];
    }
    score.shadow_limit = 0;
    //if (Score.UnderExposureCount < UnderExposureNoiseLimit)
    {
        score.shadow_limit = under_exposure_limit;
        for (size_t i = under_exposure_limit + 1; i <= over_exposure_limit; ++i)
        {
            if (h[i] > under_exposure_noise_limit)
            {
                break;
            }
            score.shadow_limit++;
        }
        int lower_q = 0;
        score.lower_q = 0;
        for (size_t i = under_exposure_limit + 1; i <= over_exposure_limit; ++i)
        {
            lower_q += h[i];
            if (lower_q > total_weight / 4)
            {
                break;
            }
            score.lower_q++;
        }
    }

    for (size_t i = over_exposure_limit; i <= 255; ++i)
    {
        score.over_exposure_count += h[i];
    }

    score.highlight_limit = 255;
    //if (Score.OverExposureCount < OverExposureNoiseLimit)
    {
        score.highlight_limit = over_exposure_limit;
        for (size_t i = over_exposure_limit; i >= under_exposure_limit; --i)
        {
            if (h[i] > over_exposure_noise_limit)
            {
                break;
            }
            score.highlight_limit--;
        }
        int upper_q = 0;
        score.upper_q = over_exposure_limit;
        for (size_t i = over_exposure_limit; i >= under_exposure_limit; --i)
        {
            upper_q += h[i];
            if (upper_q > total_weight / 4)
            {
                break;
            }
            score.upper_q--;
        }

    }
    int32_t m1 = 0;
    int64_t m2 = 0;

    double nn = (double)total_weight - score.under_exposure_count - score.over_exposure_count;
    if (nn == 0)
    {
        nn = (double)total_weight;
        for (int i = 0; i <= 255; ++i)
        {
            m1 += h[i] * i;
            m2 += h[i] * sqr(i);
        }
    }
    else
    {
        for (int i = under_exposure_limit + 1; i < over_exposure_limit; ++i)
        {
            m1 += h[i] * i;
            m2 += h[i] * sqr(i);
        }
    }
    score.main_mean = (float)((double)m1 / nn);
    double Var = (double)m2 / nn - sqr((double)m1 / nn);
    if (Var > 0)
    {
        score.main_std = (float)sqrt(Var);
    }
    else
    {
        score.main_std = 0.0f;
    }
}
