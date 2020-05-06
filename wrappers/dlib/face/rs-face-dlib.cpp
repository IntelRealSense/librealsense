// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>   // Include RealSense Cross Platform API
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing/render_face_detections.h>
#include <dlib/image_processing.h>
#include <dlib/gui_widgets.h>     // image_window, etc.
#include "../rs_frame_image.h"
#include "validate_face.h"
#include "render_face.h"


/*
    The data pointed to by 'frame.get_data()' is a uint16_t 'depth unit' that needs to be scaled.

    We return the scaling factor to convert these pixels to meters. This can differ depending
    on different cameras.
*/
float get_depth_scale( rs2::device dev )
{
    // Go over the device's sensors
    for( rs2::sensor& sensor : dev.query_sensors() )
    {
        // Check if the sensor if a depth sensor
        if( rs2::depth_sensor dpt = sensor.as<rs2::depth_sensor>() )
        {
            return dpt.get_depth_scale();
        }
    }
    throw std::runtime_error( "Device does not have a depth sensor" );
}


int main(int argc, char * argv[]) try
{
    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    rs2::pipeline_profile profile = pipe.start();
    // Each depth camera might have different units for depth pixels, so we get it here
    // Using the pipeline's profile, we can retrieve the device that the pipeline uses
    float const DEVICE_DEPTH_SCALE = get_depth_scale( profile.get_device() );


    /*
        The face detector we use is made using the classic Histogram of Oriented
        Gradients (HOG) feature combined with a linear classifier, an image pyramid,
        and sliding window detection scheme, using dlib's implementation of:
           One Millisecond Face Alignment with an Ensemble of Regression Trees by
           Vahid Kazemi and Josephine Sullivan, CVPR 2014
        and was trained on the iBUG 300-W face landmark dataset (see
        https://ibug.doc.ic.ac.uk/resources/facial-point-annotations/):
           C. Sagonas, E. Antonakos, G, Tzimiropoulos, S. Zafeiriou, M. Pantic.
           300 faces In-the-wild challenge: Database and results.
           Image and Vision Computing (IMAVIS), Special Issue on Facial Landmark Localisation "In-The-Wild". 2016.
        
        You can get the trained model file from:
        http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2
        
        Note that the license for the iBUG 300-W dataset excludes commercial use.
        So you should contact Imperial College London to find out if it's OK for
        you to use this model file in a commercial product.
    */
    dlib::frontal_face_detector face_bbox_detector = dlib::get_frontal_face_detector();
    dlib::shape_predictor face_landmark_annotator;
    dlib::deserialize( "shape_predictor_68_face_landmarks.dat" ) >> face_landmark_annotator;
    /*
        The 5-point landmarks model file can be used, instead. It's 10 times smaller and runs
        faster, from:
        http://dlib.net/files/shape_predictor_5_face_landmarks.dat.bz2
        But the validate_face() and render_face() functions will then need to be changed
        to handle 5 points.
    */

    /*
        We need to map pixels on the color frame to a depth frame, so we can
        determine their depth. The problem is that the frames may (and probably
        will!) be of different resolutions, and so alignment is necessary.

        See the rs-align example for a better understanding.

        We align the depth frame so it fits in resolution with the color frame
        since we go from color to depth.
    */
    rs2::align align_to_color( RS2_STREAM_COLOR );

    // Display annotations in one of two colors: if the face is determined to be that of a real
    // person, it is "good". Otherwise, "bad".
    dlib::rgb_pixel bad_color( 255, 0, 0 );
    dlib::rgb_pixel good_color( 0, 255, 0 );

    dlib::image_window win;
    while( ! win.is_closed() )
    {
        rs2::frameset data = pipe.wait_for_frames(); // Wait for next set of frames from the camera
        data = align_to_color.process( data );       // Replace with aligned frames
        auto depth_frame = data.get_depth_frame();
        auto color_frame = data.get_color_frame();

        // Create a dlib image for face detection
        rs_frame_image< dlib::rgb_pixel, RS2_FORMAT_RGB8 > image( color_frame );

        // Detect faces: find bounding boxes around all faces, then annotate each to find its landmarks
        std::vector< dlib::rectangle > face_bboxes = face_bbox_detector( image );
        std::vector< dlib::full_object_detection > faces;
        for( auto const & bbox : face_bboxes )
            faces.push_back( face_landmark_annotator( image, bbox ));

        // Display it all on the screen
        win.clear_overlay();
        win.set_image( image );
        for( auto const & face : faces )
        {
            dlib::rgb_pixel & color =
                validate_face( depth_frame, DEVICE_DEPTH_SCALE, face )
                    ? good_color : bad_color;
            win.add_overlay( render_face( face, color ));
        }
    }

    return EXIT_SUCCESS;
}
catch( dlib::serialization_error const & e )
{
    std::cerr << "You need dlib's default face landmarking model file to run this example." << std::endl;
    std::cerr << "You can get it from the following URL: " << std::endl;
    std::cerr << "   http://dlib.net/files/shape_predictor_68_face_landmarks.dat.bz2" << std::endl;
    std::cerr << std::endl << e.what() << std::endl;
    return EXIT_FAILURE;
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

