// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2/rs_advanced_mode.hpp>
#include <opencv2/opencv.hpp>   // Include OpenCV API

#include <fstream>

#include "tiny-profiler.h"
#include "downsample.h"

rs2_intrinsics operator/(const rs2_intrinsics& i, float f)
{
    rs2_intrinsics  res = i;
    res.width /= f; res.height /= f;
    res.fx /= f;    res.fy /= f;
    res.ppx /= f;   res.ppy /= f;
    return res;
}

class high_confidence_filter : public rs2::filter
{
public:
    high_confidence_filter()
        : filter([this](rs2::frame f, rs2::frame_source& src) {
                scoped_timer t("collision_avoidance_filter");
                sdk_handle(f, src);
            })
    {
    }

private:
    void downsample(const cv::Mat& depth, const cv::Mat& ir)
    {
        scoped_timer t("Depth and IR Downsample");

        constexpr float DOWNSAMPLE_FRACTION = 1.0f / DOWNSAMPLE_FACTOR;

        if (_decimated_depth.cols != depth.cols / DOWNSAMPLE_FACTOR ||
            _decimated_depth.rows != depth.rows / DOWNSAMPLE_FACTOR)
        {
            _decimated_depth = cv::Mat(depth.rows / DOWNSAMPLE_FACTOR, depth.cols / DOWNSAMPLE_FACTOR, CV_16U);
        }

        downsample_min_4x4(depth, &_decimated_depth);
        cv::resize(ir, _decimated_ir, cv::Size(), DOWNSAMPLE_FRACTION, DOWNSAMPLE_FRACTION, cv::INTER_NEAREST);
    }

