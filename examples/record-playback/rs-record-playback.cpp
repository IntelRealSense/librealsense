// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering
#include <chrono>

#include <imgui.h>
#include "imgui_impl_glfw.h"

// Includes for time display
#include <sstream>
#include <iostream>
#include <iomanip>


// Helper function for dispaying time conveniently
std::string pretty_time(std::chrono::nanoseconds duration);
// Helper function for rendering a seek bar
void draw_seek_bar(rs2::playback& playback, int* seek_pos, float2& location, float width);

int main(int argc, char * argv[]) try
{
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Record and Playback Example");
    ImGui_ImplGlfw_Init(app, false);

	// Declare a texture for the depth image on the GPU
    texture depth_image;

	// Declare frameset and frames which will hold the data from the camera
	rs2::frameset frames;
	rs2::frame depth;

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

	// Create a shared pointer to a pipeline
    auto pipe = std::make_shared<rs2::pipeline>();
	//rs2::pipeline pipe;
	// Start streaming with default configuration
    pipe->start();
	
	// Initialize a shared pointer to a device with the current device on the pipeline
	auto device = std::make_shared<rs2::device>(pipe->get_active_profile().get_device());

	// Create booleans to control the record-playback flow
    bool first_iteration_rec = true;
    bool first_iteration_play = true;
	bool recorded = false;

	// Create a variable to control the seek bar
	int seek_pos;

	// While application is running
    while(app) {
     // Flags for displaying ImGui window
        static const int flags = ImGuiWindowFlags_NoCollapse
            | ImGuiWindowFlags_NoScrollbar
            | ImGuiWindowFlags_NoSavedSettings
            | ImGuiWindowFlags_NoTitleBar
            | ImGuiWindowFlags_NoResize
            | ImGuiWindowFlags_NoMove;

        ImGui_ImplGlfw_NewFrame(1);
        ImGui::SetNextWindowSize({ app.width(), app.height() });
        ImGui::Begin("app", nullptr, flags);
		
		// If the device is sreaming live and not from a file
        if (!device->as<rs2::playback>())
        {
            frames = pipe->wait_for_frames(); // wait for next set of frames from the camera
            depth = color_map(frames.get_depth_frame()); // Find and colorize the depth data
        }		
	
		// Set options for the ImGui buttons
		ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, { 1, 1, 1, 1 });
		ImGui::PushStyleColor(ImGuiCol_Button, { 36 / 255.f, 44 / 255.f, 51 / 255.f, 1 });
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, { 40 / 255.f, 170 / 255.f, 90 / 255.f, 1 });
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, { 36 / 255.f, 44 / 255.f, 51 / 255.f, 1 });
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);
		

		if (!device->as<rs2::playback>()) // Disable recording while device is playing
		{ 
			ImGui::SetCursorPos({ app.width() / 2 - 100, app.height() / 2 + 50});
			ImGui::Text("Press 'record' to start recording");
			ImGui::SetCursorPos({ app.width() / 2 - 100, app.height() / 2 + 70 });
			if (ImGui::Button("record", { 50, 50 }))
			{
				// If it is the start of a new recording
				if (first_iteration_rec)
				{
					//pipe->stop(); // Stop the pipeline with the default configuration
					pipe->stop();
					pipe.reset();
					device.reset();
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					pipe = std::make_shared<rs2::pipeline>();
					rs2::config cfg; // Declare a new configuration
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					cfg.enable_record_to_file("b.bag");
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
					pipe->start(cfg); //File will be opened at this point
					//pipe.start(cfg);

					//device.reset(new rs2::device(pipe->get_active_profile().get_device()));

					// Reset the shared pointer and make it point to the current device
					//device.reset(new rs2::device(pipe->get_active_profile().get_device()));
					device = std::make_shared<rs2::device>(pipe->get_active_profile().get_device());
					//device = pipe.get_active_profile().get_device();
					first_iteration_rec = false;
				}
				else { // If the recording is resumed after a pause, there's no need to restart the shared pointers
					rs2::recorder recorder = device->as<rs2::recorder>(); // rs2::recorder allows access to 'resume' function
					recorder.resume();
				}
			}

			/*
			When pausing, device still holds the file.
			*/
			if (device->as<rs2::recorder>())
			{
				ImGui::SetCursorPos({ app.width() / 2, app.height() / 2 + 70 });
				if (ImGui::Button("pause\nrecord", { 50, 50 }))
				{
					//	if (device->as<rs2::recorder>())
					//	{
					device->as<rs2::recorder>().pause();
					//	}
				}


				ImGui::SetCursorPos({ app.width() / 2 + 100, app.height() / 2 + 70 });
				if (ImGui::Button(" stop\nrecord", { 50, 50 }))
				{
					//rs2::recorder recorder = device->as<rs2::recorder>();
					//pipe->stop(); // Stop the pipeline that holds the file and the recorder
					pipe->stop();
					pipe.reset();
					pipe = std::make_shared<rs2::pipeline>();
					//pipe = std::make_shared<rs2::pipeline>();
					//pipe.reset(new rs2::pipeline()); // Reset the shared pointer with a new pipeline
					device.reset(); // Reset the shared pointer to the device that holds the file
					pipe->start(); // Resume streaming with default configuration
					device = std::make_shared<rs2::device>(pipe->get_active_profile().get_device()); // Point to the current device
					//device = pipe.get_active_profile().get_device();
					first_iteration_rec = true;
					recorded = true; // Now we can run the file
				}
			}
		}

		// After a recording is done, we can play it
		//if (recorded) {
			ImGui::SetCursorPos({ app.width() / 2 - 100, 3 * app.height() / 4 - 20 });
			ImGui::Text("Press 'play' to start playing");
			ImGui::SetCursorPos({ app.width() / 2 - 100, 3 * app.height() / 4 });
			if (ImGui::Button("play", { 50, 50 }))
			{
				if (first_iteration_play)
				{
					pipe->stop(); // Stop streaming with default configuration
					//pipe.stop(); // Stop streaming with default configuration
					rs2::config cfg;
					device.reset();
					pipe.reset();
					pipe = std::make_shared<rs2::pipeline>();
					cfg.enable_device_from_file("b.bag");
					pipe->start(cfg); //File will be opened in read mode at this point
					//pipe.start(cfg);
					//device = std::make_shared<rs2::device>(pipe->get_active_profile().get_device());
					device = std::make_shared<rs2::device>(pipe->get_active_profile().get_device());
					//device = pipe.get_active_profile().get_device();
					first_iteration_play = false;
				}
				else
				{
					//rs2::playback playback = device->as<rs2::playback>();
					device->as<rs2::playback>().resume();
				}
			}
		//}
		
		// If device is playing a recording, we allow pause and stop
        if (device->as<rs2::playback>())
        {
            //rs2::playback playback = device->as<rs2::playback>();

		//	if (device.as<rs2::playback>().current_status() == RS2_PLAYBACK_STATUS_PLAYING)
		//	{
			if (pipe->poll_for_frames(&frames)) // Check if new frames are ready
			{
				depth = color_map(frames.get_depth_frame()); // Find and colorize the depth data for rendering
			}

			// Render a seek bar for the player
			float2 location = { app.width() / 4, 3 * app.height() / 4 + 100 };
			draw_seek_bar(device->as<rs2::playback>(), &seek_pos, location, app.width() / 2);


			ImGui::SetCursorPos({ app.width() / 2, 3 * app.height() / 4 });
			if (ImGui::Button(" pause\nplaying", { 50, 50 }))
			{
				//playback = device->as<rs2::playback>();
				device->as<rs2::playback>().pause();
			}

			ImGui::SetCursorPos({ app.width() / 2 + 100, 3 * app.height() / 4 });
			if (ImGui::Button("  stop\nplaying", { 50, 50 }))
			{
				device->as<rs2::playback>().stop();
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				pipe->stop();
				pipe.reset();
				device.reset();
				pipe = std::make_shared<rs2::pipeline>();
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				pipe->start();
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				device = std::make_shared<rs2::device>(pipe->get_active_profile().get_device()); // Point to the current device
				first_iteration_play = true;
			}
			//}
        }

		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar();

        ImGui::End();
        ImGui::Render();


		// Render depth frames from the default configuration, the recorder or the playback
        depth_image.render(depth, { app.width() / 4, 0, app.width() / 2, app.height() / 2 });
    }
    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cout << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}


