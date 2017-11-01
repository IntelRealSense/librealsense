// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <fstream>
#include <vector>
#include <set>
#include <map>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <mutex>
#include <regex>

#include "../../third-party/realsense-file/rosbag/rosbag_storage/include/rosbag/bag.h"
#include "../../third-party/realsense-file/rosbag/rosbag_storage/include/rosbag/view.h"
#include "../../third-party/realsense-file/rosbag/msgs/sensor_msgs/Imu.h"
#include "../../third-party/realsense-file/rosbag/msgs/sensor_msgs/Image.h"
#include "../../third-party/realsense-file/rosbag/msgs/diagnostic_msgs/KeyValue.h"
#include "../../third-party/realsense-file/rosbag/msgs/std_msgs/UInt32.h"
#include "../../third-party/realsense-file/rosbag/msgs/std_msgs/String.h"
#include "../../third-party/realsense-file/rosbag/msgs/std_msgs/Float32.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/StreamInfo.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/ImuIntrinsic.h"

#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/compressed_frame_info.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/controller_command.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/controller_event.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/controller_vendor_data.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/extrinsics.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/frame_info.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/ImuIntrinsic.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/metadata.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/motion_intrinsics.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/motion_stream_info.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/occupancy_map.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/pose.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/stream_extrinsics.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/stream_info.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/StreamInfo.h"
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/vendor_data.h"

#include "../../third-party/realsense-file/rosbag/msgs/sensor_msgs/CameraInfo.h"
#include "../../third-party/realsense-file/rosbag/msgs/sensor_msgs/TimeReference.h"
#include "../../third-party/realsense-file/rosbag/msgs/geometry_msgs/Transform.h"

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_internal.h>
#include <noc_file_dialog.h>

std::ostream& operator<<(std::ostream& os, const rosbag::MessageInstance& m);

template <size_t N, typename T>
std::ostream& operator<<(std::ostream& os, const std::array<T,N>& arr)
{
    for (size_t i = 0; i < arr.size(); ++i)
    {
        if (i != 0)
            os << ",";
        os << arr[i];
    }
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (i != 0)
            os << ",";
        os << v[i];
    }
    return os;
}

struct to_string
{
    std::ostringstream ss;
    template<class T> to_string & operator << (const T & val) { ss << val; return *this; }
    operator std::string() const { return ss.str(); }
};

std::string pretty_time(std::chrono::nanoseconds d)
{
    auto hhh = std::chrono::duration_cast<std::chrono::hours>(d);
    d -= hhh;
    auto mm = std::chrono::duration_cast<std::chrono::minutes>(d);
    d -= mm;
    auto ss = std::chrono::duration_cast<std::chrono::seconds>(d);
    d -= ss;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(d);

    std::ostringstream stream;
    stream << std::setfill('0') << std::setw(3) << hhh.count() << ':' <<
        std::setfill('0') << std::setw(2) << mm.count() << ':' <<
        std::setfill('0') << std::setw(2) << ss.count() << '.' <<
        std::setfill('0') << std::setw(3) << ms.count();
    return stream.str();
}


std::ostream& operator<<(std::ostream& os, rosbag::compression::CompressionType c)
{
	switch (c)
	{
    case rosbag::CompressionType::Uncompressed: os << "Uncompressed"; break;
	case rosbag::CompressionType::BZ2: os << "BZ2"; break;
	case rosbag::CompressionType::LZ4: os << "LZ4"; break;
	default: break;
	}
	return os;
}

