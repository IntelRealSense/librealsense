/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
#pragma once

#include <librealsense2/rs.hpp>

#include "depth-metrics.h"
#include "model-views.h"
#include "viewer.h"
#include "ux-window.h"
#include "os.h"

#include <tuple>
#include <vector>
#include <thread>
#include <mutex>

namespace rs2
{
    namespace depth_quality
    {
        class metrics_model;

        struct sample
        {
            sample(std::vector<single_metric_data> samples, double timestamp, unsigned long long frame_number) :
                samples(std::move(samples)), timestamp(timestamp), frame_number(frame_number) {}

            std::vector<single_metric_data> samples;

            double timestamp;
            unsigned long long frame_number;
        };

        struct metric_definition
        {
            std::string name;
            std::string units;
        };

        class metrics_recorder
        {
        public:
            metrics_recorder(viewer_model& viewer_model) :
                _recording(false), _viewer_model(viewer_model)
            {}

            void add_metric(const metric_definition& data)
            {
                std::lock_guard<std::mutex> lock(_m);
                _metric_data.push_back(data);
            }

            void add_sample(rs2::frameset& frames, std::vector<single_metric_data> sample)
            {
                std::lock_guard<std::mutex> lock(_m);
                if (_recording)
                {
                    record_frames(frames);

                    if (sample.size())
                        _samples.push_back({ sample, _model_timer.elapsed_ms(), frames.get_frame_number() });
                }
            }
            void start_record(metrics_model* metrics)
            {
                std::lock_guard<std::mutex> lock(_m);
                _metrics = metrics;
                if (auto ret = file_dialog_open(save_file, NULL, NULL, NULL))
                {
                    _filename_base = ret;
                    _recording = true;
                }
            }

