// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>     // Include RealSense Cross Platform API
#include "example.hpp"              // Include short list of convenience functions for rendering

#include <string>
#include <map>
#include <algorithm>
#include <mutex>                    // std::mutex, std::lock_guard
#include <cmath>                    // std::ceil
#include <queue>

const std::string no_camera_message     = "No camera connected, please connect 1 or more";
const std::string connect_more_cameras  = "Please connect more cameras";
const std::string platform_camera_name  = "Platform Camera";

class device_events
{
public:
    void add_event(const rs2::event_information& evt)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        _dev_events.push(evt);
    }

    bool try_get_event(rs2::event_information& evt)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (_dev_events.empty())
            return false;

        evt = std::move(_dev_events.front());
        _dev_events.pop();
        return true;
    }
private:
    std::queue<rs2::event_information> _dev_events;
    std::mutex  _mtx;
};

class device_container
{
    class frames_container
    {
    public:
        frames_container() : _raw(1){};
        rs2::frame_queue    _raw;
        rs2::frame          _processed;
    };

    // Helper struct per pipeline
    struct view_port
    {
    public:
        view_port()
        {
        }
        ~view_port() {};
        view_port(const view_port&) = delete;               // non construction-copyable
        view_port(view_port&& vp):                          // moveable
            colorize_frame(std::move(vp.colorize_frame)),
            tex(std::move(vp.tex)),
            pipe(std::move(vp.pipe)),
            profile(std::move(vp.profile)){}
        std::map<int, frames_container> frames_per_stream;
        rs2::colorizer          colorize_frame;
        texture                 tex;
        rs2::pipeline           pipe;
        rs2::pipeline_profile   profile;
    };

public:

    device_container(rs2::context& ctx) : _ctx(ctx) {}

    void refresh_devices(void)
    {
        rs2::event_information info({}, {});
        // Process one event at a time
        if (_dev_events.try_get_event(info))
        {
            remove_devices(info);
            for (auto&& dev : info.get_new_devices())
            {
                std::cout << "Adding new device !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << dev.get_info(RS2_CAMERA_INFO_NAME) << " , " << dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) << std::endl;
                enable_device(dev);
            }
        }
    }