    void main_filter()
    {
        scoped_timer t("Main Filter");
        
        const auto num_sub_images = sub_areas.size();

        #pragma omp parallel for
        for (int i = 0; i < num_sub_images; i++)
        {
            filter_edges(&sub_areas[i]);
            filter_harris(&sub_areas[i]);
            cv::bitwise_or(sub_areas[i].edge_mask, sub_areas[i].harris_mask, sub_areas[i].combined_mask);

            // morphology: open(src, element) = dilate(erode(src,element))
            cv::morphologyEx(sub_areas[i].combined_mask, sub_areas[i].combined_mask,
                cv::MORPH_OPEN, cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3)));
            sub_areas[i].decimated_depth.copyTo(sub_areas[i].output_depth, sub_areas[i].combined_mask);
        }
    }

    void sdk_handle(rs2::frame& f, rs2::frame_source& src)
    {
        auto frames = f.as<rs2::frameset>();
        assert(frames);

        auto depth_frame = frames.get_depth_frame();
        assert(depth_frame);

        auto ir_frame = frames.get_infrared_frame();
        assert(ir_frame);

        if (!_output_ir_profile || _input_ir_profile.get() != ir_frame.get_profile().get())
        {
            auto p = ir_frame.get_profile().as<rs2::video_stream_profile>();
            auto intr = p.get_intrinsics() / DOWNSAMPLE_FACTOR;
            _input_ir_profile = p;
            _output_ir_profile = p.clone(p.stream_type(), p.stream_index(), p.format(),
                p.width() / DOWNSAMPLE_FACTOR, p.height() / DOWNSAMPLE_FACTOR, intr);
        }

        if (!_output_depth_profile || _input_depth_profile.get() != depth_frame.get_profile().get())
        {
            auto p = depth_frame.get_profile().as<rs2::video_stream_profile>();
            auto intr = p.get_intrinsics() / DOWNSAMPLE_FACTOR;
            _input_depth_profile = p;
            _output_depth_profile = p.clone(p.stream_type(), p.stream_index(), p.format(),
                p.width() / DOWNSAMPLE_FACTOR, p.height() / DOWNSAMPLE_FACTOR, intr);
        }

        cv::Mat matDepth(cv::Size(depth_frame.get_width(), depth_frame.get_height()), CV_16U, (void*)depth_frame.get_data(), cv::Mat::AUTO_STEP);
        cv::Mat matGray(cv::Size(ir_frame.get_width(), ir_frame.get_height()), CV_8U, (void*)ir_frame.get_data(), cv::Mat::AUTO_STEP);
        
        downsample(matDepth, matGray);

        init_matrices(_decimated_ir);

        main_filter();

        auto newW = ir_frame.get_width() / DOWNSAMPLE_FACTOR;
        auto newH = ir_frame.get_height() / DOWNSAMPLE_FACTOR;

        auto res_ir = src.allocate_video_frame(_output_ir_profile, ir_frame, 0,
            newW, newH, ir_frame.get_bytes_per_pixel() * newW);
        memcpy((void*)res_ir.get_data(), _decimated_ir.data, newW * newH);

        auto res_depth = src.allocate_video_frame(_output_depth_profile, depth_frame, 0,
            newW, newH, depth_frame.get_bytes_per_pixel() * newW, RS2_EXTENSION_DEPTH_FRAME);
        memcpy((void*)res_depth.get_data(), _depth_output.data, newW * newH * 2);

        std::vector<rs2::frame> fs{ res_ir, res_depth };
        auto cmp = src.allocate_composite_frame(fs);
        src.frame_ready(cmp);
    }

    static constexpr auto DOWNSAMPLE_FACTOR = 4;
    static constexpr size_t NUM_SUB_IMAGES = 4; // this number directly relates to how many threads are used

    struct sub_area 
    {
        cv::Mat decimated_depth;
        cv::Mat decimated_ir;
        cv::Mat edge_mask;
        cv::Mat harris_mask;
        cv::Mat combined_mask;
        cv::Mat output_depth;
        cv::Mat float_ir;
        cv::Mat corners;
        cv::Mat scharr_x;
        cv::Mat scharr_y;
        cv::Mat abs_scharr_x;
        cv::Mat abs_scharr_y;
    };

    std::vector<sub_area> sub_areas;

    // To avoid re-allocation every cycle some matrices are members
    cv::Mat _decimated_depth;
    cv::Mat _decimated_ir;
    cv::Mat _mask_edge;
    cv::Mat _mask_harris;
    cv::Mat _mask_combined;
    cv::Mat _depth_output;
    cv::Mat _ir_float;
    cv::Mat _corners;
    cv::Mat _scharr_x;
    cv::Mat _scharr_y;
    cv::Mat _abs_scharr_x;
    cv::Mat _abs_scharr_y;

    rs2::stream_profile _output_ir_profile;
    rs2::stream_profile _output_depth_profile;

    rs2::stream_profile _input_ir_profile;
    rs2::stream_profile _input_depth_profile;

    void filter_harris(sub_area* area)
    {
        area->decimated_ir.convertTo(area->float_ir, CV_32F);
        cv::cornerHarris(area->float_ir, area->corners, 2, 3, 0.04);
        cv::threshold(area->corners, area->corners, 300, 255, cv::THRESH_BINARY);
        area->corners.convertTo(area->harris_mask, CV_8U);
    }

    void filter_edges(sub_area* area)
    {
        cv::Scharr(area->decimated_ir, area->scharr_x, CV_16S, 1, 0);
        cv::convertScaleAbs(area->scharr_x, area->abs_scharr_x);
        cv::Scharr(area->decimated_ir, area->scharr_y, CV_16S, 0, 1);
        cv::convertScaleAbs(area->scharr_y, area->abs_scharr_y);
        cv::addWeighted(area->abs_scharr_y, 0.5, area->abs_scharr_y, 0.5, 0, area->edge_mask);
        cv::threshold(area->edge_mask, area->edge_mask, 192, 255, cv::THRESH_BINARY);
    }

    void init_matrices(const cv::Mat& ir_resized) 
    {
        const int sizeX = ir_resized.cols;
        const int sizeY = ir_resized.rows;
        const int sizeYperSubImage = sizeY / NUM_SUB_IMAGES;
        const int sizeXtimesSizeY = sizeX * sizeY;

        // test on one case if something has changed - this would then apply to all other image matrices as well
        bool needToReinitialize = _mask_edge.size() != ir_resized.size() || _mask_edge.type() != CV_8U;

        if (!needToReinitialize)
        {
            // this needs resetting every frame
            memset(_depth_output.data, 0, 2 * sizeXtimesSizeY);
            return;
        }

        static_assert(NUM_SUB_IMAGES >= 1 && NUM_SUB_IMAGES < 64, "NUM_SUB_IMAGES value might be wrong");

        _mask_edge      = cv::Mat(ir_resized.size(), CV_8U);
        _mask_harris    = cv::Mat(ir_resized.size(), CV_8U);
        _mask_combined  = cv::Mat(ir_resized.size(), CV_8U);
        _depth_output   = cv::Mat(ir_resized.size(), CV_16U);
        _ir_float       = cv::Mat(ir_resized.size(), CV_32F);
        _corners        = cv::Mat(ir_resized.size(), CV_32F);
        _scharr_x       = cv::Mat(ir_resized.size(), CV_16S);
        _scharr_y       = cv::Mat(ir_resized.size(), CV_16S);
        _abs_scharr_x   = cv::Mat(ir_resized.size(), CV_8U);
        _abs_scharr_y   = cv::Mat(ir_resized.size(), CV_8U);

        // this needs to be cleared out, as we only copy to it with a mask later
        memset(_depth_output.data, 0, 2 * sizeXtimesSizeY);

        std::vector<cv::Rect> rects;
        for (int i = 0; i < NUM_SUB_IMAGES; i++) 
        {
            cv::Rect rect(0, i * sizeYperSubImage, sizeX, sizeYperSubImage);
            rects.push_back(rect);
        }

        for (auto&& rect : rects)
        {
            sub_area si;
            si.decimated_depth   = _decimated_depth(rect);
            si.decimated_ir      = _decimated_ir(rect);
            si.edge_mask         = _mask_edge(rect);
            si.harris_mask       = _mask_harris(rect);
            si.combined_mask     = _mask_combined(rect);
            si.output_depth      = _depth_output(rect);
            si.float_ir          = _ir_float(rect);
            si.corners           = _corners(rect);
            si.scharr_x          = _scharr_x(rect);
            si.scharr_y          = _scharr_y(rect);
            si.abs_scharr_x      = _abs_scharr_x(rect);
            si.abs_scharr_y      = _abs_scharr_y(rect);
            sub_areas.push_back(si);
        }
    }
};

