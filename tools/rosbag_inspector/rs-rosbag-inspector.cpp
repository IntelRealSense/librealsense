#include <iostream>
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
#include "../../third-party/realsense-file/rosbag/msgs/realsense_msgs/StreamInfo.h"
#include "../../third-party/realsense-file/rosbag/msgs/sensor_msgs/CameraInfo.h"
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
//#include "../common/rendering.h"
//#include <imgui.h>
//#include "imgui_impl_glfw.h"
#include "../../src/media/ros/ros_file_format.h"

#include "../../third-party/stb_easy_font.h"
#include "std_msgs/String.h"
#include "realsense_msgs/ImuIntrinsic.h"
#include "sensor_msgs/TimeReference.h"
using namespace std::chrono;
using namespace rosbag;
using namespace std;

std::ostream& operator<<(std::ostream& os, rosbag::compression::CompressionType c)
{
	switch (c)
	{
	case CompressionType::Uncompressed: os << "Uncompressed"; break;
	case CompressionType::BZ2: os << "BZ2"; break;
	case CompressionType::LZ4: os << "LZ4"; break;
	default: break;
	}
	return os;
}

inline void draw_text(int x, int y, const char * text)
{
    char buffer[60000]; // ~300 chars
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer);
    glDrawArrays(GL_QUADS, 0, 4 * stb_easy_font_print((float)x, (float)(y - 7), (char *)text, nullptr, buffer, sizeof(buffer)));
    glDisableClientState(GL_VERTEX_ARRAY);
}

class Window
{
public:

    Window(uint32_t width, uint32_t height, std::string title) : _width(width), _height(height)
    {
        glfwInit();
        _window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
        glfwMakeContextCurrent(_window);
        glfwSetWindowUserPointer(_window, this);
        glfwSetDropCallback(_window, [](GLFWwindow* w, int count, const char** paths)
        {
            auto dis = reinterpret_cast<Window*>(glfwGetWindowUserPointer(w));

            if (count <= 0)
                return;

            dis->OnFileDropped(std::vector<std::string>(paths, paths + count));
        });
    }

    operator bool()
    {
        glPopMatrix();
        glfwSwapBuffers(_window);

        auto res = !glfwWindowShouldClose(_window);

        glfwPollEvents();
        glfwGetFramebufferSize(_window, &_width, &_height);
        
        // Clear the framebuffer
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, _width, _height);

        // Draw the images
        glPushMatrix();
        glfwGetWindowSize(_window, &_width, &_height);
        glOrtho(0, _width, _height, 0, -1, +1);

        return res;
    }

    ~Window()
    {
        glfwDestroyWindow(_window);
        glfwTerminate();
    }

    std::function<void(std::vector<std::string>)> OnFileDropped;
private:

    GLFWwindow* _window;
    int _width, _height;
};

std::string pretty_time(nanoseconds d)
{
    auto hhh = duration_cast<hours>(d);
    d -= hhh;
    auto mm = duration_cast<minutes>(d);
    d -= mm;
    auto ss = duration_cast<seconds>(d);
    d -= ss;
    auto ms = duration_cast<milliseconds>(d);

    std::ostringstream stream;
    stream << std::setfill('0') << std::setw(3) << hhh.count() << ':' <<
        std::setfill('0') << std::setw(2) << mm.count() << ':' <<
        std::setfill('0') << std::setw(2) << ss.count() << '.' <<
        std::setfill('0') << std::setw(3) << ms.count();
    return stream.str();
}

std::string print_messages_topics(const std::map<std::string, std::vector<std::string>>& messages)
{
	std::ostringstream oss;
	for (auto&& m : messages)
	{
		oss << std::left << std::setw(50) << m.first
			<< " " << std::left << std::setw(10) << m.second.size() << std::setw(6) << std::string(" msg") + (m.second.size() > 1 ? "s" : "")
			<< ": " << m.second.front() << std::endl;
	}
	return oss.str();
}

nanoseconds get_duration(const Bag& bag)
{
	View entire_bag_view(bag, librealsense::FrameQuery());
	
	return nanoseconds((entire_bag_view.getEndTime() - entire_bag_view.getBeginTime()).toNSec());
}

