// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2/rsutil.h>
#include "example.hpp"          // Include short list of convenience functions for rendering

// This example will require several standard data-structures and algorithms:
#define _USE_MATH_DEFINES
#include <math.h>
#include <queue>
#include <unordered_set>
#include <map>
#include <thread>

using pixel = std::pair<int, int>;

// Neighbors function returns 12 fixed neighboring pixels in an image
std::array<pixel, 12> neighbors(rs2::depth_frame frame, pixel p);

// Distance 3D is used to calculate real 3D distance between two pixels
float dist_3d(const rs2_intrinsics& intr, const rs2::depth_frame& frame, pixel u, pixel v);
// Distance 2D returns the distance in pixels between two pixels
float dist_2d(const pixel& a, const pixel& b);

// Toggle helper class will be used to render the two buttons
// controlling the edges of our ruler
struct toggle
{
    toggle() : x(0.f), y(0.f) {}
    toggle(float x, float y)
        : x(std::min(std::max(x, 0.f), 1.f)),
          y(std::min(std::max(y, 0.f), 1.f))
    {}

    // Move from [0,1] space to pixel space of specific frame
    pixel get_pixel(rs2::depth_frame frame) const
    {
        int px = x * frame.get_width();
        int py = y * frame.get_height();
        return{ px, py };
    }

    void render(const window& app)
    {
        // Render a circle
        const float r = 10;
        const float segments = 16;
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);
        glColor3f(0.f, 1.0f, 0.0f);
        glLineWidth(2);
        glBegin(GL_LINE_STRIP);
        for (auto i = 0; i <= segments; i++)
        {
            auto t = 2 * M_PI * float(i) / segments;
            glVertex2f(x * app.width()  + cos(t) * r,
                       y * app.height() + sin(t) * r);
        }
        glEnd();
        glDisable(GL_BLEND);
        glColor3f(1.f, 1.f, 1.f);
    }

    // This helper function is used to find the button
    // closest to the mouse cursor
    // Since we are only comparing this distance, sqrt can be safely skipped
    float dist_2d(const toggle& other) const
    {
        return pow(x - other.x, 2) + pow(y - other.y, 2);
    }

    float x;
    float y;
    bool selected = false;
};

// Application state shared between the main-thread and GLFW events
struct state
{
    bool mouse_down = false;
    toggle ruler_start;
    toggle ruler_end;
};

// Helper function to register to UI events
void register_glfw_callbacks(window& app, state& app_state);

// Distance rendering functions:

// Simple distance is the classic pythagorean distance between 3D points
// This distance ignores the topology of the object and can cut both through
// air and through solid
void render_simple_distance(const rs2::depth_frame& depth,
                            const state& s,
                            const window& app,
                            const rs2_intrinsics& intr);

// Shortest-path distance approximates the geodesic.
// Given two points on a surface it will follow with that surface
void render_shortest_path(const rs2::depth_frame& depth,
                          const std::vector<pixel>& path,
                          const window& app,
                          float total_dist);

