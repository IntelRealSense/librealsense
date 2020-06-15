// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <map>
#include <vector>

#include <librealsense2/rs.hpp>  // Include RealSense Cross Platform API

#include "example.hpp"              // Include short list of convenience functions for rendering

struct self_calibration_result {
  rs2::calibration_table current_calibration_table;
  rs2::calibration_table new_calibration_table;
  float health_score;
  bool success;
};

struct self_calibration_health_threshold {
  constexpr static float OPTIMAL = 0.25;
  constexpr static float USABLE = 0.75;
  constexpr static float UNUSABLE = 1.0;
};

self_calibration_result self_calibrate(const std::string& json_config, rs2::device& dev)
{
  self_calibration_result result;

  // Create auto calibration handler.
  rs2::auto_calibrated_device calib_dev = dev.as<rs2::auto_calibrated_device>();

  // Get current calibration table.
  result.current_calibration_table = calib_dev.get_calibration_table();

  // Run actual self-calibration.
  try {
    result.new_calibration_table = calib_dev.run_on_chip_calibration(json_config, &result.health_score, [&](const float progress) { /* On Progress */ });
    result.success = true;
  } catch (const rs2::error& e) {
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl; 
    result.success = false;
  }

  return result;
}

bool validate_self_calibration(const self_calibration_result& result)
{
  std::cout << "Self Calibration finished. Health = " << result.health_score << std::endl;
  if (result.health_score < self_calibration_health_threshold::USABLE)
  {
    // Usable results.
    if (result.health_score < self_calibration_health_threshold::OPTIMAL)
    {
      std::cout << "Optimal calibration results. You can proceed to write them to the device ROM." << std::endl;
    } else if (result.health_score < self_calibration_health_threshold::USABLE)
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

void apply_calibration_table(const rs2::calibration_table& table, rs2::device& dev) {
  // Create auto calibration handler.
  rs2::auto_calibrated_device calib_dev = dev.as<rs2::auto_calibrated_device>();

  // Apply self calibration results.
  calib_dev.set_calibration_table(table);
}

void write_calibration_permanently(rs2::device& dev) {
  // Create auto calibration handler.
  rs2::auto_calibrated_device calib_dev = dev.as<rs2::auto_calibrated_device>();

  // Write results to device's ROM.
  calib_dev.write_calibration();
}

// Self Calibration example demonstrates the basics of connecting to a RealSense device
// and running self calibration through the C++ API.
int main(int argc, char* argv[]) try {
  window app(640, 480, "CPP Self-Calibration Example");
  rs2::colorizer colorizer; // Utility class to convert depth data to RGB colorspace.

  // Create a Pipeline - this serves as a top-level API for streaming and processing frames
  rs2::pipeline pipe;

  // Configure and start the pipeline
  rs2::config cfg;
  cfg.enable_stream(RS2_STREAM_INFRARED, 256, 144, RS2_FORMAT_Y8, 90);
  cfg.enable_stream(RS2_STREAM_DEPTH, 256, 144, RS2_FORMAT_Z16, 90);
  rs2::device dev = pipe.start(cfg).get_device();

  // Self calibration parameters;
  std::stringstream ss;
        ss << "{\n \"speed\":" << 0 <<
               ",\n \"average step count\":20" <<
               ",\n \"step count\":20" <<
               ",\n \"accuracy\":2}";

  const std::string json = ss.str();
  const auto calib_results = self_calibrate(json,dev);
  if (validate_self_calibration(calib_results)) {
    apply_calibration_table(calib_results.new_calibration_table, dev);
  }

  // Show calibration to the user.
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

  return EXIT_SUCCESS;
} catch (const rs2::error& e) {
  std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
  return EXIT_FAILURE;
} catch (const std::exception& e) {
  std::cerr << e.what() << std::endl;
  return EXIT_FAILURE;
}