std::ostream& operator<<(std::ostream& os, const rosbag::MessageInstance& m)
{
    if (m.isType<std_msgs::UInt32>())
    {
        os << "Value : " << m.instantiate<std_msgs::UInt32>()->data << std::endl;
    }
    if (m.isType<std_msgs::String>())
    {
        os << "Value : " << m.instantiate<std_msgs::String>()->data << std::endl;
    }
    if (m.isType<std_msgs::Float32>())
    {
        os << "Value : " << m.instantiate<std_msgs::Float32>()->data << std::endl;
    }
    if (m.isType<diagnostic_msgs::KeyValue>())
    {
        auto kvp = m.instantiate<diagnostic_msgs::KeyValue>();
        os << "Key   : " << kvp->key << std::endl;
        os << "Value : " << kvp->value << std::endl;
    }
    if (m.isType<realsense_msgs::StreamInfo>())
    {
        auto stream_info = m.instantiate<realsense_msgs::StreamInfo>();
        os << "Encoding       : " << stream_info->encoding << std::endl;
        os << "FPS            : " << stream_info->fps << std::endl;
        os << "Is Recommended : " << (stream_info->is_recommended ? "True" : "False") << std::endl;
    }
    if (m.isType<sensor_msgs::CameraInfo>())
    {
        auto camera_info = m.instantiate<sensor_msgs::CameraInfo>();
        os << "Width      : " << camera_info->width << std::endl;
        os << "Height     : " << camera_info->height << std::endl;
        os << "Intrinsics : " << std::endl;
        os << "  Focal Point      : " << camera_info->K[0] << ", " << camera_info->K[4] << std::endl;
        os << "  Principal Point  : " << camera_info->K[2] << ", " << camera_info->K[5] << std::endl;
        os << "  Coefficients     : "
           << camera_info->D[0] << ", "
           << camera_info->D[1] << ", "
           << camera_info->D[2] << ", "
           << camera_info->D[3] << ", "
           << camera_info->D[4] << std::endl;
        os << "  Distortion Model : " << camera_info->distortion_model << std::endl;

    }
    if (m.isType<realsense_msgs::ImuIntrinsic>())
    {
        auto data = m.instantiate<realsense_msgs::ImuIntrinsic>();
        os << "bias_variances : " << data->bias_variances << "\n";
        os << "data : " << data->data << "\n";
        os << "noise_variances : " << data->noise_variances << "\n";
    }
    if (m.isType<sensor_msgs::Image>())
    {
        auto image = m.instantiate<sensor_msgs::Image>();
        os << "Encoding     : " << image->encoding << std::endl;
        os << "Width        : " << image->width << std::endl;
        os << "Height       : " << image->height << std::endl;
        os << "Step         : " << image->step << std::endl;
        os << "Frame Number : " << image->header.seq << std::endl;
        os << "Timestamp    : " << pretty_time(std::chrono::nanoseconds(image->header.stamp.toNSec())) << std::endl;
    }
    if (m.isType<sensor_msgs::Imu>())
    {
        auto imu = m.instantiate<sensor_msgs::Imu>();
        os << "header : " << imu->header << "\n";
        os << "orientation : " << imu->orientation << "\n";
        os << "orientation_covariance : " << imu->orientation_covariance << "\n";
        os << "angular_velocity : " << imu->angular_velocity << "\n";
        os << "angular_velocity_covariance : " << imu->angular_velocity_covariance << "\n";
        os << "linear_acceleration : " << imu->linear_acceleration << "\n";
        os << "linear_acceleration_covariance : " << imu->linear_acceleration_covariance << "\n";
        os << "orientation_covariance : " << imu->orientation_covariance << "\n";
        os << "angular_velocity_covariance : " << imu->angular_velocity_covariance << "\n";
        os << "linear_acceleration_covariance : " << imu->linear_acceleration_covariance << "\n";
    }
    if (m.isType<sensor_msgs::TimeReference>())
    {
        auto tr = m.instantiate<sensor_msgs::TimeReference>();
        os << "  Header        : " << tr->header << std::endl;
        os << "  Source        : " << tr->source << std::endl;
        os << "  TimeReference : " << tr->time_ref << std::endl;
    }
    if (m.isType<geometry_msgs::Transform>())
    {
        os << "  Extrinsics : " << std::endl;
        auto tf = m.instantiate<geometry_msgs::Transform>();
        os << "    Rotation    : " << tf->rotation << std::endl;
        os << "    Translation : " << tf->translation << std::endl;
    }




    /*************************************************************/
    /*************************************************************/
    /*                  Older Version                            */
    /*************************************************************/
    /*************************************************************/




    if (m.isType<realsense_msgs::compressed_frame_info>())
    {
        auto data = m.instantiate<realsense_msgs::compressed_frame_info>();
        os << data << "\n";
        os << "encoding: " << data->encoding << "\n";
        os << "frame_metadata: " << data->frame_metadata << "\n";
        os << "height: " << data->height << "\n";
        os << "step: " << data->step << "\n";
        os << "system_time: " << data->system_time << "\n";
        os << "time_stamp_domain: " << data->time_stamp_domain << "\n";
        os << "width: " << data->width << "\n";
    }
    if (m.isType<realsense_msgs::controller_command>())
    {
        auto data = m.instantiate<realsense_msgs::controller_command>();
        os << "controller_id : " << data->controller_id << "\n";
        os << "mac_address : " << data->mac_address << "\n";
        os << "type : " << data->type << "\n";
    }
    if (m.isType<realsense_msgs::controller_event>())
    {
        auto data = m.instantiate<realsense_msgs::controller_event>();
        os << "controller_id : " << data->controller_id << "\n";
        os << "mac_address : " << data->mac_address << "\n";
        os << "type : " << data->type << "\n";
        os << "type : " << data->timestamp << "\n";
    }
    if (m.isType<realsense_msgs::controller_vendor_data>())
    {
        auto data = m.instantiate<realsense_msgs::controller_vendor_data>();
        os << "controller_id : " << data->controller_id << "\n";
        os << "data : " << data->data << "\n";
        os << "timestamp : " << data->timestamp << "\n";
        os << "vendor_data_source_index : " << data->vendor_data_source_index << "\n";
        os << "vendor_data_source_type : " << data->vendor_data_source_type << "\n";

    }
    if (m.isType<realsense_msgs::extrinsics>())
    {
        auto data = m.instantiate<realsense_msgs::extrinsics>();
        os << "  Extrinsics : " << std::endl;
        os << "    Rotation    : " << data->rotation << std::endl;
        os << "    Translation : " << data->translation << std::endl;
    }
    if (m.isType<realsense_msgs::frame_info>())
    {
        auto data = m.instantiate<realsense_msgs::frame_info>();
        os << "frame_metadata :" << data->frame_metadata << "\n";
        os << "system_time :" << data->system_time << "\n";
        os << "time_stamp_domain :" << data->time_stamp_domain << "\n";
    }
    if (m.isType<realsense_msgs::metadata>())
    {
        auto data = m.instantiate<realsense_msgs::metadata>();
        os << "type : " << data->type << "\n";
        os << "data : " << data->data << "\n";
    }
    if (m.isType<realsense_msgs::motion_intrinsics>())
    {
        auto data = m.instantiate<realsense_msgs::motion_intrinsics>();
        os << "bias_variances : " << data->bias_variances << "\n";
        os << "data : " << data->data << "\n";
        os << "noise_variances : " << data->noise_variances << "\n";
    }
    if (m.isType<realsense_msgs::motion_stream_info>())
    {
        auto data = m.instantiate<realsense_msgs::motion_stream_info>();
        os << "fps : " << data->fps << "\n";
        os << "motion_type : " << data->motion_type << "\n";
        os << "stream_extrinsics : " << data->stream_extrinsics << "\n";
        os << "stream_intrinsics : " << data->stream_intrinsics << "\n";
    }
    if (m.isType<realsense_msgs::occupancy_map>())
    {
        auto data = m.instantiate<realsense_msgs::occupancy_map>();
        os << "accuracy : " << data->accuracy << "\n";
        os << "reserved : " << data->reserved << "\n";
        os << "tiles : " << data->tiles << "\n";
        os << "tile_count : " << data->tile_count << "\n";
    }
    if (m.isType<realsense_msgs::pose>())
    {
        auto data = m.instantiate<realsense_msgs::pose>();
        os << "translation : " << data->translation << "\n";
        os << "rotation : " << data->rotation << "\n";
        os << "velocity : " << data->velocity << "\n";
        os << "angular_velocity : " << data->angular_velocity << "\n";
        os << "acceleration : " << data->acceleration << "\n";
        os << "angular_acceleration : " << data->angular_acceleration << "\n";
        os << "timestamp : " << data->timestamp << "\n";
        os << "system_timestamp : " << data->system_timestamp << "\n";
    }
    if (m.isType<realsense_msgs::stream_extrinsics>())
    {
        auto data = m.instantiate<realsense_msgs::stream_extrinsics>();
        os << "extrinsics : " << data->extrinsics << "\n";
        os << "reference_point_id : " << data->reference_point_id << "\n";
    }
    if (m.isType<realsense_msgs::stream_info>())
    {
        auto data = m.instantiate<realsense_msgs::stream_info>();
        os << "stream_type : " << data->stream_type << "\n";
        os << "fps : " << data->fps << "\n";
        os << "camera_info : " << data->camera_info << "\n";
        os << "stream_extrinsics : " << data->stream_extrinsics << "\n";
        os << "width : " << data->width << "\n";
        os << "height : " << data->height << "\n";
        os << "encoding : " << data->encoding << "\n";
    }
    if (m.isType<realsense_msgs::vendor_data>())
    {
        auto data = m.instantiate<realsense_msgs::vendor_data>();
        os << "name : " << data->name << "\n";
        os << "value : " << data->value << "\n";
    }

    
    return os;
}
struct compression_info
{
    compression_info() : compressed(0), uncompressed(0) {}
    compression_info(const std::tuple<std::string, uint64_t, uint64_t>& t)
    {
        compression_type = std::get<0>(t);
        compressed = std::get<1>(t);
        uncompressed = std::get<2>(t);
    }
    std::string compression_type;
    uint64_t compressed;
    uint64_t uncompressed;
};
struct rosbag_content
{
    rosbag_content(const std::string& file)
    {
        bag.open(file);

        rosbag::View entire_bag_view(bag);

        for (auto&& m : entire_bag_view)
        {
            topics_to_message_types[m.getTopic()].push_back(m.getDataType());
        }

        path = bag.getFileName();
        for (auto rit = path.rbegin(); rit != path.rend(); ++rit)
        {
            if (*rit == '\\' || *rit == '/')
                break;
            file_name += *rit;
        }
        std::reverse(file_name.begin(), file_name.end());

        version = to_string() << bag.getMajorVersion() << "." << bag.getMinorVersion();
        file_duration = get_duration(bag);
        size = 1.0 * bag.getSize() / (1024LL * 1024LL);
        compression_info = bag.getCompressionInfo();
    }