int main(int argc, char * argv[]) try
{
    // OpenGL textures for the color and depth frames
    texture depth_image, color_image;

    // Colorizer is used to visualize depth data
    rs2::colorizer color_map;
    // Decimation filter reduces the amount of data (while preserving best samples)
    rs2::decimation_filter dec;
    // If the demo is too slow, make sure you run in Release (-DCMAKE_BUILD_TYPE=Release)
    // but you can also increase the following parameter to decimate depth more (reducing quality)
    dec.set_option(RS2_OPTION_FILTER_MAGNITUDE, 2);
    // Define transformations from and to Disparity domain
    rs2::disparity_transform depth2disparity;
    rs2::disparity_transform disparity2depth(false);
    // Define spatial filter (edge-preserving)
    rs2::spatial_filter spat;
    // Enable hole-filling
    // Hole filling is an agressive heuristic and it gets the depth wrong many times
    // However, this demo is not built to handle holes
    // (the shortest-path will always prefer to "cut" through the holes since they have zero 3D distance)
    spat.set_option(RS2_OPTION_HOLES_FILL, 5); // 5 = fill all the zero pixels
    // Define temporal filter
    rs2::temporal_filter temp;
    // Spatially align all streams to depth viewport
    // We do this because:
    //   a. Usually depth has wider FOV, and we only really need depth for this demo
    //   b. We don't want to introduce new holes
    rs2::align align_to(RS2_STREAM_DEPTH);

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;

    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH); // Enable default depth
    // For the color stream, set format to RGBA
    // To allow blending of the color frame on top of the depth frame
    cfg.enable_stream(RS2_STREAM_COLOR, RS2_FORMAT_RGBA8);
    auto profile = pipe.start(cfg);

    auto sensor = profile.get_device().first<rs2::depth_sensor>();

    // TODO: At the moment the SDK does not offer a closed enum for D400 visual presets
    // (because they keep changing)
    // As a work-around we try to find the High-Density preset by name
    // We do this to reduce the number of black pixels
    // The hardware can perform hole-filling much better and much more power efficient then our software
    auto range = sensor.get_option_range(RS2_OPTION_VISUAL_PRESET);
    for (auto i = range.min; i < range.max; i += range.step)
        if (std::string(sensor.get_option_value_description(RS2_OPTION_VISUAL_PRESET, i)) == "High Density")
            sensor.set_option(RS2_OPTION_VISUAL_PRESET, i);

    auto stream = profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
    auto intrinsics = stream.get_intrinsics(); // Calibration data

    // Create a simple OpenGL window for rendering:
    window app(stream.width(), stream.height(), "RealSense Measure Example");

    // Define application state and position the ruler buttons
    state app_state;
    app_state.ruler_start = { 0.45f, 0.5f };
    app_state.ruler_end   = { 0.55f, 0.5f };
    register_glfw_callbacks(app, app_state);

    // After initial post-processing, frames will flow into this queue:
    rs2::frame_queue postprocessed_frames;

    // In addition, depth frames will also flow into this queue:
    rs2::frame_queue pathfinding_queue;

    // Alive boolean will signal the worker threads to finish-up
    std::atomic_bool alive{ true };

    // The pathfinding thread will write its output to this memory:
    std::vector<pixel> path;
    float total_dist = 0.f;
    std::mutex path_mutex; // It is protected by a mutex

    // Video-processing thread will fetch frames from the camera,
    // apply post-processing and send the result to the main thread for rendering
    // It recieves synchronized (but not spatially aligned) pairs
    // and outputs synchronized and aligned pairs
    std::thread video_processing_thread([&]() {
        // In order to generate new composite frames, we have to wrap the processing
        // code in a lambda
        rs2::processing_block frame_processor(
            [&](rs2::frameset data, // Input frameset (from the pipeline)
                rs2::frame_source& source) // Frame pool that can allocate new frames
        {
            // First make the frames spatially aligned
            data = align_to.process(data);

            // Next, apply depth post-processing
            rs2::frame depth = data.get_depth_frame();
            // Decimation will reduce the resultion of the depth image,
            // closing small holes and speeding-up the algorithm
            depth = dec.process(depth);
            // To make sure far-away objects are filtered proportionally
            // we try to switch to disparity domain
            depth = depth2disparity.process(depth);
            // Apply spatial filtering
            depth = spat.process(depth);
            // Apply temporal filtering
            depth = temp.process(depth);
            // If we are in disparity domain, switch back to depth
            depth = disparity2depth.process(depth);
            // Send the post-processed depth for path-finding
            pathfinding_queue.enqueue(depth);

            // Apply color map for visualization of depth
            auto colorized = color_map(depth);
            auto color = data.get_color_frame();
            // Group the two frames together (to make sure they are rendered in sync)
            rs2::frameset combined = source.allocate_composite_frame({ colorized, color });
            // Send the composite frame for rendering
            source.frame_ready(combined);
        });
        // Indicate that we want the results of frame_processor
        // to be pushed into postprocessed_frames queue
        frame_processor >> postprocessed_frames;

        while (alive)
        {
            // Fetch frames from the pipeline and send them for processing
            rs2::frameset fs;
            if (pipe.poll_for_frames(&fs)) frame_processor.invoke(fs);
        }
    });

    // Shortest-path thread is recieving depth frame and
    // runs classic Dijkstra on it to find the shortest path (in 3D)
    // between the two points the user have chosen
    std::thread shortest_path_thread([&]() {
        while (alive)
        {
            // Try to fetch frames from the pathfinding_queue
            rs2::frame depth;
            if (pathfinding_queue.poll_for_frame(&depth))
            {
                // Define vertex+distance struct
                using dv = std::pair<float, pixel>;

                // Define source, target and a terminator pixel
                pixel src = app_state.ruler_start.get_pixel(depth);
                pixel trg = app_state.ruler_end.get_pixel(depth);
                pixel token{ -1, -1 }; // When we see this value we know we reached source

                // Dist holds distances of every pixel from source
                std::map<pixel, float> dist;
                // Parent map is used to reconsturct the shortest-path
                std::map<pixel, pixel> parent;
                // Priority queue holds pixels (ordered by their distance)
                std::priority_queue<dv, std::vector<dv>, std::greater<dv>> q;

                // Initialize the source pixel:
                dist[src] = 0.f;
                parent[src] = token;
                q.emplace(0.f, src);

                // To save calculation we apply a heuristic
                // Don't visit pixels that are too far away in 2D space
                // It is very rare for objects in 3D space to violate this
                auto max_2d_dist = dist_2d(src, trg) * 1.2;

                while (!q.empty() && alive)
                {
                    // Fetch the closest pixel from the queue
                    pixel u = q.top().second; q.pop();

                    // If we reached the max radius, don't continue to expand
                    if (dist_2d(src, u) > max_2d_dist) continue;

                    // Fetch the list of neighboring pixels
                    auto n = neighbors(depth, u);
                    for (auto&& v : n)
                    {
                        // If this pixel was not yet visited, initialize
                        // its distance as +INF
                        if (dist.find(v) == dist.end()) dist[v] = INFINITY;

                        // Calculate distance in 3D between the two neighboring pixels
                        auto d = dist_3d(intrinsics, depth, u, v);
                        // Calculate total distance from source
                        auto total_dist = dist[u] + d;

                        // If we encounter a potential improvement,
                        if (dist[v] > total_dist)
                        {
                            // Update parent and distance
                            parent[v] = u;
                            dist[v] = total_dist;
                            // And re-visit that pixel by re-introducing it to the queue
                            q.emplace(total_dist, v);
                        }
                    }
                }

                {
                    // Write the shortest-path to the path variable
                    std::lock_guard<std::mutex> lock(path_mutex);
                    total_dist = dist[trg];
                    path.clear();
                    // Iterate until encounter token special pixel
                    while (trg != token)
                    {
                        // Handle the case we didn't find a path
                        if (trg == parent[trg]) break;

                        path.emplace_back(trg);
                        trg = parent[trg];
                    }
                }
            }
        }
    });

    while(app) // Application still alive?
    {
        // Fetch the latest available post-processed frameset
        static rs2::frameset current_frameset;
        postprocessed_frames.poll_for_frame(&current_frameset);

        if (current_frameset)
        {
            auto depth = current_frameset.get_depth_frame();
            auto color = current_frameset.get_color_frame();

            glEnable(GL_BLEND);
            // Use the Alpha channel for blending
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // First render the colorized depth image
            depth_image.render(depth, { 0, 0, app.width(), app.height() });
            // Next, set global alpha for the color image to 90%
            // (to make it slightly translucent)
            //glColor4f(1.f, 1.f, 1.f, 0.9f);
            // Render the color frame (since we have selected RGBA format
            // pixels out of FOV will appear transparent)
            color_image.render(color, { 0, 0, app.width(), app.height() });

            {
                // Take the lock, to make sure the path is not modified
                // while we are rendering it
                std::lock_guard<std::mutex> lock(path_mutex);

                // Use 1-Color model to invert background colors
                glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ONE_MINUS_SRC_COLOR);

                // Render the shortest-path as calculated
                render_shortest_path(depth, path, app, total_dist);
                // Render the simple pythagorean distance
                render_simple_distance(depth, app_state, app, intrinsics);

                // Render the ruler
                app_state.ruler_start.render(app);
                app_state.ruler_end.render(app);

                glColor3f(1.f, 1.f, 1.f);
            }
            glDisable(GL_BLEND);
        }
    }

    // Signal threads to finish and wait until they do
    alive = false;
    video_processing_thread.join();
    shortest_path_thread.join();

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

