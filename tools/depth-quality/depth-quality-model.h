#pragma once
#include <vector>
#include <thread>

namespace rs2
{
    namespace depth_quality
    {
        class metrics_model
        {
        public:
            metrics_model(frame_queue* _input_queue, std::mutex &m/*, depth_profiler_viewer* viewer*/);
            ~metrics_model();

            void render();

            void update_stream_attributes(const rs2_intrinsics &intrinsic, const float& scale_units)
            {
                _depth_intrinsic = intrinsic; _depth_scale_units = scale_units;
            };

            void update_frame_attributes(const region_of_interest& roi)
            {
                _roi = roi;
            }

        private:
            metrics_model(const metrics_model&);

            void visualize(snapshot_metrics stats, int w, int h, bool plane) const;

            frame_queue*            _frame_queue;
            std::thread             _worker_thread;

            rs2_intrinsics      _depth_intrinsic;
            float               _depth_scale_units;
            region_of_interest  _roi;
            snapshot_metrics    _latest_metrics;
            bool                _active;

            PlotMetric      avg_plot;
            PlotMetric      std_plot;
            PlotMetric      fill_plot;
            PlotMetric      dist_plot;
            PlotMetric      angle_plot;
            PlotMetric      out_plot;
        };

        class tool_model
        {
        public:
            tool_model() :
                _calc_queue(1),
                _device_model(nullptr), 
                _metrics_model(&_calc_queue, _m),
                _viewer_model(1280, 720, "RealSense Depth Quality Tool", _device_model, &_metrics_model)
            {
            }

            void update_configuration(const rs2::pipeline& pipe);

            void enqueue_for_processing(rs2::frame &depth_frame)
            {
                _calc_queue.enqueue(depth_frame);
            }

            void upload(rs2::frameset &frameset);

        private:
            frame_queue                     _calc_queue;
            std::mutex                      _m;
            std::shared_ptr<device_model>   _device_model;
            metrics_model                   _metrics_model;
            tool_window           _viewer_model;

            std::string                     _error_message;
            mouse_info                      _mouse;
        };

    }
}
