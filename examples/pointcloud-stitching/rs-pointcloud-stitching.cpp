// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

//#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <rs-pointcloud-stitching.h>
#include <librealsense2/hpp/rs_internal.hpp>
#include <librealsense2/rsutil.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>
#include <int-rs-splash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <fstream>
#include <regex>
#include <thread>
#include "example-imgui.hpp"    // Include short list of convenience functions for rendering

const int W = 640;
const int H = 480;
const int BPP = 2;

using namespace std;
using namespace rs_pointcloud_stitching;

class custom_frame_source
{
public:
    custom_frame_source()
    {
        depth_frame.x = W;
        depth_frame.y = H;
        depth_frame.bpp = BPP;

        last = std::chrono::high_resolution_clock::now();

        std::vector<uint8_t> pixels_depth(depth_frame.x * depth_frame.y * depth_frame.bpp, 0);
        depth_frame.frame = std::move(pixels_depth);

        auto realsense_logo = stbi_load_from_memory(splash, (int)splash_size, &color_frame.x, &color_frame.y, &color_frame.bpp, false);

        std::vector<uint8_t> pixels_color(color_frame.x * color_frame.y * color_frame.bpp, 0);

        memcpy(pixels_color.data(), realsense_logo, color_frame.x * color_frame.y * 4);

        for (auto i = 0; i < color_frame.y; i++)
            for (auto j = 0; j < color_frame.x * 4; j += 4)
            {
                if (pixels_color.data()[i * color_frame.x * 4 + j] == 0)
                {
                    pixels_color.data()[i * color_frame.x * 4 + j] = 22;
                    pixels_color.data()[i * color_frame.x * 4 + j + 1] = 115;
                    pixels_color.data()[i * color_frame.x * 4 + j + 2] = 185;
                }
            }
        color_frame.frame = std::move(pixels_color);
    }

    synthetic_frame& get_synthetic_texture()
    {
        return color_frame;
    }

    synthetic_frame& get_synthetic_depth(glfw_state& app_state)
    {
        draw_text(50, 50, "This point-cloud is generated from a synthetic device:");

        auto now = std::chrono::high_resolution_clock::now();
        if (now - last > std::chrono::milliseconds(1))
        {
            app_state.yaw -= 1;
            wave_base += 0.1f;
            last = now;

            for (int i = 0; i < depth_frame.y; i++)
            {
                for (int j = 0; j < depth_frame.x; j++)
                {
                    auto d = 2 + 0.1 * (1 + sin(wave_base + j / 50.f));
                    ((uint16_t*)depth_frame.frame.data())[i * depth_frame.x + j] = (int)(d * 0xff);
                }
            }
        }
        return depth_frame;
    }

    rs2_intrinsics create_texture_intrinsics()
    {
        rs2_intrinsics intrinsics = { color_frame.x, color_frame.y,
            (float)color_frame.x / 2, (float)color_frame.y / 2,
            (float)color_frame.x / 2, (float)color_frame.y / 2,
            RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };

        return intrinsics;
    }

    rs2_intrinsics create_depth_intrinsics()
    {
        rs2_intrinsics intrinsics = { depth_frame.x, depth_frame.y,
            (float)depth_frame.x / 2, (float)depth_frame.y / 2,
            (float)depth_frame.x , (float)depth_frame.y ,
            RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };

        return intrinsics;
    }

    rs2_intrinsics create_depth_intrinsics(const float fov_x, const float fov_y)    // fov_x, fov_y in radians.
    {
        float fx = depth_frame.x / (2 * tanf(fov_x / 2));
        float fy = depth_frame.y / (2 * tanf(fov_y / 2));
        rs2_intrinsics intrinsics = { depth_frame.x, depth_frame.y,
            (float)depth_frame.x / 2, (float)depth_frame.y / 2,
            fx , fy ,
            RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };

        return intrinsics;
    }

private:
    synthetic_frame depth_frame;
    synthetic_frame color_frame;

    std::chrono::high_resolution_clock::time_point last;
    float wave_base = 0.f;
};

glfw_state g_app_state;
custom_frame_source g_app_data;

bool parse_configuration(const std::string& line, const std::vector<std::string>& tokens,
    rs2_stream& type, int& width, int& height, rs2_format& format, int& fps, int& index)
{
    bool res = false;
    try
    {
        auto tokens = tokenize(line, ',');
        if (tokens.size() < e_stream_index)
            return res;

        // Convert string to uppercase
        type = parse_stream_type(to_lower(tokens[e_stream_type]));
        width = parse_number(tokens[e_res_width].c_str());
        height = parse_number(tokens[e_res_height].c_str());
        fps = parse_fps(tokens[e_fps]);
        format = parse_format(to_lower(tokens[e_format]));
        // Backward compatibility
        if (tokens.size() > e_stream_index)
            index = parse_number(tokens[e_stream_index].c_str());
        res = true;
        std::cout << "Request added for " << line << std::endl;
    }
    catch (...)
    {
        std::cout << "Invalid syntax in configuration line " << line << std::endl;
    }

    return res;
}

