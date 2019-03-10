#include <librealsense2/rs.hpp>
#include "example.hpp"          // Include short list of convenience functions for rendering
#include <thread>


struct short3
{
    uint16_t x, y, z;
};

#include "t265.h"

struct tracked_point
{
    rs2_vector point;
    unsigned int confidence;
};

// matrix4 class for operations on 4x4 matrices
struct matrix4
{
    float mat[4][4];

    operator float*() const
    {
        return (float*)&mat;
    }

    matrix4()
    {
        std::memset(mat, 0, sizeof(mat));
    }

    matrix4(const float vals[4][4])
    {
        std::memcpy(mat, vals, sizeof(mat));
    }

    //init rotation matrix from quaternion
    matrix4(rs2_quaternion q)
    {
        mat[0][0] = 1 - 2 * q.y*q.y - 2 * q.z*q.z; mat[0][1] = 2 * q.x*q.y - 2 * q.z*q.w;     mat[0][2] = 2 * q.x*q.z + 2 * q.y*q.w;     mat[0][3] = 0.0f;
        mat[1][0] = 2 * q.x*q.y + 2 * q.z*q.w;     mat[1][1] = 1 - 2 * q.x*q.x - 2 * q.z*q.z; mat[1][2] = 2 * q.y*q.z - 2 * q.x*q.w;     mat[1][3] = 0.0f;
        mat[2][0] = 2 * q.x*q.z - 2 * q.y*q.w;     mat[2][1] = 2 * q.y*q.z + 2 * q.x*q.w;     mat[2][2] = 1 - 2 * q.x*q.x - 2 * q.y*q.y; mat[2][3] = 0.0f;
        mat[3][0] = 0.0f;                          mat[3][1] = 0.0f;                          mat[3][2] = 0.0f;                          mat[3][3] = 1.0f;
    }

    //init translation matrix from vector
    matrix4(rs2_vector t)
    {
        mat[0][0] = 1.0f; mat[0][1] = 0.0f; mat[0][2] = 0.0f; mat[0][3] = t.x;
        mat[1][0] = 0.0f; mat[1][1] = 1.0f; mat[1][2] = 0.0f; mat[1][3] = t.y;
        mat[2][0] = 0.0f; mat[2][1] = 0.0f; mat[2][2] = 1.0f; mat[2][3] = t.z;
        mat[3][0] = 0.0f; mat[3][1] = 0.0f; mat[3][2] = 0.0f; mat[3][3] = 1.0f;
    }

    void to_column_major(float column_major[16])
    {
        column_major[0] = mat[0][0];
        column_major[1] = mat[1][0];
        column_major[2] = mat[2][0];
        column_major[3] = mat[3][0];
        column_major[4] = mat[0][1];
        column_major[5] = mat[1][1];
        column_major[6] = mat[2][1];
        column_major[7] = mat[3][1];
        column_major[8] = mat[0][2];
        column_major[9] = mat[1][2];
        column_major[10] = mat[2][2];
        column_major[11] = mat[3][2];
        column_major[12] = mat[0][3];
        column_major[13] = mat[1][3];
        column_major[14] = mat[2][3];
        column_major[15] = mat[3][3];
    }
};

inline matrix4 operator*(matrix4 a, matrix4 b)
{
    matrix4 res;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++)
            {
                sum += a.mat[i][k] * b.mat[k][j];
            }
            res.mat[i][j] = sum;
        }
    }
    return res;
}


