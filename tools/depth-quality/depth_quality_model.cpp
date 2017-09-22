#include <iostream>
#include <string>
#include "depth_quality_viewer.h"
#include "depth_quality_model.h"

using namespace std;

namespace rs2
{
    namespace depth_profiler
    {
        void depth_profiler_model::acquire_cam::update(depth_profiler_model& app)
        {
            if (app._device_model.get())
                app.set_state(e_states::e_configure);
        }

        // A transient state required to acquire and distribute depth stream specifics among relevant parties
        void depth_profiler_model::configure_cam::update(depth_profiler_model& app)
        {
            auto dev = app._device_model.get()->dev;
            auto depth_scale_units = dev.first<depth_sensor>().get_depth_scale();
            auto profiles = app._device_model.get()->subdevices.front()->get_selected_profiles();
            auto depth_intrinsic = profiles.front().as<video_stream_profile>().get_intrinsics();

            {
                std::lock_guard<std::mutex> lock(app._m);
                app._metrics_model.update_stream_attributes(depth_intrinsic, depth_scale_units);
            }

            app.set_state(e_states::e_profile);
        }

        void depth_profiler_model::profile_metrics::update(depth_profiler_model& app)
        {
            // Update active ROI for computation
            std::lock_guard<std::mutex> lock(app._m);
            // Evgeni - get actual ROI selection from stream_model
            //auto roi = app._viewer_model.streams[0].dev->roi_rect; 
            //app._metrics_model.update_frame_attributes({ (int)roi.x, (int)roi.y, (int)(roi.x+roi.w), (int)(roi.y + roi.h)});
            auto res = app._viewer_model.streams[0].size;
            region_of_interest roi{ int(res.x / 3), int(res.y / 3), int(res.x * 2 / 3), int(res.y * 2 / 3) };
            app._metrics_model.update_frame_attributes(roi);
        }

        void depth_profiler_model::use_device(rs2::device dev)
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

        void depth_profiler_model::upload(rs2::frameset &frameset)
        {
            // Upload new frames for rendering
            for (size_t i = 0; i < frameset.size(); i++)
                _viewer_model.upload_frame(frameset[i]);
        }

        metrics_model::metrics_model(frame_queue* _input_queue, std::mutex &m) :
            _frame_queue(_input_queue),
            _depth_scale_units(0.f), _active(true),
            avg_plot("AVG", 0, 10, { 180, 50 }, " (mm)"),
            std_plot("STD", 0, 10, { 180, 50 }, " (mm)"),
            fill_plot("FILL", 0, 100, { 180, 50 }, "%"),
            dist_plot("DIST", 0, 5, { 180, 50 }, " (m)"),
            angle_plot("ANGLE", 0, 180, { 180, 50 }, " (deg)"),
            out_plot("OUTLIERS", 0, 100, { 180, 50 }, "%")
        {
            _worker_thread = std::thread([&]() {

                while (_active)
                {
                    rs2::frame depth_frame;
                    try { depth_frame = _frame_queue->wait_for_frame(10); }
                    catch (...) { continue; }

                    if (depth_frame)
                    {
                        auto stream_type = depth_frame.get_profile().stream_type();

                        if (RS2_STREAM_DEPTH == stream_type)
                        {
                            float su = 0;
                            rs2_intrinsics intrin;
                            region_of_interest roi;
                            {
                                std::lock_guard<std::mutex> lock(m);
                                su = _depth_scale_units;
                                intrin = _depth_intrinsic;
                                roi = _roi;
                            }

                            auto metrics = analyze_depth_image(depth_frame, su, &intrin, _roi);

                            {
                                std::lock_guard<std::mutex> lock(m);
                                _latest_metrics = metrics;
                            }
                            cout << "Depth Average distance is : " << _latest_metrics.depth.avg_dist << endl;
                            cout << "Standard_deviation is : " << _latest_metrics.depth.std_dev << endl;
                            cout << "Fillrate is : " << _latest_metrics.non_null_pct << endl;
                        }
                        else
                        {
                            std::cout << __FUNCTION__ << " : unexpected frame type received - " << rs2_stream_to_string(stream_type) << std::endl;
                        }
                    }
                }
            });
        }

        metrics_model::~metrics_model()
        {
            _active = false;
            _worker_thread.join();
            // Evgeni flush queue hack - not-empty frame_queue delete fails on continuation
            try { _frame_queue->wait_for_frame(10); }
            catch (...) {}
        }

        void metrics_model::render()
        {
            metrics data = true/*use_rect_fitting*/ ? _latest_metrics.plane : _latest_metrics.depth; // Evgeni

            avg_plot.add_value(data.avg_dist);
            std_plot.add_value(data.std_dev);
            fill_plot.add_value(_latest_metrics.non_null_pct);
            dist_plot.add_value(data.distance);
            angle_plot.add_value(data.angle);
            out_plot.add_value(data.outlier_pct);

            avg_plot.plot();
            std_plot.plot();
            fill_plot.plot();
            dist_plot.plot();
            angle_plot.plot();
            out_plot.plot();

        }

        void metrics_model::visualize(snapshot_metrics stats, int w, int h, bool plane) const
        {
            float x_scale = w / float(stats.width);
            float y_scale = h / float(stats.height);
            //ImGui::PushStyleColor(ImGuiCol_WindowBg, );
            //ImGui::SetNextWindowPos({ float(area.x), float(area.y) });
            //ImGui::SetNextWindowSize({ float(area.size), float(area.size) });

            ImGui::GetWindowDrawList()->AddRectFilled({ float(stats.roi.min_x)*x_scale, float(stats.roi.min_y)*y_scale }, { float(stats.roi.max_x)*x_scale, float(stats.roi.max_y)*y_scale },
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.f + ((plane) ? stats.plane.fit : stats.depth.fit), 1.f - ((plane) ? stats.plane.fit : stats.depth.fit), 0, 0.25f)), 5.f, 15.f);

            //std::stringstream ss; ss << stats.roi.min_x << ", " << stats.roi.min_y;
            //auto s = ss.str();
            /*ImGui::Begin(s.c_str(), nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize);*/


            /*if (ImGui::IsItemHovered())
            ImGui::SetTooltip(s.c_str());*/

            //ImGui::End();
            //ImGui::PopStyleColor();
        }
    }
}
