#pragma once

#include <librealsense2/rs.hpp>

#include "depth-metrics.h"
#include "model-views.h"
#include "ux-window.h"

#include <vector>
#include <thread>

namespace rs2
{
    namespace depth_quality
    {
        class metrics_model;

        class metric_plot
        {
        private:
            const static int SIZE = 200;
            int _idx;
            float _vals[SIZE];
            float _min, _max;
            std::string _id, _label, _tail;
            ImVec2 _size;

            timer _model_timer;
            temporal_event _trending_up;
            temporal_event _trending_down;

            friend class metrics_model;
        public:
            enum range
            {
                GREEN_RANGE,
                YELLOW_RANGE,
                RED_RANGE,
                MAX_RANGE
            };

            float2 ranges[MAX_RANGE];
            std::string description;

            range get_range(float val) const
            {
                for (int i = 0; i < MAX_RANGE; i++)
                {
                    if (ranges[i].x < val && val <= ranges[i].y)
                        return (range)i;
                }
                return MAX_RANGE;
            }

            metric_plot(const std::string& name, float min, float max, ImVec2 size, const std::string& tail)
                : _idx(0), _vals(), _min(min), _max(max), _id("##" + name), _label(name + " = "),
                  _tail(tail), _size(size), description(""),
                  _trending_up(std::chrono::milliseconds(700)),
                  _trending_down(std::chrono::milliseconds(700))
            {
                for (int i = 0; i < MAX_RANGE; i++) ranges[i] = { 0.f, 0.f };
            }
            ~metric_plot() {}

            bool has_trend(bool positive);

            void add_value(float val)
            {
                _vals[_idx] = val;
                _idx = (_idx + 1) % SIZE;
            }
            void render(ux_window& win);
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

            void update_stream_attributes(const rs2_intrinsics &intrinsic, const float& scale_units)
            {
                std::lock_guard<std::mutex> lock(_m);
                _depth_intrinsic = intrinsic;
                _depth_scale_units = scale_units;
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

            void begin_process_frame(rs2::frame f) { _frame_queue.enqueue(std::move(f)); }

            void serialize_to_csv(const std::string& filename) const;
        private:
            metrics_model(const metrics_model&);

            frame_queue             _frame_queue;
            std::thread             _worker_thread;

            rs2_intrinsics          _depth_intrinsic;
            float                   _depth_scale_units;
            region_of_interest      _roi;
            snapshot_metrics        _latest_metrics;
            bool                    _active;

            metric_plot             _avg_plot;
            metric_plot             _std_plot;
            metric_plot             _fill_plot;
            metric_plot             _dist_plot;
            metric_plot             _angle_plot;
            metric_plot             _out_plot;
            std::mutex              _m;
        };

        class tool_model
        {
        public:
            tool_model();

            void start();

            void render(ux_window& win);

            void update_configuration();

            void snapshot_metrics();

            void draw_instructions(ux_window& win, const rect& viewer_rect);

        private:
            pipeline                        _pipe;
            std::shared_ptr<device_model>   _device_model;
            viewer_model                    _viewer_model;
            std::shared_ptr<subdevice_model> _depth_sensor_model;
            std::shared_ptr<metrics_model>   _metrics_model;
            std::string                     _error_message;
            bool                            _first_frame = true;
            periodic_timer                  _update_readonly_options_timer;

            float                           _roi_percent = 0.33f;
            int                             _roi_combo_index = 1;
            temporal_event                  _roi_located;
        };
    }
}
