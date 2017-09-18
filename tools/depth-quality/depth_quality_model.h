#pragma once
#include <vector>
#include "model-views.h"
#include "depth_quality_viewer.h"

using namespace rs2;

namespace rs2_depth_quality
{
    class dq_logic_model : public rs2::viewer_model
    {
    public:
        enum e_states {e_acquire, e_configure, e_execute, e_state_max};

        dq_logic_model() : _cur_state(nullptr),
            _device_model(nullptr),
            _calc_queue(1),
            _viewer_model(1280, 720, "Depth Quality Tool")
        {
            _states.push_back(new acquire_cam);
            _states.push_back(new configure_cam);
            _states.push_back(new generate_metrics);

            _cur_state = _states[e_acquire];
        }

        ~dq_logic_model()
        {
            _cur_state = nullptr;
            _states.clear();
        }

        operator bool()
        {
            auto res = true;

            // Update logic models
            _cur_state->update(this);

            // Update rendering subsystem
            if (!_viewer_model) res = false;

            //Render internal models
            std::cout << " render GUIS" << std::endl;
            //app_model.update();
            //metrics.render();
            ///dev_model.render()
            // stream.render();

            return res;
        }

        void use_device(rs2::device &dev);

        void enqueue_for_processing(rs2::frame &depth_frame)
        {
            _calc_queue.enqueue(depth_frame);
        }

        void upload(rs2::frameset &frameset);

    private:

        class app_state
        {
        public:
            virtual void update(dq_logic_model *dq) abstract;
        };

        class acquire_cam : public app_state
        {
            void update(dq_logic_model *dq);
        };

        class configure_cam : public app_state
        {
            void update(dq_logic_model *dq);
        };

        class generate_metrics : public app_state
        {
            void update(dq_logic_model *dq);
            void set_state(e_states state);
        };

        void set_state(e_states state) { _cur_state = _states[state]; };

        app_state*                  _cur_state;
        std::vector<app_state*>     _states;

        std::unique_ptr<device_model>   _device_model;
        dq_viewer_model                 _viewer_model;
        std::vector<stream_model>       _streams_models;
        frame_queue                     _calc_queue;
        std::mutex                      _m;

        std::string                     _error_message;
        mouse_info                      _mouse;
    };
}