
#include <opencv2/imgproc.hpp>
#include <opencv2/rgbd/kinfu.hpp>

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <example.hpp>         // Include short list of convenience functions for rendering

#include <chrono>
#include <thread>
#include <queue>
#include <atomic>
#include <fstream>

using namespace cv;
using namespace cv::kinfu;

static float max_dist = 2;
static float min_dist = 0;


void colorize_pointcloud(const Mat points, Mat& color)
{
    // Define a vector of 3 Mat arrays which will hold the channles of points
    std::vector<Mat> channels(points.channels());
    split(points, channels);
    // Get the depth channel which we'll use to colorize the pointcloud
    color = channels[2];

    // Convert the depth matrix to unsigned char values
    float min = min_dist;
    float max = max_dist+0.5;
    color.convertTo(color, CV_8UC1, 255 / (max - min), -255 * min / (max - min));
    // Get an rgb value for each point
    applyColorMap(color, color, COLORMAP_JET);

}

// Handles all the OpenGL calls needed to display the point cloud
void draw_kinfu_pointcloud(float width, float height, glfw_state& app_state, Mat points)
{
    // Define new matrix which will later hold the coloring of the pointcloud
    Mat color;
    colorize_pointcloud(points, color);
   /* std::vector<Mat> channels(points.channels());
    split(points, channels);
    // Get the depth channel which we'll use to colorize the pointcloud
    color = channels[2];

    // Convert the depth matrix to unsigned char values
    float min = min_dist;
    float max = max_dist;
    color.convertTo(color, CV_8UC1, 255 / (max - min), -255 * min / (max - min));
    // Get an rgb value for each point
    applyColorMap(color, color, COLORMAP_JET); */

    // OpenGL commands that prep screen for the pointcloud
    glPopMatrix();
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glClearColor(153.f / 255, 153.f / 255, 153.f / 255, 1);
    glClear(GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    gluPerspective(65, width / height, 0.01f, 10.0f);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    gluLookAt(0, 0, 0, 0, 0, 1, 0, -1, 0);

    glTranslatef(0, 0, +0.5f + app_state.offset_y*0.05f);
    glRotated(app_state.pitch-10, 1, 0, 0);
    glRotated(app_state.yaw, 0, 1, 0);
    glTranslatef(0, 0, -0.5f);

    glPointSize(width / 640);
    glEnable(GL_DEPTH_TEST);

    glBegin(GL_POINTS);
    // this segment actually prints the pointcloud
    for (int i = 0; i < points.rows; i++)
    {
        // Get point coordinates from 'points' matrix
        float x = points.at<float>(i, 0);
        float y = points.at<float>(i, 1);
        float z = points.at<float>(i, 2);

        // Get the r, g, b values for the current point
        uchar r = color.at<uchar>(i, 0);
        uchar g = color.at<uchar>(i, 1);
        uchar b = color.at<uchar>(i, 2);

        // Register color and coordinates of the current point
        glColor3ub(r, g, b);
        glVertex3f(x, y, z);

    }
   // points.release();
  //  color.release();
    // OpenGL cleanup
    glEnd();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
    glPushMatrix();
}


void export_to_ply(const std::string& fname, Mat points)
{
    Mat color;
    colorize_pointcloud(points, color);

    std::ofstream out(fname);
    out << "ply\n";
    out << "format binary_little_endian 1.0\n";
    out << "comment pointcloud saved from Realsense Viewer\n";
    out << "element vertex " << points.rows << "\n";
    out << "property float" << sizeof(float) * 8 << " x\n";
    out << "property float" << sizeof(float) * 8 << " y\n";
    out << "property float" << sizeof(float) * 8 << " z\n";

    out << "property uchar red\n";
    out << "property uchar green\n";
    out << "property uchar blue\n";
    out << "end_header\n";
    out.close();

    out.open(fname, std::ios_base::app | std::ios_base::binary);
    for (int i = 0; i < points.rows; i++)
    {
        // write vertices
        out.write(reinterpret_cast<const char*>(&(points.at<float>(i, 0))), sizeof(float));
        out.write(reinterpret_cast<const char*>(&(points.at<float>(i, 1))), sizeof(float));
        out.write(reinterpret_cast<const char*>(&(points.at<float>(i, 2))), sizeof(float));

        // wrtie colors
        out.write(reinterpret_cast<const char*>(&(color.at<uchar>(i, 0))), sizeof(uint8_t));
        out.write(reinterpret_cast<const char*>(&(color.at<uchar>(i, 1))), sizeof(uint8_t));
        out.write(reinterpret_cast<const char*>(&(color.at<uchar>(i, 2))), sizeof(uint8_t));
    }
}


// Thread-safe queue for OpenCV's Mat objects
class mat_queue
{
public:
    void push(Mat& item)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        queue.push(item);
    }
    int try_get_next_item(Mat& item)
    {
        std::lock_guard<std::mutex> lock(_mtx);
        if (queue.empty())
            return false;
        item = std::move(queue.front());
        queue.pop();
        return true;
    }
private:
    std::queue<Mat> queue;
    std::mutex _mtx;
};


