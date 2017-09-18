#include <iostream>
#include "depth_quality_states.h"

using namespace rs2_depth_quality;

namespace rs2_depth_quality
{
    void depth_quality_logic::acquire_cam::update(depth_quality_logic *dq)
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    void depth_quality_logic::configure_cam::update(depth_quality_logic *dq)
    {
        std::cout << __FUNCTION__ << std::endl;
    }

    void depth_quality_logic::generate_metrics::update(depth_quality_logic *dq)
    {
        //std::cout << __FUNCTION__ << std::endl;
    }
}
