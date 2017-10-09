// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include "depth-quality-model.h"

int main(int argc, const char * argv[]) try
{
    rs2::depth_quality::tool_model model;
    rs2::ux_window window("Depth Quality Tool");

    using namespace rs2::depth_quality;

    // ===============================
    //       Metrics Definitions
    // ===============================

    auto avg = model.make_metric(
               "Average Error", 0, 10, "(mm)",
               "Average Distance from Plane Fit\n"
               "This metric approximates a plane within\n"
               "the ROI and calculates the average\n"
               "distance of points in the ROI\n"
               "from that plane, in mm")->
               set(metric_plot::GREEN_RANGE,  0, 1)->
               set(metric_plot::YELLOW_RANGE, 1, 7)->
               set(metric_plot::RED_RANGE,    7, 1000);

    auto std = model.make_metric(
               "STD (Error)", 0, 10, "(mm)",
               "Standard Deviation from Plane Fit\n"
               "This metric approximates a plane within\n"
               "the ROI and calculates the\n"
               "standard deviation of distances\n"
               "of points in the ROI from that plane")->
               set(metric_plot::GREEN_RANGE,  0, 1)->
               set(metric_plot::YELLOW_RANGE, 1, 7)->
               set(metric_plot::RED_RANGE,    7, 1000);

    auto rms = model.make_metric(
                "Subpixel RMS", 0.f, 1.f, "(pixel)",
                "Normalized RMS .\n"
                "This metric provides the subpixel accuracy\n"
                "and is calculated as follows:\n"
                "Zi - depth range of i-th pixel (mm)\n"
                "Zpi -depth of Zi projection onto plane fit (mm)\n"
                "BL - optical baseline (mm)\n"
                "FL - focal length, as a multiple of pixel width\n"
                "Di = BL*FL/Zi; Dpi = Bl*FL/Zpi\n"
                "              n      \n"
                "RMS = SQRT((SUM(Di-Dpi)^2)/n)\n"
                "             i=0    \n")->
                set(metric_plot::GREEN_RANGE, 0, 0.1f)->
                set(metric_plot::YELLOW_RANGE, 0.1f, 0.5f)->
                set(metric_plot::RED_RANGE, 0.5f, 100.f);

    auto fill = model.make_metric(
                "Fill-Rate", 0, 100, "%",
                "Fill Rate\n"
                "Percentage of pixels with valid depth values\n"
                "out of all pixels within the ROI")->
                set(metric_plot::GREEN_RANGE,  90, 100)->
                set(metric_plot::YELLOW_RANGE, 50, 90)->
                set(metric_plot::RED_RANGE,    0,  50);

    auto dist = model.make_metric(
                "Distance", 0, 10000, "(mm)",
                "Approximate Distance\n"
                "When facing a flat wall at right angle\n"
                "this metric estimates the distance\n"
                "in meters of the pixel in the center of FOV")->
                set(metric_plot::GREEN_RANGE,   0, 2000)->
                set(metric_plot::YELLOW_RANGE,  2000, 3000)->
                set(metric_plot::RED_RANGE,     3000, 7000);

    auto angle = model.make_metric(
                 "Angle", 0, 180, "(deg)",
                 "Wall Angle\n"
                 "When facing a flat wall this metric\n"
                 "estimates the angle to the wall.")->
                 set(metric_plot::GREEN_RANGE,   -5,   5)->
                 set(metric_plot::YELLOW_RANGE,  -10,  10)->
                 set(metric_plot::RED_RANGE,     -100, 100);

    // ===============================
    //       Metrics Calculation
    // ===============================

    model.on_frame([&](const std::vector<rs2::float3>& points, rs2::plane p, rs2::region_of_interest roi,
        float baseline_mm, float focal_length_pixels, double timestamp_msec)
    {
        const double bf_factor = baseline_mm * focal_length_pixels * 0.001; // also convert point units from meter to mm
        metric_sample sample{ 0,timestamp_msec };

        //std::vector<double> distances; // Calculate the distances of all points in the ROI to the fitted plane
        std::vector<std::pair<double, double> > calc;   // Distances and disparities
        calc.reserve(points.size());        // Calculate the distances of all points in the ROI to the fitted plane

        // Calculate the distance and disparity errors from the point cloud to the fitted plane
        for (auto point : points)
        {
            // Find distance from point to the reconstructed plane
            auto dist2plane = p.a*point.x + p.b*point.y + p.c*point.z + p.d;
            // Project the point to plane in 3D and find distance to the intersection point
            rs2::float3 plane_intersect = { float(point.x - dist2plane*p.a),
                                            float(point.y - dist2plane*p.b),
                                            float(point.z - dist2plane*p.c) };

            // Store distance and disparity errors
            calc.emplace_back(std::make_pair(std::fabs(dist2plane) * 1000,
                                             bf_factor / point.length() - bf_factor / plane_intersect.length()));
        }

        std::sort(calc.begin(), calc.end()); // Filter out the X% of the samples that are further away from the mean
        auto begin = calc.begin(), end = calc.end();

        // Calculate average distance from the plane fit
        double total_distance = 0;
        for (auto itr = begin; itr < end; ++itr) total_distance += (*itr).first;
        float avg_dist = sample._val = total_distance / (calc.size());
        avg->add_value(sample);

        // Calculate STD and RMS
        double total_sq_diffs = 0;
        double total_sq_disparity_diff = 0;
        for (auto itr = begin; itr < end; ++itr)
        {
            total_sq_diffs += std::pow((*itr).first - avg_dist, 2);
            total_sq_disparity_diff += (*itr).second*(*itr).second;
        }
        sample._val = static_cast<float>(std::sqrt(total_sq_diffs / (points.size())));
        std->add_value(sample);

        dist->add_value({ static_cast<float>(-p.d*1000),timestamp_msec }); // Distance of origin (the camera) from the plane is encoded in parameter D of the plane
        sample._val = static_cast<float>(std::acos(std::abs(p.c)) / M_PI * 180.);
        angle->add_value(sample); // Angle can be calculated from param C

        // Calculate Subpixel RMS for Stereo-based Depth sensors
        sample._val = static_cast<float>(std::sqrt(total_sq_disparity_diff / (points.size())));
        rms->add_value(sample);

        // Calculate fill ratio relative to the ROI
        sample._val = points.size() / float((roi.max_x - roi.min_x)*(roi.max_y - roi.min_y)) * 100;
        fill->add_value(sample);
    });

    // ===============================
    //      Device plug-in/out handler
    // ===============================
    rs2::context ctx;
    std::mutex m;
    ctx.set_devices_changed_callback([&model, &window, &m](rs2::event_information info) mutable
    {
        auto dev = model.get_active_device();
        if (dev && info.was_removed(dev))
        {
            std::unique_lock<std::mutex> lock(m);
            model.reset(window);
        }
    });

    // ===============================
    //         Rendering Loop
    // ===============================

    window.on_load = [&]()
    {
        return model.start(window);
    };

    while(window)
    {
        model.render(window);
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error& e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
