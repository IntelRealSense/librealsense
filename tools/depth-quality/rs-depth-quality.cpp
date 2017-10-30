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

    metric zaccuracy = model.make_metric(
                       "Z Accuracy", 0, 100, "%",
                       "Z Accuracy given Ground Truth\n"
                       "Preformed on the depth image in the ROI\n"
                       "Calculate the depth-errors map\n"
                       "i.e., Image – GT (signed values).\n"
                       "Calculate the median of the depth-errors\n");

    metric avg = model.make_metric(
                 "Average Error", 0, 10, "(mm)",
                 "Average Distance from Plane Fit\n"
                 "This metric approximates a plane within\n"
                 "the ROI and calculates the average\n"
                 "distance of points in the ROI\n"
                 "from that plane, in mm");

    metric rms = model.make_metric(
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
                 "             i=0    ");

    metric fill = model.make_metric(
                  "Fill-Rate", 0, 100, "%",
                  "Fill Rate\n"
                  "Percentage of pixels with valid depth\n"
                  "values out of all pixels within the ROI\n");

    // ===============================
    //       Metrics Calculation
    // ===============================

    model.on_frame([&](
        const std::vector<rs2::float3>& points, 
        const rs2::plane p,
        const rs2::region_of_interest roi,
        const float baseline_mm, 
        const float focal_length_pixels, 
        const float* ground_thruth_mm)
    {
        const float TO_METERS = 0.001f;
        const float TO_MM = 1000.f;
        const float TO_PERCENT = 100.f;

        const float bf_factor = baseline_mm * focal_length_pixels * TO_METERS; // also convert point units from mm to meter

        // Calculate the distances of all points in the ROI to the fitted plane
        std::vector<float> distances;
        std::vector<float> disparities;
        std::vector<float> errors;

        // Reserve memory for the data
        distances.reserve(points.size());
        disparities.reserve(points.size());
        if (ground_thruth_mm) errors.reserve(points.size());

        // Calculate the distance and disparity errors from the point cloud to the fitted plane
        for (auto point : points)
        {
            // Find distance from point to the reconstructed plane
            auto dist2plane = p.a*point.x + p.b*point.y + p.c*point.z + p.d;
            // Project the point to plane in 3D and find distance to the intersection point
            rs2::float3 plane_intersect = { float(point.x - dist2plane*p.a),
                                            float(point.y - dist2plane*p.b),
                                            float(point.z - dist2plane*p.c) };

            // Store distance, disparity and error
            distances.push_back(std::fabs(dist2plane) * TO_MM);
            disparities.push_back(bf_factor / point.length() - bf_factor / plane_intersect.length());
            if (ground_thruth_mm) errors.push_back(point.z * TO_MM - *ground_thruth_mm);
        }

        // Show Z accuracy metric only when Ground Truth is available
        zaccuracy->visible(ground_thruth_mm != nullptr);
        if (ground_thruth_mm && *ground_thruth_mm > 0)
        {
            std::sort(begin(errors), end(errors));
            auto median = errors[errors.size() / 2];
            auto accuracy = TO_PERCENT * (fabs(median) / *ground_thruth_mm);
            zaccuracy->add_value(accuracy);
        }

        // Calculate average distance from the plane fit
        double total_distance = 0;
        for (auto dist : distances) total_distance += dist;
        float avg_dist = total_distance / distances.size();
        avg->add_value(avg_dist);

        // Calculate RMS
        double total_sq_disparity_diff = 0;
        for (auto disparity : disparities)
        {
            total_sq_disparity_diff += disparity*disparity;
        }
        // Calculate Subpixel RMS for Stereo-based Depth sensors
        auto rms_val = static_cast<float>(std::sqrt(total_sq_disparity_diff / points.size()));
        rms->add_value(rms_val);

        // Calculate fill ratio relative to the ROI
        auto fill_ratio = points.size() / float((roi.max_x - roi.min_x)*(roi.max_y - roi.min_y)) * TO_PERCENT;
        fill->add_value(fill_ratio);
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
