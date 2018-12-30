#include <librealsense2/rs.hpp>
#include <algorithm>
#include <iostream>
#include <thread>
#include <../third-party/eigen/Eigen/Geometry>

#include <mutex>

#include "example.hpp"          // Include short list of convenience functions for rendering
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

struct int3
{
    int x, y, z;
};

#include "d435.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif


class GyroBias
{
private:
    int calibrationUpdates;
    double minX, maxX;
    double minY, maxY;
    double minZ, maxZ;


public:
    bool isSet;

    double x;
    double y;
    double z;

    GyroBias()
    {
        reset();
    }

    void reset()
    {
        calibrationUpdates = 0;

        minX = 1000;
        minY = 1000;
        minZ = 1000;
        maxX = -1000;
        maxY = -1000;
        maxZ = -1000;

        x = 0;
        y = 0;
        z = 0;

        isSet = false;
    }


    bool update(double gx, double gy, double gz)
    {
        if (calibrationUpdates < 50)
        {
            maxX = std::max(gx, maxX);
            maxY = std::max(gy, maxY);
            maxZ = std::max(gz, maxZ);

            minX = std::min(gx, minX);
            minY = std::min(gy, minY);
            minZ = std::min(gz, minZ);

            calibrationUpdates++;
            return false;
        }
        else if (calibrationUpdates == 50)
        {
            x = (maxX + minX) / 2.0;
            y = (maxY + minY) / 2.0;
            z = (maxZ + minZ) / 2.0;
            calibrationUpdates++;

            /*std::cout << "BIAS-X: " << minX << " - " << maxX << std::endl;
            std::cout << "BIAS-Y: " << minY << " - " << maxY << std::endl;
            std::cout << "BIAS-Z: " << minZ << " - " << maxZ << std::endl;
            std::cout << "BIAS: " << x << " " << y << " " << z << std::endl;
            */
            isSet = true;

            return true;
        }
        else
        {
            return false;
        }
    }
};