class tracker
{
    std::vector<tracked_point> trajectory;
    rs2_vector max_coord;
    rs2_vector min_coord;
public:
    matrix4 tm2_pose_to_world_transformation(const rs2_pose& pose)
    {
        // Get rotation and translation matrices from device's pose data
        matrix4 rotation(pose.rotation);
        matrix4 translation(pose.translation);
        // Get the transformation matrix by multiplying translation and rotation
        matrix4 G_tm2_body_to_tm2_world = translation * rotation;
        // Rotate view so that the transformation matches the desired axes directions (facing the user)
        float rotate_180_y[4][4] = { { -1, 0, 0, 0 },
        { 0, 1, 0, 0 },
        { 0, 0,-1, 0 },
        { 0, 0, 0, 1 } };
        matrix4 G_vr_body_to_tm2_body(rotate_180_y);
        matrix4 G_vr_body_to_tm2_world = G_tm2_body_to_tm2_world * G_vr_body_to_tm2_body;

        float rotate_90_x[4][4] = { { 1, 0, 0, 0 },
        { 0, 0, -1, 0 },
        { 0, 1, 0, 0 },
        { 0, 0, 0, 1 } };

        return  G_vr_body_to_tm2_world;
    }

    matrix4 calc_transform(rs2_pose& pose_data)
    {
        // Calculate the transformation matrix from pose data
        auto t = tm2_pose_to_world_transformation(pose_data);
        float model[4][4];
        t.to_column_major((float*)model);
        auto m = model;
        matrix4 r(m);
        return r;
    }

    void update_min_max(rs2_vector point)
    {
        max_coord.x = std::max(max_coord.x, point.x);
        max_coord.y = std::max(max_coord.y, point.y);
        max_coord.z = std::max(max_coord.z, point.z);
        min_coord.x = std::min(min_coord.x, point.x);
        min_coord.y = std::min(min_coord.y, point.y);
        min_coord.z = std::min(min_coord.z, point.z);
    }

    void add_to_trajectory(tracked_point& p)
    {
        //insert first element anyway
        if (trajectory.size() == 0)
        {
            trajectory.push_back(p);
            max_coord = p.point;
            min_coord = p.point;
        }
        else
        {
            //check if new element is far enough - more than 1 mm
            rs2_vector prev = trajectory.back().point;
            rs2_vector curr = p.point;
            if (sqrt(pow((curr.x - prev.x), 2) + pow((curr.y - prev.y), 2) + pow((curr.z - prev.z), 2)) < 0.001)
            {
                // If new point is too close to previous point has higher confidence, replace the previous point with the new one
                if (p.confidence > trajectory.back().confidence)
                {
                    trajectory.back() = p;
                    update_min_max(p.point);
                }
            }
            else
            {
                //sample is far enough - keep it
                trajectory.push_back(p);
                update_min_max(p.point);
            }
        }
    }

    void draw_trajectory()
    {
        glLineWidth(2.0f);
        glBegin(GL_LINE_STRIP);
        for (auto&& v : trajectory)
        {
            switch (v.confidence) //color the line according to confidence
            {
            case 3:
                glColor3f(0.0f, 1.0f, 0.0f); //green
                break;
            case 2:
                glColor3f(1.0f, 1.0f, 0.0f); //yellow
                break;
            case 1:
                glColor3f(1.0f, 0.0f, 0.0f); //red
                break;
            case 0:
                glColor3f(0.7f, 0.7f, 0.7f); //grey - failed pose
                break;
            default:
                throw std::runtime_error("Invalid pose confidence value");
            }
            glVertex3f(v.point.x, v.point.y, v.point.z);
        }
        glEnd();
        glLineWidth(0.5f);
    }

    rs2_vector get_max_borders()
    {
        return max_coord;
    }

    rs2_vector get_min_borders()
    {
        return min_coord;
    }
};

// green down, blue towards me, red left
void draw_axes()
{
    glLineWidth(2);
    glBegin(GL_LINES);
    // Draw x, y, z axes
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0);  glVertex3f(-0.2, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0);  glVertex3f(0, -0.2, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0);  glVertex3f(0, 0, 0.2);
    glEnd();

    glLineWidth(1);
}


class camera_renderer
{
    std::vector<float3> positions, normals;
    std::vector<short3> indexes;
    float x = static_cast<float>(-PI / 2);
    // Set the initial position of the camera (facing the user)
    float _rx[4][4] = {
        { 1 , 0, 0, 0 },
        { 0, cos(x), -sin(x), 0 },
        { 0, sin(x), cos(x), 0 },
        { 0, 0, 0, 1 }
    };
public:
    // Initialize renderer with data needed to draw the camera
    camera_renderer()
    {
        uncompress_t265_obj(positions, normals, indexes);
    }

