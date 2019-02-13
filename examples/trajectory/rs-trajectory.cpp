#include <librealsense2/rs.hpp>
#include <mutex>
#include "example.hpp"          // Include short list of convenience functions for rendering

struct short3
{
    uint16_t x, y, z;
};

#include "t265.h"
#include <thread>
#include <atomic>


void draw_floor()
{
    glBegin(GL_LINES);
    glColor4f(0.4f, 0.4f, 0.4f, 1.f);
    // Render "floor" grid
    for (int i = 0; i <= 8; i++)
    {
        glVertex3i(i - 4, 1, 0);
        glVertex3i(i - 4, 1, 8);
        glVertex3i(-4, 1, i);
        glVertex3i(4, 1, i);
    }
    glEnd();
}


bool is_tm2()
{
    rs2::context ctx;
    for (auto dev : ctx.query_devices())
    {
        for (auto sensor : dev.query_sensors())
        {
            for (auto profile : sensor.get_stream_profiles())
            {
                if (profile.stream_type() == RS2_STREAM_POSE)
                    return true;
            }
        }
    }
    return false;
}

struct tracked_point
{
    rs2_vector point;
    unsigned int confidence;
};

class pose_estimator
{
    std::vector<tracked_point> trajectory;
    std::mutex mtx;
public:
    void add_to_trajectory(tracked_point& p)
    {
        //std::lock_guard<std::mutex> lock(mtx);
        //insert first element anyway
        if (trajectory.size() == 0)
        {
            trajectory.push_back(p);
        }
        else
        {
            //check if new element is far enough - more than 1 mm
            rs2_vector prev = trajectory.back().point;
            rs2_vector curr = p.point;
            if (sqrt(pow((curr.x - prev.x), 2) + pow((curr.y - prev.y), 2) + pow((curr.z - prev.z), 2)) < 0.001)
            {
                //if too close - check confidence and replace element
                if (p.confidence > trajectory.back().confidence)
                {
                    trajectory.back() = p;
                }
                //else - discard this sample
            }
            else
            {
                //sample is far enough - keep it
                trajectory.push_back(p);
            }
        }
    }

    void draw_trajectory()
    {
        //std::lock_guard<std::mutex> lock(mtx);
        glLineWidth(3.0f);
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
        glLineWidth(1);
    }

};


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

    matrix4(float vals[4][4])
    {
        std::memcpy(mat, vals, sizeof(mat));
    }

    //init rotation matrix from quaternion
    matrix4(rs2_quaternion q)
    {
        mat[0][0] = 1 - 2 * q.y*q.y - 2 * q.z*q.z; mat[0][1] = 2 * q.x*q.y - 2 * q.z*q.w;     mat[0][2] = 2 * q.x*q.z + 2 * q.y*q.w;     mat[0][3] = 0.0f;
        mat[1][0] = 2 * q.x*q.y + 2 * q.z*q.w;     mat[1][1] = 1 - 2 * q.x*q.x - 2 * q.z*q.z; mat[1][2] = 2 * q.y*q.z - 2 * q.x*q.w;     mat[1][3] = 0.0f;
        mat[2][0] = 2 * q.x*q.z - 2 * q.y*q.w;     mat[2][1] = 2 * q.y*q.z + 2 * q.x*q.w;     mat[2][2] = 1 - 2 * q.x*q.x - 2 * q.y*q.y; mat[2][3] = 0.0f;
        mat[3][0] = 0.0f;                      mat[3][1] = 0.0f;                      mat[3][2] = 0.0f;                      mat[3][3] = 1.0f;
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

inline matrix4 tm2_pose_to_world_transformation(const rs2_pose& pose)
{
    matrix4 rotation(pose.rotation);
    matrix4 translation(pose.translation);
    matrix4 G_tm2_body_to_tm2_world = translation * rotation;
    float rotate_180_y[4][4] = { { -1, 0, 0, 0 },
    { 0, 1, 0, 0 },
    { 0, 0,-1, 0 },
    { 0, 0, 0, 1 } };
    matrix4 G_vr_body_to_tm2_body(rotate_180_y);
    matrix4 G_vr_body_to_tm2_world = G_tm2_body_to_tm2_world * G_vr_body_to_tm2_body;

    float rotate_90_x[4][4] = { { 1, 0, 0, 0 },
    { 0, 0,-1, 0 },
    { 0, 1, 0, 0 },
    { 0, 0, 0, 1 } };
    matrix4 G_tm2_world_to_vr_world(rotate_90_x);
    matrix4 G_vr_body_to_vr_world = G_tm2_world_to_vr_world * G_vr_body_to_tm2_world;

    return G_vr_body_to_vr_world;
}


int main(int argc, char * argv[]) try
{
    // Before running the example, check that a device supporting IMU is connected
   /* if (!is_tm2())
    {
        std::cerr << "Device supporting pose stream (T265) not found";
        return EXIT_FAILURE;
    }*/

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

    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Add pose stream
    cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);

    // Declare object that handles camera pose calculations
    pose_estimator tracker;

    pipe.start(cfg);

    float max_x = 0;
    float max_y = 0;
    float max_z = 0;

    // Main loop
    while (app)
    {
        // Wait for the next set of frames from the camera
        auto frames = pipe.wait_for_frames();
        auto f = frames.first_or_default(RS2_STREAM_POSE);
        auto pose_data = f.as<rs2::pose_frame>().get_pose_data();

        auto t = tm2_pose_to_world_transformation(pose_data);

        rs2_vector translation{ t.mat[0][3], t.mat[1][3], 3+t.mat[2][3] };

        if (abs(t.mat[0][3]) >= 4)
            max_x = t.mat[0][3];
        if (abs(t.mat[1][3]) >= 4)
            max_y = t.mat[1][3];
        if (abs(t.mat[1][3]) >= 6)
            max_z = t.mat[1][3];
        tracked_point p{ translation , pose_data.tracker_confidence };
        // register the new viewpoint
        tracker.add_to_trajectory(p);

        glClearColor(0.0, 0.0, 0.0, 1.0);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(80.0, 4 / 3, 1, 40);

        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);

        glLoadIdentity();
        gluLookAt(1, -3, 10, 1, 0, 0, 0, -1, 0);

        glTranslatef(-max_x, -max_y, max_z + app_state.offset_y*0.2f);
        glRotated(app_state.pitch, -1, 0, 0);
        glRotated(app_state.yaw, 0, 1, 0);
        draw_floor();
        tracker.draw_trajectory();
        glPopMatrix();
    }
    pipe.stop();

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