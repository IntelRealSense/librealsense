
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>   // Include OpenCV API

using namespace cv;

bool is_yaw{false};
bool is_roll{false};

cv::RotateFlags angle(cv::RotateFlags::ROTATE_90_CLOCKWISE);

void changeMode() {
    static int i = 0;

    i++;
    if (i==5) i=0;

    switch (i) {
    case 0:
        is_yaw = false;
        is_roll = false;
        break;
    case 1:
        is_yaw = true;
        is_roll = false;
        break;
    case 2:
        is_yaw = false;
        is_roll = true;

        angle = cv::RotateFlags::ROTATE_90_CLOCKWISE;

        break;
    case 3:
        is_yaw = false;
        is_roll = true;

        angle = cv::RotateFlags::ROTATE_180;

        break;
    case 4:
        is_yaw = false;
        is_roll = true;

        angle = cv::RotateFlags::ROTATE_90_COUNTERCLOCKWISE;

        break;
    default:
        break;
    }
}

int main(int argc, char * argv[]) try
{
    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

    // Aligning frames to color size
    rs2::align depthToColorAlignment(RS2_STREAM_COLOR);

    // Declare threshold filter for work with dots in range
    rs2::threshold_filter threshold;
    float threshold_min = 0.3f;
    float threshold_max = 1.5f;

    // Keep dots on the depth frame in range
    threshold.set_option(RS2_OPTION_MIN_DISTANCE, threshold_min);
    threshold.set_option(RS2_OPTION_MAX_DISTANCE, threshold_max);

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start();

    rs2::processing_block procBlock( [&](rs2::frame f, rs2::frame_source& src )
    {
        const int origWidth = f.as<rs2::video_frame>().get_width();
        const int origHeight = f.as<rs2::video_frame>().get_height();

        cv::Mat image(cv::Size(origWidth, origHeight), CV_16UC1, (void*)f.get_data(), cv::Mat::AUTO_STEP);
        cv::Mat rotated;

        if ( !is_yaw && !is_roll )
            rotated = image;

        if ( is_yaw ) {
            int rotWidth(static_cast<int>(threshold_max * 1000));

            rotated = cv::Mat::zeros(cv::Size(rotWidth, image.size().height), CV_16UC1 );

            for(int h = 0; h < rotated.size().height; h++) {
                for(int w = 0; w < rotated.size().width; w++) {

                    if ( ( h >= image.size().height ) || ( w >= image.size().width ) )
                        continue;

                    unsigned short value = image.at<unsigned short>(h, w);

                    if ( value == 0 )
                        continue;

                    rotated.at<unsigned short>( h, value ) = w;
                }
            }
        }

        if (is_roll) {
            cv::rotate( image, rotated, angle );
        }

        auto res = src.allocate_video_frame(f.get_profile(), f, 0, rotated.size().width, rotated.size().height);
        memcpy((void*)res.get_data(), rotated.data, rotated.size().width * rotated.size().height * 2);

        src.frame_ready(res);
    });

    rs2::frame_queue frame_queue;
    procBlock.start(frame_queue);

    while ( true )
    {
        // get set of frames
        rs2::frameset frames = pipe.wait_for_frames(); // Wait for next set of frames from the camera

        // align images
        frames = depthToColorAlignment.process(frames);

        // get depth frames
        rs2::frame depthFrame = frames.get_depth_frame();

        // keep points in range from threshold_min to threshold_max
        depthFrame = threshold.process(depthFrame);

        // call processing block for handle cloud point
        procBlock.invoke( depthFrame );
        depthFrame = frame_queue.wait_for_frame();

        // Query frame size (width and height)
        const int w = depthFrame.as<rs2::video_frame>().get_width();
        const int h = depthFrame.as<rs2::video_frame>().get_height();

        // Create OpenCV matrix of size (w,h) from the colorized depth data
        cv::Mat image(cv::Size(w, h), CV_8UC3, (void*)depthFrame.apply_filter(color_map).get_data());

        // Rescale image for convenience
        if ( ( image.size().width > 1000 ) || (image.size().height > 1000) )
            resize( image, image, Size(), 0.5, 0.5);

        // Update the window with new data
        imshow("window_name", image);
        int k = waitKey(1);

        if ( k == 'q' )
            break;

        if ( k == 'c' )
            changeMode();
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



