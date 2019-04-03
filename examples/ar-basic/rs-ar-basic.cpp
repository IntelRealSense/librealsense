// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>
#include <array>
#include <cmath>
#include <iostream>
#include <vector>
#include "example.hpp"

struct point3d {
    float f[3];

    point3d() {}
    point3d(float x, float y, float z) : f{x, y, z} {}
    float x() const { return f[0]; }
    float y() const { return f[1]; }
    float z() const { return f[2]; }
};

struct pixel {
    float f[2];

    pixel() {}
    pixel(float x, float y) : f{x, y} {}
    float x() const { return f[0]; }
    float y() const { return f[1]; }
};

// We define a virtual object as a collection of vertices that will be connected by lines
typedef std::array<point3d, 4> object;

static rs2_pose identity_pose();
static rs2_pose reset_object_pose(const rs2_pose& device_pose_in_world = identity_pose());
static rs2_pose pose_inverse(const rs2_pose& p);
static rs2_pose pose_multiply(const rs2_pose& ref2_in_ref1, const rs2_pose& ref3_in_ref2);
static rs2_quaternion quaternion_conjugate(const rs2_quaternion& q);
static rs2_quaternion quaternion_multiply(const rs2_quaternion& a, const rs2_quaternion& b);
static rs2_vector quaternion_rotate_vector(const rs2_quaternion& q, const rs2_vector& v);
static rs2_vector pose_transform_point(const rs2_pose& pose, const rs2_vector& p);
static rs2_vector vector_addition(const rs2_vector& a, const rs2_vector& b);
static rs2_vector vector_negate(const rs2_vector& v);

static object convert_object_coordinates(const object& obj, const rs2_pose& object_pose);

static std::vector<point3d> raster_line(const point3d& a, const point3d& b, float step);
static void render_line(const std::vector<pixel>& line, int color_code);
static void render_text(int win_height, const std::string& text);