    rosbag_content(const rosbag_content& other)
    {
        bag.open(other.path);
        cache = other.cache;
        file_duration = other.file_duration;
        file_name = other.file_name;
        path = other.path;
        version = other.version;
        size = other.size;
        compression_info = other.compression_info;
        topics_to_message_types = other.topics_to_message_types;
    }
    rosbag_content(rosbag_content&& other)
    {
        other.bag.close();
        bag.open(other.path);
        cache = other.cache;
        file_duration = other.file_duration;
        file_name = other.file_name;
        path = other.path;
        version = other.version;
        size = other.size;
        compression_info = other.compression_info;
        topics_to_message_types = other.topics_to_message_types;

        other.cache.clear();
        other.file_duration = std::chrono::nanoseconds::zero();
        other.file_name.clear();
        other.path.clear();
        other.version.clear();
        other.size = 0;
        other.compression_info.compressed = 0;
        other.compression_info.uncompressed = 0;
        other.compression_info.compression_type = "";
        other.topics_to_message_types.clear();
    }
    std::string instanciate_and_cache(const rosbag::MessageInstance& m)
    {
        auto key = std::make_tuple(m.getCallerId(), m.getDataType(), m.getMD5Sum(), m.getTopic(), m.getTime());
        if (cache.find(key) != cache.end())
        {
            return cache[key];
        }
        std::ostringstream oss;
        oss << m;
        cache[key] = oss.str();
        return oss.str();
    }

