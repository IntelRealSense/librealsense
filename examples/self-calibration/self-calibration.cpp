// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <map>
#include <vector>

#include <librealsense2/rs.hpp>  // Include RealSense Cross Platform API

#include "example.hpp"              // Include short list of convenience functions for rendering

struct self_calibration_result
{
  rs2::calibration_table new_calibration_table;
  float health_score;
  bool success;
};

struct self_calibration_health_thresholds
{
  constexpr static float OPTIMAL = 0.25;
  constexpr static float USABLE = 0.75;
  constexpr static float UNUSABLE = 1.0;
  constexpr static std::size_t MAX_NUM_UNSUCCESSFUL_ITERATIONS = 10;
};

enum class self_calibration_action
{
  UNKNOWN = 0,
  WRITE = 1,
  REPEAT = 2,
  RESET_FACTORY = 3,
  EXIT = 4
};

rs2::calibration_table get_current_calibration_table(rs2::device& dev)
{
  // Create auto calibration handler.
  rs2::auto_calibrated_device calib_dev = dev.as<rs2::auto_calibrated_device>();

  // Get current calibration table.
  return calib_dev.get_calibration_table();
}

self_calibration_result self_calibration_step(const std::string& json_config, rs2::device& dev)
{
  self_calibration_result result;

  // Create auto calibration handler.
  rs2::auto_calibrated_device calib_dev = dev.as<rs2::auto_calibrated_device>();

  // Run actual self-calibration.
  try {
    result.new_calibration_table = calib_dev.run_on_chip_calibration(json_config, &result.health_score);
    result.success = true;
  } catch (const rs2::error& e) {
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl; 
    result.success = false;
  }

  return result;
}

bool validate_self_calibration(const self_calibration_result& result)
{
  if(!result.success) {
    std::cout << "Self Calibration procedure was unsuccessful. Please point your sensor to an area with visible texture." << std::endl;
    return false;
  }

  std::cout << "Self Calibration finished. Health = " << result.health_score << std::endl;
  const auto abs_score = std::abs(result.health_score);
  if (abs_score < self_calibration_health_thresholds::USABLE)
  {
    // Usable results.
    if (abs_score < self_calibration_health_thresholds::OPTIMAL)
    {
      std::cout << "Optimal calibration results. You can proceed to write them to the device ROM." << std::endl;
    } else if (abs_score < self_calibration_health_thresholds::USABLE)
    {
      std::cout << "Calibration results are usable but not ideal. It is recommended to repeat the procedure." << std::endl;
    }
    return true;
  } else {
    // Unusable results
    std::cout << "Calibration results are unusable, please repeat the procedure." << std::endl;
    return false;
  }
}

void restore_factory_calibration(rs2::device& dev)
{
  // Create auto calibration handler.
  rs2::auto_calibrated_device calib_dev = dev.as<rs2::auto_calibrated_device>();

  // Restore factory calibration.
  calib_dev.reset_to_factory_calibration();
}

self_calibration_result self_calibrate(rs2::device& dev)
{
  self_calibration_result calib_results;

  // Self calibration parameters;
  std::stringstream ss;
        ss << "{\n \"speed\":" << 3 
           << ",\n \"scan parameter\":" << 1 << "}";
  const std::string json = ss.str();

  // Loop and prompt the user.
  bool valid_calibration = false;
  std::size_t num_calib_attempts = 0;
  while(!valid_calibration && 
         num_calib_attempts < self_calibration_health_thresholds::MAX_NUM_UNSUCCESSFUL_ITERATIONS)
  {
    std::cout << "Self calibration, iteration " << num_calib_attempts << "..." << std::endl;
    calib_results = self_calibration_step(json, dev);
    valid_calibration = validate_self_calibration(calib_results);

    num_calib_attempts++;
  }

  return calib_results;
}

void apply_calibration_table(const rs2::calibration_table& table, rs2::device& dev)
{
  // Create auto calibration handler.
  rs2::auto_calibrated_device calib_dev = dev.as<rs2::auto_calibrated_device>();

  // Apply self calibration results.
  calib_dev.set_calibration_table(table);
}

