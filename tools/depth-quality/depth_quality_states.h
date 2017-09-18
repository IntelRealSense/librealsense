#pragma once
#include <vector>

namespace rs2_depth_quality
{
    class depth_quality_logic
    {
    public:
        enum e_states {e_aquire,e_configure, e_execute, e_state_max};

        depth_quality_logic()  : _cur_state(nullptr)
        {
            _states.push_back(new acquire_cam);
            _states.push_back(new configure_cam);
            _states.push_back(new generate_metrics);

            _cur_state = _states[e_aquire];
        }

        void set_state(e_states state) { _cur_state = _states[state]; }
        ~depth_quality_logic()
        {
            _cur_state = nullptr;
            _states.clear();
        }

        void update()
        {
            _cur_state->update(this);
        }

    private:

        class app_state
        {
        public:
            virtual void update(depth_quality_logic *dq) abstract;
        };

        class acquire_cam : public app_state
        {
            void update(depth_quality_logic *dq);
        };

        class configure_cam : public app_state
        {
            void update(depth_quality_logic *dq);
        };

        class generate_metrics : public app_state
        {
            void update(depth_quality_logic *dq);
        };

        app_state*  _cur_state;
        std::vector<app_state*> _states;
    };
}