    // Takes the calculated angle as input and rotates the 3D camera model accordignly
    void render_camera()
    {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        // Scale camera drawing
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

};

void draw_grid()
{
    int i;
    float x, y;

    glPushMatrix();
    glBegin(GL_LINES);
    glColor4f(0.4f, 0.4f, 0.4f, 1.f);
    // Render grid
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

void render_scene(glfw_state app_state)
{
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glColor3f(1.0, 1.0, 1.0);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, 4 / 3, 1, 40);

    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
    gluLookAt(1, -1, 4, 0, 1, 0, 0, 0, 1);
    // Use the translation calculated from device to track it
    glTranslatef(0, 0, app_state.offset_y);
    glRotated(app_state.pitch, -1, 0, 0);
    glRotated(app_state.yaw, 0, 0, -1);
    draw_grid();
}


void draw_borders(int width, int height)
{
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
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


float display_scale(float scale_factor, float x_pos, float y_pos)
{
    glColor3f(1.0f, 1.0f, 1.0f);
    // Set default width for the scale bar
    float bar_width = 0.1;
    float bar_scale = bar_width / scale_factor;
    // if scale is less than 1 meter, display it as is; if it is above 5 meters, 
    if (bar_scale > 1)
    {
        bar_scale = floor(bar_scale);
        if (bar_scale > 2) // If scale is above 2, round to the closest number divisble by 5
        {
            int diff = 5 - int(bar_scale) % 5;
            bar_scale = bar_scale + diff;
        }
        bar_width = scale_factor * bar_scale;
    }
    std::stringstream ss;
    ss << bar_scale << " m";
    auto str = ss.str();
    draw_text(x_pos, y_pos, str.c_str());
    return bar_width;
}


class split_screen_renderer
{
    float width, height;
    float scale_threshold_front_x, scale_threshold_front_y;
    float scale_threshold_top_x, scale_threshold_top_z;
    float scale_threshold_side_z, scale_threshold_side_y;
    float scale_factor_front, scale_factor_top, scale_factor_side;
    float aspect;
    float x = static_cast<float>(-PI / 2);

    const float _rx_180[4][4] = {
        { 1 , 0, 0, 0 },
    { 0, cos(x * 2), -sin(x * 2), 0 },
    { 0, sin(x * 2), cos(x * 2), 0 },
    { 0, 0, 0, 1 }
    };

    const float _ry_180[4][4] = {
        { cos(x * 2) , 0, sin(x * 2) },
    { 0, 1, 0, 0 },
    { -sin(x * 2), 0, cos(x * 2), 0 },
    { 0, 0, 0, 1 }
    };

    const float _rx_90[4][4] = {
        { 1 , 0, 0, 0 },
    { 0, cos(x), -sin(x), 0 },
    { 0, sin(x), cos(x), 0 },
    { 0, 0, 0, 1 }
    };

    const float _rz_180[4][4] = {
        { cos(-x * 2) , -sin(-x * 2), 0, 0 },
    { sin(-x * 2), cos(-x * 2), 0, 0 },
    { 0, 0, 1, 0 },
    { 0, 0, 0, 1 }
    };
    

public:
    split_screen_renderer(float app_width, float app_height)
    {
        width = app_width;
        height = app_height;
        aspect = height / width;
        scale_threshold_front_x = 1.0;
        scale_threshold_front_y = 1.0 * aspect;
        scale_threshold_top_x = 1.0;
        scale_threshold_top_z = 1.0 * aspect;
        scale_threshold_side_z = 1.0;
        scale_threshold_side_y = 1.0 * aspect;
        scale_factor_front = 1;
        scale_factor_top = 1;
        scale_factor_side = 1;
    }

    void draw_windows(float app_width, float app_height, glfw_state app_state,  tracker& tracker, camera_renderer& renderer, matrix4& r)
    {
        auto min_borders = tracker.get_min_borders();
        auto max_borders = tracker.get_max_borders();
        width = app_width;
        height = app_height;
        glLineWidth(0.5f);
        // Clear screen
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        glEnable(GL_BLEND);
        glColor3f(1.0f, 0.0f, 0.0f);
        draw_text(4.5 * app_width / 70, 34.7 * app_height / 70, "x");
        draw_text(4.5 * app_width / 70, 35.8 * app_height / 70, "x");

        glColor3f(0.0f, 1.0f, 0.0f);
        draw_text(app_width / 150, 6.1 * app_height / 10, "y");
        draw_text(148.5 * app_width / 150, 6.1 * app_height / 10, "y");

        glColor3f(0.0f, 0.0f, 1.0f);
        draw_text(140 * app_width / 150, 35.8 * app_height / 70, "z");
        draw_text(app_width / 150, 4 * app_height / 10, "z");

        float bar_width_top = display_scale(scale_factor_top, 10.2 * app_width / 22, 34.5 * app_height / 70);
        float bar_width_front = display_scale(scale_factor_front, 10.2 * app_width / 22, 69.5 * app_height / 70);
        float bar_width_side = display_scale(scale_factor_side, 21.2 * app_width / 22, 69.5 * app_height / 70);

        // Enable scissor test
        glEnable(GL_SCISSOR_TEST);

        // Enable depth test
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);

        // Setup orthogonal projection matrix, we chose to map the screen so that it covers 2 meters of the space from right to left
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(-1.0, 1.0, -1.0 * aspect, 1.0 * aspect, -50.0, 50.0);

        matrix4 ry_180(_ry_180);
        matrix4 rx_90(_rx_90);
        matrix4 rz_180(_rz_180);
        matrix4 rx_180(_rx_180);

        // Upper left view (TOP VIEW)
        glViewport(0, height / 2, width / 2, height / 2);
        glScissor(0, height / 2, width / 2, height / 2);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(0, 10, -(1e-3f), 0, 0, 0, 0, 1, 0);
        glPushMatrix();
        glTranslatef(0.97, 0, -0.97 * aspect);
        draw_axes();
        glPopMatrix();

        glBegin(GL_LINES);
        // draw scale bar
        glColor3f(1.0f, 1.0f, 1.0f); glVertex3f(-0.8, 0, -0.9 * aspect);  glVertex3f(-0.8-bar_width_top, 0, -0.9 * aspect);
        glEnd();

        glScalef(scale_factor_top, scale_factor_top, scale_factor_top);
        if (min_borders.x < -scale_threshold_top_x || max_borders.x > scale_threshold_top_x
            || min_borders.z < -scale_threshold_top_z || max_borders.z > scale_threshold_top_z)
        {
            glScalef(0.5, 0.5, 0.5);
            scale_factor_top *= 0.5;
            scale_threshold_top_x = scale_threshold_top_x * 2;
            scale_threshold_top_z = scale_threshold_top_z * 2;
        }
        glPushMatrix();
        glMultMatrixf(ry_180);
        tracker.draw_trajectory();
        glMultMatrixf(r);
        renderer.render_camera();
        glPopMatrix();

        // Lower left view (FRONT VIEW) // normal view
        glViewport(0, 0, width / 2, height / 2);
        glScissor(0, 0, width / 2, height / 2);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(0, 0, -10, 0, 0, 0, 0, 1, 0);
        glPushMatrix();
        glTranslatef(0.97, 0.97 * aspect, 0.0);
        draw_axes();
        glPopMatrix();
        glBegin(GL_LINES);
        // draw scale bar
        glColor3f(1.0f, 1.0f, 1.0f); glVertex3f(-0.8, -0.9 * aspect, 0);  glVertex3f(-0.8-bar_width_front, -0.9 * aspect, 0);
        glEnd();
        glScalef(scale_factor_front, scale_factor_front, scale_factor_front);
        if (min_borders.x < -scale_threshold_front_x || max_borders.x > scale_threshold_front_x
            || min_borders.y < -scale_threshold_front_y || max_borders.y > scale_threshold_front_y)
        {
            glScalef(0.5, 0.5, 0.5);
            scale_factor_front *= 0.5;
            scale_threshold_front_x = scale_threshold_front_x * 2;
            scale_threshold_front_y = scale_threshold_front_y * 2;
        }
        glPushMatrix();
        glMultMatrixf(ry_180);
        tracker.draw_trajectory();
        glMultMatrixf(r);
        renderer.render_camera();
        glPopMatrix();

        // Lower right view (SIDE VIEW) - z (blue) is left, y (green) is down, x (red)
        glViewport(width / 2, 0, width / 2, height / 2);
        glScissor(width / 2, 0, width / 2, height / 2);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        gluLookAt(10, 0, 0, 1, 0, 0, 0, 1, 0);
        glPushMatrix();
        glTranslatef(0, 0.97 * aspect, -0.97);
        draw_axes();
        glPopMatrix();
        glBegin(GL_LINES);
        // draw scale bar
        glColor3f(1.0f, 1.0f, 1.0f); glVertex3f(0, -0.9 * aspect , -0.8);  glVertex3f(0, -0.9 * aspect, -0.8-bar_width_side);
        glEnd();
        glScalef(scale_factor_side, scale_factor_side, scale_factor_side);
        if (min_borders.z < -scale_threshold_side_z || max_borders.z > scale_threshold_side_z
            || min_borders.y < -scale_threshold_side_y || max_borders.y > scale_threshold_side_y)
        {
            glScalef(0.5, 0.5, 0.5);
            scale_factor_side *= 0.5;
            scale_threshold_side_z = scale_threshold_side_z * 2;
            scale_threshold_side_y = scale_threshold_side_y * 2;
        }
        glPushMatrix();
        glMultMatrixf(rz_180*rx_180);
        tracker.draw_trajectory();
        glMultMatrixf(r);
        renderer.render_camera();
        glPopMatrix();

        // Upper right view - 3D VIEW
        glViewport(width / 2, height / 2, width / 2, height / 2);
        glScissor(width / 2, height / 2, width / 2, height / 2);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        render_scene(app_state);
        glPushMatrix();
        glMultMatrixf(rx_90);
        tracker.draw_trajectory();
        glMultMatrixf(r);
        draw_axes();
        renderer.render_camera();
        glPopMatrix();

        glDisable(GL_BLEND);
        // Disable depth test
        glDisable(GL_DEPTH_TEST);
        // Disable scissor test
        glDisable(GL_SCISSOR_TEST);

        // Draw lines bounding each viewport
        draw_borders(width, height);
    }
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

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Create a configuration for configuring the pipeline with a non default profile
    rs2::config cfg;
    // Add pose stream
    cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);

    // Start pipeline with chosen configuration
    pipe.start(cfg);

    camera_renderer cam_renderer;
    tracker tracker;
    split_screen_renderer screen_renderer(app.width(), app.height());

    // Main loop
    while (app)
    {
        // Wait for the next set of frames from the camera
        auto frames = pipe.wait_for_frames();
        // Get a frame from the pose stream
        auto f = frames.first_or_default(RS2_STREAM_POSE);
        // Cast the frame to pose_frame and get its data
        auto pose_data = f.as<rs2::pose_frame>().get_pose_data();
        // Calculate current transformation matrix
        auto r = tracker.calc_transform(pose_data);
        // From the matrix we found, get the new location point
        rs2_vector tr{ r.mat[3][0], r.mat[3][1], r.mat[3][2] };
        // Create a new point to be added to the trajectory
        tracked_point p{ tr , pose_data.tracker_confidence };
        // Register the new point
        tracker.add_to_trajectory(p);
        // Draw the trajectory from different perspectives
        screen_renderer.draw_windows(app.width(), app.height(), app_state, tracker, cam_renderer, r);
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
