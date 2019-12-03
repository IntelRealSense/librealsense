// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>   // Include RealSense Cross Platform API

#include "cv-helpers.hpp"         // frame_to_mat
#include <opencv2/core/utils/filesystem.hpp>   // glob
namespace fs = cv::utils::fs;

#include <rs-vino/face-detection.h>
#include <rs-vino/detected-face.h>

#include <easylogging++.h>
INITIALIZE_EASYLOGGINGPP

#include <rs-vino/openvino-helpers.h>
namespace openvino = InferenceEngine;

#include <chrono>
using namespace std::chrono;


/*
    Enable loading multiple detectors at once, so we can switch at runtime.
    Each detector has its associated labels.
*/
struct detector_and_labels
{
    std::shared_ptr< openvino_helpers::face_detection > detector;
    std::vector< std::string > labels;

    detector_and_labels( std::string const & path_to_xml )
        : detector( std::make_shared< openvino_helpers::face_detection >( path_to_xml, 0.5 ) )
    {
    }

    openvino_helpers::face_detection * operator->() { return detector.get(); }

    void load_labels()
    {
        try
        {
            labels = openvino_helpers::read_labels( openvino_helpers::remove_ext( detector->pathToModel ) + ".labels" );
        }
        catch( const std::exception & e )
        {
            // If we have no labels, warn and continue... we can continue without them
            LOG(WARNING) << "Failed to load labels: " << e.what();
        }
    }
};


/*
    Populate a collection of detectors from those we find on disk (*.xml), load
    their labels, add them to the engine & device, etc.

    The detectors are loaded with all default values.
*/
void load_detectors_into(
    std::vector< detector_and_labels > & detectors,
    openvino::Core & engine,
    std::string const & device_name
)
{
    std::vector< std::string > xmls;
    fs::glob_relative( ".", "*.xml", xmls );
    for( auto path_to_xml : xmls )
    {
        detector_and_labels detector { path_to_xml };
        try
        {
            detector->load_into( engine, device_name );  // May throw!
            detector.load_labels();
            detectors.push_back( detector );
            LOG(INFO) << "   ... press '" << char( '0' + detectors.size() ) << "' to switch to it";
        }
        catch( const std::exception & e )
        {
            // The model files should have been downloaded automatically by CMake into build/wrappers/openvino/dnn,
            // which is also where Visual Studio runs the sample from. However, you may need to copy these files:
            //     *.bin
            //     *.xml
            //     *.labels  [optional]
            // Into the local directory where you run from (or change the path given in the ctor above)
            LOG(ERROR) << "Failed to load model: " << e.what();
        }
    }
}