std::vector<stream_request> parse_profiles_file(const std::string& config_filename)
{
    std::vector<stream_request> user_requests;

    ifstream file(config_filename);

    if (!file.is_open())
        throw runtime_error(stringify() << "Given .cfg configure file " << config_filename << " was not found!");

    // Line starting with non-alpha characters will be treated as comments
    const static std::regex starts_with("^[a-zA-Z]");
    string line;
    rs2_stream stream_type;
    rs2_format format;
    int width{}, height{}, fps{}, index{};

    // Parse the config file
    while (getline(file, line))
    {
        auto tokens = tokenize(line, ',');

        if (std::regex_search(line, starts_with))
        {
            if (parse_configuration(line, tokens, stream_type, width, height, format, fps, index))
                user_requests.push_back({ stream_type, format, width, height, fps,  index });
        }
    }

    // Sanity test agains multiple conflicting requests for the same sensor
    if (user_requests.size())
    {
        std::sort(user_requests.begin(), user_requests.end(),
            [](const stream_request& l, const stream_request& r) { return l._stream_type < r._stream_type; });

        for (auto i = 0; i < user_requests.size() - 1; i++)
        {
            if ((user_requests[i]._stream_type == user_requests[i + 1]._stream_type) && ((user_requests[i]._stream_idx == user_requests[i + 1]._stream_idx)))
                throw runtime_error(stringify() << "Invalid configuration file - multiple requests for the same sensor:\n"
                    << user_requests[i] << user_requests[+i]);
        }
    }
    else
        throw std::runtime_error(stringify() << "Invalid configuration file - " << config_filename << " zero requests accepted");

    return user_requests;
}

CPointcloudStitcher::CPointcloudStitcher(const std::string& working_dir) :
    _working_dir(working_dir),
    _frame_number(0)
{
    // init frames map
    //for initilize only - an empty frame with its properties
    rs2::frame frame;

    //set each frame with its properties:
    //  { tile's x coordinate, tiles's y coordinate, tile's width (in tiles), tile's height (in tiles), priority (default value=0) }, (x=0,y=0) <-> left bottom corner
    //priority sets the order of drawing frame when two frames share part of the same tile, 
    //meaning if there are two frames: frame1 with priority=-1 and frame2 with priority=0, both with { 0,0,1,1 } as property,
    //frame2 will be drawn on top of frame1
    _frames_map[COLOR1] = frame_and_tile_property(frame, { 0,0,1,1,Priority::high });
    _frames_map[COLOR_UNITED] = frame_and_tile_property(frame, { 1,0,1,1,Priority::high });
    _frames_map[COLOR2] = frame_and_tile_property(frame, { 2,0,1,1,Priority::high });
    _frames_map[DEPTH1] = frame_and_tile_property(frame, { 0,1,1,1,Priority::high });
    _frames_map[DEPTH_UNITED] = frame_and_tile_property(frame, { 1,1,1,1,Priority::high });
    _frames_map[DEPTH2] = frame_and_tile_property(frame, { 2,1,1,1,Priority::high });
}

