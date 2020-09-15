// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include "../example.hpp"
#include "types.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
/*
 This example introduces the concept of spatial stream reprojection.
 The need for spatial reprojection (from here "reproject") arises from the fact
 that sometimes there is a need to transform the depth streams to different viewport.
 Reproject process lets the user translate and rotate images to another viewport using spatial information.
 That said, results of reproject are synthetic stream, and suffer from several artifacts:
 1. Sampling - mapping stream to a different viewport will modify the resolution of the frame
               to match the resolution of target viewport. This will either cause downsampling or
               upsampling via interpolation. The interpolation used needs to be of type
               Nearest Neighbor to avoid introducing non-existing values.
 2. Occlussion - Some pixels in the resulting image correspond to 3D coordinates that the original
               sensor did not see, because these 3D points were occluded in the original viewport.
               Such pixels may hold invalid texture values.
*/

// This example assumes camera with depth stream

//Helper function to convert euler angles to rotation matrix
void set_extrinsics_rotation(rs2_extrinsics &extr, float* euler)
{
    float* A=extr.rotation;
    //Precompute sines and cosines of Euler angles
    double su = sin(euler[0]);
    double cu = cos(euler[0]);
    double sv = sin(euler[1]);
    double cv = cos(euler[1]);
    double sw = sin(euler[2]);
    double cw = cos(euler[2]);

    //Create and populate RotationMatrix        
    A[0] = cv * cw;
    A[1] = su * sv * cw - cu * sw;
    A[2] = su * sw + cu * sv * cw;
    A[3] = cv * sw;
    A[4] = cu * cw + su * sv * sw;
    A[5] = cu * sv * sw - su * cw;
    A[6] = -sv;
    A[7] = su * cv;
    A[8] = cu * cv;
}

int main(int argc, char * argv[]) try
{
    // Create and initialize GUI related objects
    window app(1280, 720, "RealSense Reproject Example"); // Simple window handling
    ImGui_ImplGlfw_Init(app, false);      // ImGui library intializition
    rs2::colorizer c;                     // Helper to colorize depth images
    texture depth_image, color_image;     // Helpers for renderig images

    // Create a pipeline to easily configure and start the camera
    rs2::pipeline pipe;
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH);
    auto selection =pipe.start(cfg);
    auto depth_stream = selection.get_stream(RS2_STREAM_DEPTH)
        .as<rs2::video_stream_profile>();
    //set default intrinsics and extrinsics data
    rs2_intrinsics intr = depth_stream.get_intrinsics();
    rs2_extrinsics extr = { {1,0,0,0,1,0,0,0,1},{0,0,0} };
    
    // Define reproject object. 
    // to reproject depth viewport to another perspective.
    // Creating reproject object is an expensive operation    
    rs2::reproject reproject_depth(intr, extr);
    
    //this variable will hold the euler angles
    float R[3] = { 0 };
    

    while (app) // Application still alive?
    {
        // Using the reproject object, we block the application until a frameset is available
        rs2::frameset frameset = pipe.wait_for_frames();

        
        // reproject depth frame to another viewport
        auto depth = reproject_depth.process(frameset.first_or_default(RS2_STREAM_DEPTH));
        // colorize the result       
        auto colorized_depth = c.colorize(depth);

        glEnable(GL_BLEND);
        //render the result
        depth_image.render(colorized_depth, { 0, 0, app.width(), app.height() });

        glColor4f(1.f, 1.f, 1.f, 1.f);
        glDisable(GL_BLEND);

        // Render the UI:
        ImGui_ImplGlfw_NewFrame(1);
        auto changed=ImGui::SliderFloat3("t (m)", extr.translation, -3.0, 3.0f);
        changed |= ImGui::SliderFloat3("Rot (rad)", R, -PI/2, PI/2);
        
        //if changes were made to the perspective recreate the reproject filter object
        if (changed) {
            set_extrinsics_rotation(extr,R);            
            reproject_depth = rs2::reproject(intr, extr);
        }        
        if (ImGui::Button("Reset"))
        {
            extr = { {1,0,0,0,1,0,0,0,1},{0,0,0} };
            reproject_depth = rs2::reproject(intr, extr);  
        }

        ImGui::Render();
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
