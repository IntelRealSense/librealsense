#include <iostream>
#include <string>
//#include <librealsense2/rs.h>
#include <librealsense2/hpp/rs_sensor.hpp>
#include <librealsense2/rs.hpp>
#include "rendering.h"
#include "depth_quality_model.h"
#include "depth_quality_viewer.h"

namespace rs2
{
    namespace depth_profiler
    {
        void depth_profiler_viewer::render_metrics_panel()
        {
            {
                // Set metrics panel position and size
                ImGui::SetNextWindowPos({ (float)(_width - viewer_ui_traits::metrics_panel_width + 5),
                    (float)viewer_ui_traits::control_panel_height });
                ImGui::SetNextWindowSize({ (float)viewer_ui_traits::metrics_panel_width,
                    float(_height - viewer_ui_traits::control_panel_height) });
                ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
                ImGui::PushStyleColor(ImGuiCol_WindowBg, sensor_bg);

                ImGui::Begin("Depth Metrics Panel", nullptr, viewer_ui_traits::imgui_flags | ImGuiWindowFlags_AlwaysVerticalScrollbar);

                _metrics_model->render();

                ImGui::End();
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();
            }
        }
    }
}