int main()
{

    // Declare pointcloud object, for calculating pointclouds and texture mappings
    rs2::pointcloud pc;

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;

    // Create a configuration for configuring the pipeline with a non default profile
    rs2::config cfg;

    // Add desired streams to configuration
    cfg.enable_stream(RS2_STREAM_ACCEL, RS2_FORMAT_MOTION_XYZ32F);
    cfg.enable_stream(RS2_STREAM_GYRO, RS2_FORMAT_MOTION_XYZ32F);

    cfg.enable_stream(RS2_STREAM_DEPTH, 640, 480, RS2_FORMAT_Z16, 30);
    //cfg.enable_stream(RS2_STREAM_DEPTH, 848, 480, RS2_FORMAT_Z16, 90);

    //
    // Create the PCL visualizer
    //
    //pcl::visualization::PCLVisualizer viewer("whitestick");

    // Add some features to our viewport
  //  viewer.addCoordinateSystem(1.0, "axis", 0);

    float thetaX = 0.0;
    float thetaY = 0.0;
    float thetaZ = 0.0;

    GyroBias bias;

    //
    // Start capturing
    //
    auto profile = pipe.start(cfg);

    bool firstAccel = true;
    double last_ts[RS2_STREAM_COUNT];
    double dt[RS2_STREAM_COUNT];
    Eigen::Affine3f rot;
    std::mutex mtx;
    bool rot_valid = false;

    window app(1280, 720, "RealSense Motion Example");
    // Construct an object to manage view state
    glfw_state app_state(0.0, 0.0);
    // register callbacks to allow manipulation of the pointcloud
    register_glfw_callbacks(app, app_state);

    auto depth_stream = profile.get_stream(RS2_STREAM_DEPTH);
    auto imu_stream = profile.get_stream(RS2_STREAM_ACCEL);

    auto extrinsics = depth_stream.get_extrinsics_to(imu_stream);

    std::vector<float3> positions;
    std::vector<float3> normals;
    std::vector<int3> indexes;
    uncompress_d435_obj(positions, normals, indexes);

    double sx = 0.;
    for (auto& xyz : positions)
    {
        xyz.x = (xyz.x - extrinsics.translation[0] * 1000.f) / 30.f;
        xyz.y = (xyz.y - extrinsics.translation[1] * 1000.f) / 30.f;
        xyz.z = (xyz.z - extrinsics.translation[2] * 1000.f) / 30.f;
        sx += xyz.x;
    }
    sx /= positions.size();

    // Main loop
    while (app)
    {
        rs2::frameset frames;
        frames = pipe.wait_for_frames();
      /*  if (!pipe.poll_for_frames(&frames))
        {
            // Redraw etc
            viewer.spinOnce();
            continue;
        } */

        //std::cout << "Num frames: " << frames.size() << " ";
        for (auto f : frames)
        {
            rs2::stream_profile profile = f.get_profile();

            unsigned long fnum = f.get_frame_number();
            double ts = f.get_timestamp();
            dt[profile.stream_type()] = (ts - last_ts[profile.stream_type()]) / 1000.0;
            last_ts[profile.stream_type()] = ts;
        }

        auto depth = frames.get_depth_frame();
        auto colored_frame = frames.get_color_frame();

        auto fa = frames.first(RS2_STREAM_ACCEL, RS2_FORMAT_MOTION_XYZ32F);
        rs2::motion_frame accel = fa.as<rs2::motion_frame>();

        auto fg = frames.first(RS2_STREAM_GYRO, RS2_FORMAT_MOTION_XYZ32F);
        rs2::motion_frame gyro = fg.as<rs2::motion_frame>();

        if (gyro)
        {
            rs2_vector gv = gyro.get_motion_data();

            bias.update(gv.x, gv.y, gv.z);

            double gyroX = gv.x - bias.x;
            double gyroY = gv.y - bias.y;
            double gyroZ = gv.z - bias.z;

            gyroX *= dt[RS2_STREAM_GYRO];
            gyroY *= dt[RS2_STREAM_GYRO];
            gyroZ *= dt[RS2_STREAM_GYRO];

            // 
            // Get compensation from GYRO
            // 

            // Around "blue", poisitive => right
            thetaX += gyroZ;

            // Yaw...don't compensate...
            thetaY -= gyroY;

            // Around "red", positve => right
            thetaZ -= gyroX;

        }

        if (accel)
        {
            rs2_vector av = accel.get_motion_data();
            float R = sqrtf(av.x * av.x + av.y * av.y + av.z * av.z);

            float accelX = acos(av.x / R);
            float accelY = acos(av.y / R);
            float accelZ = acos(av.z / R);

            //std::cout << "accelX=" << accelX*180.0/M_PI << " accelY=" << accelY*180.0/M_PI << " accelZ=" << accelZ*180.0/M_PI << std::endl;

            if (firstAccel)
            {
                firstAccel = false;
                thetaX = accelX;
                thetaY = accelY;
                thetaZ = accelZ;
            }
            else
            {
                // Compensate GYRO-drift with ACCEL
                thetaX = thetaX * 0.98 + accelX * 0.02;
                thetaY = thetaY * 0.98 + accelY * 0.02;
                thetaZ = thetaZ * 0.98 + accelZ * 0.02;
            }
        }

        // Transform the cloud according to built in IMU (to get it straight)
        Eigen::Affine3f rx = Eigen::Affine3f(Eigen::AngleAxisf(-(thetaZ - M_PI/2), Eigen::Vector3f(1, 0, 0)));
        Eigen::Affine3f ry = Eigen::Affine3f(Eigen::AngleAxisf(0.0, Eigen::Vector3f(0, 1, 0)));
        Eigen::Affine3f rz = Eigen::Affine3f(Eigen::AngleAxisf(thetaX - M_PI/2, Eigen::Vector3f(0, 0, 1)));
        rot = rz * ry * rx;

        // Set the current clear color to black and the current drawing color to
        // white.
        glClearColor(0.0, 0.0, 0.0, 1.0);
        glColor3f(1.0, 1.0, 1.0);

        // Set the camera lens to have a 60 degree (vertical) field of view, an
        // aspect ratio of 4/3, and have everything closer than 1 unit to the
        // camera and greater than 40 units distant clipped away.
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(60.0, 4.0 / 3.0, 1, 40);

        //  std::lock_guard<std::mutex> lock(mutex);

        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_MODELVIEW);

        glLoadIdentity();
        gluLookAt(sx, 0, 5, sx, 0, 0, 0, -1, 0);
        //gluLookAt(0, 0, 0, 0, 0, 1, 0, -1, 0);

        glTranslatef(0, 0, +0.5f + app_state.offset_y*0.05f);
        glRotated(app_state.pitch, -1, 0, 0);
        glRotated(app_state.yaw, 0, 1, 0);

        glColor3f(1.0, 1.0, 1.0);

        glLineWidth(1);
        glBegin(GL_LINES);
        glColor4f(0.4f, 0.4f, 0.4f, 1.f);

        // Render "floor" grid
        for (int i = 0; i <= 6; i++)
        {
            glVertex3i(i - 3, 1, 0);
            glVertex3i(i - 3, 1, 6);
            glVertex3i(-3, 1, i);
            glVertex3i(3, 1, i);
        }
        glEnd();

        glBegin(GL_LINES);

        // std::lock_guard<std::mutex> lock(mtx);
        glColor3f(0.5, 0, 0); glVertex3f(0, 0, 0); glVertex3f(rot(0, 0), rot(1, 0), rot(2, 0));
        glColor3f(0, 0.5, 0); glVertex3f(0, 0, 0); glVertex3f(rot(0, 1), rot(1, 1), rot(2, 1));
        glColor3f(0, 0, 0.5); glVertex3f(0, 0, 0); glVertex3f(rot(0, 2), rot(1, 2), rot(2, 2));
        glEnd();
        //glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE);

        float curr[4][4];
        float mult[4][4];
        float mat[4][4] = {
            { rot(0,0),rot(1,0),rot(2,0),0.f },
            { rot(0,1),rot(1,1),rot(2,1),0.f },
            { rot(0,2),rot(1,2),rot(2,2),0.f },
            { 0.f,0.f,0.f,1.f }
        };

        glGetFloatv(GL_MODELVIEW_MATRIX, (float*)curr);

        for (int i = 0; i < 4; i++)
            for (int j = 0; j < 4; j++)
            {
                auto sum = 0.f;
                for (int k = 0; k < 4; k++)
                {
                    sum += mat[i][k] * curr[k][j];
                }
                mult[i][j] = sum;
            }

        glLoadMatrixf((float*)mult);

        glBegin(GL_TRIANGLES);
        for (auto& i : indexes)
        {
            auto v0 = positions[i.x];
            auto v1 = positions[i.y];
            auto v2 = positions[i.z];
            glVertex3fv(&v0.x);
            glVertex3fv(&v1.x);
            glVertex3fv(&v2.x);
            glColor4f(0.05f, 0.05f, 0.05f, 0.3f);
        }
        glEnd();

        glDisable(GL_BLEND);
        //glEnable(GL_DEPTH_TEST);


        glFlush();
    }

    return 0;
}
