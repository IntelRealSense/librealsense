#include "active_obj.hpp"
#include "conversions.hpp"
#include <iostream>

// Called when Legacy device produces a frame
void legacy_active_obj::on_frame(rs::frame f) {
    auto sensor = sensors.at(stream_conv(f.get_stream_type()));
    // Make sure the stream is still open in modern API
    auto active_profiles = sensor.get_active_streams();
    if (active_profiles.size() == 0) return;

    // Find the correct stream profile
    for (int i = 0; i < active_profiles.size(); ++i) {
        auto vp = active_profiles[i].as<rs2::video_stream_profile>();

        // Make sure this is the correct profile
        if (fmt_conv(f.get_format()) != vp.format() || f.get_framerate() != vp.fps()
            || f.get_width() != vp.width() || f.get_height() != vp.height()) continue;

        std::cout << "[legacy_active_obj::on_frame] " << vp.stream_name() << " " << f.get_frame_number() << " " << f.get_timestamp() << std::endl;

        // Copy data from legacy buffer to heap so we can use it with modern API.
        int frame_size = f.get_stride() * f.get_height();
        auto buf = new uint8_t[frame_size];

        sensor.on_video_frame({ std::memcpy(buf, f.get_data(), frame_size), [](void* ptr) { delete[] reinterpret_cast<uint8_t*>(ptr); },
            f.get_stride(), f.get_bpp()/8, f.get_timestamp(), // Legacy API returns bits/pixel, Modern API wants bytes/pixel
            RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, int(f.get_frame_number()), vp.get() });
    }
}

// Called when the modern API destroys the camera device
legacy_active_obj::~legacy_active_obj() {
    alive = false;
    if (legacy_dev->is_streaming())
        legacy_dev->stop();
    
    if (heart.joinable())
        heart.join();
}

