#pragma once
#include <vector>
#include "model-views.h"

namespace rs2_depth_quality
{
    class dq_logic_model : public rs2::viewer_model
    {
    public:
        enum e_states {e_aquire,e_configure, e_execute, e_state_max};

        dq_logic_model() : cur_state(nullptr)
        {
            states.push_back(new acquire_cam);
            states.push_back(new configure_cam);
            states.push_back(new generate_metrics);

            cur_state = states[e_aquire];
        }

        ~dq_logic_model()
        {
            cur_state = nullptr;
            states.clear();
        }

        void update()
        {
            cur_state->update(this);
        }

    private:

        class app_state
        {
        public:
            virtual void update(dq_logic_model *dq) abstract;
            virtual void set_state(e_states state) abstract;
        };

        class acquire_cam : public app_state
        {
            void update(dq_logic_model *dq);
            void set_state(e_states state);
        };

        class configure_cam : public app_state
        {
            void update(dq_logic_model *dq);
            void set_state(e_states state);
        };

        class generate_metrics : public app_state
        {
            void update(dq_logic_model *dq);
            void set_state(e_states state);
        };

        app_state*  cur_state;
        std::vector<app_state*> states;

        //device_model
    };
}