    std::chrono::nanoseconds get_duration(const rosbag::Bag& bag)
    {
        rosbag::View only_frames(bag, [](rosbag::ConnectionInfo const* info) {
            std::regex exp(R"RRR(/device_\d+/sensor_\d+/.*_\d+/(image|imu))RRR");
            return std::regex_search(info->topic, exp);
        });
        return std::chrono::nanoseconds((only_frames.getEndTime() - only_frames.getBeginTime()).toNSec());
    }

    std::map<std::tuple<std::string, std::string, std::string, std::string, ros::Time>, std::string> cache;
    std::chrono::nanoseconds file_duration;
    std::string file_name;
    std::string path;
    std::string version;
    double size;
    compression_info compression_info;
    std::map<std::string, std::vector<std::string>> topics_to_message_types;
    rosbag::Bag bag;
};

class Files
{
public:
    int size() const
    {
        return m_files.size();
    }

    rosbag_content& operator[](int index)
    {
        std::lock_guard<std::mutex> lock(mutex);
        return m_files[index];
    }

    void AddFiles(std::vector<std::string> const& files)
    {
        std::lock_guard<std::mutex> lock(mutex);

        for (auto&& file : files)
        {
            try
            {
                if (std::find_if(m_files.begin(), m_files.end(), [file](const rosbag_content& r) { return r.path == file; }) != m_files.end())
                {
                    throw std::runtime_error(to_string() << "File \"" << file << "\" already loaded");
                }
                m_files.emplace_back(file);
            }
            catch (const std::exception& e)
            {
                last_error_message += e.what();
                last_error_message += "\n";
            }
        }
    }
    bool has_errors() const
    {
        return !last_error_message.empty();
    }
    std::string get_last_error()
    {
        return last_error_message;
    }
    void clear_errors()
    {
        last_error_message.clear();
    }
private:
    std::mutex mutex;
    std::vector<rosbag_content> m_files;
    std::string last_error_message;
};