std::array<pixel, 12> neighbors(rs2::depth_frame frame, pixel p)
{
    // Define 12 neighboring pixels in some fixed pattern
    int px = p.first;
    int py = p.second;
    std::array<pixel, 12> res{
        pixel{ px, py - 1 },
        pixel{ px - 1, py },
        pixel{ px + 1, py },
        pixel{ px, py + 1 },
        pixel{ px - 1, py - 2 },
        pixel{ px + 1, py - 2 },
        pixel{ px - 1, py + 2 },
        pixel{ px + 1, py + 2 },
        pixel{ px - 2, py - 1 },
        pixel{ px + 2, py - 1 },
        pixel{ px - 2, py + 1 },
        pixel{ px + 2, py + 1 }
    };
    for (auto&& r : res)
    {
        r.first  = std::min(std::max(r.first, 0),  frame.get_width() - 1);
        r.second = std::min(std::max(r.second, 0), frame.get_height() - 1);
    }
    return res;
}

float dist_3d(const rs2_intrinsics& intr, const rs2::depth_frame& frame, pixel u, pixel v)
{
    float upixel[2]; // From pixel
    float upoint[3]; // From point (in 3D)

    float vpixel[2]; // To pixel
    float vpoint[3]; // To point (in 3D)

    // Copy pixels into the arrays (to match rsutil signatures)
    upixel[0] = u.first;
    upixel[1] = u.second;
    vpixel[0] = v.first;
    vpixel[1] = v.second;

    // Query the frame for distance
    // Note: this can be optimized
    // It is not recommended to issue an API call for each pixel
    // (since the compiler can't inline these)
    // However, in this example it is not one of the bottlenecks
    auto udist = frame.get_distance(upixel[0], upixel[1]);
    auto vdist = frame.get_distance(vpixel[0], vpixel[1]);

    // Deproject from pixel to point in 3D
    rs2_deproject_pixel_to_point(upoint, &intr, upixel, udist);
    rs2_deproject_pixel_to_point(vpoint, &intr, vpixel, vdist);

    // Calculate euclidean distance between the two points
    return sqrt(pow(upoint[0] - vpoint[0], 2) +
                pow(upoint[1] - vpoint[1], 2) +
                pow(upoint[2] - vpoint[2], 2));
}

