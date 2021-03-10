// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2/h/rs_internal.h> // Access librealsense internal clock
#include <opencv2/opencv.hpp>   // Include OpenCV API
#include "../cv-helpers.hpp"    // Helper functions for conversions between RealSense and OpenCV
#include "../../../src/concurrency.h" // We are borrowing from librealsense concurrency infrastructure for this sample
#include <algorithm>

// Helper class to keep track of measured data
// and generate basic statistics
template<class T>
class measurement
{
public:
    measurement(int cap = 10) : _capacity(cap), _sum() {}

    // Media over rolling window
    T median() const
    {
        std::lock_guard<std::mutex> lock(_m);

        if (_total == 0) return _sum;

        std::vector<T> copy(begin(_data), end(_data));
        std::sort(begin(copy), end(copy));
        return copy[copy.size() / 2];
    }

    // Average over all samples
    T avg() const
    {
        std::lock_guard<std::mutex> lock(_m);
        if (_total > 0) return _sum / _total;
        return _sum;
    }

    // Total count of measurements
    int total() const 
    { 
        std::lock_guard<std::mutex> lock(_m);
        return _total; 
    }

    // Add new measurement
    void add(T val)
    {
        std::lock_guard<std::mutex> lock(_m);
        _data.push_back(val);
        if (_data.size() > _capacity)
        {
            _data.pop_front();
        }
        _total++;
        _sum += val;
    }

private:
    mutable std::mutex _m;
    T _sum;
    std::deque<T> _data;
    int _capacity;
    int _total = 0;
};

// Helper class to encode / decode numbers into
// binary sequences, with 2-bit checksum
class bit_packer
{
public:
    bit_packer(int digits) 
        : _digits(digits), _bits(digits, false)
    {
    }

    void reset()
    {
        std::fill(_bits.begin(), _bits.end(), false);
    }

    // Try to reconstruct the number from bits inside the class
    bool try_unpack(int* number)
    {
        // Calculate and verify Checksum
        auto on_bits = std::count_if(_bits.begin() + 2, _bits.end(),
            [](bool f) { return f; });
        if ((on_bits % 2 == 1) == _bits[0] &&
            ((on_bits / 2) % 2 == 1) == _bits[1])
        {
            int res = 0;
            for (int i = 2; i < _digits; i++)
            {
                res = res * 2 + _bits[i];
            }
            *number = res;
            return true;
        }
        else return false;
    }

    // Try to store the number as bits into the class
    bool try_pack(int number)
    {
        if (number < 1 << (_digits - 2))
        {
            _bits.clear();
            while (number)
            {
                _bits.push_back(number & 1);
                number >>= 1;
            }

            // Pad with zeros
            while (_bits.size() < _digits) _bits.push_back(false);
            reverse(_bits.begin(), _bits.end());

            // Apply 2-bit Checksum
            auto on_bits = std::count_if(_bits.begin() + 2, _bits.end(),
                [](bool f) { return f; });
            _bits[0] = (on_bits % 2 == 1);
            _bits[1] = ((on_bits / 2) % 2 == 1);

            return true;
        }
        else return false;
    }

    // Access bits array
    std::vector<bool>& get() { return _bits; }

private:
    std::vector<bool> _bits;
    int _digits;
};

// Main class in charge of detecting latency measurements
class detector
{
public:
    detector(int digits, int display_w) 
        : _digits(digits), _packer(digits), 
          _display_w(display_w),
          _t([this]() { detect(); }),
          _preview_size(600, 350),
          _next_value(0), _next(false), _alive(true)
    {
        using namespace cv;

        _start_time = std::chrono::high_resolution_clock::now();
        _render_start = std::chrono::high_resolution_clock::now();

        _last_preview = Mat::zeros(_preview_size, CV_8UC1);
        _instructions = Mat::zeros(Size(display_w, 120), CV_8UC1);

        putText(_instructions, "Point the camera at the screen. Ensure all white circles are being captured",
            Point(display_w / 2 - 470, 30), FONT_HERSHEY_SIMPLEX,
            0.8, Scalar(255, 255, 255), 2, LINE_AA);

        putText(_instructions, "Press any key to exit...",
            Point(display_w / 2 - 160, 70), FONT_HERSHEY_SIMPLEX,
            0.8, Scalar(255, 255, 255), 2, LINE_AA);
    }

    void begin_render()
    {
        _render_start = std::chrono::high_resolution_clock::now();
    }

    void end_render()
    {
        auto duration = std::chrono::high_resolution_clock::now() - _render_start;
        _render_time.add(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
    }

    ~detector()
    {
        _alive = false;
        _t.join();
    }

    // Add new frame from the camera
    void submit_frame(rs2::frame f)
    {
        record frame_for_processing;

        frame_for_processing.f = f;

        // Read how much time the frame spent from
        // being released by the OS-dependent driver (Windows Media Foundation, V4L2 or libuvc)
        // to this point in time (when it is first accessible to the application)
        auto toa = f.get_frame_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL);
        rs2_error* e;
        _processing_time.add(rs2_get_time(&e) - toa);

        auto duration = std::chrono::high_resolution_clock::now() - _start_time;
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        frame_for_processing.ms = ms; // Store current clock into the record

        _queue.enqueue(std::move(frame_for_processing));
    }

    // Get next value to transmit
    // This will either be the same as the last time
    // Or a new value when needed
    int get_next_value()
    {
        // Capture clock for next cycle
        if (_next.exchange(false))
        {
            auto now = std::chrono::high_resolution_clock::now();
            auto duration = now - _start_time;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
            auto next = ms % (1 << (_digits - 2));
            if (next == _next_value) next++;
            _next_value = next;
        }
        return _next_value;
    }

