// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include "example.hpp"          // Include short list of convenience functions for rendering
#include <cstring>

struct short3
{
    uint16_t x, y, z;
};

#include "t265.h"

// Struct to store trajectory points
struct tracked_point
{
    rs2_vector point;
    unsigned int confidence;
};

// Class to store the trajectory and draw it
class tracker
{
public:
    // Calculates transformation matrix based on pose data from the device
    void calc_transform(rs2_pose& pose_data, float mat[16])
    {
        auto q = pose_data.rotation;
        auto t = pose_data.translation;
        // Set the matrix as column-major for convenient work with OpenGL and rotate by 180 degress (by negating 1st and 3rd columns)
        mat[0] = -(1 - 2 * q.y*q.y - 2 * q.z*q.z); mat[4] = 2 * q.x*q.y - 2 * q.z*q.w;     mat[8] = -(2 * q.x*q.z + 2 * q.y*q.w);      mat[12] = t.x;
        mat[1] = -(2 * q.x*q.y + 2 * q.z*q.w);     mat[5] = 1 - 2 * q.x*q.x - 2 * q.z*q.z; mat[9] = -(2 * q.y*q.z - 2 * q.x*q.w);      mat[13] = t.y;
        mat[2] = -(2 * q.x*q.z - 2 * q.y*q.w);     mat[6] = 2 * q.y*q.z + 2 * q.x*q.w;     mat[10] = -(1 - 2 * q.x*q.x - 2 * q.y*q.y); mat[14] = t.z;
        mat[3] = 0.0f;                             mat[7] = 0.0f;                          mat[11] = 0.0f;                             mat[15] = 1.0f;
    }

    // Updates minimum and maximum coordinates of the trajectory, used in order to scale the viewport accordingly
    void update_min_max(rs2_vector point)
    {
        max_coord.x = std::max(max_coord.x, point.x);
        max_coord.y = std::max(max_coord.y, point.y);
        max_coord.z = std::max(max_coord.z, point.z);
        min_coord.x = std::min(min_coord.x, point.x);
        min_coord.y = std::min(min_coord.y, point.y);
        min_coord.z = std::min(min_coord.z, point.z);
    }

    // Register new point in the trajectory
    void add_to_trajectory(tracked_point& p)
    {
        // If first element, add to trajectory and initialize minimum and maximum coordinates
        if (trajectory.size() == 0)
        {
            trajectory.push_back(p);
            max_coord = p.point;
            min_coord = p.point;
        }
        else
        {
            // Check if new element is far enough - more than 1 mm (to keep trajectory vector as small as possible)
            rs2_vector prev = trajectory.back().point;
            rs2_vector curr = p.point;
            if (sqrt(pow((curr.x - prev.x), 2) + pow((curr.y - prev.y), 2) + pow((curr.z - prev.z), 2)) < 0.001)
            {
                // If new point is too close to previous point and has higher confidence, replace the previous point with the new one
                if (p.confidence > trajectory.back().confidence)
                {
                    trajectory.back() = p;
                    update_min_max(p.point);
                }
            }
            else
            {
                // If new point is far enough, add it to trajectory
                trajectory.push_back(p);
                update_min_max(p.point);
            }
        }
    }

    // Draw trajectory with colors according to point confidence
    void draw_trajectory()
    {
        float3 colors[]{
            { 0.7f, 0.7f, 0.7f },
            { 1.0f, 0.0f, 0.0f },
            { 1.0f, 1.0f, 0.0f },
            { 0.0f, 1.0f, 0.0f },
        };

        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        for (auto&& v : trajectory)
        {
            auto c = colors[v.confidence];
            glColor3f(c.x, c.y, c.z);
            glVertex3f(v.point.x, v.point.y, v.point.z);
        }
        glEnd();
        glLineWidth(0.5f);
    }

    rs2_vector get_max_coord()
    {
        return max_coord;
    }

    rs2_vector get_min_coord()
    {
        return min_coord;
    }
private:
    std::vector<tracked_point> trajectory;
    rs2_vector max_coord;
    rs2_vector min_coord;
};

