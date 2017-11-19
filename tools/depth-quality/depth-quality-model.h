#pragma once

#include <librealsense2/rs.hpp>

#include "depth-metrics.h"
#include "model-views.h"
#include "ux-window.h"

#include <vector>
#include <thread>
#include <mutex>

namespace rs2
{
    namespace depth_quality
    {
        class metrics_model;

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
                        const std::string& units, const std::string& description)
                : _idx(0), _first_idx(0),_vals(), _min(min), _max(max), _id("##" + name),
                  _label(name + " = "), _name(name),
                  _units(units), _description(description),
                  _trending_up(std::chrono::milliseconds(700)),
                  _trending_down(std::chrono::milliseconds(700))
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
                _visible = is_visible; 
            }

        private:
            bool has_trend(bool positive);

            std::mutex _m;
            const static size_t SIZE = 200;
            size_t _idx, _first_idx;
            std::array<float, SIZE> _vals;
            std::array<double, SIZE> _timestamps;
            float _min, _max;
            std::string _id, _label, _units, _name, _description;
            bool _visible = true;

            timer _model_timer;
            temporal_event _trending_up;
            temporal_event _trending_down;

            float2 ranges[MAX_RANGE];

            friend class metrics_model; // For CSV export
        };

        class metrics_model
        {
        public:
            metrics_model();
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

            void update_frame_attributes(const region_of_interest& roi)
            {
                std::lock_guard<std::mutex> lock(_m);
                _roi = roi;
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

            void serialize_to_csv(const std::string& filename, const std::string& camera_info) const;

            void add_metric(std::shared_ptr<metric_plot> metric) { _plots.push_back(metric); }

            callback_type callback;

            void set_ground_truth(float gt)
            {
                std::lock_guard<std::mutex> lock(_m);
                _ground_truth = gt;
                _use_gt = true;
            }
            void disable_ground_truth() 
            {
                std::lock_guard<std::mutex> lock(_m);
                _use_gt = false; 
            }
            float get_ground_truth() const { std::lock_guard<std::mutex> lock(_m); return _ground_truth; }

            void reset() {
                rs2::frame f;
                while (_frame_queue.poll_for_frame(&f));
            }

        private:
            metrics_model(const metrics_model&);

            frame_queue             _frame_queue;
            std::thread             _worker_thread;

            rs2_intrinsics          _depth_intrinsic;
            float                   _depth_scale_units;
            float                   _stereo_baseline_mm;
            float                   _ground_truth;
            float                   _ground_truth_copy;
            bool                    _use_gt = false;
            region_of_interest      _roi;
            snapshot_metrics        _latest_metrics;
            bool                    _active;

            std::vector<std::shared_ptr<metric_plot>> _plots;
            mutable std::mutex      _m;
        };

        using metric = std::shared_ptr<metric_plot>;

        class tool_model
        {
        public:
            tool_model();

            bool start(ux_window& win);

            void render(ux_window& win);

            void update_configuration();

            void reset(ux_window& win);

            void snapshot_metrics();

            bool draw_instructions(ux_window& win, const rect& viewer_rect, bool& distance, bool& orientation);

            void draw_guides(ux_window& win, const rect& viewer_rect, bool distance_guide, bool orientation_guide);

            std::shared_ptr<metric_plot> make_metric(
                const std::string& name, float min, float max,
                const std::string& units,
                const std::string& description);

            void on_frame(callback_type callback) { _metrics_model.callback = callback; }

            rs2::device get_active_device(void) const;
        private:

            std::string capture_description();

            pipeline                        _pipe;
            std::shared_ptr<device_model>   _device_model;
            viewer_model                    _viewer_model;
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
            temporal_event                  _sku_left;
            temporal_event                  _sku_right;
            temporal_event                  _sku_up;
            temporal_event                  _sku_down;
            temporal_event                  _angle_alert;
            std::map<int, temporal_event>   _depth_scale_events;

            float                           _min_dist, _max_dist, _max_angle;
            std::mutex                      _mutex;
            rs2::context                    _ctx;

            bool                            _use_ground_truth = false;
            int                             _ground_truth = 0;
        };
    }
}