std::string pretty_time(std::chrono::nanoseconds duration)
{
	using namespace std::chrono;
	auto hhh = duration_cast<hours>(duration);
	duration -= hhh;
	auto mm = duration_cast<minutes>(duration);
	duration -= mm;
	auto ss = duration_cast<seconds>(duration);
	duration -= ss;
	auto ms = duration_cast<milliseconds>(duration);

	std::ostringstream stream;
	stream << std::setfill('0') << std::setw(hhh.count() >= 10 ? 2 : 1) << hhh.count() << ':' <<
		std::setfill('0') << std::setw(2) << mm.count() << ':' <<
		std::setfill('0') << std::setw(2) << ss.count();
	return stream.str();
}


void draw_seek_bar(rs2::playback& playback, int* seek_pos, float2& location, float width)
{
  //  auto pos = ImGui::GetCursorPos();

   // auto p = dev.as<playback>();
  //  rs2_playback_status current_playback_status = p.current_status();
    int64_t playback_total_duration = playback.get_duration().count();
    auto progress = playback.get_position();
    double part = (1.0 * progress) / playback_total_duration;
    *seek_pos = static_cast<int>(std::max(0.0, std::min(part, 1.0)) * 100);
    auto playback_status = playback.current_status();
    ImGui::PushItemWidth(width);
	ImGui::SetCursorPos({ location.x, location.y });
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12);
	//ImGui::PushStyleColor(ImGuiCol_SliderGrab, { 40 / 255.f, 170 / 255.f, 90 / 255.f, 1 });
	//ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, { 40 / 255.f, 170 / 255.f, 90 / 255.f, 1 });
	//ImGui::GetStyle().GrabRounding = 12;
	if (ImGui::SliderInt("##seek bar", seek_pos, 0, 100, "", true))
    {
        //Seek was dragged
        if (playback_status != RS2_PLAYBACK_STATUS_STOPPED) //Ignore seek when playback is stopped
        {
            auto duration_db = std::chrono::duration_cast<std::chrono::duration<double, std::nano>>(playback.get_duration());
            auto single_percent = duration_db.count() / 100;
            auto seek_time = std::chrono::duration<double, std::nano>((*seek_pos) * single_percent);
            playback.seek(std::chrono::duration_cast<std::chrono::nanoseconds>(seek_time));
        }
    }
	std::string time_elapsed = pretty_time(std::chrono::nanoseconds(progress));
	ImGui::SetCursorPos({ location.x + width + 10, location.y });
	ImGui::Text("%s", time_elapsed.c_str());

	ImGui::PopStyleVar();
	//ImGui::PopStyleColor(2);
	ImGui::PopItemWidth();

}