// Class that handles rendering of the T265 3D model
class camera_renderer
{
public:
    // Initialize renderer with data needed to draw the camera
    camera_renderer()
    {
        uncompress_t265_obj(positions, normals, indexes);
    }

    // Render the camera model
    void render_camera()
    {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        // Scale camera drawing to match view
        glScalef(0.001, 0.001, 0.001);

        glBegin(GL_TRIANGLES);
        // Draw the camera
        for (auto& i : indexes)
        {
            glVertex3fv(&positions[i.x].x);
            glVertex3fv(&positions[i.y].x);
            glVertex3fv(&positions[i.z].x);
            glColor4f(0.036f, 0.044f, 0.051f, 0.3f);
        }
        glEnd();
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }
private:
    std::vector<float3> positions, normals;
    std::vector<short3> indexes;
};

// Renders grid for the 3D view
void draw_grid()
{
    glPushMatrix();
    glBegin(GL_LINES);
    glColor4f(0.4f, 0.4f, 0.4f, 1.f);
    for (int i = 0; i <= 8; i++)
    {
        glVertex3i(-i + 4, -4, 0);
        glVertex3i(-i + 4, 4, 0);
        glVertex3i(4, -i+4, 0);
        glVertex3i(-4, -i+4, 0);
    }
    glEnd();
    glPopMatrix();
}

// Sets the 3D view and handles user manipulations
void render_scene(glfw_state app_state)
{
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glColor3f(1.0, 1.0, 1.0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 4 / 3, 0.5, 100);

    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
    gluLookAt(1, -1, 4, 0, 1, 0, 0, 0, 1);
    glTranslatef(0, 0, app_state.offset_y);
    glRotated(app_state.pitch, -1, 0, 0);
    glRotated(app_state.yaw, 0, 0, -1);
    draw_grid();
}


// Draws seprating lines between viewports
void draw_borders()
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glColor4f(0.4f, 0.4f, 0.4f, 1.f);
    glBegin(GL_LINE_STRIP);
    glVertex2i(1, 0);
    glVertex2i(-1, 0);
    glVertex2i(0, 0);
    glVertex2i(0, 1);
    glVertex2i(0, -1);
    glEnd();
}

// Calculates and prints the scale of the viewport in meters
float display_scale(float scale_factor, float x_pos, float y_pos)
{
    glColor3f(1.0f, 1.0f, 1.0f);
    // Set default width for the scale bar (in OpenGL units)
    float bar_width = 0.1;
    // Calculate the ratio of the default width to the current scale factor
    float bar_scale = bar_width / scale_factor;
    // If scale is less than 1 meter, display it as is
    if (bar_scale > 1)
    {
        // If scale is between 1 and 2, round to 1
        bar_scale = floor(bar_scale);
        // If scale is above 2, round down to the nearest multiple of 5
        if (bar_scale > 2)
        {
            int diff = 5 - int(bar_scale) % 5;
            bar_scale = bar_scale + diff;
        }
        // Calculate the bar width matching the calculated scale
        bar_width = scale_factor * bar_scale;
    }
    // Print scale
    std::stringstream ss;
    ss << bar_scale << " m";
    auto str = ss.str();
    draw_text(x_pos, y_pos, str.c_str());
    ss.clear(); ss.str("");
    return bar_width;
}

// Draw x, y, z axes
void draw_axes()
{
    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0);  glVertex3f(-1, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0);  glVertex3f(0, -1, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0);  glVertex3f(0, 0, 1);
    glEnd();
}

typedef float rs2_vector::* pos;

// Base class for rendering a viewport
class view
{
public:
    view(float width, float height, tracker& tracker, camera_renderer& r)
        : width(width), height(height), aspect (height / width), t(tracker), renderer(r) {  } 

    // Setup viewport
    void load_matrices(float2 pos, float app_width, float app_height)
    {
        width = app_width;
        height = app_height;
        aspect = height / width;

        // Set viewport to 1/4 of the app window and enable scissor test to avoid rendering outside the current viewport
        glViewport(pos.x, pos.y, width / 2, height / 2);
        glEnable(GL_SCISSOR_TEST);
        glScissor(pos.x, pos.y, width / 2, height / 2);

        // Setup orthogonal projection matrix
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glOrtho(-1.0, 1.0, -1.0 * aspect, 1.0 * aspect, -100.0, 100.0);

        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
    }

