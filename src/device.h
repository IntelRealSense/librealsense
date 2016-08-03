// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_DEVICE_H
#define LIBREALSENSE_DEVICE_H

#include "uvc.h"
#include "stream.h"
#include <chrono>
#include <deque>
#include "sync.h"
#include <algorithm>

namespace rsimpl
{
    // This class is used to buffer up several writes to a structure-valued XU control, and send the entire structure all at once
    // Additionally, it will ensure that any fields not set in a given struct will retain their original values
    template<class T, class R, class W> struct struct_interface
    {
        T struct_;
        R reader;
        W writer;
        bool active;

        struct_interface(R r, W w) : reader(r), writer(w), active(false) {}

        void activate() { if (!active) { struct_ = reader(); active = true; } }
        template<class U> double get(U T::* field) { activate(); return static_cast<double>(struct_.*field); }
        template<class U, class V> void set(U T::* field, V value) { activate(); struct_.*field = static_cast<U>(value); }
        void commit() { if (active) writer(struct_); }
    };

    template<class T, class R, class W> struct_interface<T, R, W> make_struct_interface(R r, W w) { return{ r,w }; }

    template <typename T>
    class wraparound_mechanism
    {
    public:
        wraparound_mechanism(T min_value, T max_value)
            : max_number(max_value - min_value + 1), last_number(min_value), num_of_wraparounds(0)
        {}

        T fix(T number)
        {
            if ((number + (num_of_wraparounds*max_number)) < last_number)
                ++num_of_wraparounds;


            number += (num_of_wraparounds*max_number);
            last_number = number;
            return number;
        }

    private:
        unsigned long long num_of_wraparounds;
        T max_number;
        T last_number;
    };

    struct frame_timestamp_reader
    {
        virtual bool validate_frame(const subdevice_mode & mode, const void * frame) const = 0;
        virtual double get_frame_timestamp(const subdevice_mode & mode, const void * frame) = 0;
        virtual unsigned long long get_frame_counter(const subdevice_mode & mode, const void * frame) = 0;
    };


