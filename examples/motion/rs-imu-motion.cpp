
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

    window app(1280, 720, "RealSense Pointcloud Example");
    // Construct an object to manage view state
    glfw_state app_state;
    // register callbacks to allow manipulation of the pointcloud
    register_glfw_callbacks(app, app_state);

    auto depth_stream = profile.get_stream(RS2_STREAM_DEPTH);
    auto imu_stream = profile.get_stream(RS2_STREAM_ACCEL);

    auto extrinsics = depth_stream.get_extrinsics_to(imu_stream);

    std::vector<float3> positions;
    std::vector<float3> normals;
    std::vector<int3> indexes;
    uncompress_d435_obj(positions, normals, indexes);
    for (auto& xyz : positions)
    {
        xyz.x = (xyz.x - extrinsics.translation[0] * 1000.f) / 30.f;
        xyz.y = (xyz.y - extrinsics.translation[1] * 1000.f) / 30.f;
        xyz.z = (xyz.z - extrinsics.translation[2] * 1000.f) / 30.f;
    }

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

            //std::cout << "dt=" << dt[RS2_STREAM_GYRO] << " gyroX=" << gyroX * 180.0 / M_PI << " gyroY=" << gyroY * 180.0 / M_PI << " gyroZ=" << gyroZ * 180.0 / M_PI << std::endl;
            //std::cout << " gyroX=" << gyroX * 180.0 / M_PI << " gyroY=" << gyroY * 180.0 / M_PI << " gyroZ=" << gyroZ * 180.0 / M_PI << std::endl;
            //std::cout << last_ts[RS2_STREAM_GYRO] / 1000.0 << " " << gyroZ * 180.0 / M_PI << std::endl;

            // 
            // Get compensation from GYRO
            // 

            // Around "blue", poisitive => right
            thetaX += gyroZ;

            // Yaw...don't compensate...
            thetaY += 0.0;

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

            //std::cout << "thetaX=" << thetaX*180.0/M_PI << " thetaY=" << thetaY*180.0/M_PI << " thetaZ=" << thetaZ*180.0/M_PI << std::endl;
            //std::cout << last_ts[RS2_STREAM_GYRO] / 1000.0 << " " << thetaX*180.0/M_PI << " " << thetaY*180.0/M_PI << " " << thetaZ*180.0/M_PI << std::endl;
            //std::cout << last_ts[RS2_STREAM_GYRO] / 1000.0 << " " << thetaX*180.0/M_PI << std::endl;

        }

       // auto points = pc.calculate(depth);

        // Actual calling of conversion and saving XYZRGB cloud to file
      //  cloud = points_to_pcl_no_texture(points);

        // Downsize to a voxel grid (i.e small boxes of the cloud)
   //     WSPointCloudPtr voxel_cloud(new WSPointCloud());
    //    pcl::VoxelGrid<WSPoint> sor;
  //      sor.setInputCloud(cloud);
   //     sor.setLeafSize(0.07f, 0.07f, 0.07f);
   //     sor.filter(*voxel_cloud);

        // Transform the cloud according to built in IMU (to get it straight)
        Eigen::Affine3f rx = Eigen::Affine3f(Eigen::AngleAxisf(-(thetaZ - M_PI/2), Eigen::Vector3f(1, 0, 0)));
        Eigen::Affine3f ry = Eigen::Affine3f(Eigen::AngleAxisf(0.0, Eigen::Vector3f(0, 1, 0)));
        Eigen::Affine3f rz = Eigen::Affine3f(Eigen::AngleAxisf(thetaX - M_PI/2, Eigen::Vector3f(0, 0, 1)));
        rot = rz * ry * rx;
       // std::lock_guard<std::mutex> lock(mtx);
       // rot_valid = true;

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
        gluLookAt(0, 2, 5, 0, 0, 0, 0, 1, 0);
        glTranslatef(0, 0, 1 + app_state.offset_y*0.05f);
        glRotated(app_state.pitch - 20, 1, 0, 0);
        glRotated(app_state.yaw + 5, 0, 1, 0);

        // auto e = q.toRotationMatrix().eulerAngles(0, 1, 2);

        /*  glRotatef(e[0] * 180 / M_PI, 1, 0, 0);
        glRotatef(e[1] * 180 / M_PI, 0, 1, 0);
        glRotatef(e[2] * 180 / M_PI, 0, 0, 1); */

        glColor3f(1.0, 1.0, 1.0);
        //glutWireTorus(0.5, 3, 15, 30);

        // Draw a red x-axis, a green y-axis, and a blue z-axis.  Each of the
        // axes are ten units long.
        /*  glBegin(GL_LINES);
        glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(10, 0, 0);
        glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, 10, 0);
        glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, 10);
        glEnd(); */

        glBegin(GL_LINES);

        // std::lock_guard<std::mutex> lock(mtx);
        glColor3f(0.5, 0, 0); glVertex3f(0, 0, 0); glVertex3f(rot(0, 0), rot(1, 0), rot(2, 0));
        glColor3f(0, 0.5, 0); glVertex3f(0, 0, 0); glVertex3f(rot(0, 2), rot(1, 2), rot(2, 2));
        glColor3f(0, 0, 0.5); glVertex3f(0, 0, 0); glVertex3f(rot(0, 1), rot(1, 1), rot(2, 1));
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




        // Executing the transformation
      //  pcl::PointCloud<WSPoint>::Ptr transformed_cloud(new WSPointCloud());
      //  pcl::transformPointCloud(*voxel_cloud, *transformed_cloud, rot);

        // Show camera as coordinate system
       // viewer.removeCoordinateSystem("camera");
       // viewer.addCoordinateSystem(0.5, rot, "camera", 0);