float dist_2d(const pixel& a, const pixel& b)
{
    return pow(a.first - b.first, 2) + pow(a.second - b.second, 2);
}

void render_simple_distance(const rs2::depth_frame& depth,
                            const state& s,
                            const window& app,
                            const rs2_intrinsics& intr)
{
    pixel center;
    glColor3f(1.f, 0.0f, 1.0f);
    glPushAttrib(GL_ENABLE_BIT);
    glLineStipple(1, 0x00ff);
    glEnable(GL_LINE_STIPPLE);
    glLineWidth(5);
    glBegin(GL_LINE_STRIP);
    glVertex2f(s.ruler_start.x * app.width(),
               s.ruler_start.y * app.height());
    glVertex2f(s.ruler_end.x * app.width(),
               s.ruler_end.y * app.height());
    glEnd();
    glPopAttrib();

    auto from_pixel = s.ruler_start.get_pixel(depth);
    auto to_pixel =   s.ruler_end.get_pixel(depth);
    float air_dist = dist_3d(intr, depth, from_pixel, to_pixel);

    center.first  = (from_pixel.first + to_pixel.first) / 2;
    center.second = (from_pixel.second + to_pixel.second) / 2;

    std::stringstream ss;
    ss << int(air_dist * 100) << " cm";
    auto str = ss.str();
    auto x = (float(center.first)  / depth.get_width())  * app.width();
    auto y = (float(center.second) / depth.get_height()) * app.height();
    draw_text(x + 15, y + 15, str.c_str());
}

void render_shortest_path(const rs2::depth_frame& depth,
                          const std::vector<pixel>& path,
                          const window& app,
                          float total_dist)
{
    pixel center;
    glColor3f(0.f, 1.0f, 0.0f);
    glLineWidth(5);
    glBegin(GL_LINE_STRIP);

    for (int i = 0; i < path.size(); i++)
    {
        auto&& pixel = path[i];
        auto x = (float(pixel.first)  /  depth.get_width()) * app.width();
        auto y = (float(pixel.second) / depth.get_height()) * app.height();
        glVertex2f(x, y);

        if (i == path.size() / 2) center = { x, y };
    }
    glEnd();

    if (path.size() > 1)
    {
        std::stringstream ss;
        ss << int(total_dist * 100) << " cm";
        auto str = ss.str();
        draw_text(center.first + 15, center.second + 15, str.c_str());
    }
}

// Implement drag&drop behaviour for the buttons:
void register_glfw_callbacks(window& app, state& app_state)
{
    app.on_left_mouse = [&](bool pressed)
    {
        app_state.mouse_down = pressed;
    };

    app.on_mouse_move = [&](double x, double y)
    {
        toggle cursor{ float(x) / app.width(), float(y) / app.height() };
        std::vector<toggle*> toggles{
            &app_state.ruler_start,
            &app_state.ruler_end };

        if (app_state.mouse_down)
        {
            toggle* best = toggles.front();
            for (auto&& t : toggles)
            {
                if (t->dist_2d(cursor) < best->dist_2d(cursor))
                {
                    best = t;
                }
            }
            best->selected = true;
        }
        else
        {
            for (auto&& t : toggles) t->selected = false;
        }

        for (auto&& t : toggles)
        {
            if (t->selected) *t = cursor;
        }
    };
}