void display_calibration_results(rs2::pipeline& pipe)
{
  std::cout << "Showing calibration results. Close display window to continue..." << std::endl;
  window app(640, 480, "CPP Self-Calibration Example");
  rs2::colorizer colorizer; // Utility class to convert depth data to RGB colorspace.
  std::map<int, rs2::frame> render_frames;
   while (app)
   {
    // Collect new frames from the connected device.
    std::vector<rs2::frame> new_frames;
    rs2::frameset fs;
    if (pipe.poll_for_frames(&fs))
    {
        for (const rs2::frame& f : fs)
            new_frames.emplace_back(f);
    }

    // Convert the newly-arrived frames to render-friendly format
    for (const auto& frame : new_frames)
    {
      // Apply the colorizer of the matching device and store the colorized frame
      render_frames[frame.get_profile().unique_id()] = colorizer.process(frame);
    }

    // Present all the collected frames with openGl mosaic
    app.show(render_frames);
  }
}

void write_calibration_permanently(rs2::device& dev)
{
  // Create auto calibration handler.
  rs2::auto_calibrated_device calib_dev = dev.as<rs2::auto_calibrated_device>();

  // Write results to device's ROM.
  calib_dev.write_calibration();
}

self_calibration_action user_action_prompt()
{
  self_calibration_action action = self_calibration_action::UNKNOWN;

  std::cout << "From the following options please introduce the number corresponding to your desired action:" << std::endl;
  std::cout << "  1 - Write it to the device." << std::endl;
  std::cout << "  2 - Repeat it." << std::endl;
  std::cout << "  3 - Reset calibration to factory values." << std::endl;
  std::cout << "  4 - Exit and discard results." << std::endl;
  
  char user_input;
  do
  {
      std::cout << "Action [1/2/3/4]: ";
      std::cin >> user_input;
      std::cout << std::endl;
  } while (!std::cin.fail() && 
            user_input != '1' && user_input != '2' && user_input != '3' && user_input != '4');

  switch(user_input) {
    case '1':
      action = self_calibration_action::WRITE;
      break;
    case '2':
      action = self_calibration_action::REPEAT;
      break;
    case '3':
      action = self_calibration_action::RESET_FACTORY;
      break;
    case '4':
      action = self_calibration_action::EXIT;
      break;
  }

  return action;
}

// Self Calibration example demonstrates the basics of connecting to a RealSense device
// and running self calibration through the C++ API.
int main(int argc, char* argv[]) try {
  // Create a Pipeline - this serves as a top-level API for streaming and processing frames
  rs2::pipeline pipe;

  // Configure device and start streaming.
  rs2::config cfg;
  cfg.enable_stream(RS2_STREAM_INFRARED, 256, 144, RS2_FORMAT_Y8, 90);
  cfg.enable_stream(RS2_STREAM_DEPTH, 256, 144, RS2_FORMAT_Z16, 90);
  rs2::device dev = pipe.start(cfg).get_device();

  // Get current calibration table.
  std::cout << "Fetching current calibration table from device..." << std::endl;
  const rs2::calibration_table prev_calibration_table = get_current_calibration_table(dev);

  // Self calibration prompt.
  self_calibration_action user_decision = self_calibration_action::UNKNOWN;
  while ((user_decision != self_calibration_action::WRITE) && (user_decision != self_calibration_action::EXIT))
  {
    // Self calibration.
    const self_calibration_result calib_results = self_calibrate(dev);

    // We temporarily apply the new calibration values.
    apply_calibration_table(calib_results.new_calibration_table, dev);

    // Show calibration to the user.
    display_calibration_results(pipe);

    // Ask the user for input.
    user_decision = user_action_prompt();

    if (user_decision == self_calibration_action::REPEAT)
    {
      apply_calibration_table(prev_calibration_table, dev);
    }
  }

  if(user_decision == self_calibration_action::RESET_FACTORY) {
    restore_factory_calibration(dev);
    std::cout << "Factory calibration restored." << std::endl;
  }

  if(user_decision == self_calibration_action::WRITE)
  {
    write_calibration_permanently(dev);
    std::cout << "Self calibration written to device." << std::endl;
  }

  return EXIT_SUCCESS;
} catch (const rs2::error& e) {
  std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
  return EXIT_FAILURE;
} catch (const std::exception& e) {
  std::cerr << e.what() << std::endl;
  return EXIT_FAILURE;
}
