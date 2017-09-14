#include <iostream>
#include "depth_quality_model.h"

using namespace rs2_depth_quality;

namespace rs2_depth_quality
{
    void dq_logic_model::acquire_cam::update(dq_logic_model *dq)
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    void dq_logic_model::acquire_cam::set_state(e_states state)
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    void dq_logic_model::configure_cam::update(dq_logic_model *dq)
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    void dq_logic_model::configure_cam::set_state(e_states state)
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    void dq_logic_model::generate_metrics::update(dq_logic_model *dq)
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    void dq_logic_model::generate_metrics::set_state(e_states state)
    {
        std::cout << __FUNCTION__ << std::endl;
    }
}
