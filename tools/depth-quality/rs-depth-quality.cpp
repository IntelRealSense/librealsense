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
    
    auto fill = model.make_metric(
                "Fill-Rate", 0, 100, "%",
                "Fill Rate\n"
                "Percentage of pixels with valid depth values\n"
                "out of all pixels within the ROI")->
                set(metric_plot::GREEN_RANGE,  90, 100)->
                set(metric_plot::YELLOW_RANGE, 50, 90)->
                set(metric_plot::RED_RANGE,    0,  50);

    auto dist = model.make_metric(
                "Distance", 0, 5, "(m)",
                "Approximate Distance\n"
                "When facing a flat wall at right angle\n"
                "this metric estimates the distance\n"
                "in meters to that wall")->
                set(metric_plot::GREEN_RANGE,   0, 2)->
                set(metric_plot::YELLOW_RANGE,  2, 3)->
                set(metric_plot::RED_RANGE,     3, 7);

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

    model.on_frame([&](const std::vector<rs2::float3>& points, rs2::plane p, rs2::region_of_interest roi)
    {
        const float outlier_crop = 2.5; // Treat 5% of the extreme points as outliers

        std::vector<double> distances; // Calculate the distances of all points in the ROI to the fitted plane
        distances.reserve(points.size());
        for (auto point : points)
            distances.push_back(std::abs(p.a*point.x + p.b*point.y + p.c*point.z + p.d) * 1000);

        std::sort(distances.begin(), distances.end()); // Ignore 5% that are further away from the mean
        int n_outliers = int(distances.size() * (outlier_crop / 100));
        auto begin = distances.begin() + n_outliers, end = distances.end() - n_outliers;

        // Calculate average distance from the plane fit
        double total_distance = 0;
        for (auto itr = begin; itr < end; ++itr) total_distance += *itr;
        float avg_dist = total_distance / (distances.size() - 2 * n_outliers);
        avg->add_value(avg_dist);

        // Calculate STD
        double total_sq_diffs = 0;
        for (auto itr = begin; itr < end; ++itr)
        {
            total_sq_diffs += std::pow(*itr - avg_dist, 2);
        }
        std->add_value(std::sqrt(total_sq_diffs / points.size()));

        dist->add_value(-p.d); // Distance of origin (the camera) from the plane is encoded in parameter D of the plane
        angle->add_value(std::acos(std::abs(p.c)) / M_PI * 180.); // Angle can be calculated from param C

        // Calculate fill ration relative to the ROI
        fill->add_value(points.size() / float((roi.max_x - roi.min_x)*(roi.max_y - roi.min_y)) * 100);
    });

    // ===============================
    //         Rendering Loop         
    // ===============================

    window.on_load = [&]() { model.start(); };

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