// known issue: when camera recieves 'black' frames (lenses are covered), algorithm restarts multiple times but eventually manages to 'update',
// even though the pointcloud matrix is of dims: cols = 0, rows = 2, which is illegal and causes an exception from 'getPoints'.
// TODO: check if there's a minimum available depth for cameras
int main(int argc, char **argv)
{
    // Declare objects for kinfu and kinfu's parameters
    Ptr<KinFu> kf;
    Ptr<Params> params = Params::defaultParams();

    // Enables OpenCL explicitly TODO: check if this is needed
    cv::setUseOptimized(true);

    // Create a pipeline and configure it
    rs2::pipeline p;
    rs2::config cfg;
    float depth_scale;
    cfg.enable_stream(RS2_STREAM_DEPTH, 1280, 720, RS2_FORMAT_Z16);
    auto profile = p.start(cfg);
    auto dev = profile.get_device();
    auto stream_depth = profile.get_stream(RS2_STREAM_DEPTH);

    // Get a new frame from the camera
    rs2::frameset data = p.wait_for_frames();
    auto d = data.get_depth_frame();
    
    for (rs2::sensor& sensor : dev.query_sensors())
    {
        if (rs2::depth_sensor dpt = sensor.as<rs2::depth_sensor>())
        {
            // Set some presets for better results
            dpt.set_option(RS2_OPTION_VISUAL_PRESET, RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY);
            // Depth scale is needed for the kinfu set-up
            depth_scale = dpt.get_depth_scale();
            break;
        }
    }

    // Declare post-processing filters for better results
    auto decimation = rs2::decimation_filter();
    auto spatial = rs2::spatial_filter();
    auto temporal = rs2::temporal_filter();

    auto intrin = stream_depth.as<rs2::video_stream_profile>().get_intrinsics();

    auto clipping_dist = max_dist;

    // Use decimation once to get the final size of the frame
    d = decimation.process(d);
    auto w = d.get_width();
    auto h = d.get_height();

    window app(1280, 720, "RealSense KinectFusion Example");
    glfw_state app_state;
    register_glfw_callbacks(app, app_state);

    Mat f(h, w, CV_16UC1, (void*)d.get_data());

    auto size = f.size();

    // Configure kinfu's parameters
    params->frameSize = size;
    params->intr = Matx33f(intrin.fx, 0, intrin.ppx,
                           0, intrin.fy, intrin.ppy,
                           0, 0, 1);
    params->depthFactor = 1 / depth_scale;

    kf = KinFu::create(params);

    bool after_reset = false;
    mat_queue points_queue;

    std::atomic_bool stopped(false);

    // This thread actually runs KinFu algorithm and calculate the resulting pointcloud
    std::thread calc_cloud_thread([&]() {
        while (!stopped)
        {
            rs2::frameset data = p.wait_for_frames(); // Wait for next set of frames from the camera

            auto d = data.get_depth_frame();
            // Use post processing to improve results
            d = decimation.process(d);
            d = spatial.process(d);
            d = temporal.process(d);

            // Set depth values higher than clipping_dist to 0, to avoid unnecessary noise in the pointcloud
    #pragma omp parallel for schedule(dynamic) //Using OpenMP to try to parallelise the loop
            uint16_t* p_depth_frame = reinterpret_cast<uint16_t*>(const_cast<void*>(d.get_data()));
            for (int y = 0; y < h; y++)
            {
                auto depth_pixel_index = y * w;
                for (int x = 0; x < w; x++, ++depth_pixel_index)
                {
                    // Get the depth value of the current pixel
                    auto pixels_distance = depth_scale * p_depth_frame[depth_pixel_index];

                    // Check if the depth value is invalid (<=0) or greater than the threshold
                    if (pixels_distance <= 0.f || pixels_distance > clipping_dist)
                    {
                        p_depth_frame[depth_pixel_index] = 0;
                    }
                }
            }

            // Define matrix on the GPU for KinFu's use
            UMat points;
            // Copy frame from CPU to GPU
            Mat f(h, w, CV_16UC1, (void*)d.get_data());
            UMat frame(h, w, CV_16UC1);
            f.copyTo(frame);
            f.release();

            //  auto start = std::chrono::high_resolution_clock::now();
            // Run KinFu on the new frame(on GPU)
            if (!kf->update(frame))
            {
                kf->reset(); // If the algorithm failed, reset current state
                after_reset = true;
                points.release();
                std::cout << "reset" << std::endl;
            }

            // Get current pointcloud
            if (!after_reset)
            {
                kf->getPoints(points);
            }

            if (!points.empty())
            {
                // copy points from GPU to CPU for rendering
                Mat _points(h, w, CV_16UC1);
                points.copyTo(_points);
                points.release();
                // Push to queue for rendering
                points_queue.push(_points);
            }

            after_reset = false;
        }
    });
    
    // Main thread handles rendering of the pointcloud
    Mat points;
    while (app)
    {
        // Get the current state of the pointcloud
        points_queue.try_get_next_item(points);
        if (!points.empty()) // points might not be ready on first iterations
            draw_kinfu_pointcloud(w, h, app_state, points);
        //points.release();
    }
    stopped = true;
    calc_cloud_thread.join();
    std::cout << "exporting to '1.ply'";
    export_to_ply("1.ply", points);
    return 0;
}