bool CPointcloudStitcher::OpenSensors(std::shared_ptr<rs2::device> dev)
{
    size_t profiles_found(0);
    std::string serial(dev->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
    std::vector<stream_request> dev_wanted_profiles = _wanted_profiles[serial];
    std::vector<stream_request> requests_to_go = dev_wanted_profiles;
    std::vector<rs2::stream_profile> matches;

    // Configure and starts streaming
    for (auto&& sensor : dev->query_sensors())
    {
        for (auto& profile : sensor.get_stream_profiles())
        {
            // All requests have been resolved
            if (!requests_to_go.size())
                break;

            // Find profile matches
            auto fulfilled_request = std::find_if(requests_to_go.begin(), requests_to_go.end(), [&matches, profile](const stream_request& req)
                {
                    bool res = false;
                    if ((profile.stream_type() == req._stream_type) &&
                        (profile.format() == req._stream_format) &&
                        (profile.stream_index() == req._stream_idx) &&
                        (profile.fps() == req._fps))
                    {
                        if (auto vp = profile.as<rs2::video_stream_profile>())
                        {
                            if ((vp.width() != req._width) || (vp.height() != req._height))
                                return false;
                        }
                        res = true;
                        matches.push_back(profile);
                    }

                    return res;
                });

            // Remove the request once resolved
            if (fulfilled_request != requests_to_go.end())
                requests_to_go.erase(fulfilled_request);
        }

        // Aggregate resolved requests
        if (matches.size())
        {
            profiles_found += matches.size();
            sensor.open(matches);
            _active_sensors[serial].emplace_back(sensor);
            matches.clear();
        }
    }
    if (profiles_found == dev_wanted_profiles.size())
    {
        return true;
    }
    else
    {
        std::cerr << "ERROR: Managed to validate only %d/%d profiles for dev: " << serial << std::endl;
        return false;
    }
}

bool CPointcloudStitcher::Init()
{
    rs2::context ctx;
    rs2::device_list list;

    list = ctx.query_devices();

    if (list.size() != 2)
    {
        std::cout << "Connect 2 Realsense Cameras to proceed" << std::endl;
        return false;
    }

    for (auto dev_info : list)
    {
        _devices.push_back(std::make_shared<rs2::device>(dev_info));
    }

    std::vector<std::string> serials;
    for (auto dev : _devices)
    {
        std::string dev_name = dev->get_info(RS2_CAMERA_INFO_NAME);
        std::string dev_serial = dev->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
        serials.push_back(dev_serial);
        std::cout << "Got Device: " << dev_name << " : " << dev_serial << std::endl;
        std::stringstream filename;
        filename << _working_dir << "/" << dev_serial << ".cfg";
        _wanted_profiles[dev_serial] = parse_profiles_file(filename.str());
    }
    // TODO: Read transformations.cfg file
    // This is a rotation matrix of 30 degrees around z-axis:
    //array([[ 0.8660254, -0.5, 0. ],
    //    [0.5, 0.8660254, 0.],
    //    [0., 0., 1.]] )

    // This is a rotation matrix of 15 degrees around z-axis: (== virtual to left camera)
    //array([[ 0.96592583, -0.25881905, 0. ],
        //[0.25881905, 0.96592583, 0.],
        //[0., 0., 1.]] )
    CreateVirtualDevice();
    //double tranlation_a_b(200); // 200 mm
    double tranlation_a_b(0); // 200 mm

    std::string serial_vir(_soft_dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
    //_ir_extrinsics[serials[0]][serial_vir] = std::vector<double>({ 0.96592583, 0.25881905, 0.,     -0.25881905, 0.96592583, 0,     0., 0. ,1.,    -tranlation_a_b / 2.0, 0, 0 });
    //_ir_extrinsics[serials[1]][serial_vir] = std::vector<double>({ 0.96592583, -0.25881905, 0.,     0.25881905, 0.96592583, 0,     0., 0., 1. ,    tranlation_a_b / 2.0, 0, 0 });

    //_ir_extrinsics[serials[0]][serial_vir] = std::vector<double>({ 1, 0, 0.,     0, 1, 0,     0., 0. ,1.,    0, 0, 0 });
    //_ir_extrinsics[serials[1]][serial_vir] = std::vector<double>({ 1, 0, 0.,     0, 1, 0,     0., 0. ,1.,    0, 0, 0 });


    //_ir_extrinsics[serials[0]][serial_vir] = rs2_extrinsics({ {0.96592583, -0.25881905, 0.,     0.25881905, 0.96592583, 0,  0,0,1}, {0,0,0} }); // Around Z axis ?
    //_ir_extrinsics[serials[1]][serial_vir] = rs2_extrinsics({ {0.96592583,  0.25881905, 0.,    -0.25881905, 0.96592583, 0,  0,0,1}, {0,0,0} });

    _ir_extrinsics[serials[0]][serial_vir] = rs2_extrinsics({ {0.96592583, 0,  0.25881905,  0,1,0,      -0.25881905, 0., 0.96592583}, {0,0,0} });   //15 Around Y axis ?
    //_ir_extrinsics[serials[1]][serial_vir] = rs2_extrinsics({ {0.96592583, 0, -0.25881905, 0,1,0,        0.25881905,  0., 0.96592583}, {0,0,0} });

    _ir_extrinsics[serials[0]][serial_vir] = rs2_extrinsics({ {0.8660254, 0,  0.5,  0,1,0,      -0.5, 0., 0.8660254}, {0,0,0} });   //30 Around Y axis ?
    _ir_extrinsics[serials[1]][serial_vir] = rs2_extrinsics({ {0.8660254, 0, -0.5, 0,1,0,        0.5,  0., 0.8660254}, {0,0,0} });

    //_ir_extrinsics[serials[0]][serial_vir] = rs2_extrinsics({ {1,0,0,   0,1,0,  0,0,1}, {0,0,0} });
    //_ir_extrinsics[serials[1]][serial_vir] = rs2_extrinsics({ {1,0,0,   0,1,0,  0,0,1}, {0,0,0} });

    _left_device = serials[0];  // TODO: define left device based on transformation between devices.
    return true;
}

rs2_intrinsics CPointcloudStitcher::create_intrinsics(const synthetic_frame& _virtual_frame, const float fov_x, const float fov_y)    // fov_x, fov_y in radians.
{
    float fx = _virtual_frame.x / (2 * tanf(fov_x / 2));
    float fy = _virtual_frame.y / (2 * tanf(fov_y / 2));
    rs2_intrinsics intrinsics = { _virtual_frame.x, _virtual_frame.y,
        (float)_virtual_frame.x / 2, (float)_virtual_frame.y / 2,
        fx , fy ,
        RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };

    return intrinsics;
}


void CPointcloudStitcher::InitializeVirtualFrames()
{
    const int W = 640;
    const int H = 480;

    _virtual_depth_frame.x = W;
    _virtual_depth_frame.y = H;
    _virtual_depth_frame.bpp = 2;

    _virtual_color_frame.x = W;
    _virtual_color_frame.y = H;
    _virtual_color_frame.bpp = 4;


    std::vector<uint8_t> pixels_depth(_virtual_depth_frame.x * _virtual_depth_frame.y * _virtual_depth_frame.bpp, 0);
    _virtual_depth_frame.frame = std::move(pixels_depth);

    std::vector<uint8_t> pixels_color(_virtual_color_frame.x * _virtual_color_frame.y * _virtual_color_frame.bpp, 0);

    _virtual_color_frame.frame = std::move(pixels_color);
}


#if false
void CPointcloudStitcher::CreateVirtualDevice()
{
    auto texture = g_app_data.get_synthetic_texture();
    
    rs2_intrinsics color_intrinsics = g_app_data.create_texture_intrinsics();
    rs2_intrinsics depth_intrinsics = g_app_data.create_depth_intrinsics();
    
    //==================================================//
    //           Declare Software-Only Device           //
    //==================================================//
    
    //rs2::software_device _soft_dev; // Create software-only device
    std::string serial("12345678");
    _soft_dev.register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, serial);
    auto depth_sensor = _soft_dev.add_sensor("Depth"); // Define single sensor
    _active_software_sensors.insert(std::pair<std::string, rs2::software_sensor>("Depth", depth_sensor));
    auto color_sensor = _soft_dev.add_sensor("Color"); // Define single sensor
    _active_software_sensors.insert(std::pair<std::string, rs2::software_sensor>("Color", color_sensor));

    auto depth_stream = depth_sensor.add_video_stream({  RS2_STREAM_DEPTH, 0, 0,
                                W, H, 15, BPP,
                                RS2_FORMAT_Z16, depth_intrinsics });
    
    depth_sensor.add_read_only_option(RS2_OPTION_DEPTH_UNITS, 0.001f);
    
    
    auto color_stream = color_sensor.add_video_stream({  RS2_STREAM_COLOR, 0, 1, texture.x,
                                texture.y, 15, texture.bpp,
                                RS2_FORMAT_RGBA8, color_intrinsics });
    
    depth_stream.register_extrinsics_to(color_stream, { { 1,0,0,0,1,0,0,0,1 },{ 0,0,0 } });
    _soft_dev.create_matcher(RS2_MATCHER_DLR_C);
}
#else
void CPointcloudStitcher::CreateVirtualDevice()
{
    InitializeVirtualFrames();

    auto texture = g_app_data.get_synthetic_texture();
    //float fov_x(74.0 * PI / 180.0);
    //float fov_y(62.0 * PI / 180.0);
    float fov_x(120.0 * PI / 180.0);
    float fov_y(82.0 * PI / 180.0);
    rs2_intrinsics color_intrinsics = create_intrinsics(_virtual_color_frame, fov_x, fov_y);
    rs2_intrinsics depth_intrinsics = create_intrinsics(_virtual_depth_frame, fov_x, fov_y);

    //==================================================//
    //           Declare Software-Only Device           //
    //==================================================//

    //rs2::software_device _soft_dev; // Create software-only device
    std::string serial("12345678");
    _soft_dev.register_info(RS2_CAMERA_INFO_SERIAL_NUMBER, serial);
    auto depth_sensor = _soft_dev.add_sensor("Depth"); // Define single sensor
    _active_software_sensors.insert(std::pair<std::string, rs2::software_sensor>("Depth", depth_sensor));
    auto color_sensor = _soft_dev.add_sensor("Color"); // Define single sensor
    _active_software_sensors.insert(std::pair<std::string, rs2::software_sensor>("Color", color_sensor));

    auto depth_stream = depth_sensor.add_video_stream({ RS2_STREAM_DEPTH, 0, 0,
                                _virtual_depth_frame.x, _virtual_depth_frame.y, 15, _virtual_depth_frame.bpp,
                                RS2_FORMAT_Z16, depth_intrinsics });

    depth_sensor.add_read_only_option(RS2_OPTION_DEPTH_UNITS, 0.001f);


    auto color_stream = color_sensor.add_video_stream({ RS2_STREAM_COLOR, 0, 1, 
                                _virtual_color_frame.x, _virtual_color_frame.y, 15, _virtual_color_frame.bpp,
                                RS2_FORMAT_RGBA8, color_intrinsics });

    depth_stream.register_extrinsics_to(color_stream, { { 1,0,0,0,1,0,0,0,1 },{ 0,0,0 } });
    _soft_dev.create_matcher(RS2_MATCHER_DLR_C);
}
#endif

bool CPointcloudStitcher::Start()
{
    int count(0);
    for (auto dev : _devices)
    {
        count++;
        //if (count == 1) continue;
        if (!OpenSensors(dev))
        {
            CloseSensors();
            return false;
        }
        //break;
    }

    for (auto sensors : _active_sensors)
    {
        const string& serial = sensors.first;
        //std::cout << "init rs2::frame_queue" << std::endl;
        _frames[serial] = rs2::frame_queue(2, true);

        _syncer[sensors.first].start([this, serial](rs2::frame frame) {
            frame_callback(frame, serial); });

        for (auto sensor : sensors.second)
        {
            std::cout << "Starting sensor: " << sensor.get_info(RS2_CAMERA_INFO_NAME) << std::endl;
            sensor.start(_syncer[sensors.first]);
            std::cout << "Ok:" << std::endl;
        }
    }

    std::string serial(_soft_dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
    //_syncer[serial].start([this](rs2::frame frame) {
    //    frame_callback_soft(frame); });
    for (auto&& sensor : _soft_dev.query_sensors())
    {
        sensor.open(sensor.get_stream_profiles());
        _active_sensors[serial].emplace_back(sensor);
        std::cout << "ADD SENSOR TO SYNC" << std::endl;
        sensor.start(_soft_sync);
    }
    return true;
}

void CPointcloudStitcher::StopSensors()
{
    for (auto sensors : _active_sensors)
    {
        for (auto sensor : sensors.second)
        {
            sensor.stop();
        }
    }
}

void CPointcloudStitcher::CloseSensors()
{
    for (auto sensors : _active_sensors)
    {
        for (auto sensor : sensors.second)
        {
            sensor.close();
        }
    }
}

void CPointcloudStitcher::frame_callback(rs2::frame frame, const string& serial)
{
    rs2::frameset fset = frame.as<rs2::frameset>();
    if (fset.size() == 2)
    {
        _frames[serial].enqueue(std::move(fset));
    }
}

//void CPointcloudStitcher::frame_callback_soft(rs2::frame frame)
//{
//
//}

void multiply_vector4_by_matrix_4x3(const GLfloat vec[], const GLfloat mat[], GLfloat* result)
{
    const auto M = 4;
    const auto N = 3;
    int max_idx(0);
    for (int i = 0; i < N; i++)
    {
        result[i] = 0;
        for (int j = 0; j < M; j++)
        {
            result[i] += vec[j] * mat[N * j + i];
            max_idx = std::max(max_idx, N * j + i);
        }
    }
    std::cout << "max_idx: " << max_idx << std::endl;
    return;
}

#if true
void CPointcloudStitcher::ProjectFramesOnOtherDevice(rs2::frameset frames, const string& from_serial, const string& to_serial)
{
    rs2::points points;
    rs2::frame depth = frames.first_or_default(RS2_STREAM_DEPTH);
    rs2::frame color = frames.first_or_default(RS2_STREAM_COLOR);
    float depth_point[3] = { 0 };
    float virtual_point[3] = { 0 };
    float color_pixel[2] = { 0 };
    float virtual_pixel[2] = { 0 };

    rs2::software_sensor virtual_depth_sensor = _active_software_sensors.at("Depth");
    const rs2_extrinsics& extrinsics(_ir_extrinsics[from_serial][to_serial]);
    const rs2_intrinsics& virtual_depth_intinsics(virtual_depth_sensor.get_active_streams()[0].as<rs2::video_stream_profile>().get_intrinsics());
    
    rs2_intrinsics color_intinsics;
    int color_bpp(0);
    if (color)
    {
        rs2::video_frame color_frame = color.as<rs2::video_frame>();
        color_intinsics = (color_frame.get_profile().as<rs2::video_stream_profile>().get_intrinsics());
        color_bpp = color_frame.get_bytes_per_pixel();
        std::cout << "color_bpp: " << color_bpp << std::endl;
    }

    if (!depth) return;

    if (auto as_depth = depth.as<rs2::depth_frame>())
    {
        const rs2_intrinsics& depth_intinsics(as_depth.get_profile().as<rs2::video_stream_profile>().get_intrinsics());
        //float HFOV = (2 * atan(depth_intinsics.width / (2 * depth_intinsics.fx))) * 180.0 / PI;
        //float VFOV = (2 * atan(depth_intinsics.height / (2 * depth_intinsics.fy))) * 180.0 / PI;
        std::cout << "Projecting from " << from_serial << std::endl;
        int counter(0);
        const uint16_t* ptr = (const uint16_t * )as_depth.get_data();
        for (int y = 0; y < as_depth.get_height(); y++)
        {
            for (int x = 0; x < as_depth.get_width(); x++, ptr++)
            {
                float pixel[] = { x, y };
                //std::cout << x << ", " << y << ": *ptr=" << *ptr << std::endl;
                if (as_depth.get_distance(x, y) <= 0)
                    continue;

                rs2_deproject_pixel_to_point(depth_point, &depth_intinsics, pixel, as_depth.get_distance(x, y));


                //counter++;
                //std::cout << depth_point[0] << ", " << depth_point[1] << ", " << depth_point[2] << std::endl;

                rs2_transform_point_to_point(virtual_point, &extrinsics, depth_point);
                //std::cout << virtual_point[0] << ", " << virtual_point[1] << ", " << virtual_point[2] << std::endl;
                rs2_project_point_to_pixel(virtual_pixel, &virtual_depth_intinsics, virtual_point);
                //std::cout << virtual_pixel[0] << ", " << virtual_pixel[1] << std::endl;

                //std::cout << "-----" << std::endl;
                //if (counter == 5) break;

                if (virtual_pixel[0] < 0 || virtual_pixel[0] > virtual_depth_intinsics.width ||
                    virtual_pixel[1] < 0 || virtual_pixel[1] > virtual_depth_intinsics.height)
                    continue;
                //_virtual_depth_frame[]
                int offset((int)(virtual_pixel[1]) * _virtual_depth_frame.x + (int)(virtual_pixel[0]));
                uint16_t crnt_val(((uint16_t*)_virtual_depth_frame.frame.data())[offset]);
                if (crnt_val && crnt_val < *ptr)
                {
                    counter++;
                    continue;
                }
                ((uint16_t*)_virtual_depth_frame.frame.data())[offset] = *ptr;
                if (color)
                {
                    rs2_project_point_to_pixel(color_pixel, &color_intinsics, depth_point);
                    if (color_pixel[0] < 0 || color_pixel[0] > color_intinsics.width ||
                        color_pixel[1] < 0 || color_pixel[1] > color_intinsics.height)
                        continue;
                    int offset_dest( ((int)virtual_pixel[1] * _virtual_color_frame.x + (int)virtual_pixel[0]) * _virtual_color_frame.bpp) ;
                    int offset_src ( ((int)pixel[1] * color_intinsics.width + (int)pixel[0]) * color_bpp);
                    //const uint8_t* src_ptr = ((const uint8_t*)(color.get_data())) + offset_src;
                    memcpy((uint8_t*)_virtual_color_frame.frame.data() + offset_dest,
                           (uint8_t*)(color.get_data()) + offset_src, 
                            color_bpp * sizeof(uint8_t));
                }
            }
            //if (counter == 5) break;
        }
        std::cout << "skipped: " << counter << std::endl;
        {
            virtual_depth_sensor.on_video_frame({ _virtual_depth_frame.frame.data(), // Frame pixels from capture API
                [](void*) {}, // Custom deleter (if required)
                _virtual_depth_frame.x * _virtual_depth_frame.bpp, _virtual_depth_frame.bpp, // Stride and Bytes-per-pixel
                depth.get_timestamp(), depth.get_frame_timestamp_domain(), _frame_number, // Timestamp, Frame# for potential sync services
                *(virtual_depth_sensor.get_active_streams().begin()) });
        }
        if (color)
        {
            rs2::software_sensor virtual_color_sensor = _active_software_sensors.at("Color");
            virtual_color_sensor.on_video_frame({ _virtual_color_frame.frame.data(), // Frame pixels from capture API
                [](void*) {}, // Custom deleter (if required)
                _virtual_color_frame.x * _virtual_color_frame.bpp, _virtual_color_frame.bpp, // Stride and Bytes-per-pixel
                color.get_timestamp(), color.get_frame_timestamp_domain(), _frame_number, // Timestamp, Frame# for potential sync services
                *(virtual_color_sensor.get_active_streams().begin()) });
        }
    }
}

#else
void CPointcloudStitcher::ProjectFramesOnOtherDevice(rs2::frameset frames, const string& from_serial, const string& to_serial)
{
    rs2::points points;
    rs2::frame depth = frames.first_or_default(RS2_STREAM_DEPTH);
    rs2::frame color = frames.first_or_default(RS2_STREAM_COLOR);

    if (!depth) return;
    if (color)
    {
        std::cout << "map to color" << std::endl;
        _pc.map_to(color);
    }
    if (auto as_depth = depth.as<rs2::depth_frame>())
    {
        std::cout << "calculate depth points" << std::endl;
        points = _pc.calculate(as_depth);
    }
    auto vertices = points.get_vertices();              // get vertices
    auto tex_coords = points.get_texture_coordinates(); // and texture coordinates
    std::cout << "Vertices: from left?" << (from_serial == _left_device) << std::endl;
    float tmp_result[4] = { 0 };
    float virtual_point[3] = { 0 };
    float virtual_pixel[2] = { 0 };
    int counter(0);
    //float vector[] = { 0,10,0 };
    //float mat[12];
    //for (int i = 0; i < 12; i++)
    //    mat[i] = static_cast<float>(_ir_extrinsics[from_serial][to_serial][i]);
    const rs2_extrinsics& extrinsics(_ir_extrinsics[from_serial][to_serial]);
    const rs2_intrinsics& virtual_depth_intinsics(_active_software_sensors.at("Depth").get_active_streams()[0].as<rs2::video_stream_profile>().get_intrinsics());
    for (int i = 0; i < points.size(); i++)
    {
        if (vertices[i].z > 0)
        {
            counter++;
            std::cout << vertices[i].x << ", " << vertices[i].y << ", " << vertices[i].z << std::endl;
            //for (int i = 0; i < 12; i++) std::cout << mat[i] << ", "; std::cout << std::endl;

            rs2_transform_point_to_point(virtual_point, &extrinsics, vertices[i]);
            //multiply_vector4_by_matrix_4x3(vertices[i], mat, tmp_result);
            //std::cout << tmp_result[0] << ", " << tmp_result[1] << ", " << tmp_result[2] << ", " << tmp_result[3] << std::endl;
            std::cout << virtual_point[0] << ", " << virtual_point[1] << ", " << virtual_point[2] << std::endl;
            rs2_project_point_to_pixel(virtual_pixel, &virtual_depth_intinsics, virtual_point);
            std::cout << virtual_pixel[0] << ", " << virtual_pixel[1] << std::endl;

            std::cout << "-----" << std::endl;
            if (counter == 5) break;

            if (virtual_pixel[0] < 0 || virtual_pixel[0] > virtual_depth_intinsics.width ||
                virtual_pixel[1] < 0 || virtual_pixel[1] > virtual_depth_intinsics.height)
                continue;
            //_virtual_depth_frame[]
            int i(virtual_pixel[1]);
            int j(virtual_pixel[0]);
            ((uint16_t*)_virtual_depth_frame.frame.data())[i * _virtual_depth_frame.x + j] = (int)(d * 0xff);


        }
    }



    counter = 0;
    if (color)
    {
        std::cout << "tex_coords:" << std::endl;
        for (int i = 0; i < points.size(); i++)
        {
            if (vertices[i].z)
            {
                counter++;
                std::cout << tex_coords[i].u << ", " << tex_coords[i].v << std::endl;
                if (counter == 5) break;
            }
        }
    }
}
#endif

void CPointcloudStitcher::Run(window& app)
{
    const auto SLEEP_TIME = std::chrono::milliseconds(500);
    std::string serial_vir(_soft_dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
    bool display_original_images(true);

    int frame_number(0);
    while (app) // Application still alive?
    {
        int offset(0);
        memset(_virtual_depth_frame.frame.data(), 0, _virtual_depth_frame.x * _virtual_depth_frame.y * _virtual_depth_frame.bpp);
        memset(_virtual_color_frame.frame.data(), 0, _virtual_color_frame.x * _virtual_color_frame.y * _virtual_color_frame.bpp);
        for (auto frames : _frames)
        {
            rs2::frame frm = frames.second.wait_for_frame();    // Wait for next set of frames from the camera
            ProjectFramesOnOtherDevice(frm, frames.first, serial_vir);

            // DISPLAY ORIGINAL IMAGES
            if (display_original_images)
            {
                for (auto frame : frm.as<rs2::frameset>())
                {
                    if (_left_device == frames.first) offset = 0; else offset = 2;
                    if (frame.is<rs2::depth_frame>())
                    {
                        offset += 3;

                        _frames_map[offset].first = frame.apply_filter(_colorizer);
                    }
                    else
                    {
                        _frames_map[offset].first = frame;
                    }
                }
            }
        }

        // Visualize virtual device:
        if (false)
        {
            rs2::software_sensor sensor = _active_software_sensors.at("Depth");
            synthetic_frame& depth_frame = g_app_data.get_synthetic_depth(g_app_state);
            sensor.on_video_frame({ depth_frame.frame.data(), // Frame pixels from capture API
                [](void*) {}, // Custom deleter (if required)
                depth_frame.x * depth_frame.bpp, depth_frame.bpp, // Stride and Bytes-per-pixel
                (rs2_time_t)frame_number * 16, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, frame_number, // Timestamp, Frame# for potential sync services
                *(sensor.get_active_streams().begin())});

            sensor = _active_software_sensors.at("Color");
            synthetic_frame& texture = g_app_data.get_synthetic_texture();
            sensor.on_video_frame({ texture.frame.data(), // Frame pixels from capture API
                [](void*) {}, // Custom deleter (if required)
                texture.x * texture.bpp, texture.bpp, // Stride and Bytes-per-pixel
                (rs2_time_t)frame_number * 16, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, frame_number, // Timestamp, Frame# for potential sync services
                *(sensor.get_active_streams().begin())});
            ++frame_number;
        }
        rs2::frameset fset = _soft_sync.wait_for_frames();

#if true
        std::cout << "fset.size(): " << fset.size() << std::endl;
        for (auto it = fset.begin(); it != fset.end(); ++it)
        {
            auto f = (*it);
            auto stream_type = f.get_profile().stream_type();
            auto stream_index = f.get_profile().stream_index();
            auto stream_format = f.get_profile().format();

            printf("Frameset contain (%s, %d, %s) frame. frame_number: %llu ; frame_TS: %f ; ros_TS(NSec)",
                rs2_stream_to_string(stream_type), stream_index, rs2_format_to_string(stream_format), f.get_frame_number(), f.get_timestamp());
            if (f.is<rs2::video_frame>())
                std::cout << "frame: " << f.as<rs2::video_frame>().get_width() << " x " << f.as<rs2::video_frame>().get_height() << std::endl;
        }
#endif
        rs2::frame depth = fset.first_or_default(RS2_STREAM_DEPTH);
        rs2::frame color = fset.first_or_default(RS2_STREAM_COLOR);
        if (depth)
            _frames_map[DEPTH_UNITED].first = depth.apply_filter(_colorizer);

        if (color)
            _frames_map[COLOR_UNITED].first = color;

        app.show(_frames_map);
    }

}

int main(int argc, char* argv[]) try
{
    rs2::log_to_file(RS2_LOG_SEVERITY_DEBUG, "C:\\projects\\librealsense\\build\\Debug\\lrs_log.txt");

    std::string working_dir;
    for (int c = 1; c < argc; ++c) {
        if (!std::strcmp(argv[c], "-h") || !std::strcmp(argv[c], "--help")) {
            std::cout << "USAGE: " << std::endl;
            std::cout << argv[0] << " <working_directory>" << std::endl;
            std::cout << std::endl;
            std::cout << "Connect exactly 2 RS devices." << std::endl;
            std::cout << "For every camera, a file should be located in <working_directory> named <serial_no>.cfg" << std::endl;
            std::cout << std::endl;
            std::cout << "The <serial_no>.cfg file should contain the following:" << std::endl;
            std::cout << "DEPTH, WIDTH1, HEIGHT1, FPS1, FORMAT1, STREAM_INDEX1" << std::endl;
            std::cout << "COLOR, WIDTH2, HEIGHT2, FPS2, FORMAT2, STREAM_INDEX2 " << std::endl;
            std::cout << std::endl;
            std::cout << "In addition, there will be another transformations.cfg file, of the following format:" << std::endl;
            std::cout << "<from_serial>, <to_serial>, <t1, t2, t3, ...., t12>" << std::endl;
            std::cout << "The t1, t2,... t12 line represents transformation from 1 device to the other." << std::endl;
            std::cout << std::endl;
            return -1;
        }
    }
    if (argc > 1)
        working_dir = argv[1];
    else
        working_dir = ".";

    CPointcloudStitcher pc_stitcher(working_dir);
    if (!pc_stitcher.Init())
        return -1;
    if (!pc_stitcher.Start())
        return -1;

    unsigned tiles_in_row = 3;
    unsigned tiles_in_col = 2;

    window app(1280, 720, "RealSense Pointcloud-Stitching Example", tiles_in_row, tiles_in_col);
    ImGui_ImplGlfw_Init(app, false);
    register_glfw_callbacks(app, g_app_state);

    pc_stitcher.Run(app);
    pc_stitcher.StopSensors();
    pc_stitcher.CloseSensors();

    return EXIT_SUCCESS;
}
catch (const rs2::error& e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}


//int main(int argc, char * argv[]) try
//{
//    window app(1280, 1500, "RealSense Capture Example");
//    glfw_state app_state;
//    register_glfw_callbacks(app, app_state);
//    rs2::colorizer color_map; // Save colorized depth for preview
//
//    rs2::pointcloud pc;
//    rs2::points points;
//    int frame_number = 0;
//
//    custom_frame_source app_data;
//
//    auto texture = app_data.get_synthetic_texture();
//
//    rs2_intrinsics color_intrinsics = app_data.create_texture_intrinsics();
//    rs2_intrinsics depth_intrinsics = app_data.create_depth_intrinsics();
//
//    //==================================================//
//    //           Declare Software-Only Device           //
//    //==================================================//
//
//    rs2::software_device dev; // Create software-only device
//
//    auto depth_sensor = dev.add_sensor("Depth"); // Define single sensor
//    auto color_sensor = dev.add_sensor("Color"); // Define single sensor
//
//    auto depth_stream = depth_sensor.add_video_stream({  RS2_STREAM_DEPTH, 0, 0,
//                                W, H, 60, BPP,
//                                RS2_FORMAT_Z16, depth_intrinsics });
//
//    depth_sensor.add_read_only_option(RS2_OPTION_DEPTH_UNITS, 0.001f);
//
//
//    auto color_stream = color_sensor.add_video_stream({  RS2_STREAM_COLOR, 0, 1, texture.x,
//                                texture.y, 60, texture.bpp,
//                                RS2_FORMAT_RGBA8, color_intrinsics });
//
//    dev.create_matcher(RS2_MATCHER_DLR_C);
//    rs2::syncer sync;
//
//    depth_sensor.open(depth_stream);
//    color_sensor.open(color_stream);
//
//    depth_sensor.start(sync);
//    color_sensor.start(sync);
//
//    depth_stream.register_extrinsics_to(color_stream, { { 1,0,0,0,1,0,0,0,1 },{ 0,0,0 } });
//
//    while (app) // Application still alive?
//    {
//        synthetic_frame& depth_frame = app_data.get_synthetic_depth(app_state);
//
//        depth_sensor.on_video_frame({ depth_frame.frame.data(), // Frame pixels from capture API
//            [](void*) {}, // Custom deleter (if required)
//            depth_frame.x*depth_frame.bpp, depth_frame.bpp, // Stride and Bytes-per-pixel
//            (rs2_time_t)frame_number * 16, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, frame_number, // Timestamp, Frame# for potential sync services
//            depth_stream });
//
//
//        color_sensor.on_video_frame({ texture.frame.data(), // Frame pixels from capture API
//            [](void*) {}, // Custom deleter (if required)
//            texture.x*texture.bpp, texture.bpp, // Stride and Bytes-per-pixel
//            (rs2_time_t)frame_number * 16, RS2_TIMESTAMP_DOMAIN_HARDWARE_CLOCK, frame_number, // Timestamp, Frame# for potential sync services
//            color_stream });
//
//        ++frame_number;
//
//        rs2::frameset fset = sync.wait_for_frames();
//        rs2::frame depth = fset.first_or_default(RS2_STREAM_DEPTH);
//        rs2::frame color = fset.first_or_default(RS2_STREAM_COLOR);
//
//        if (depth && color)
//        {
//            if (auto as_depth = depth.as<rs2::depth_frame>())
//                points = pc.calculate(as_depth);
//            pc.map_to(color);
//
//            // Upload the color frame to OpenGL
//            app_state.tex.upload(color);
//        }
//        draw_pointcloud(app.width(), app.height(), app_state, points);
//    }
//
//    return EXIT_SUCCESS;
//}
//catch (const rs2::error & e)
//{
//    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
//    return EXIT_FAILURE;
//}
//catch (const std::exception& e)
//{
//    std::cerr << e.what() << std::endl;
//    return EXIT_FAILURE;
//}
//