int main(int argc, char * argv[]) try
{
    rs2::colorizer color_map;

    rs2::pipeline pipe;

    // NOTE: This example is strongly coupled with D43x cameras
    // With minor modifications it can be executed with other D400 cameras,
    // and even the SR300.
    // However, part of the value of this example is the real-life case-study it is based on
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH, 848, 480);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1);

    using namespace cv;
    const auto window_name = "Display Image";
    namedWindow(window_name, WINDOW_AUTOSIZE);

    high_confidence_filter filter;

    // See camera-settings.json next to the source / binaries
    std::ifstream file("./camera-settings.json");
    if (file.good())
    {
        std::string str((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

        auto prof = cfg.resolve(pipe);
        if (auto advanced = prof.get_device().as<rs400::advanced_mode>())
        {
            advanced.load_json(str);
        }
    }
    else
    {
        std::cout << "Couldn't find camera-settings.json, skipping custom settings!" << std::endl;
    }

    pipe.start(cfg);

    while (waitKey(1) < 0 && getWindowProperty(window_name, WND_PROP_AUTOSIZE) >= 0)
    {
        rs2::frameset data = pipe.wait_for_frames(); // Wait for next set of frames from the camera

        data = data.apply_filter(filter);

        rs2::frame depth = data.get_depth_frame().apply_filter(color_map);

        // Query frame size (width and height)
        const int w = depth.as<rs2::video_frame>().get_width();
        const int h = depth.as<rs2::video_frame>().get_height();

        // Create OpenCV matrix of size (w,h) from the colorized depth data
        Mat image(Size(w, h), CV_8UC3, (void*)depth.get_data(), Mat::AUTO_STEP);

        // Update the window with new data
        imshow(window_name, image);
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}