int main(int argc, char * argv[]) try
{
    el::Configurations conf;
    conf.set( el::Level::Global, el::ConfigurationType::Format, "[%level] %msg" );
    //conf.set( el::Level::Debug, el::ConfigurationType::Enabled, "false" );
    el::Loggers::reconfigureLogger( "default", conf );
    rs2::log_to_console( RS2_LOG_SEVERITY_WARN );    // only warnings (and above) should come through

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    pipe.start();
    rs2::align align_to( RS2_STREAM_COLOR );

    // Start the inference engine, needed to accomplish anything. We also add a CPU extension, allowing
    // us to run the inference on the CPU. A GPU solution may be possible but, at least without a GPU,
    // a CPU-bound process is faster. To change to GPU, use "GPU" instead (and disable the exception):
    openvino::Core engine;
    openvino_helpers::error_listener error_listener;
    engine.SetLogCallback( error_listener );
    std::string const device_name { "CPU" };
    engine.AddExtension( std::make_shared< openvino::Extensions::Cpu::CpuExtensions >(), device_name );

    std::vector< detector_and_labels > detectors;
    load_detectors_into( detectors, engine, device_name );
    if( detectors.empty() )
    {
        LOG(ERROR) << "No detectors available in: " << fs::getcwd();
        return EXIT_FAILURE;
    }
    // Look for the mobilenet-ssd so it always starts the same... otherwise default to the first detector we found
    size_t current_detector = 0;
    for( size_t i = 1; i < detectors.size(); ++i )
    {
        if( detectors[i]->pathToModel == "mobilenet-ssd.xml" )
        {
            current_detector = i;
            break;
        }
    }
    auto p_detector = detectors[current_detector].detector;
    LOG(INFO) << "Current detector set to (" << current_detector+1 << ") \"" << openvino_helpers::remove_ext( p_detector->pathToModel ) << "\"";
    auto p_labels = &detectors[current_detector].labels;

    const auto window_name = "OpenVINO DNN sample";
    cv::namedWindow( window_name, cv::WINDOW_AUTOSIZE );

    cv::Mat prev_image;
    openvino_helpers::detected_faces faces;
    size_t id = 0;
    uint64 last_frame_number = 0;
    high_resolution_clock::time_point switch_time = high_resolution_clock::now();

    while( cv::getWindowProperty( window_name, cv::WND_PROP_AUTOSIZE ) >= 0 )
    {
        // Wait for the next set of frames
        auto frames = pipe.wait_for_frames();
        // Make sure the frames are spatially aligned
        frames = align_to.process( frames );

        auto color_frame = frames.get_color_frame();
        auto depth_frame = frames.get_depth_frame();

        // If we only received a new depth frame, but the color did not update, continue
        if( color_frame.get_frame_number() == last_frame_number )
            continue;
        last_frame_number = color_frame.get_frame_number();

        auto image = frame_to_mat( color_frame );

        // We process the previous frame so if this is our first then queue it and continue
        if( ! p_detector->_request )
        {
            p_detector->enqueue( image );
            p_detector->submit_request();
            prev_image = image;
            continue;
        }

        // Wait for the results of the previous frame we enqueued: we're going to process these
        p_detector->wait();
        auto const results = p_detector->fetch_results();

        // Enqueue the current frame so we'd get the results when the next frame comes along!
        p_detector->enqueue( image );
        p_detector->submit_request();

        openvino_helpers::detected_faces prev_faces { std::move( faces ) };
        faces.clear();
        for( size_t i = 0; i < results.size(); ++i )
        {
            auto const & result = results[i];
            if( result.label <= 0 )
                continue;  // ignore "background", though not clear why we'd get it
            cv::Rect rect = result.location;
            rect = rect & cv::Rect( 0, 0, image.cols, image.rows );
            auto face_ptr = openvino_helpers::find_face( rect, prev_faces );
            if( !face_ptr )
            {
                // New face
                std::string label;
                if( result.label < p_labels->size() )
                    label = (*p_labels)[result.label];
                face_ptr = std::make_shared< openvino_helpers::detected_face >( id++, label, rect );
            }
            else
            {
                // Existing face; just update its parameters
                face_ptr->move( rect );
            }
            faces.push_back( face_ptr );
        }

        // Keep this image so we can actually process pieces of it once we have the results
        prev_image = image;

        // Display the results (from the last frame) as rectangles on top (of the current frame)
        for( auto && face : faces )
        {
            cv::Scalar green( 0, 255, 0 );  // BGR
            auto r = face->get_location();
            cv::rectangle( image, r, green );

            // Output the distance to the center
            auto center_x = (r.x + r.width / 2) * depth_frame.get_width() / color_frame.get_width();
            auto center_y = (r.y + r.height / 2) * depth_frame.get_height() / color_frame.get_height();
            auto d = depth_frame.get_distance( center_x, center_y );
            if( d )
            {
                std::ostringstream ss;
                ss << face->get_label() << " ";
                ss << std::setprecision( 2 ) << d;
                ss << " meters away";
                cv::Scalar white( 255, 255, 255 );  // BGR
                cv::putText( image, ss.str(), cv::Point( r.x + 5, r.y + r.height - 5 ), cv::FONT_HERSHEY_SIMPLEX, 0.4, white );
            }
        }

        // Show the current detector number as an overlay over the image for 1 second
        double alpha = std::max( 0LL, 1000 - duration_cast<milliseconds>(high_resolution_clock::now() - switch_time).count() ) / 1000.;
        std::string str( 1, char( '1' + current_detector ));
        auto size = cv::getTextSize( str, cv::FONT_HERSHEY_SIMPLEX, 3, 1, nullptr );
        cv::Point center { image.cols / 2, image.rows / 2 };
        cv::Rect r { center.x - size.width, center.y - size.height, size.width * 2, size.height * 2 };
        cv::Mat roi = image( r );
        cv::Mat overlay( roi.size(), CV_8UC3, cv::Scalar( 32, 32, 32 ) );
        cv::putText( overlay, str, cv::Point { r.width / 2 - size.width / 2, r.height / 2 + size.height / 2 }, cv::FONT_HERSHEY_SIMPLEX, 3, cv::Scalar { 255, 255, 255 } );
        cv::addWeighted( overlay, alpha, roi, 1 - alpha, 0, roi );   // roi = overlay * alpha + roi * (1-alpha) + 0

        imshow( window_name, image );
        const int key = cv::waitKey( 1 );
        if( key == 27 )
            break;  // escape
        if( key >= '1'  &&  key < '1' + detectors.size() )
        {
            size_t detector_index = key - '1';
            if( detector_index != current_detector )
            {
                current_detector = detector_index;
                p_detector = detectors[current_detector].detector;
                p_labels = &detectors[current_detector].labels;
                faces.clear();
                LOG(INFO) << "Current detector set to (" << current_detector+1 << ") \"" << openvino_helpers::remove_ext( p_detector->pathToModel ) << "\"";
            }
            switch_time = high_resolution_clock::now();
        }
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    LOG(ERROR) << "Caught RealSense exception from " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what();
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    LOG(ERROR) << "Unknown exception caught: " << e.what();
    return EXIT_FAILURE;
}