    namespace motion_module
    {
        struct motion_module_parser;
    }

}

    struct rs_device_base : rs_device
    {
    private:
        const std::shared_ptr<rsimpl::uvc::device>  device;
    protected:
        rsimpl::device_config                       config;
    private:
        rsimpl::native_stream                       depth, color, infrared, infrared2, fisheye;
        rsimpl::point_stream                        points;
        rsimpl::rectified_stream                    rect_color;
        rsimpl::aligned_stream                      color_to_depth, depth_to_color, depth_to_rect_color, infrared2_to_depth, depth_to_infrared2;
        rsimpl::native_stream *                     native_streams[RS_STREAM_NATIVE_COUNT];
        rsimpl::stream_interface *                  streams[RS_STREAM_COUNT];

        bool                                        capturing;
        bool                                        data_acquisition_active;
        std::chrono::high_resolution_clock::time_point capture_started;
        std::atomic<uint32_t>                       max_publish_list_size;
        std::atomic<uint32_t>                       event_queue_size;
        std::atomic<uint32_t>                       events_timeout;
        std::shared_ptr<rsimpl::syncronizing_archive> archive;

        mutable std::string                         usb_port_id;
        mutable std::mutex                          usb_port_mutex;
    protected:
        const rsimpl::uvc::device &                 get_device() const { return *device; }
        rsimpl::uvc::device &                       get_device() { return *device; }

        virtual void                                start_video_streaming();
        virtual void                                stop_video_streaming();
        virtual void                                start_motion_tracking();
        virtual void                                stop_motion_tracking();

        virtual void                                disable_auto_option(int subdevice, rs_option auto_opt);

        bool                                        motion_module_ready = false;
    public:
        rs_device_base(std::shared_ptr<rsimpl::uvc::device> device, const rsimpl::static_device_info & info);
        virtual ~rs_device_base();

        const rsimpl::stream_interface &            get_stream_interface(rs_stream stream) const override { return *streams[stream]; }

        const char *                                get_name() const override { return config.info.name.c_str(); }
        const char *                                get_serial() const override { return config.info.serial.c_str(); }
        const char *                                get_firmware_version() const override { return config.info.firmware_version.c_str(); }
        float                                       get_depth_scale() const override { return config.depth_scale; }

        const char*                                 get_camera_info(rs_camera_info info) const override;

        void                                        enable_stream(rs_stream stream, int width, int height, rs_format format, int fps, rs_output_buffer_format output) override;
        void                                        enable_stream_preset(rs_stream stream, rs_preset preset) override;
        void                                        disable_stream(rs_stream stream) override;

        rs_motion_intrinsics                        get_motion_intrinsics() const override;
        rs_extrinsics                               get_motion_extrinsics_from(rs_stream from) const override;
        void                                        enable_motion_tracking() override;
        void                                        set_stream_callback(rs_stream stream, void(*on_frame)(rs_device * device, rs_frame_ref * frame, void * user), void * user) override;
        void                                        set_stream_callback(rs_stream stream, rs_frame_callback * callback) override;
        void                                        disable_motion_tracking() override;

        void                                        set_motion_callback(rs_motion_callback * callback) override;
        void                                        set_motion_callback(void(*on_event)(rs_device * device, rs_motion_data data, void * user), void * user) override;
        void                                        set_timestamp_callback(void(*on_event)(rs_device * device, rs_timestamp_data data, void * user), void * user) override;
        void                                        set_timestamp_callback(rs_timestamp_callback * callback) override;

        virtual void                                start(rs_source source) override;
        virtual void                                stop(rs_source source) override;

        bool                                        is_capturing() const override { return capturing; }
        int                                         is_motion_tracking_active() const override { return data_acquisition_active; }

        void                                        wait_all_streams() override;
        bool                                        poll_all_streams() override;

        virtual bool                                supports(rs_capabilities capability) const override;

        virtual bool                                supports_option(rs_option option) const override;
        virtual void                                get_option_range(rs_option option, double & min, double & max, double & step, double & def) override;
        virtual void                                set_options(const rs_option options[], size_t count, const double values[]) override;
        virtual void                                get_options(const rs_option options[], size_t count, double values[])override;
        virtual void                                on_before_start(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) = 0;
        virtual rs_stream                           select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) = 0;
        virtual std::shared_ptr<rsimpl::frame_timestamp_reader>  create_frame_timestamp_reader(int subdevice) const = 0;
        void                                        release_frame(rs_frame_ref * ref) override;
        const char *                                get_usb_port_id() const override;
        rs_frame_ref *                              clone_frame(rs_frame_ref * frame) override;

        virtual void                                send_blob_to_device(rs_blob_type type, void * data, int size) { throw std::runtime_error("not supported!"); }
        static void                                 update_device_info(rsimpl::static_device_info& info);
    };

    class auto_exposure_algorithm {
    public:
        void ModifyExposure(float& Exposure_, bool& ExposureModified, float& Gain_, bool& GainModified)
        {
            float TotalExposure = Exposure * Gain;
            float PrevExposure = Exposure;
            //    EXPOSURE_LOG("TotalExposure " << TotalExposure << ", TargetExposure " << TargetExposure << std::endl);
            if (fabs(TargetExposure - TotalExposure) > EPS)
            {
                RoundingModeType RoundingMode;

                if (TargetExposure > TotalExposure)
                {
                    float TargetExposure0 = TotalExposure * (1.0f + ExposureStep);

                    TargetExposure0 = std::min(TargetExposure0, TargetExposure);
                    IncreaseExposureGain(TargetExposure, TargetExposure0, Exposure, Gain);
                    RoundingMode = RoundingModeType::Ceil;
                    //            EXPOSURE_LOG(" ModifyExposure: IncreaseExposureGain: ");
                    //            EXPOSURE_LOG(" TargetExposure0 " << TargetExposure0);
                }
                else
                {
                    float TargetExposure0 = TotalExposure / (1.0f + ExposureStep);

                    TargetExposure0 = std::max(TargetExposure0, TargetExposure);
                    DecreaseExposureGain(TargetExposure, TargetExposure0, Exposure, Gain);
                    RoundingMode = RoundingModeType::Floor;
                    //            EXPOSURE_LOG(" ModifyExposure: DecreaseExposureGain: ");
                    //            EXPOSURE_LOG(" TargetExposure0 " << TargetExposure0);
                }
                //        EXPOSURE_LOG(" Exposure " << Exposure << ", Gain " << Gain << std::endl);
                if (Exposure_ != Exposure)
                {
                    ExposureModified = true;
                    Exposure_ = Exposure;
                    //            EXPOSURE_LOG("ExposureModified: Exposure = " << Exposure_);
                    Exposure_ = ExposureToValue(Exposure_, RoundingMode);
                    //            EXPOSURE_LOG(" rounded to: " << Exposure_ << std::endl);

                    if (std::fabs(PrevExposure - Exposure) < MinimalExposureStep)
                    {
                        Exposure_ = Exposure + Direction * MinimalExposureStep;
                    }
                }
                if (Gain_ != Gain)
                {
                    GainModified = true;
                    Gain_ = Gain;
                    //            EXPOSURE_LOG("GainModified: Gain = " << Gain_);
                    Gain_ = GainToValue(Gain_, RoundingMode);
                    //            EXPOSURE_LOG(" rounded to: " << Gain_ << std::endl);
                }
            }
        }

        bool AnalyzeImage(const rs_frame_ref* Image)
        {
            int cols = Image->get_frame_width();
            int rows = Image->get_frame_height();

            const int NumberOfPixels = cols * rows; //VGA
            if (NumberOfPixels == 0)
            {
                // empty image
                return false;
            }
            std::vector<int> H(256);
            int TotalWeight;
            //    if (UseWeightedHistogram)
            //    {
            //        if ((Weights.cols != cols) || (Weights.rows != rows)) { /* weights matrix size != image size */ }
            //        ImHistW(Image.get_data(), Weights.data, cols, rows, Image.step, Weights.step, &H[0], TotalWeight);
            //    }
            //    else
            {
                ImHist((uint8_t*)Image->get_frame_data(), cols, rows, Image->get_frame_bpp() / 8 * cols, &H[0]);
                TotalWeight = NumberOfPixels;
            }
            HistogramMetric Score;
            HistogramScore(H, TotalWeight, Score);
            int EffectiveDynamicRange = (Score.HighlightLimit - Score.ShadowLimit);
            ///
            float s1 = (Score.MainMean - 128.0f) / 255.0f;
            float s2 = 0;
            if (TotalWeight != 0)
            {
                s2 = (Score.OverExposureCount - Score.UnderExposureCount) / (float)TotalWeight;
            }
            else
            {
                //        WRITE2LOG(eLogVerbose, "Weight=0 Error");
                return false;
            }
            float s = -0.3f * (s1 + 5.0f * s2);
            //    EXPOSURE_LOG(" AnalyzeImage Score: " << s << std::endl);
            //std::cout << "----------------- " << s << std::endl;
            /*if (fabs(s) < Hysteresis)
            {
            EXPOSURE_LOG(" AnalyzeImage < Hysteresis" << std::endl);
            return false;
            }*/
            if (s > 0)
            {
                //        EXPOSURE_LOG(" AnalyzeImage: IncreaseExposure" << std::endl);
                Direction = +1;
                IncreaseExposureTarget(s, TargetExposure);
            }
            else
            {
                //        EXPOSURE_LOG(" AnalyzeImage: DecreaseExposure" << std::endl);
                Direction = -1;
                DecreaseExposureTarget(s, TargetExposure);
            }
            //if ((PrevDirection != 0) && (PrevDirection != Direction))
            {
                if (fabs(1.0f - (Exposure * Gain) / TargetExposure) < Hysteresis)
                {
                    //            EXPOSURE_LOG(" AnalyzeImage: Don't Modify (Hysteresis): " << TargetExposure << " " << Exposure * Gain << std::endl);
                    return false;
                }
            }
            PrevDirection = Direction;
            //    EXPOSURE_LOG(" AnalyzeImage: Modify" << std::endl);
            return true;
        }

    private:
        struct HistogramMetric { int UnderExposureCount; int OverExposureCount; int ShadowLimit; int HighlightLimit; int LowerQ; int UpperQ; float MainMean; float MainStd; };
        enum class RoundingModeType { Round, Ceil, Floor };


        inline void ImHist(const uint8_t* data, const int width, const int height, const int rowStep, int H[])
        {
            for (int i = 0; i < 256; ++i) H[i] = 0;
            const uint8_t* rowData = data;
            for (int i = 0; i < height; ++i, rowData += rowStep) for (int j = 0; j < width; ++j) ++H[rowData[j]];
        }

        void IncreaseExposureTarget(float Mult, float& TargetExposure)
        {
            TargetExposure = std::min((Exposure * Gain) * (1.0f + Mult), MaximalExposure * GainLimit);
        }
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        void DecreaseExposureTarget(float Mult, float& TargetExposure)
        {
            TargetExposure = std::max((Exposure * Gain) * (1.0f + Mult), MinimalExposure * BaseGain);
        }

        void IncreaseExposureGain(const float& TargetExposure, const float& TargetExposure0, float& Exposure, float& Gain)
        {
            Exposure = std::max(MinimalExposure, std::min(TargetExposure0 / BaseGain, MaximalExposure));
            Gain = std::min(GainLimit, std::max(TargetExposure0 / Exposure, BaseGain));
        }
        void DecreaseExposureGain(const float& TargetExposure, const float& TargetExposure0, float& Exposure, float& Gain)
        {
            Exposure = std::max(MinimalExposure, std::min(TargetExposure0 / BaseGain, MaximalExposure));
            Gain = std::min(GainLimit, std::max(TargetExposure0 / Exposure, BaseGain));
        }


#if defined(_WINDOWS) || defined(WIN32) || defined(WIN64)
        inline float Round(float x) { return std::round(x); }
#else
        inline float Round(float x) { return x < 0.0 ? ceil(x - 0.5f) : floor(x + 0.5f); }
#endif

        float ExposureToValue(float ExpMSec, RoundingModeType RoundingMode)
        {
            const float LinePeriodUs = 19.33333333f;

            float ExposureTimeLine = (ExpMSec * 1000.0f / LinePeriodUs);
            if (RoundingMode == RoundingModeType::Ceil) ExposureTimeLine = std::ceil(ExposureTimeLine);
            else if (RoundingMode == RoundingModeType::Floor) ExposureTimeLine = std::floor(ExposureTimeLine);
            else ExposureTimeLine = Round(ExposureTimeLine);
            return ((float)ExposureTimeLine * LinePeriodUs) / 1000.0f;
        }

        float GainToValue(float Gain, RoundingModeType RoundingMode)
        {

            if (Gain < 2.0f) { return 2.0f; }
            else if (Gain > 32.0f) { return 32.0f; }
            else {
                if (RoundingMode == RoundingModeType::Ceil) return std::ceil(Gain * 8.0f) / 8.0f;
                else if (RoundingMode == RoundingModeType::Floor) return std::floor(Gain * 8.0f) / 8.0f;
                else return Round(Gain * 8.0f) / 8.0f;
            }
        }

        template <typename T> inline T Sqr(const T& X) { return (X*X); }
        void HistogramScore(std::vector<int>& H, const int TotalWeight, HistogramMetric& Score)
        {
            Score.UnderExposureCount = 0;
            Score.OverExposureCount = 0;

            for (size_t i = 0; i <= UnderExposureLimit; ++i)
            {
                Score.UnderExposureCount += H[i];
            }
            Score.ShadowLimit = 0;
            //if (Score.UnderExposureCount < UnderExposureNoiseLimit)
            {
                Score.ShadowLimit = UnderExposureLimit;
                for (size_t i = UnderExposureLimit + 1; i <= OverExposureLimit; ++i)
                {
                    if (H[i] > UnderExposureNoiseLimit)
                    {
                        break;
                    }
                    Score.ShadowLimit++;
                }
                int LowerQ = 0;
                Score.LowerQ = 0;
                for (size_t i = UnderExposureLimit + 1; i <= OverExposureLimit; ++i)
                {
                    LowerQ += H[i];
                    if (LowerQ > TotalWeight / 4)
                    {
                        break;
                    }
                    Score.LowerQ++;
                }
            }

            for (size_t i = OverExposureLimit; i <= 255; ++i)
            {
                Score.OverExposureCount += H[i];
            }

            Score.HighlightLimit = 255;
            //if (Score.OverExposureCount < OverExposureNoiseLimit)
            {
                Score.HighlightLimit = OverExposureLimit;
                for (size_t i = OverExposureLimit; i >= UnderExposureLimit; --i)
                {
                    if (H[i] > OverExposureNoiseLimit)
                    {
                        break;
                    }
                    Score.HighlightLimit--;
                }
                int UpperQ = 0;
                Score.UpperQ = OverExposureLimit;
                for (size_t i = OverExposureLimit; i >= UnderExposureLimit; --i)
                {
                    UpperQ += H[i];
                    if (UpperQ > TotalWeight / 4)
                    {
                        break;
                    }
                    Score.UpperQ--;
                }

            }
            int32_t M1 = 0;
            int64_t M2 = 0;

            double NN = (double)TotalWeight - Score.UnderExposureCount - Score.OverExposureCount;
            if (NN == 0)
            {
                NN = (double)TotalWeight;
                for (int i = 0; i <= 255; ++i)
                {
                    M1 += H[i] * i;
                    M2 += H[i] * Sqr(i);
                }
            }
            else
            {
                for (int i = UnderExposureLimit + 1; i < OverExposureLimit; ++i)
                {
                    M1 += H[i] * i;
                    M2 += H[i] * Sqr(i);
                }
            }
            Score.MainMean = (float)((double)M1 / NN);
            double Var = (double)M2 / NN - Sqr((double)M1 / NN);
            if (Var > 0)
            {
                Score.MainStd = (float)sqrt(Var);
            }
            else
            {
                Score.MainStd = 0.0f;
            }
        }

        float MinimalExposure = 0.25f, MaximalExposure = 20.f, BaseGain = 2.0f, GainLimit = 15.0f;
        float Exposure = 10.0f, Gain = 2.0f, TargetExposure = 0.0f;
        uint8_t UnderExposureLimit = 5, OverExposureLimit = 250; int UnderExposureNoiseLimit = 50, OverExposureNoiseLimit = 50;
        int Direction = 0, PrevDirection = 0; float Hysteresis = 0.075f;// 05;
        float EPS = 0.01f, ExposureStep = 0.05f, MinimalExposureStep = 0.15f;
    };

    class auto_exposure_mechanism {
    public:
        auto_exposure_mechanism(rs_device_base* dev, std::shared_ptr<rsimpl::syncronizing_archive> archive) : action(true), sync_archive(archive), device(dev)
        {
            exposure_thread = std::make_shared<std::thread>([this]() {
                while (action)
                {
                    std::unique_lock<std::mutex> lk(queue_mtx);
                    cv.wait(lk, [&] {return (get_queue_size() || !action); });
                    if (!action)
                        return;

                    lk.unlock();

                    rs_frame_ref* frame_ref = nullptr;
                    if (pop_front_data(&frame_ref))
                    {
                        auto frame_data = frame_ref->get_frame_data();
                        float exposure_value = static_cast<float>(frame_ref->get_frame_metadata(RS_FRAME_METADATA_EXPOSURE) / 10.);
                        float gain_value = static_cast<float>(frame_ref->get_frame_metadata(RS_FRAME_METADATA_GAIN));

                        bool sts = auto_exposure_algo.AnalyzeImage(frame_ref);
                        if (sts)
                        {
                            bool modify_exposure, modify_gain;
                            auto_exposure_algo.ModifyExposure(exposure_value, modify_exposure, gain_value, modify_gain);

                            if (modify_exposure)
                            {
                                //printf("exposure value: %f\n", exposure_value * 10);
                                rs_option option[] = { RS_OPTION_FISHEYE_COLOR_EXPOSURE };
                                double value[] = { exposure_value * 10. };
                                device->set_options(option, 1, value);
                            }

                            if (modify_gain)
                            {
                                rs_option option[] = { RS_OPTION_FISHEYE_COLOR_GAIN };
                                double value[] = { gain_value };
                                device->set_options(option, 1, value);
                            }
                        }
                    }
                    sync_archive->release_frame_ref((rsimpl::frame_archive::frame_ref *)frame_ref);
                }
        });
    }

    ~auto_exposure_mechanism()
    {
        action = false;
        {
            std::lock_guard<std::mutex> lk(queue_mtx);
            clear_queue();
        }
        cv.notify_one();
        exposure_thread->join();
    }

    void add_frame(rs_frame_ref* frame)
    {
        if (!action)
            return;

        {
            std::lock_guard<std::mutex> lk(queue_mtx);
            push_back_data(frame);
        }
        cv.notify_one();
    }

    private:
        void push_back_data(rs_frame_ref* data)
        {
            data_queue.push_back(data);
        }

        bool pop_front_data(rs_frame_ref** data)
        {
            if (!data_queue.size())
                return false;

            *data = data_queue.front();
            data_queue.pop_front();

            return true;
        }

        size_t get_queue_size()
        {
            //std::lock_guard<std::mutex> lock(queue_mtx);
            return data_queue.size();
        }

        void clear_queue()
        {
            rs_frame_ref* frame_ref = nullptr;
            while (pop_front_data(&frame_ref))
            {
                sync_archive->release_frame_ref((rsimpl::frame_archive::frame_ref *)frame_ref);
            }
        }

        rs_device_base* device = nullptr;
        auto_exposure_algorithm auto_exposure_algo;
        std::shared_ptr<rsimpl::syncronizing_archive> sync_archive;
        std::shared_ptr<std::thread> exposure_thread;
        std::condition_variable cv;
        std::atomic<bool> action;
        std::deque<rs_frame_ref*> data_queue;
        std::mutex queue_mtx;
    };

#endif