std::string print_known_msgs(const Bag& bag)
{
	std::ostringstream oss;
	RegexTopicQuery all_non_frame_topics(librealsense::to_string() << R"RRR(/device_\d+/sensor_\d+/.*_\d+)RRR" << "/(" << RegexTopicQuery::data_msg_types() << ")", true);
	View entire_bag_view(bag, all_non_frame_topics);
	for (auto&& m : entire_bag_view)
	{
		oss << "Topic : " << m.getTopic() << std::endl;
		oss << "Type  : " << m.getDataType() << std::endl;
		if (m.isType<std_msgs::UInt32>())
		{
			oss << "  Value : " << m.instantiate<std_msgs::UInt32>()->data << std::endl;
		}
		if (m.isType<diagnostic_msgs::KeyValue>())
		{
			oss << "  Key   : " << m.instantiate<diagnostic_msgs::KeyValue>()->key << std::endl;
			oss << "  Value : " << m.instantiate<diagnostic_msgs::KeyValue>()->value << std::endl;
		}
		if (m.isType<realsense_msgs::StreamInfo>())
		{
			oss << "  Encoding       : " << m.instantiate<realsense_msgs::StreamInfo>()->encoding << std::endl;
			oss << "  FPS            : " << m.instantiate<realsense_msgs::StreamInfo>()->fps << std::endl;
			oss << "  Is Recommended : " << (m.instantiate<realsense_msgs::StreamInfo>()->is_recommended ? "True" : "False") << std::endl;
		}
		if (m.isType<sensor_msgs::CameraInfo>())
		{
			oss << "  Width      : " << m.instantiate<sensor_msgs::CameraInfo>()->width << std::endl;
			oss << "  Height     : " << m.instantiate<sensor_msgs::CameraInfo>()->height << std::endl;
			oss << "  Intrinsics : " << std::endl;
			oss << "    Focal Point      : " << m.instantiate<sensor_msgs::CameraInfo>()->K[0] << ", " << m.instantiate<sensor_msgs::CameraInfo>()->K[4] << std::endl;
			oss << "    Principal Point  : " << m.instantiate<sensor_msgs::CameraInfo>()->K[2] << ", " << m.instantiate<sensor_msgs::CameraInfo>()->K[5] << std::endl;
			oss << "    Coefficients     : "
				<< m.instantiate<sensor_msgs::CameraInfo>()->D[0] << ", "
				<< m.instantiate<sensor_msgs::CameraInfo>()->D[1] << ", "
				<< m.instantiate<sensor_msgs::CameraInfo>()->D[2] << ", "
				<< m.instantiate<sensor_msgs::CameraInfo>()->D[3] << ", "
				<< m.instantiate<sensor_msgs::CameraInfo>()->D[4] << std::endl;
			oss << "    Distortion Model : " << m.instantiate<sensor_msgs::CameraInfo>()->distortion_model << std::endl;

		}
		if (m.isType<realsense_msgs::ImuIntrinsic>())
		{

		}
		if (m.isType<sensor_msgs::Image>())
		{

		}
		if (m.isType<sensor_msgs::Imu>())
		{

		}
		if (m.isType<sensor_msgs::TimeReference>())
		{
			oss << "  Header        : " << m.instantiate<sensor_msgs::TimeReference>()->header << std::endl;
			oss << "  Source        : " << m.instantiate<sensor_msgs::TimeReference>()->source << std::endl;
			oss << "  TimeReference : " << m.instantiate<sensor_msgs::TimeReference>()->time_ref << std::endl;
		}
		if (m.isType<geometry_msgs::Transform>())
		{
			oss << "  Extrinsics : " << std::endl;
			oss << "    Rotation    : " << m.instantiate<geometry_msgs::Transform>()->rotation << std::endl;
			oss << "    Translation : " << m.instantiate<geometry_msgs::Transform>()->translation << std::endl;
		}
		oss << std::endl;
	}
	return oss.str();
}

std::string InspectBag(const std::string& f)
{
	std::ostringstream oss;
	Bag bag(f);

    View entire_bag_view(bag);

    std::map<std::string, std::vector<std::string>> messages;
    for (auto&& m : entire_bag_view)
    {
        messages[m.getTopic()].push_back(m.getDataType());
    }
    oss << "========================================================\n";
    oss << std::left << std::setw(20) << "Path: " << bag.getFileName() << std::endl;
    oss << std::left << std::setw(20) << "Bag Version: " << bag.getMajorVersion() << "." << bag.getMinorVersion() << std::endl;
	oss << std::left << std::setw(20) << "Duration: " << pretty_time(get_duration(bag)) << std::endl;
	oss << std::left << std::setw(20) << "Size: " << 1.0 * bag.getSize() / (1024LL * 1024LL) << " MB" << std::endl;
	oss << std::left << std::setw(20) << "Compression: " << bag.getCompression() << std::endl;

	oss << "Topics:" << std::endl;
	oss << print_messages_topics(messages);
	oss << std::endl;
	oss << print_known_msgs(bag);
	oss << std::endl << std::endl << std::endl;
	return oss.str();
}
int main(int argc, const char** argv)
{
    Window w(100u, 100u, "ROSBAG Inspector");

    w.OnFileDropped = [](std::vector<std::string> files)
    {
        for (auto&& file : files)
        {
            try
            {
				std::string file_info = InspectBag(file);
				std::cout << file_info << std::endl;
				std::ofstream results(file + ".info");
				results << file_info;
            }
            catch (const std::exception& e)
            {
                std::cout << e.what() << std::endl;
            }
        }
    };
    while (w)
    {
        draw_text(0, 10, "Drag bag files here...");
        std::this_thread::sleep_for(milliseconds(10));
    }
    return 0;
}