// Polls changes to active streams and options
void legacy_active_obj::heartbeat() {
    while (alive) {
        try {
            // Poll changes in active streams
            std::map<int, rs2::video_stream_profile> profiles;
            for (auto &kvp : sensors) {
                for (auto profile : kvp.second.get_active_streams())
                    profiles.emplace(profile.unique_id(), profile.as<rs2::video_stream_profile>());
            }
            if (profiles != active_profiles) {
                // Need to update streams.
                if (legacy_dev->is_streaming()) legacy_dev->stop();

                // Disable all old streams
                for (int i = 0; i<int(rs::stream::points); ++i) {
                    auto stream = rs::stream(i);
                    if (legacy_dev->is_stream_enabled(stream))
                        legacy_dev->disable_stream(stream);
                }

                // Enable all active streams
                for (auto& kvp : profiles) {
                    auto vp = kvp.second;
                    legacy_dev->enable_stream(stream_iconv(vp.stream_type()), vp.width(),
                        vp.height(), fmt_iconv(vp.format()), vp.fps());
                    legacy_dev->set_frame_callback(stream_iconv(vp.stream_type()), [this](rs::frame f) { 
                        on_frame(std::move(f)); 
                    });
                }
                active_profiles.swap(profiles);
                legacy_dev->start();
            }

            // Poll changes to options
            for (auto & kvp : cur_options) {
                rs2_stream stream;
                rs2_option opt;
                bool is_writable;
                std::tie(stream, opt, is_writable) = option_conv(kvp.first);
                auto sensor = sensors.at(stream);
                if (is_writable) {
                    auto new_val = sensor.get_option(opt);
                    if (new_val != kvp.second) {
                        kvp.second = new_val;
                        if (opt == RS2_OPTION_DEPTH_UNITS) new_val *= 1e6; // Legacy API uses micrometers, Modern API uses meters.
                        legacy_dev->set_option(kvp.first, new_val);

                        // Setting these options causes the auto control to change.
                        if (kvp.first == rs::option::color_exposure) {
                            cur_options[rs::option::color_enable_auto_exposure] = 0;
                            sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0);
                        }
                        else if (kvp.first == rs::option::color_white_balance) {
                            cur_options[rs::option::color_enable_auto_white_balance] = 0;
                            sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0);
                        }
                        else if (kvp.first == rs::option::r200_lr_exposure) {
                            cur_options[rs::option::r200_lr_auto_exposure_enabled] = 0;
                            sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0);
                        }
                        else if (kvp.first == rs::option::fisheye_exposure) {
                            cur_options[rs::option::fisheye_color_auto_exposure] = 0;
                            sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0);
                        }
                    }
                }
                else sensor.set_read_only_option(opt, legacy_dev->get_option(kvp.first));
            }

            // Update exposure/gain options under auto controls
            if (bool(cur_options[rs::option::color_enable_auto_exposure]))
                sensors.at(RS2_STREAM_COLOR).set_option(RS2_OPTION_EXPOSURE, legacy_dev->get_option(rs::option::color_exposure));
            if (bool(cur_options[rs::option::color_enable_auto_white_balance]))
                sensors.at(RS2_STREAM_COLOR).set_option(RS2_OPTION_WHITE_BALANCE, legacy_dev->get_option(rs::option::color_white_balance));
            if (bool(cur_options[rs::option::r200_lr_auto_exposure_enabled]))
                sensors.at(RS2_STREAM_DEPTH).set_option(RS2_OPTION_EXPOSURE, legacy_dev->get_option(rs::option::r200_lr_exposure));
            if (bool(cur_options[rs::option::fisheye_color_auto_exposure]))
                sensors.at(RS2_STREAM_FISHEYE).set_option(RS2_OPTION_EXPOSURE, legacy_dev->get_option(rs::option::fisheye_exposure));
        }
        catch (...) {
            // TODO: Handle errors somehow?
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

legacy_active_obj::legacy_active_obj(int legacy_dev_idx, rs2::software_device dev)
    try : alive(true), legacy_ctx(), legacy_dev(legacy_ctx.get_device(legacy_dev_idx))
{
    map_infos(dev);
    map_profiles(dev);
    map_options();
} catch (...) {

}

void legacy_active_obj::finalize_binding(rs2::software_device dev) try {
    // Set up destruction of active object when wrapper device is destroyed
    auto shared_this = shared_from_this();
    dev.set_destruction_callback([shared_this]() mutable { shared_this.reset(); });

    heart = std::thread([this]() { heartbeat(); });
} catch (std::exception &e) {
    rs2::log(RS2_LOG_SEVERITY_ERROR, "Derp");
}

void legacy_active_obj::map_infos(rs2::software_device dev) {
    // Query legacy API for device infos and register them in wrapper
    for (auto info : { rs::camera_info::serial_number, rs::camera_info::camera_type, rs::camera_info::oem_id }) {
        if (!legacy_dev->supports(info)) continue;
        dev.register_info(info_conv(info), legacy_dev->get_info(info));
    }
    // Set firmware version as both current and recommended versions
    if (legacy_dev->supports(rs::camera_info::camera_firmware_version)) {
        dev.register_info(RS2_CAMERA_INFO_FIRMWARE_VERSION, legacy_dev->get_info(rs::camera_info::camera_firmware_version));
        dev.register_info(RS2_CAMERA_INFO_RECOMMENDED_FIRMWARE_VERSION, legacy_dev->get_info(rs::camera_info::camera_firmware_version));
    }
    // Device name is automatically initialized to "Software Device", so we update to overwrite that.
    if (legacy_dev->supports(rs::camera_info::device_name)) {
        dev.update_info(RS2_CAMERA_INFO_NAME, std::string("[Legacy Adaptor] ") + legacy_dev->get_info(rs::camera_info::device_name));
    }
    // Set a couple other infos just for completion's sake
    dev.register_info(RS2_CAMERA_INFO_ADVANCED_MODE, "NO");
    dev.register_info(RS2_CAMERA_INFO_CAMERA_LOCKED, "NO");
}

void legacy_active_obj::map_profiles(rs2::software_device dev) {
    bool has_depth = false;
    std::map<rs2_stream, rs2::stream_profile> profiles;
    // Register all the different streaming modes
    for (auto legacy_stream : { rs::stream::depth, rs::stream::color, rs::stream::fisheye }) {
        auto stream = stream_conv(legacy_stream);
        bool found_default = false;
        rs::format default_format;
        switch (legacy_stream) {
        case rs::stream::depth: default_format = rs::format::z16; break;
        case rs::stream::color: default_format = rs::format::rgb8; break;
        case rs::stream::fisheye: default_format = rs::format::raw8; break;
        }

        int n_modes = legacy_dev->get_stream_mode_count(legacy_stream);
        if (n_modes > 0) { // Device supports this stream?
            // Make sensor in software device for the stream
            auto sensor = dev.add_sensor(rs2_stream_to_string(stream) + std::string(" Sensor"));
            sensors.emplace(stream, sensor);

            // Register recommended processing blocks
            switch (stream) {
            case RS2_STREAM_DEPTH:
                sensor.add_recommended_processing(rs2::decimation_filter());
                sensor.add_recommended_processing(rs2::threshold_filter());
                sensor.add_recommended_processing(rs2::disparity_transform(true));
                sensor.add_recommended_processing(rs2::spatial_filter());
                sensor.add_recommended_processing(rs2::temporal_filter());
                sensor.add_recommended_processing(rs2::hole_filling_filter());
                sensor.add_recommended_processing(rs2::disparity_transform(false));
                break;
            case RS2_STREAM_COLOR:
                sensor.add_recommended_processing(rs2::decimation_filter());
                break;
            default: break;
            }

            // Register all stream profiles
            for (int i = 0; i < n_modes; ++i) {
                // uid uniquely identifies each stream on the device. That is, at most one stream_profile
                // with each uid can be active at the same time on each device.
                rs2_video_stream vs{ stream, 0, int(legacy_stream), 0, 0, 0, 0, RS2_FORMAT_ANY, {} };

                // Get stream data from legacy device
                rs::format fmt;
                legacy_dev->get_stream_mode(legacy_stream, i, vs.width, vs.height, fmt, vs.fps);
                vs.fmt = fmt_conv(fmt); // convert from legacy enum
                vs.bpp = fmt_to_bpp(fmt); // Legacy API only exposes this value from live frames, but it depends only on format.

                // VGA (640x480@30fps) of the optimal format should be default, but if the camera doesn't
                // support that, then just use the last stream mode defined by the camera.
                bool is_default = false;
                if (!found_default &&
                    ((vs.width == 640 && vs.height == 480 && fmt == default_format && vs.fps == 30)
                        || i == (n_modes - 1))) {
                    is_default = found_default = true;
                }

                // Enable legacy stream in order to get stream intrinsics
                legacy_dev->enable_stream(legacy_stream, vs.width, vs.height, fmt, vs.fps);
                rs::intrinsics legacy_intrin = legacy_dev->get_stream_intrinsics(legacy_stream);
                vs.intrinsics = { legacy_intrin.width, legacy_intrin.height, legacy_intrin.ppx, legacy_intrin.ppy, legacy_intrin.fx, legacy_intrin.fy, distortion_conv(legacy_intrin.model()) };
                for (int i = 0; i < 5; ++i) vs.intrinsics.coeffs[i] = legacy_intrin.coeffs[i];
                legacy_dev->disable_stream(legacy_stream);

                // Register stream on software device wrapper
                auto profile = sensor.add_video_stream(vs, is_default);
                if (i == 0) profiles[stream] = profile;
                else {
                    // Register identity extrinsics to first profile on sensor.
                    rs2_extrinsics extrin = { {1,0,0, 0,1,0, 0,0,1}, {0,0,0} };
                    profiles[stream].register_extrinsics_to(profile, extrin);
                }
            }

            // Register extrinsics from depth sensor to other sensors
            if (stream == RS2_STREAM_DEPTH) has_depth = true;
            else if (has_depth) {
                try {
                    rs::extrinsics legacy_extrin = legacy_dev->get_extrinsics(rs::stream::depth, legacy_stream);
                    rs2_extrinsics extrin;
                    for (int i = 0; i < 9; ++i) extrin.rotation[i] = legacy_extrin.rotation[i];
                    for (int i = 0; i < 3; ++i) extrin.translation[i] = legacy_extrin.translation[i];
                    profiles[RS2_STREAM_DEPTH].register_extrinsics_to(profiles[stream], extrin);
                } catch (...) {
                    std::string msg("Couldn't get extrinsics between depth and ");
                    msg += rs2_stream_to_string(stream);
                    rs2::log(RS2_LOG_SEVERITY_WARN, msg.c_str());
                }
            }
        }

        dev.create_matcher(RS2_MATCHER_DEFAULT);
    }
}

void legacy_active_obj::map_options() {
    // Map options between legacy and modern API.
    // The modern API uses a slightly more modular approach to options, hence mapping to (sensor, option) pairs.
    for (int i = 0; i < int(rs::option::total_frame_drops); ++i) {
        auto legacy_opt = rs::option(i);
        if (!legacy_dev->supports_option(legacy_opt)) continue; // Legacy device doesn't support the option

        rs2_stream stream;
        rs2_option opt;
        bool is_writable;
        std::tie(stream, opt, is_writable) = option_conv(legacy_opt);

        if (opt == RS2_OPTION_COUNT) continue; // modern API doesn't support the option
        if (sensors.find(stream) == sensors.end()) continue; // Failed to build the relevant sensor

        // Get range from legacy device
        double min, max, step;
        legacy_dev->get_option_range(legacy_opt, min, max, step);

        // Get current value (for default) and register option in relevant software sensor
        float def = legacy_dev->get_option(legacy_opt);
        if (opt == RS2_OPTION_DEPTH_UNITS) { // Legacy API uses micrometers, Modern API uses meters.
            min /= 1e6;
            max /= 1e6;
            def /= 1e6;
            step /= 1e6;
        }
        sensors.at(stream).add_option(opt, { float(min), float(max), def, float(step) }, is_writable);
        cur_options[legacy_opt] = def;

    }

}