int main(int argc, const char** argv)
{
    Files files;

    if (!glfwInit())
        return -1;
    
    auto window = glfwCreateWindow(1280, 720, "RealSense Rosbag Inspector", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetWindowUserPointer(window, &files);
    glfwSetDropCallback(window, [](GLFWwindow* w, int count, const char** paths)
    {
        auto files = reinterpret_cast<Files*>(glfwGetWindowUserPointer(w));

        if (count <= 0)
            return;

        files->AddFiles(std::vector<std::string>(paths, paths + count));
    });

    ImGui_ImplGlfw_Init(window, true);

    glfwSetScrollCallback(window, [](GLFWwindow * w, double xoffset, double yoffset)
    {
        ImGuiIO& io = ImGui::GetIO();
        io.MouseWheel += yoffset;
    });

    std::map<std::string, int> num_topics_to_show;
    int w, h;
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glfwGetWindowSize(window, &w, &h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        ImGui_ImplGlfw_NewFrame(1);

        ImGuiStyle& style = ImGui::GetStyle();
        style.Colors[ImGuiCol_Header] = ImVec4(0.00f, 0.3f, 1.00f, 1.00f);
        style.Colors[ImGuiCol_Button] = ImVec4(0.22f, 0.47f, 0.67f, 1.00f);

        style.FramePadding.x = 10;
        style.FramePadding.y = 5;
        bool open = true;
        bool* p_open = &open;
        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoSavedSettings;
        ImGui::SetNextWindowSize({ float(w), float(h) }, flags | ImGuiSetCond_FirstUseEver);
        if (ImGui::Begin("Rosbag Inspector", p_open, flags | ImGuiWindowFlags_MenuBar))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Load File..."))
                    {
                        auto ret = noc_file_dialog_open(NOC_FILE_DIALOG_OPEN, "ROS-bag\0*.bag\0", NULL, NULL);
                        if (ret)
                        {
                            files.AddFiles({ ret });
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::BeginChild("Loaded Files", ImVec2(150, 0), true, flags);

            static int selected = 0;
            ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(25.0f / 255, 151.0f / 255, 198.0f / 255, 1.00f));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(37.0f / 255, 40.0f / 255, 48.0f / 255, 1.00f));
            for (int i = 0; i < files.size(); i++)
            {
                if (ImGui::Selectable(files[i].file_name.c_str(), selected == i))
                {
                    selected = i;
                    num_topics_to_show.clear();
                }
            }
            ImGui::PopStyleColor(2);
            ImGui::EndChild();
            ImGui::SameLine();
            if (files.size() > 0)
            {
                ImGui::BeginChild("Bag Content", ImVec2(0, 0), false, flags);

                auto& bag = files[selected];
                std::ostringstream oss;

                ImGui::Text("\t%s", std::string(to_string() << std::left << std::setw(20) << "Path: " << bag.path).c_str());
                ImGui::Text("\t%s", std::string(to_string() << std::left << std::setw(20) << "Bag Version: " << bag.version).c_str());
                ImGui::Text("\t%s", std::string(to_string() << std::left << std::setw(20) << "Duration: " << pretty_time(bag.file_duration)).c_str());
                ImGui::Text("\t%s", std::string(to_string() << std::left << std::setw(20) << "Size: " << bag.size << " MB").c_str());
                ImGui::Text("\t%s", std::string(to_string() << std::left << std::setw(20) << "Compression: " << bag.compression_info.compression_type).c_str());
                ImGui::Text("\t%s", std::string(to_string() << std::left << std::setw(20) << "uncompressed: " << bag.compression_info.compressed).c_str());
                ImGui::Text("\t%s", std::string(to_string() << std::left << std::setw(20) << "compressed: " << bag.compression_info.uncompressed).c_str());
                /*
                compression:  lz4 [119/119 chunks; 25.39%]
                uncompressed: 207.5 MB @ 51.9 MB/s
                compressed:    52.7 MB @ 13.2 MB/s (25.39%)
                */
                if (ImGui::CollapsingHeader("Topics"))
                {
                    for (auto&& topic_to_message_type : bag.topics_to_message_types)
                    {
                        std::string topic = topic_to_message_type.first;
                        std::vector<std::string> messages_types = topic_to_message_type.second;
                        std::ostringstream oss;
                        oss << std::left << std::setw(100) << topic
                            << " " << std::left << std::setw(10) << messages_types.size()
                            << std::setw(6) << std::string(" msg") + (messages_types.size() > 1 ? "s" : "")
                            << ": " << messages_types.front() << std::endl;
                        std::string line = oss.str();
                        auto pos = ImGui::GetCursorPos();
                        ImGui::SetCursorPos({ pos.x + 20, pos.y });
                        if (ImGui::CollapsingHeader(line.c_str()))
                        {
                            rosbag::View messages(bag.bag, rosbag::TopicQuery(topic));
                            int count = 0;
                            num_topics_to_show[topic] = std::max(num_topics_to_show[topic], 10);
                            int max = num_topics_to_show[topic];
                            auto win_pos = ImGui::GetWindowPos();
                            ImGui::SetWindowPos({ win_pos.x + 20, win_pos.y });
                            for (auto&& m : messages)
                            {
                                count++;
                                ImGui::Columns(2, "Message", true);
                                ImGui::Separator();
                                ImGui::Text("Timestamp"); ImGui::NextColumn();
                                ImGui::Text("Content"); ImGui::NextColumn();
                                ImGui::Separator();
                                ImGui::Text(pretty_time(std::chrono::nanoseconds(m.getTime().toNSec())).c_str()); ImGui::NextColumn();
                                ImGui::Text(bag.instanciate_and_cache(m).c_str());
                                ImGui::Columns(1);
                                ImGui::Separator();
                                if (count >= max)
                                {
                                    int left = messages.size() - max;
                                    if (left > 0)
                                    {
                                        ImGui::Text("... %d more messages", left);
                                        ImGui::SameLine();
                                        std::string label = to_string() << "Show More ##" << topic;
                                        if (ImGui::Button(label.c_str()))
                                        {
                                            num_topics_to_show[topic] += 10;
                                        }
                                        else
                                        {
                                            break;
                                        }
                                    }
                                }
                            }
                            ImGui::SetWindowPos(win_pos);
                        }
                    }
                }
                ImGui::EndChild();
            }

            if (files.has_errors())
            {
                ImGui::OpenPopup("Error");
                if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
                {
                    std::string msg = to_string() << "Error: " << files.get_last_error();
                    ImGui::Text(msg.c_str());
                    ImGui::Separator();

                    if (ImGui::Button("OK", ImVec2(120, 0)))
                    {
                        ImGui::CloseCurrentPopup();
                        files.clear_errors();
                    }
                    ImGui::EndPopup();
                }
            }
        }
        ImGui::End();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0,0,0,1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui::Render();
        glfwSwapBuffers(window);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ImGui_ImplGlfw_Shutdown();
    glfwTerminate();
    return 0;
}

#ifdef WIN32
int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow

) {
    main(0, nullptr);
}
#endif