/*
        if (!viewer.updatePointCloud(transformed_cloud, "cloud"))
        {
            // Add to the viewer
            viewer.addPointCloud<WSPoint>(transformed_cloud, "cloud");
            viewer.setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 5, "cloud");
        }
*/
        // Redraw etc
      //  viewer.spinOnce();
    }

    return 0;
}


/*
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <iostream>
#include <fstream>
#include <map>
#include <chrono>
#include <mutex>
#include <thread>
#include <../third-party/eigen/Eigen/Geometry>
#include "example.hpp"          // Include short list of convenience functions for rendering
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

std::mutex mutex;

rs2_vector accel;
rs2_vector gyro;
Eigen::Quaternionf q(1.0, 0.0, 0.0, 0.0);

Eigen::Vector3f gravity(0, -9.81, 0);

Eigen::Vector3f world_accel(0, 0, 0);
Eigen::Vector3f accel_raw(0, 0, 0);
Eigen::Vector3f accel_rot(0, 0, 0);
Eigen::Vector3f accel_invrot(0, 0, 0);

// Clears the window and draws the torus.
void display() {

    std::lock_guard<std::mutex> lock(mutex);

    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);

    glLoadIdentity();
    gluLookAt(4, 6, 5, 0, 0, 0, 0, 1, 0);

    auto e = q.toRotationMatrix().eulerAngles(0, 1, 2);

    glRotatef(e[0] * 180 / M_PI, 1, 0, 0);
    glRotatef(e[1] * 180 / M_PI, 0, 1, 0);
    glRotatef(e[2] * 180 / M_PI, 0, 0, 1);

    glColor3f(1.0, 1.0, 1.0);
    //glutWireTorus(0.5, 3, 15, 30);

    // Draw a red x-axis, a green y-axis, and a blue z-axis.  Each of the
    // axes are ten units long.
    glBegin(GL_LINES);
    glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(10, 0, 0);
    glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, 10, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, 10);
    glEnd();

    glBegin(GL_LINES);
    //glColor3f(0.5, 0, 0); glVertex3f(0,0,0); glVertex3f(accel_raw[0], accel_raw[1], accel_raw[2]);
    //glColor3f(0, 0.5, 0); glVertex3f(0,0,0); glVertex3f(accel_rot[0], accel_rot[1], accel_rot[2]);
    glColor3f(0, 0, 0.5); glVertex3f(0, 0, 0); glVertex3f(world_accel[0], world_accel[1], world_accel[2]);
    glEnd();

    glFlush();
    // glutSwapBuffers();
}

// Sets up global attributes like clear color and drawing color, and sets up
// the desired projection and modelview matrices.
void init() {
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

    // Position camera at (4, 6, 5) looking at (0, 0, 0) with the vector
    // <0, 1, 0> pointing upward.
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(4, 6, 5, 0, 0, 0, 0, 1, 0);
}
/*
void timer(int v) {
glutPostRedisplay();
glutTimerFunc(1000 / 60, timer, v);
}

void display_thread(window& app) {
glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
glutInitWindowPosition(80, 80);
glutInitWindowSize(800, 600);
glutCreateWindow("Orientation");
glutDisplayFunc(display);
init();
glutTimerFunc(100, timer, 0);

glutMainLoop();
}


int main(int argc, char * argv[]) try
{
    //glutInit(&argc, argv);

    // Create a simple OpenGL window for rendering:
    

    std::thread display = std::thread([&]() {
        window app(1280, 720, "RealSense Pointcloud Example");
        // Construct an object to manage view state
        glfw_state app_state;
        // register callbacks to allow manipulation of the pointcloud
        register_glfw_callbacks(app, app_state);
        while (app)
        {
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

            // Position camera at (4, 6, 5) looking at (0, 0, 0) with the vector
            // <0, 1, 0> pointing upward.
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            gluLookAt(4, 6, 5, 0, 0, 0, 0, 1, 0);


            std::lock_guard<std::mutex> lock(mutex);

            glClear(GL_COLOR_BUFFER_BIT);
            glMatrixMode(GL_MODELVIEW);

            glLoadIdentity();
            gluLookAt(4, 6, 5, 0, 0, 0, 0, 1, 0);

            auto e = q.toRotationMatrix().eulerAngles(0, 1, 2);

            glRotatef(e[0] * 180 / M_PI, 1, 0, 0);
            glRotatef(e[1] * 180 / M_PI, 0, 1, 0);
            glRotatef(e[2] * 180 / M_PI, 0, 0, 1);

            glColor3f(1.0, 1.0, 1.0);
            //glutWireTorus(0.5, 3, 15, 30);

            // Draw a red x-axis, a green y-axis, and a blue z-axis.  Each of the
            // axes are ten units long.
            glBegin(GL_LINES);
            glColor3f(1, 0, 0); glVertex3f(0, 0, 0); glVertex3f(10, 0, 0);
            glColor3f(0, 1, 0); glVertex3f(0, 0, 0); glVertex3f(0, 10, 0);
            glColor3f(0, 0, 1); glVertex3f(0, 0, 0); glVertex3f(0, 0, 10);
            glEnd();

            glBegin(GL_LINES);
            //glColor3f(0.5, 0, 0); glVertex3f(0,0,0); glVertex3f(accel_raw[0], accel_raw[1], accel_raw[2]);
            //glColor3f(0, 0.5, 0); glVertex3f(0,0,0); glVertex3f(accel_rot[0], accel_rot[1], accel_rot[2]);
            glColor3f(0, 0, 0.5); glVertex3f(0, 0, 0); glVertex3f(world_accel[0], world_accel[1], world_accel[2]);
            glEnd();

            glFlush();
            // glutSwapBuffers();
        }
    });

    rs2::pipeline pipe;
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_ACCEL);
    cfg.enable_stream(RS2_STREAM_GYRO);
    rs2::pipeline_profile profile = pipe.start(cfg);
    rs2::device dev = profile.get_device();

    auto sensors = dev.query_sensors();
    rs2::sensor motion_sensor;
    std::string mname("Motion Module");
    for (auto const& s : sensors) {
        auto a = s.get_info(RS2_CAMERA_INFO_NAME);
        std::string name(a);
        if (mname.compare(name) == 0) {
            std::cout << a << std::endl;
            motion_sensor = s;
        }

        if (s.supports(RS2_OPTION_ASIC_TEMPERATURE)) {
            auto f = s.get_option(RS2_OPTION_ASIC_TEMPERATURE);
            std::cout << "RS2_OPTION_ASIC_TEMPERATURE: " << f << std::endl;
        }
        if (s.supports(RS2_OPTION_PROJECTOR_TEMPERATURE)) {
            auto f = s.get_option(RS2_OPTION_PROJECTOR_TEMPERATURE);
            std::cout << "RS2_OPTION_PROJECTOR_TEMPERATURE: " << f << std::endl;
        }
        if (s.supports(RS2_OPTION_MOTION_MODULE_TEMPERATURE)) {
            auto f = s.get_option(RS2_OPTION_MOTION_MODULE_TEMPERATURE);
            std::cout << "RS2_OPTION_MOTION_MODULE_TEMPERATURE: " << f << std::endl;
        }
    }

    auto accel_stream = profile.get_stream(RS2_STREAM_ACCEL).as<rs2::motion_stream_profile>();
    auto gyro_stream = profile.get_stream(RS2_STREAM_GYRO).as<rs2::motion_stream_profile>();

    std::cout << "Accel FPS: " << accel_stream.fps() << std::endl;
    std::cout << "Gyro FPS: " << gyro_stream.fps() << std::endl;

    //auto accel_intr = accel_stream.get_motion_intrinsics();
    //auto gyro_intr = gyro_stream.get_motion_intrinsics();
    //std::cout << accel_intr.data << std::endl;
    //std::cout << accel_intr.noise_variances << std::endl;
    //std::cout << accel_intr.bias_variances << std::endl;

    //std::ofstream dump;
    //dump.open ("readings.csv");

    while (true) {
        auto frameset = pipe.wait_for_frames();

        mutex.lock();

        auto aframe = frameset.first(RS2_STREAM_ACCEL).as<rs2::motion_frame>();
        auto gframe = frameset.first(RS2_STREAM_GYRO).as<rs2::motion_frame>();

        accel = aframe.get_motion_data();
        gyro = gframe.get_motion_data();

        MadgwickAHRSupdateIMU(gyro.x, gyro.y, gyro.z, accel.x, accel.y, accel.z);

        //dump << accel.x << "," << accel.y << "," << accel.z  << "," << 
        //    gyro.x << "," << gyro.y << "," << gyro.z  << ","  << 
        //    q0 << "," << q1 << "," << q2  << "," << q3  << std::endl;

        float q0_ = q0;
        float q1_ = q1;
        float q2_ = q2;
        float q3_ = q3;
        q = Eigen::Quaternionf(q0_, q1_, q2_, q3_);

        auto invrot = q.normalized().inverse().toRotationMatrix();
        auto rot = q.normalized().toRotationMatrix();

        accel_raw = Eigen::Vector3f(accel.x, accel.y, accel.z);
        accel_rot = rot * accel_raw;
        world_accel = accel_rot - gravity;

//        auto f = motion_sensor.get_option(RS2_OPTION_ASIC_TEMPERATURE);
   //     std::cout << "RS2_OPTION_ASIC_TEMPERATURE: " << f << std::endl;

        mutex.unlock();
    }

    //dump.close();
    display.join();
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
*/