int main(int argc, char * argv[]) try
{
    std::cout << "Waiting for device..." << std::endl;

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Create a configuration for configuring the pipeline with a non default profile
    rs2::config cfg;
    // Enable fisheye and pose streams
    cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);
    cfg.enable_stream(RS2_STREAM_FISHEYE, 1);
    cfg.enable_stream(RS2_STREAM_FISHEYE, 2);
    // Start pipeline with chosen configuration
    rs2::pipeline_profile pipe_profile = pipe.start(cfg);

    // T265 has two fisheye sensors, we can choose any of them (index 1 or 2)
    const int fisheye_sensor_idx = 1;

    // Get fisheye sensor intrinsics parameters
    rs2::stream_profile fisheye_stream = pipe_profile.get_stream(RS2_STREAM_FISHEYE, fisheye_sensor_idx);
    rs2_intrinsics intrinsics = fisheye_stream.as<rs2::video_stream_profile>().get_intrinsics();

    // Get fisheye sensor extrinsics parameters.
    // This is the pose of the fisheye sensor relative to the T265 coordinate system.
    rs2_extrinsics extrinsics = fisheye_stream.get_extrinsics_to(pipe_profile.get_stream(RS2_STREAM_POSE));

    std::cout << "Device got. Streaming data" << std::endl;

    // Create an OpenGL display window and a texture to draw the fisheye image
    window app(intrinsics.width, intrinsics.height, "Intel RealSense T265 Augmented Reality Example");
    window_key_listener key_watcher(app);
    texture fisheye_image;

    // Create the vertices of a simple virtual object.
    // This virtual object is 4 points in 3D space that describe 3 XYZ 20cm long axes.
    // These vertices are relative to the object's own coordinate system.
    const float length = 0.20;
    const object virtual_object = {{
        { 0, 0, 0 },      // origin
        { length, 0, 0 }, // X
        { 0, length, 0 }, // Y
        { 0, 0, length }  // Z
    }};

    // This variable will hold the pose of the virtual object in world coordinates.
    // We we initialize it once we get the first pose frame.
    rs2_pose object_pose_in_world;
    bool object_pose_in_world_initialized = false;

    // Main loop
    while (app)
    {
        rs2_pose device_pose_in_world; // This will contain the current device pose
        {
            // Wait for the next set of frames from the camera
            auto frames = pipe.wait_for_frames();
            // Get a frame from the fisheye stream
            rs2::video_frame fisheye_frame = frames.get_fisheye_frame(fisheye_sensor_idx);
            // Get a frame from the pose stream
            rs2::pose_frame pose_frame = frames.get_pose_frame();

            // Copy current camera pose
            device_pose_in_world = pose_frame.get_pose_data();

            // Render the fisheye image
            fisheye_image.render(fisheye_frame, { 0, 0, app.width(), app.height() });

            // By closing the current scope we let frames be deallocated, so we do not fill up librealsense queues while we do other computation.
        }

        // If we have not set the virtual object in the world yet, set it in front of the camera now.
        if (!object_pose_in_world_initialized)
        {
            object_pose_in_world = reset_object_pose(device_pose_in_world);
            object_pose_in_world_initialized = true;
        }

        // Compute the pose of the object relative to the current pose of the device
        rs2_pose world_pose_in_device = pose_inverse(device_pose_in_world);
        rs2_pose object_pose_in_device = pose_multiply(world_pose_in_device, object_pose_in_world);

        // Get the object vertices in device coordinates
        object object_in_device = convert_object_coordinates(virtual_object, object_pose_in_device);

        // Convert object vertices from device coordinates into fisheye sensor coordinates using extrinsics
        object object_in_sensor;
        for (size_t i = 0; i < object_in_device.size(); ++i)
        {
            rs2_transform_point_to_point(object_in_sensor[i].f, &extrinsics, object_in_device[i].f);
        }

        for (size_t i = 1; i < object_in_sensor.size(); ++i)
        {
            // Discretize the virtual object line into smaller 1cm long segments
            std::vector<point3d> points_in_sensor = raster_line(object_in_sensor[0], object_in_sensor[i], 0.01);
            std::vector<pixel> projected_line;
            projected_line.reserve(points_in_sensor.size());
            for (auto& point : points_in_sensor)
            {
                // A 3D point is visible in the image if its Z coordinate relative to the fisheye sensor is positive.
                if (point.z() > 0)
                {
                    // Project 3D sensor coordinates to 2D fisheye image coordinates using intrinsics
                    projected_line.emplace_back();
                    rs2_project_point_to_pixel(projected_line.back().f, &intrinsics, point.f);
                }
            }
            // Display the line in the image
            render_line(projected_line, i);
        }

        // Display text in the image
        render_text(app.height(), "Press spacebar to reset the pose of the virtual object. Press ESC to exit");

        // Check if some key is pressed
        switch (key_watcher.get_key())
        {
        case GLFW_KEY_SPACE:
            // Reset virtual object pose if user presses spacebar
            object_pose_in_world = reset_object_pose(device_pose_in_world);
            std::cout << "Setting new pose for virtual object: " << object_pose_in_world.translation << std::endl;
            break;
        case GLFW_KEY_ESCAPE:
            // Exit if user presses escape
            app.close();
            break;
        }
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

rs2_pose identity_pose()
{
    // Return an identity pose (no translation, no rotation)
    rs2_pose pose;
    pose.translation.x = 0;
    pose.translation.y = 0;
    pose.translation.z = 0;
    pose.rotation.x = 0;
    pose.rotation.y = 0;
    pose.rotation.z = 0;
    pose.rotation.w = 1;
    return pose;
}

rs2_pose reset_object_pose(const rs2_pose& device_pose_in_world)
{
    // Set the object 50 centimeter away in front of the camera.
    // T265 coordinate system is defined here: https://github.com/IntelRealSense/librealsense/blob/master/doc/t265.md#sensor-origin-and-coordinate-system
    rs2_pose object_pose_in_device;
    object_pose_in_device.translation.x = 0;
    object_pose_in_device.translation.y = 0;
    object_pose_in_device.translation.z = -0.50;
    object_pose_in_device.rotation.x = 0;
    object_pose_in_device.rotation.y = 0;
    object_pose_in_device.rotation.z = 0;
    object_pose_in_device.rotation.w = 1;

    // Convert the pose of the virtual object from camera coordinates into world coordinates
    rs2_pose object_pose_in_world = pose_multiply(device_pose_in_world, object_pose_in_device);
    return object_pose_in_world;
}

rs2_pose pose_inverse(const rs2_pose& p)
{
    rs2_pose i;
    i.rotation = quaternion_conjugate(p.rotation);
    i.translation = vector_negate(quaternion_rotate_vector(i.rotation, p.translation));
    return i;
}

rs2_pose pose_multiply(const rs2_pose& ref2_in_ref1, const rs2_pose& ref3_in_ref2)
{
    rs2_pose ref3_in_ref1;
    ref3_in_ref1.rotation = quaternion_multiply(ref2_in_ref1.rotation, ref3_in_ref2.rotation);
    ref3_in_ref1.translation = vector_addition(quaternion_rotate_vector(ref2_in_ref1.rotation, ref3_in_ref2.translation), ref2_in_ref1.translation);
    return ref3_in_ref1;
}

rs2_vector pose_transform_point(const rs2_pose& pose, const rs2_vector& p)
{
    return vector_addition(quaternion_rotate_vector(pose.rotation, p), pose.translation);
}

rs2_quaternion quaternion_multiply(const rs2_quaternion& a, const rs2_quaternion& b)
{
    return rs2_quaternion {
        a.x * b.w + a.w * b.x - a.z * b.y + a.y * b.z,
        a.y * b.w + a.z * b.x + a.w * b.y - a.x * b.z,
        a.z * b.w - a.y * b.x + a.x * b.y + a.w * b.z,
        a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z,
    };
}

rs2_vector quaternion_rotate_vector(const rs2_quaternion& q, const rs2_vector& v)
{
    rs2_quaternion v_as_quaternion = { v.x, v.y, v.z, 0 };
    rs2_quaternion rotated_v = quaternion_multiply(quaternion_multiply(q, v_as_quaternion), quaternion_conjugate(q));
    return rs2_vector { rotated_v.x, rotated_v.y, rotated_v.z };
}

rs2_quaternion quaternion_conjugate(const rs2_quaternion& q)
{
    return rs2_quaternion { -q.x, -q.y, -q.z, q.w };
}

rs2_vector vector_addition(const rs2_vector& a, const rs2_vector& b)
{
    return rs2_vector { a.x + b.x, a.y + b.y, a.z + b.z };
}

rs2_vector vector_negate(const rs2_vector& v)
{
    return rs2_vector { -v.x, -v.y, -v.z };
}

object convert_object_coordinates(const object& obj, const rs2_pose& object_pose)
{
    object transformed_obj;
    for (size_t i = 0; i < obj.size(); ++i) {
        rs2_vector v { obj[i].x(), obj[i].y(), obj[i].z() };
        v = pose_transform_point(object_pose, v);
        transformed_obj[i].f[0] = v.x;
        transformed_obj[i].f[1] = v.y;
        transformed_obj[i].f[2] = v.z;
    }
    return transformed_obj;
}

std::vector<point3d> raster_line(const point3d& a, const point3d& b, float step)
{
    rs2_vector direction = { b.x() - a.x(), b.y() - a.y(), b.z() - a.z() };
    float distance = std::sqrt(direction.x*direction.x + direction.y*direction.y + direction.z*direction.z);
    int npoints = distance / step + 1;

    std::vector<point3d> points;
    if (npoints > 0)
    {
        direction.x = direction.x * step / distance;
        direction.y = direction.y * step / distance;
        direction.z = direction.z * step / distance;

        points.reserve(npoints);
        points.emplace_back(a);
        for (int i = 1; i < npoints; ++i)
        {
            points.emplace_back(a.x() + direction.x * i,
                                a.y() + direction.y * i,
                                a.z() + direction.z * i);
        }
    }
    return points;
}

void render_line(const std::vector<pixel>& line, int color_code)
{
    if (!line.empty())
    {
        GLfloat current_color[4];
        glGetFloatv(GL_CURRENT_COLOR, current_color);

        glLineWidth(5);
        glColor3f(color_code == 1 ? 1.f : 0.f,
                  color_code == 2 ? 1.f : 0.f,
                  color_code == 3 ? 1.f : 0.f);

        glBegin(GL_LINE_STRIP);
        for (auto& pixel : line)
        {
            glVertex3f(pixel.x(), pixel.y(), 0.f);
        }
        glEnd();

        glColor4fv(current_color);
    }
}

void render_text(int win_height, const std::string& text)
{
    GLfloat current_color[4];
    glGetFloatv(GL_CURRENT_COLOR, current_color);
    glColor3f(0, 0.5, 1);
    glScalef(2, 2, 2);
    draw_text(15, (win_height - 10) / 2, text.c_str());
    glScalef(1, 1, 1);
    glColor4fv(current_color);
}
