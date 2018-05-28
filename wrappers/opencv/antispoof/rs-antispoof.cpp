// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>   // Include OpenCV API
#include "../cv-helpers.hpp"    // Helper functions for conversions between RealSense and OpenCV

int main(int argc, char * argv[]) try
{
    using namespace rs2;

    // Define colorizer and align processing-blocks
    colorizer colorize;
    align align_to(RS2_STREAM_COLOR);

    // Start the camera
    pipeline pipe;
    auto profile = pipe.start();

    auto sensor = profile.get_device().first<rs2::depth_sensor>();

    auto range = sensor.get_option_range(RS2_OPTION_VISUAL_PRESET);
    for (auto i = range.min; i < range.max; i += range.step)
        if (std::string(sensor.get_option_value_description(RS2_OPTION_VISUAL_PRESET, i)) == "High Accuracy")
            sensor.set_option(RS2_OPTION_VISUAL_PRESET, i);

    using namespace cv;
    const auto window_name = "Display Image";
    namedWindow(window_name, WINDOW_AUTOSIZE);

    CascadeClassifier face_cascade;
    face_cascade.load("haarcascade_frontalface_alt2.xml");

    while (waitKey(1) < 0 && cvGetWindowHandle(window_name))
    {
        frameset data = pipe.wait_for_frames();
        // Make sure the frameset is spatialy aligned 
        // (each pixel in depth image corresponds to the same pixel in the color image)
        frameset aligned_set = align_to.process(data);
        auto depth = aligned_set.get_depth_frame();
        auto color_mat = frame_to_mat(aligned_set.get_color_frame());

        std::vector<Rect> faces;
        std::vector<int> neighbours;
        std::vector<double> weights;
        face_cascade.detectMultiScale(color_mat, faces, neighbours, weights, 1.1, 2, 0 | CV_HAAR_SCALE_IMAGE, Size(30, 30), cv::Size(), true);

        for (int i = 0; i < faces.size(); i++)
        {
            if (weights[i] > 50)
            {
                try
                {
                    float a, b, c, d, rms;
                    region_of_interest roi{ faces[i].x, faces[i].y, 
                        faces[i].x + faces[i].width, 
                        faces[i].y + faces[i].height };
                    depth.fit_plane(roi, 2, 0.05f, a, b, c, d, rms);

                    if (rms < 0.002f)
                    {
                        putText(color_mat, "FAKE",
                            Point(faces[i].x + faces[i].width / 2 - 25, faces[i].y + faces[i].height / 2),
                            CV_FONT_HERSHEY_SIMPLEX,
                            0.8, Scalar(0, 0, 255), 2, LINE_AA);
                    }
                    else if (rms > 0.004f)
                    {
                        Point center(faces[i].x + faces[i].width*0.5, faces[i].y + faces[i].height*0.5);
                        ellipse(color_mat, center, Size(faces[i].width*0.5, faces[i].height*0.5), 0, 0, 360, Scalar(255, 255, 255), 1, 8, 0);
                    }
                }
                catch (const rs2::error& e)
                {
                    // Not sufficient depth data to make the decision
                }
            }
        }

        cv::imshow(window_name, color_mat);
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



