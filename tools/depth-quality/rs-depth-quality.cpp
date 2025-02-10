// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-24 Intel Corporation. All Rights Reserved.

#include "depth-quality-model.h"

#include <common/cli.h>

#include <librealsense2/rs.hpp>
#include <numeric>
#include <rs-config.h>

int main(int argc, const char * argv[]) try
{
    rs2::cli cmd( "rs-depth-quality" );
    auto settings = cmd.process( argc, argv );

    rs2::context ctx( settings.dump() );
    rs2::ux_window window("Depth Quality Tool", ctx);
    
#ifdef BUILD_EASYLOGGINGPP
    bool const disable_log_to_console = cmd.debug_arg.getValue();
#else
    bool const disable_log_to_console = false;
#endif
    rs2::depth_quality::tool_model model( ctx, disable_log_to_console );

    using namespace rs2::depth_quality;

    // ===============================
    //       Metrics Definitions
    // ===============================
    metric fill = model.make_metric(
        "Fill-Rate", 0, 100, false, "%",
        "Fill Rate.\n"
        "Percentage of pixels with valid depth\n"
        "values out of all pixels within the ROI\n");

    metric z_accuracy = model.make_metric(
                "Z Accuracy", -10, 10, true, "%",
                "Z-Accuracy given Ground Truth (GT)\n"
                " as percentage of the range.\n"
                "Algorithm:\n"
                "1. Transpose Z values from the Fitted to the GT plane\n"
                "2. Calculate depth errors:\n"
                " err= signed(Transposed Z - GT).\n"
                "3. Retrieve the median of the depth errors:\n"
                "4. Interpret results:\n"
                " - Positive value indicates that the Plane Fit\n"
                "is further than the Ground Truth (overshot)\n"
                " - Negative value indicates the Plane Fit\n"
                "is in front of Ground Truth (undershot)\n");

    metric plane_fit_rms_error = model.make_metric(
                "Plane Fit RMS Error", 0.f, 5.f, true, "%",
                "Plane Fit RMS Error .\n"
                "This metric provides RMS of Z-Error (Spatial Noise)\n"
                "and is calculated as follows:\n"
                "Zi - depth range of i-th pixel (mm)\n"
                "Zpi -depth of Zi projection onto plane fit (mm)\n"
                "               n      \n"
                "RMS = SQRT((SUM(Zi-Zpi)^2)/n)\n"
                 "             i=1    ");

    metric sub_pixel_rms_error = model.make_metric(
                 "Subpixel RMS Error", 0.f, 1.f, true, "pixel",
                 "Subpixel RMS Error .\n"
                 "This metric provides the subpixel accuracy\n"
                 "and is calculated as follows:\n"
                 "Zi - depth range of i-th pixel (mm)\n"
                 "Zpi -depth of Zi projection onto plane fit (mm)\n"
                 "BL - optical baseline (mm)\n"
                 "FL - focal length, as a multiple of pixel width\n"
                 "Di = BL*FL/Zi; Dpi = Bl*FL/Zpi\n"
                 "              n      \n"
                 "RMS = SQRT((SUM(Di-Dpi)^2)/n)\n"
                 "             i=1    ");

    metric temporal_noise = model.make_metric(
        "Temporal Noise", 0.f, 100.f, true, "mm",
        "Temporal Noise .\n"
        "This metric provides the depth temporal noise\n"
        "and is calculated as follows:\n"
        "Input - N images of Depth_Image\n"
        "Loop over the N images and create Depth_Tensor\n"
        "Depth_Tensor = (Depth_Image, N)\n"
        "Remove all zeros from Depth_Tensor\n"
        "COMPUTE STD_Matrix = STD of Depth_Tensor(x, y, all)\n"
        "COMPUTE Median_STD = 50% percentile value of STD_Matrix\n"
        "Temporal Noise = Median_STD\n");


    // ===============================
    //       Metrics Calculation
    // ===============================

    model.on_frame([&](
        const std::vector<rs2::float3>& points,
        const rs2::plane p,
        const rs2::region_of_interest roi,
        const float baseline_mm,
        const rs2_intrinsics* intrin,
        const int ground_truth_mm,
        const bool plane_fit,
        const float plane_fit_to_ground_truth_mm,
        const float distance_mm,
        bool record,
        std::vector<single_metric_data>& samples)
    {
        float TO_METERS = model.get_depth_scale();
        static const float TO_MM = 1000.f;
        static const float TO_PERCENT = 100.f;

        // Calculate fill rate relative to the ROI
        auto fill_rate = points.size() / float((roi.max_x - roi.min_x)*(roi.max_y - roi.min_y)) * TO_PERCENT;
        fill->add_value(fill_rate);
        if(record) samples.push_back({fill->get_name(),  fill_rate });

        if (!plane_fit) return;

        const float bf_factor = baseline_mm * intrin->fx * TO_METERS; // also convert point units from mm to meter

        std::vector<rs2::float3> points_set;
        for (auto point : points)
        {
            float pixel[2] = { point.x, point.y };
            float pt[3];
            rs2_deproject_pixel_to_point(pt, intrin, pixel, point.z);
            points_set.push_back({ pt[0], pt[1], pt[2] });
        }
        
        std::vector<float> distances;
        std::vector<float> disparities;
        std::vector<float> gt_errors;

        // Reserve memory for the data
        distances.reserve(points.size());
        disparities.reserve(points.size());
        if (ground_truth_mm) gt_errors.reserve(points.size());

        // Remove outliers [below 0.5% and above 99.5%)
        std::sort(points_set.begin(), points_set.end(), [](const rs2::float3& a, const rs2::float3& b) { return a.z < b.z; });
        size_t outliers = points_set.size() / 200;
        points_set.erase(points_set.begin(), points_set.begin() + outliers); // crop min 0.5% of the dataset
        points_set.resize(points_set.size() - outliers); // crop max 0.5% of the dataset

        // Convert Z values into Depth values by aligning the Fitted plane with the Ground Truth (GT) plane
        // Calculate distance and disparity of Z values to the fitted plane.
        // Use the rotated plane fit to calculate GT errors
        for (auto point : points_set)
        {
            // Find distance from point to the reconstructed plane
            auto dist2plane = p.a*point.x + p.b*point.y + p.c*point.z + p.d;
            // Project the point to plane in 3D and find distance to the intersection point
            rs2::float3 plane_intersect = { float(point.x - dist2plane*p.a),
                                            float(point.y - dist2plane*p.b),
                                            float(point.z - dist2plane*p.c) };

            // Store distance, disparity and gt- error
            distances.push_back(dist2plane * TO_MM);
            disparities.push_back(bf_factor / point.length() - bf_factor / plane_intersect.length());
            // The negative dist2plane represents a point closer to the camera than the fitted plane
            if (ground_truth_mm) gt_errors.push_back(plane_fit_to_ground_truth_mm + (dist2plane * TO_MM));
        }

        // Show Z accuracy metric only when Ground Truth is available
        z_accuracy->enable(ground_truth_mm > 0);
        if (ground_truth_mm)
        {
            std::sort(begin(gt_errors), end(gt_errors));
            auto gt_median = gt_errors[gt_errors.size() / 2];
            auto accuracy = TO_PERCENT * (gt_median / ground_truth_mm);
            z_accuracy->add_value(accuracy);
            if (record) samples.push_back({ z_accuracy->get_name(),  accuracy });
        }

        // Calculate Sub-pixel RMS for Stereo-based Depth sensors
        double total_sq_disparity_diff = 0;
        for (auto disparity : disparities)
        {
            total_sq_disparity_diff += disparity*disparity;
        }
        auto rms_subpixel_val = static_cast<float>(std::sqrt(total_sq_disparity_diff / disparities.size()));
        sub_pixel_rms_error->add_value(rms_subpixel_val);
        if (record) samples.push_back({ sub_pixel_rms_error->get_name(),  rms_subpixel_val });

        // Calculate Plane Fit RMS  (Spatial Noise) mm
        double plane_fit_err_sqr_sum = std::inner_product(distances.begin(), distances.end(), distances.begin(), 0.);
        auto rms_error_val = static_cast<float>(std::sqrt(plane_fit_err_sqr_sum / distances.size()));
        auto rms_error_val_per = TO_PERCENT * (rms_error_val / distance_mm);
        plane_fit_rms_error->add_value(rms_error_val_per);
        if (record)
        {
            samples.push_back({ plane_fit_rms_error->get_name(),  rms_error_val_per });
            samples.push_back({ plane_fit_rms_error->get_name() + " mm",  rms_error_val });
        }

        // Calculate Temporal Noise
        // Temporal Noise is calculated as the median of the standard deviation of the depth values
        // across a set of N frames.
        // The standard deviation is calculated for each pixel across the N frames.
        // The median of the standard deviations is the Temporal Noise.
        // The Temporal Noise is calculated in mm.
        int num_images = 40;
        if (rs2::config_file::instance().contains(rs2::configurations::DQT::temporal_noise_num_images))
        {
            num_images = rs2::config_file::instance().get(rs2::configurations::DQT::temporal_noise_num_images);
        }
        else
        {
            rs2::config_file::instance().set(rs2::configurations::DQT::temporal_noise_num_images, num_images);
        }
        
        static std::deque<std::vector<rs2::float3>> depth_images; // FIFO buffer for depth images

        // Add the current depth image to the FIFO buffer
        depth_images.push_back(points);
        if (depth_images.size() > num_images) {

            //start calculate only once we accumulate 'num_images'
            depth_images.pop_front();

            // Create Depth_Tensor
            std::vector<std::vector<std::vector<float>>> depth_tensor(roi.max_x, std::vector<std::vector<float>>(roi.max_y, std::vector<float>(depth_images.size(), 0.0f)));

            // Fill Depth_Tensor with depth images
            for (size_t n = 0; n < depth_images.size(); ++n) {
                for (const auto& point : depth_images[n]) {
                    int x = static_cast<int>(point.x);
                    int y = static_cast<int>(point.y);
                    if (x >= roi.min_x && x < roi.max_x && y >= roi.min_y && y < roi.max_y) {
                        depth_tensor[x - roi.min_x][y - roi.min_y][n] = point.z * TO_MM;
                    }
                }
            }

            // Remove all zeros from Depth_Tensor
            for (int y = 0; y < roi.max_y; ++y) {
                for (int x = 0; x < roi.max_x; ++x) {
                    depth_tensor[x][y].erase(std::remove(depth_tensor[x][y].begin(), depth_tensor[x][y].end(), 0.0f), depth_tensor[x][y].end());
                }
            }

            // Compute STD_Matrix
            std::vector<std::vector<float>> std_matrix(roi.max_x - roi.min_x, std::vector<float>(roi.max_y - roi.min_y, 0.0f));
            for (int y = 0; y < roi.max_y - roi.min_y; ++y) {
                for (int x = 0; x < roi.max_x - roi.min_x; ++x) {
                    if (!depth_tensor[x][y].empty()) {
                        float mean = std::accumulate(depth_tensor[x][y].begin(), depth_tensor[x][y].end(), 0.0f) / depth_tensor[x][y].size();
                        float variance = 0.0f;
                        for (float value : depth_tensor[x][y]) {
                            variance += (value - mean) * (value - mean);
                        }
                        variance /= depth_tensor[x][y].size();
                        std_matrix[x][y] = std::sqrt(variance);
                    }
                }
            }

            // Compute Median_STD
            std::vector<float> std_values;
            for (int y = 0; y < roi.max_y - roi.min_y; ++y) {
                for (int x = 0; x < roi.max_x - roi.min_x; ++x) {
                    std_values.push_back(std_matrix[x][y]);
                }
            }

            std::sort(std_values.begin(), std_values.end());
            float median_std = std_values[std_values.size() / 2];

            temporal_noise->add_value(median_std);

            if (record) {
                samples.push_back({ temporal_noise->get_name(), median_std });
            }
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