    // Return to default setup
    void clean_matrices()
    {
        // Pop LookAt matrix
        glPopMatrix();
        // Pop Projection matrix
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        // Set viewport back to full screen
        glViewport(0, 0, width, height);
        glDisable(GL_SCISSOR_TEST);
    }

    // Draw trajectoy and render camera according to the given transformation matrix
    void draw_cam_trajectory(float angle, float3 axes, float r[16])
    {
        glPushMatrix();
        // Set initial camera position (rotate by given angle)
        glRotatef(angle, axes.x, axes.y, axes.z);
        t.draw_trajectory();
        glMultMatrixf(r);
        renderer.render_camera();
        glPopMatrix();
    }
protected:
    float width, height;
    float aspect;
    tracker& t;
private:     
    camera_renderer& renderer;
};

// Class for rendering 2d view of the camera and trajectory
class view_2d : public view
{
public:
    view_2d(float width, float height, tracker& tracker, camera_renderer& renderer, rs2_vector lookat_eye, pos a, pos b, pos c)
        : view(width, height, tracker, renderer), a(a), b(b), c(c), lookat_eye(lookat_eye), window_borders{ 1, aspect }
    {  }

    // Renders a viewport on 1/4 of the app window
    void draw_view(float2 pos, float width, float height, float2 scale_pos, float r[16])
    {
        // Calculate and print scale in meters
        float bar_width = display_scale(scale_factor, scale_pos.x, scale_pos.y);

        rs2_vector min_coord = t.get_min_coord();
        rs2_vector max_coord = t.get_max_coord();

        // Prepare viewport for rendering
        load_matrices(pos, width, height);

        glBegin(GL_LINES);
        // Draw scale bar
        glColor3f(1.0f, 1.0f, 1.0f); 
        glVertex3f(0.8, -0.9 * aspect, 0); glVertex3f(0.8 + bar_width, -0.9 * aspect, 0);
        glEnd();
        // Set a 2D view using OpenGL's LookAt matrix
        gluLookAt(lookat_eye.x, lookat_eye.y, lookat_eye.z, 0, 0, 0, 0, 1, 0);
        // Draw axes (only two are visible)
        draw_axes();
        // Scale viewport according to current scale factor
        glScalef(scale_factor, scale_factor, scale_factor);
        // If trajectory reached one of the viewport's borders, zoom out and update scale factor
        if (min_coord.*a < -window_borders.x || max_coord.*a > window_borders.x
            || min_coord.*b < -window_borders.y || max_coord.*b > window_borders.y)
        {
            glScalef(0.5, 0.5, 0.5);
            scale_factor *= 0.5;
            window_borders.x = window_borders.x * 2;
            window_borders.y = window_borders.y * 2;
        }
        // Draw trajectory and camera
        draw_cam_trajectory(180, { 0, 1, 0 }, r);
        // Return the default configuration of OpenGL's matrices
        clean_matrices();
    }
private:
    float2 window_borders;
    float scale_factor = 1.0;
    rs2_vector lookat_eye;
    pos a, b, c;
};

// Class for rendering 3D view of the camera and trajectory
class view_3d : public view
{
public:
    view_3d(float width, float height, tracker& tracker, camera_renderer& renderer)
        : view(width, height, tracker, renderer) { }

    void draw_view(float2 pos, float app_width, float app_height, glfw_state app_state, float r[16])
    {
        // Prepare viewport for rendering
        load_matrices(pos, app_width, app_height);
        // Set the scene configuration and handle user's manipulations
        render_scene(app_state);
        // Draw trajectory and camera
        draw_cam_trajectory(90, { 1, 0, 0 }, r);
        // // Return the default configuration of OpenGL's matrices
        clean_matrices();
    }
};