    size_t stream_count()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        size_t count = 0;
        for (auto&& sn_to_dev : _devices)
        {
            count += sn_to_dev.second.frames_per_stream.size();
        }
        return count;
    }

    size_t device_count()
    {
        std::lock_guard<std::mutex> lock(_mutex);
        return _devices.size();
    }

    void process_and_render(int cols, int rows, float view_width, float view_height)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        int stream_no = 0;
        for (auto&& view : _devices)
        {
            // For each device get its frames
            for (auto&& frames_queue : view.second.frames_per_stream)
            {
                //Enhance visuzalization, where appropriate
                rs2::frame f;
                if (frames_queue.second._raw.poll_for_frame(&f))
                {
                    if (f.as<rs2::frameset>().size() > 1)
                    {
                        throw std::runtime_error("Processing requires frames decomposition!");
                    }

                    frames_queue.second._processed = view.second.colorize_frame.process(f);
                }

                // Render frame
                if (frames_queue.second._processed)
                {
                    rect frame_location{ view_width * (stream_no % cols), view_height * (stream_no / cols), view_width, view_height };
                    if (rs2::video_frame vid_frame = frames_queue.second._processed.as<rs2::video_frame>())
                    {
                        view.second.tex.render(vid_frame, frame_location);
                        stream_no++;
                    }
                    if (rs2::motion_frame mot_frame = frames_queue.second._processed.as<rs2::motion_frame>())
                    {
                        view.second.tex.render(mot_frame, frame_location);
                        stream_no++;
                    }
                    if (rs2::pose_frame pos_frame = frames_queue.second._processed.as<rs2::pose_frame>())
                    {
                        view.second.tex.render(pos_frame, frame_location);
                        stream_no++;
                    }
                }
            }
        }
    }

    void add_dev_event(rs2::event_information& info)
    {
        _dev_events.add_event(info);
    }

    void enable_device(rs2::device dev)
    {
        std::string serial_number(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
        std::lock_guard<std::mutex> lock(_mutex);

        if (_devices.find(serial_number) != _devices.end())
        {
            std::cout << __FUNCTION__ << " Device s.n " << serial_number << " is already registered, skipped" << std::endl;
            return; //already in
        }
        std::cout << __FUNCTION__ << " !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!      Enabling device " << dev.get_info(RS2_CAMERA_INFO_NAME) << " s.n: " << serial_number << std::endl;
        // Ignoring platform cameras (webcams, etc..)
        if (platform_camera_name == dev.get_info(RS2_CAMERA_INFO_NAME))
        {
            std::cout << __FUNCTION__ << " !!!!!!!!!!!!!!!!!!!!!!!!!!        Platform camera found, return" << std::endl;
            return;
        }
        // Create a pipeline from the given device
        rs2::pipeline p(_ctx);
        rs2::config c;
        c.enable_device(serial_number);

        _devices.emplace(serial_number, view_port());

        rs2::pipeline_profile profile = p.start(c, [serial_number, this](const rs2::frame& frame)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            // With callbacks, both the synchronized and unsynchronized frames would arrive in a single frameset
            if (auto fs = frame.as<rs2::frameset>())
            {
                for (const rs2::frame& f : fs)
                {
                    //std::cout << "Enq: Frame type " << f.get_profile().stream_name() << " format: " << f.get_profile().format() << " stream id: " << f.get_profile().unique_id() << " fn: " << f.get_frame_number() << std::endl;
                    if (_devices.find(serial_number) != _devices.end())
                        _devices[serial_number].frames_per_stream[f.get_profile().unique_id()]._raw.enqueue(f);
                }
            }
            else
            {
                // Stream that bypass synchronization (such as IMU) will produce single frames
                if (_devices.find(serial_number) != _devices.end())
                    _devices[serial_number].frames_per_stream[frame.get_profile().unique_id()]._raw.enqueue(frame);
                /*std::cout << "Enq: Single  type " << frame.get_profile().stream_name() << " format: " << frame.get_profile().format()
                    << " stream id: " << frame.get_profile().unique_id() << " fn: " << frame.get_frame_number() << std::endl;*/
            }
        });

        _devices[serial_number].pipe = p;
        _devices[serial_number].profile = profile;
        // Hold it internally
        std::cout << "!!!!!!!!!!!!!!  Starting with device " << profile.get_device().get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) << " streams: " << profile.get_streams().size() << std::endl;
    }

    void remove_devices(const rs2::event_information& info)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        // Go over the list of devices and check if it was disconnected
        auto itr = _devices.begin();
        while (itr != _devices.end())
        {
            if (info.was_removed(itr->second.profile.get_device()))
            {
                std::cout << "Device erased !!!!!!!!!!!!!!!!! " << itr->first << std::endl;
                itr = _devices.erase(itr);
                std::cout << "Devices left = " << _devices.size() << std::endl;
            }
            else
            {
                ++itr;
            }
        }
    }

 private:
    std::mutex                          _mutex;
    std::map<std::string, view_port>    _devices;
    rs2::context&                       _ctx;
    device_events                       _dev_events;
};

int main(int argc, char * argv[]) try
{
    // Create a simple OpenGL window for rendering:
    window app(1280, 960, "CPP Multi-Camera Example");

    rs2::log_to_console(RS2_LOG_SEVERITY_WARN);

    rs2::context ctx;    // Create librealsense context for managing devices

    device_container connected_devices(ctx);

    // Enumerate all the connected devices
    for (auto&& dev : ctx.query_devices())
    {
        std::cout << "Enabling device " << dev.get_info(RS2_CAMERA_INFO_NAME) << " , " << dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) << std::endl;
        connected_devices.enable_device(dev);
    }

    // Register user callback for tracking device connect/disconnect events
    ctx.set_devices_changed_callback([&](rs2::event_information& info)
    {
        std::cout << "set_devices_changed_callback fired !!!!!!!!!!!!!!! " << std::endl;
        connected_devices.add_dev_event(info);
    });

    while (app) // Application still alive?
    {
        connected_devices.refresh_devices();
        auto total_streams = connected_devices.stream_count();
        if (0 == total_streams)
        {
            draw_text(int(std::max(0.f, (app.width() / 2) - no_camera_message.length() * 3)),
                      int(app.height() / 2), no_camera_message.c_str());
        }
        if (1 ==connected_devices.device_count())
        {
            draw_text(0, 10, connect_more_cameras.c_str());
        }

        // Update UI
        if (total_streams)
        {
            int cols = int(std::ceil(std::sqrt(total_streams)));
            int rows = int(std::ceil(total_streams / static_cast<float>(cols)));

            float view_width = (app.width() / cols);
            float view_height = (app.height() / rows);
            connected_devices.process_and_render(cols, rows, view_width, view_height);
        }
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
