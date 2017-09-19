#include <iostream>
#include "depth_quality_model.h"

using namespace rs2_depth_quality;

namespace rs2_depth_quality
{
    void dq_logic_model::acquire_cam::update(dq_logic_model *dq)
    {
        if (dq->_device_model.get())
            dq->set_state(e_states::e_execute);
        //std::cout << __FUNCTION__ << std::endl;
    }

    void dq_logic_model::configure_cam::update(dq_logic_model *dq)
    {
        //std::cout << __FUNCTION__ << std::endl;
    }

    void dq_logic_model::generate_metrics::update(dq_logic_model *dq)
    {
        //std::cout << __FUNCTION__ << std::endl;
    }

    void dq_logic_model::use_device(rs2::device dev)
    {
        _device_model = std::shared_ptr<rs2::device_model>(new device_model(dev, _error_message, _viewer_model));
        _viewer_model._dev_model = _device_model;

        // Connect the device_model to the viewer_model
        for (auto&& sub : _device_model.get()->subdevices)
        {
            auto profiles = sub->get_selected_profiles();
            sub->streaming = true;      // The streaming activated externally to the device_model
            for (auto&& profile : profiles)
            {
                _viewer_model.streams[profile.unique_id()].dev = sub;
            }
        }
    }

    void dq_logic_model::upload(rs2::frameset &frameset)
    {
        // Upload new frames for rendering
        for (size_t i=0; i < frameset.size(); i++)
            _viewer_model.upload_frame(frameset[i]);
    }
}