class split_screen_renderer
{
public:
    split_screen_renderer(float app_width, float app_height, tracker& tracker, camera_renderer& renderer)
        : top(app_width, app_height, tracker, renderer, rs2_vector{ 0, 10, -(1e-3f) }, &rs2_vector::x, &rs2_vector::z, &rs2_vector::y),
        front(app_width, app_height, tracker, renderer, rs2_vector{ 0, 0, -10 }, &rs2_vector::x, &rs2_vector::y, &rs2_vector::z),
        side(app_width, app_height, tracker, renderer, rs2_vector{ 10, 0, 0 }, &rs2_vector::z, &rs2_vector::y, &rs2_vector::x),
        three_dim(app_width, app_height, tracker, renderer)
    { }

    void draw_windows(float app_width, float app_height, glfw_state app_state, float r[16])
    {
        // Clear screen
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Upper left window: top perspective (x and z axes)
        top.draw_view({ 0, app_height / 2 }, app_width, app_height, float2{ float(0.457) * app_width, float(0.49) * app_height }, r);
        // Lower left window: front perspective (x and y axes)
        front.draw_view({ 0, 0 }, app_width, app_height, float2{ float(0.457) * app_width, float(0.99) * app_height }, r);
        // Lower right window: side perspective (y and z axes)
        side.draw_view({ app_width / 2, 0 }, app_width, app_height, float2{ float(0.957) * app_width, float(0.99) * app_height }, r);
        // Upper right window: 3D view
        three_dim.draw_view({ app_width / 2, app_height / 2 }, app_width, app_height, app_state, r);

        // Draw viewport titles
        glColor3f(1.0f, 1.0f, 1.0f);
        draw_text(0.005 * app_width, 0.02 * app_height, "TOP");
        draw_text(0.005 * app_width, 0.52 * app_height, "FRONT");
        draw_text(0.505 * app_width, 0.52 * app_height, "SIDE");
        draw_text(0.505 * app_width, 0.02 * app_height, "3D");

        // Label axes
        glColor3f(1.0f, 0.0f, 0.0f);
        draw_text(0.494 * app_width, 0.261 * app_height, "x");
        draw_text(0.494 * app_width, 0.761 * app_height, "x");
        glColor3f(0.0f, 0.0f, 1.0f);
        draw_text(0.245 * app_width, 0.01 * app_height, "z");
        draw_text(0.503 * app_width, 0.761 * app_height, "z");
        glColor3f(0.0f, 1.0f, 0.0f);
        draw_text(0.245 * app_width, 0.995 * app_height, "y");
        draw_text(0.745 * app_width, 0.995 * app_height, "y");

        // Draw lines bounding each viewport
        draw_borders();
    }
private:
    view_2d top, front, side;
    view_3d three_dim;
};

// In this example, we show how to track the camera's motion using a T265 device
int main(int argc, char * argv[]) try
{
    // Initialize window for rendering
    window app(1280, 720, "RealSense Trajectory Example");

    // Construct an object to manage view state
    glfw_state app_state(0.0, 0.0);
    // Register callbacks to allow manipulation of the view state
    register_glfw_callbacks(app, app_state);

    // Create objects for rendering the camera, the trajectory and the split screen
    camera_renderer cam_renderer;
    tracker tracker;
    split_screen_renderer screen_renderer(app.width(), app.height(), tracker, cam_renderer);

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Create a configuration for configuring the pipeline with a non default profile
    rs2::config cfg;
    // Add pose stream
    cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);

    // Start pipeline with chosen configuration
    pipe.start(cfg);

    // Main loop
    while (app)
    {
        // Wait for the next set of frames from the camera
        auto frames = pipe.wait_for_frames();
        // Get a frame from the pose stream
        auto f = frames.first_or_default(RS2_STREAM_POSE);
        // Cast the frame to pose_frame and get its data
        auto pose_data = f.as<rs2::pose_frame>().get_pose_data();
        float r[16];
        // Calculate current transformation matrix
        tracker.calc_transform(pose_data, r);
        // From the matrix we found, get the new location point
        rs2_vector tr{ r[12], r[13], r[14] };
        // Create a new point to be added to the trajectory
        tracked_point p{ tr , pose_data.tracker_confidence };
        // Register the new point
        tracker.add_to_trajectory(p);
        // Draw the trajectory from different perspectives
        screen_renderer.draw_windows(app.width(), app.height(), app_state, r);
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