    // Copy preview image stored inside into a matrix
    void copy_preview_to(cv::Mat& display)
    {
        std::lock_guard<std::mutex> lock(_preview_mutex);
        cv::Rect roi(cv::Point(_display_w / 2 - _preview_size.width / 2, 200), _last_preview.size());
        _last_preview.copyTo(display(roi));

        cv::Rect text_roi(cv::Point(0, 580), _instructions.size());
        _instructions.copyTo(display(text_roi));
    }

private:
    struct record
    {
        rs2::frame f;
        long long ms;
    };

    void next()
    {
        record r;
        while (_queue.try_dequeue(&r));
        _next = true;
    }

    struct detector_lock
    {
        detector_lock(detector* owner)
            : _owner(owner) {}

        void abort() { _owner = nullptr; }

        ~detector_lock() { if(_owner) _owner->next(); }

        detector* _owner;
    };

    // Render textual instructions
    void update_instructions()
    {
        using namespace cv;

        std::lock_guard<std::mutex> lock(_preview_mutex);
        _instructions = Mat::zeros(Size(_display_w, 120), CV_8UC1);

        std::stringstream ss;

        ss << "Total Collected Samples: " << _latency.total();

        putText(_instructions, ss.str().c_str(),
            Point(80, 20), FONT_HERSHEY_SIMPLEX,
            0.8, Scalar(255, 255, 255), 2, LINE_AA);

        ss.str("");
        ss << "Estimated Latency: (Rolling-Median)" << _latency.median() << "ms, ";
        ss << "(Average)" << _latency.avg() << "ms";

        putText(_instructions, ss.str().c_str(),
            Point(80, 60), FONT_HERSHEY_SIMPLEX,
            0.8, Scalar(255, 255, 255), 2, LINE_AA);

        ss.str("");
        ss << "Software Processing: " << _processing_time.median() << "ms";

        putText(_instructions, ss.str().c_str(),
            Point(80, 100), FONT_HERSHEY_SIMPLEX,
            0.8, Scalar(255, 255, 255), 2, LINE_AA);
    }

    // Detector main loop
    void detect() 
    {
        using namespace cv;

        while (_alive)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            record r;
            if (_queue.try_dequeue(&r))
            {
                // Make sure we request new number,
                // UNLESS we decide to keep waiting
                detector_lock flush_queue_after(this);

                auto color_mat = frame_to_mat(r.f);

                if (color_mat.channels() > 1)
                    cvtColor(color_mat, color_mat, COLOR_BGR2GRAY);
                medianBlur(color_mat, color_mat, 5);

                std::vector<Vec3f> circles;
                cv::Rect roi(Point(0, 0), Size(color_mat.size().width, color_mat.size().height / 4));
                HoughCircles(color_mat(roi), circles, HOUGH_GRADIENT, 1, 10, 100, 30, 1, 100);
                for (size_t i = 0; i < circles.size(); i++)
                {
                    Vec3i c = circles[i];
                    Rect r(c[0] - c[2] - 5, c[1] - c[2] - 5, 2 * c[2] + 10, 2 * c[2] + 10);
                    rectangle(color_mat, r, Scalar(0, 100, 100), -1, LINE_AA);
                }

                cv::resize(color_mat, color_mat, _preview_size);
                {
                    std::lock_guard<std::mutex> lock(_preview_mutex);
                    _last_preview = color_mat;
                }

                sort(circles.begin(), circles.end(),
                    [](const Vec3f& a, const Vec3f& b) -> bool
                {
                    return a[0] < b[0];
                });
                if (circles.size() > 1)
                {
                    int min_x = circles[0][0];
                    int max_x = circles[circles.size() - 1][0];

                    int circle_est_size = (max_x - min_x) / (_digits + 1);
                    min_x += circle_est_size / 2;
                    max_x -= circle_est_size / 2;

                    _packer.reset();
                    for (int i = 1; i < circles.size() - 1; i++)
                    {
                        const int x = circles[i][0];
                        const int idx = _digits * ((float)(x - min_x) / (max_x - min_x));
                        if (idx >= 0 && idx < _packer.get().size())
                            _packer.get()[idx] = true;
                    }

                    int res;
                    if (_packer.try_unpack(&res))
                    {
                        if (res == _next_value)
                        {
                            auto cropped = r.ms % (1 << (_digits - 2));
                            if (cropped > res)
                            {
                                auto avg_render_time = _render_time.avg();
                                _latency.add((cropped - res) - avg_render_time);
                                update_instructions();
                            }
                        }
                        else
                        {
                            // Only in case we detected valid number other then expected
                            // We continue processing (since this was most likely older valid frame)
                            flush_queue_after.abort();
                        }
                    }
                }
            }
        }
    }

    const int _digits;
    const int _display_w;

    std::atomic_bool _alive;
    bit_packer _packer;
    std::thread _t;
    single_consumer_queue<record> _queue;
    std::chrono::high_resolution_clock::time_point _start_time;
    std::chrono::high_resolution_clock::time_point _render_start;

    std::atomic_bool _next;
    std::atomic<int> _next_value;

    const cv::Size _preview_size;
    std::mutex _preview_mutex;
    cv::Mat _last_preview;
    cv::Mat _instructions;

    measurement<int> _render_time;
    measurement<int> _processing_time;
    measurement<int> _latency;
};


