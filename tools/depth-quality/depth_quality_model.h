#pragma once
#include <vector>
#include <thread>
#include "depth_quality_viewer.h"
#include "depth_metrics.h"

using namespace rs2;

namespace rs2
{
    namespace depth_profiler
    {
        class depth_profiler_model;

        class metrics_model
        {
        public:
            metrics_model(frame_queue* _input_queue, std::mutex &m, depth_profiler_viewer* viewer);
            ~metrics_model();

            void render_metrics() const;

            void visualize(snapshot_metrics stats, int w, int h, bool plane) const;

            void update_stream_attributes(const rs2_intrinsics &intrinsic, const float& scale_units)
            {
                _depth_intrinsic = intrinsic; _depth_scale_units = scale_units;
            };

            void update_frame_attributes(const region_of_interest& roi)
            {
                _roi = roi;
            }

        private:

            //analyze_depth_image
            void produce_metrics();

            frame_queue*        _frame_queue;
            depth_profiler_viewer*    _viewer_model;
            std::thread         _worker_thread;

            rs2_intrinsics      _depth_intrinsic;
            float               _depth_scale_units;
            region_of_interest  _roi;
            snapshot_metrics    _latest_metrics;
            bool                _active;
        };

        class depth_profiler_model : public rs2::viewer_model
        {
        public:
            enum e_states { e_acquire, e_configure, e_profile, e_state_max };

            depth_profiler_model() : _cur_state(nullptr),
                _device_model(nullptr),
                _calc_queue(1),
                _viewer_model(1280, 720, "RealSense Depth Quality Tool", _device_model),
                _metrics_model(&_calc_queue,_m,&_viewer_model)
            {
                _states.push_back(new acquire_cam);
                _states.push_back(new configure_cam);
                _states.push_back(new profile_metrics);

                _cur_state = _states[e_acquire];
            }

            ~depth_profiler_model()
            {
                _cur_state = nullptr;
                _states.clear();
            }

            operator bool()
            {
                auto res = true;

                // Update logic models
                _cur_state->update(*this);

                // Update rendering subsystem
                if (!_viewer_model)
                    res = false;

                return res;
            }

            void use_device(rs2::device dev);

            void enqueue_for_processing(rs2::frame &depth_frame)
            {
                _calc_queue.enqueue(depth_frame);
            }

            void upload(rs2::frameset &frameset);

        private:

            class app_state
            {
            public:
                virtual void update(depth_profiler_model& app_model) = 0;
            };

            class acquire_cam : public app_state
            {
                void update(depth_profiler_model& app_model);
            };

            class configure_cam : public app_state
            {
                void update(depth_profiler_model& app_model);
            };

            class profile_metrics : public app_state
            {
                void update(depth_profiler_model& app_model);
            };

            void set_state(e_states state) { _cur_state = _states[state]; };

            app_state*                  _cur_state;
            std::vector<app_state*>     _states;

            frame_queue                     _calc_queue;
            std::mutex                      _m;
            std::shared_ptr<device_model>   _device_model;
            depth_profiler_viewer                 _viewer_model;
            metrics_model                   _metrics_model;

            std::string                     _error_message;
            mouse_info                      _mouse;
        };

    }
}