            void stop_record(device_model* dev)
            {
                std::lock_guard<std::mutex> lock(_m);
                _recording = false;
                serialize_to_csv();

                if (dev)
                {
                    if (auto adv = dev->dev.as<rs400::advanced_mode>())
                    {
                        std::string filename = _filename_base + "_configuration.json";
                        std::ofstream out(filename);
                        try
                        {
                            out << adv.serialize_json();
                        }
                        catch (...)
                        {
                            _viewer_model.not_model.add_notification(notification_data{ to_string() << "Metrics Recording: JSON Serializaion has failed",
                                RS2_LOG_SEVERITY_WARN, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
                        }
                    }
                }
                _samples.clear();
                _viewer_model.not_model.add_notification(notification_data{ to_string() << "Finished to record frames and matrics data " ,
                    RS2_LOG_SEVERITY_INFO, RS2_NOTIFICATION_CATEGORY_UNKNOWN_ERROR });
            }

            bool is_recording()
            {
                return _recording;
            }

        private:
            void serialize_to_csv() const;
            void record_frames(const frameset & frame);
            viewer_model& _viewer_model;
            std::vector<metric_definition> _metric_data;
            std::vector<sample> _samples;
            timer _model_timer;
            std::mutex _m;
            bool _recording;
            std::string _filename_base;
            metrics_model* _metrics;
            colorizer _colorize;
            pointcloud _pc;
        };

        class metric_plot : public std::enable_shared_from_this<metric_plot>
        {
        public:
            enum range
            {
                GREEN_RANGE,
                YELLOW_RANGE,
                RED_RANGE,
                MAX_RANGE
            };

            std::shared_ptr<metric_plot> set(range r, float from, float to)
            {
                ranges[r].x = from;
                ranges[r].y = to;
                return shared_from_this();
            }

            range get_range(float val) const
            {
                for (int i = 0; i < MAX_RANGE; i++)
                {
                    if (ranges[i].x < val && val <= ranges[i].y)
                        return (range)i;
                }
                return MAX_RANGE;
            }

            metric_plot(const std::string& name, float min, float max,
                        const std::string& units, const std::string& description,
                        const bool with_plane_fit)
                : _idx(0), _first_idx(0),_vals(), _min(min), _max(max), _id("##" + name),
                _label(name + " = "), _name(name),
                _units(units), _description(description),
                _enabled(true),
                _requires_plane_fit(with_plane_fit),
                _trending_up(std::chrono::milliseconds(700)),
                _trending_down(std::chrono::milliseconds(700)),
                _persistent_visibility(std::chrono::milliseconds(2000)) // The metric's status will be absorbed to make the UI persistent
            {
                for (int i = 0; i < MAX_RANGE; i++) ranges[i] = { 0.f, 0.f };
            }
            ~metric_plot() {}

            void add_value(float val)
            {
                std::lock_guard<std::mutex> lock(_m);
                _vals[_idx]         = val;
                _timestamps[_idx]   = _model_timer.elapsed_ms();
                _idx = (_idx + 1) % SIZE;
                if (_first_idx== _idx)
                    _first_idx = (_first_idx + 1) % SIZE;
            }

            void render(ux_window& win);

            void visible(bool is_visible)
            {
                std::lock_guard<std::mutex> lock(_m);
                _persistent_visibility.add_value(is_visible);
            }

            void enable(bool enable)
            {
                std::lock_guard<std::mutex> lock(_m);
                if (enable != _enabled)
                {
                    _persistent_visibility.reset();
                    _enabled = enable;
                }
            }

            bool enabled() const { return _enabled; }
            bool requires_plane_fit() const { return _requires_plane_fit; }
            std::string get_name() { return _name; }

        private:
            bool has_trend(bool positive);

            std::mutex _m;
            const static size_t SIZE = 200;
            size_t _idx, _first_idx;
            std::array<float, SIZE> _vals;
            std::array<double, SIZE> _timestamps;
            float _min, _max;
            std::string _id, _label, _units, _name, _description;
            bool _enabled;
            const bool _requires_plane_fit;

            timer _model_timer;
            temporal_event _trending_up;
            temporal_event _trending_down;
            temporal_event _persistent_visibility;  // Control the metric visualization

            float2 ranges[MAX_RANGE];

            friend class metrics_model; // For CSV export
        };

        class metrics_model
        {
        public:
            metrics_model(viewer_model& viewer_model);
            ~metrics_model();

            void render(ux_window& win);

            std::array<float3, 4> get_plane()
            {
                std::lock_guard<std::mutex> lock(_m);
                return _latest_metrics.plane_corners;
            }

            void update_stream_attributes(const rs2_intrinsics &intrinsic, float scale_units, float baseline)
            {
                std::lock_guard<std::mutex> lock(_m);
                _depth_intrinsic = intrinsic;
                _depth_scale_units = scale_units;
                _stereo_baseline_mm = baseline;
            };

            void update_roi_attributes(const region_of_interest& roi, float roi_percent)
            {
                std::lock_guard<std::mutex> lock(_m);
                _roi = roi;
                _roi_percentage = roi_percent;
            }

            region_of_interest get_roi()
            {
                std::lock_guard<std::mutex> lock(_m);
                return _roi;
            }

            snapshot_metrics get_last_metrics()
            {
                std::lock_guard<std::mutex> lock(_m);
                return _latest_metrics;
            }

            void begin_process_frame(rs2::frame f) { _frame_queue.enqueue(std::move(f)); }

            void add_metric(std::shared_ptr<metric_plot> metric) { _plots.push_back(metric); }

            callback_type callback;

            void set_ground_truth(int gt)
            {
                std::lock_guard<std::mutex> lock(_m);
                _ground_truth_mm = gt;
                _use_gt = true;
            }

            void set_plane_fit(bool found)
            {
                std::lock_guard<std::mutex> lock(_m);
                _plane_fit = found;
                for (auto&& plot : _plots)
                {
                    if (plot->enabled())
                    {
                        bool val = plot->requires_plane_fit() ? found : true;
                        plot->visible(val);
                    }
                }
            }

            void disable_ground_truth()
            {
                std::lock_guard<std::mutex> lock(_m);
                _use_gt = false;
                _ground_truth_mm = 0;
            }
            std::tuple<int, bool> get_inputs() const
            {
                std::lock_guard<std::mutex> lock(_m);
                return std::make_tuple(_ground_truth_mm, _plane_fit);
            }

            void reset()
            {
                _plane_fit = false;
                rs2::frame f;
                while (_frame_queue.poll_for_frame(&f));
            }

            void update_device_data(const std::string& camera_info)
            {
                _camera_info = camera_info;
            }
            bool is_recording()
            {
                return _recorder.is_recording();
            }
            void start_record()
            {
                _recorder.start_record(this);
            }
            void stop_record(device_model* dev)
            {
                _recorder.stop_record(dev);
            }
        private:
            metrics_model(const metrics_model&);

            frame_queue             _frame_queue;
            std::thread             _worker_thread;

            rs2_intrinsics          _depth_intrinsic;
            float                   _depth_scale_units;
            float                   _stereo_baseline_mm;
            int                     _ground_truth_mm;
            bool                    _use_gt;
            bool                    _plane_fit;
            region_of_interest      _roi;
            float                   _roi_percentage;
            snapshot_metrics        _latest_metrics;
            bool                    _active;
            std::vector<std::shared_ptr<metric_plot>> _plots;
            metrics_recorder _recorder;
            std::string  _camera_info;
            mutable std::mutex      _m;

            friend class  metrics_recorder;
            friend class  tool_model;
        };

        using metric = std::shared_ptr<metric_plot>;

        class tool_model
        {
        public:
            tool_model(rs2::context& ctx);

            bool start(ux_window& win);

            void render(ux_window& win);

            void update_configuration();

            void reset(ux_window& win);

            bool draw_instructions(ux_window& win, const rect& viewer_rect, bool& distance, bool& orientation);

            void draw_guides(ux_window& win, const rect& viewer_rect, bool distance_guide, bool orientation_guide);

            std::shared_ptr<metric_plot> make_metric(
                const std::string& name, float min, float max, bool plane_fit,
                const std::string& units,
                const std::string& description);

            void on_frame(callback_type callback) { _metrics_model.callback = callback; }

            float get_depth_scale() const { return _metrics_model._depth_scale_units; }
            rs2::device get_active_device(void) const;

        private:

            std::string capture_description();

            rs2::context&                   _ctx;
            pipeline                        _pipe;
            std::shared_ptr<device_model>   _device_model;
            viewer_model                    _viewer_model;
            rs2::points                     _last_points;
            texture_buffer*                  _last_texture;
            std::shared_ptr<subdevice_model> _depth_sensor_model;
            metrics_model                   _metrics_model;
            std::string                     _error_message;
            bool                            _first_frame = true;
            periodic_timer                  _update_readonly_options_timer;
            bool                            _device_in_use = false;

            float                           _roi_percent = 0.4f;
            int                             _roi_combo_index = 2;
            temporal_event                  _roi_located;

            temporal_event                  _too_far;
            temporal_event                  _too_close;
            temporal_event                  _skew_left;
            temporal_event                  _skew_right;
            temporal_event                  _skew_up;
            temporal_event                  _skew_down;
            temporal_event                  _angle_alert;
            std::map<int, temporal_event>   _depth_scale_events;

            float                           _min_dist, _max_dist, _max_angle;
            std::mutex                      _mutex;

            bool                            _use_ground_truth = false;
            int                             _ground_truth = 0;
        };
    }
}
