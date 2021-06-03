#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <map>
#include "example.hpp"
#include <condition_variable>
#include <librealsense2/hpp/rs_internal.hpp>
#include <librealsense2/hpp/rs_record_playback.hpp>
#include "example-imgui.hpp"    // Include short list of convenience functions for rendering

using namespace std;

struct synthetic_frame
{
    int x, y, bpp;
    std::vector<uint8_t> frame;
};

namespace rs_pointcloud_stitching
{
    const uint64_t  DEF_FRAMES_NUMBER = 100;
    const std::string DEF_OUTPUT_FILE_NAME("frames_data.csv");

    std::string trim(const std::string& str,
        const std::string& whitespace = " \t")
    {
        const auto strBegin = str.find_first_not_of(whitespace);
        if (strBegin == std::string::npos)
            return ""; // no content

        const auto strEnd = str.find_last_not_of(whitespace);
        const auto strRange = strEnd - strBegin + 1;

        return str.substr(strBegin, strRange);
    }

    // Split string into token,  trim unreadable characters
    inline std::vector<std::string> tokenize(std::string line, char separator)
    {
        std::vector<std::string> tokens;

        // Remove trailing characters
        line.erase(line.find_last_not_of("\t\n\v\r ") + 1);

        stringstream ss(line);
        while (ss.good())
        {
            string substr;
            getline(ss, substr, separator);

            tokens.push_back(trim(substr));
        }

        return tokens;
    }

    struct stringify
    {
        std::ostringstream ss;
        template<class T> stringify& operator << (const T& val) { ss << val; return *this; }
        operator std::string() const { return ss.str(); }
    };

    template <typename T>
    inline bool val_in_range(const T& val, const std::initializer_list<T>& list)
    {
        for (const auto& i : list) {
            if (val == i) {
                return true;
            }
        }
        return false;
    }

    inline int parse_number(char const* s, int base = 0)
    {
        char c;
        stringstream ss(s);
        int i;
        ss >> i;

        if (ss.fail() || ss.get(c))
        {
            throw runtime_error(string(string("Invalid numeric input - ") + s + string("\n")));
        }
        return i;
    }

    inline std::string to_lower(const std::string& s)
    {
        auto copy = s;
        std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
        return copy;
    }

    inline rs2_format parse_format(const string str)
    {
        for (int i = RS2_FORMAT_ANY; i < RS2_FORMAT_COUNT; i++)
        {
            if (to_lower(rs2_format_to_string((rs2_format)i)) == str)
            {
                return (rs2_format)i;
            }
        }
        throw runtime_error((string("Invalid format - ") + str + string("\n")).c_str());
    }

    inline rs2_stream parse_stream_type(const string str)
    {
        for (int i = RS2_STREAM_ANY; i < RS2_STREAM_COUNT; i++)
        {
            if (to_lower(rs2_stream_to_string((rs2_stream)i)) == str)
            {
                return static_cast<rs2_stream>(i);
            }
        }
        throw runtime_error((string("Invalid stream type - ") + str + string("\n")).c_str());
    }

    inline int parse_fps(const string str)
    {
        return parse_number(str.c_str());
    }

    inline std::string get_profile_description(const rs2::stream_profile& profile)
    {
        std::stringstream ss;

        ss << profile.stream_name() << ","
            << profile.stream_type() << ","
            << profile.format() << ","
            << profile.fps();

        if (auto vp = profile.as<rs2::video_stream_profile>())
            ss << "," << vp.width() << "," << vp.height();

        ss << std::endl;
        return ss.str().c_str();
    }

    struct stream_request
    {
        rs2_stream  _stream_type;
        rs2_format  _stream_format;
        int         _width;
        int         _height;
        int         _fps;
        int         _stream_idx;
    };

    inline std::ostream& operator<<(std::ostream& os, const stream_request& req)
    {
        return os << "Type: " << req._stream_type << ", Idx: " << req._stream_idx << ", "
            << req._stream_format << ", [" << req._width << "x" << req._height << "], " << req._fps << "fps" << std::endl;
    }

    enum config_params {
        e_stream_type,
        e_res_width,
        e_res_height,
        e_fps,
        e_format,
        e_stream_index
    };

    enum application_stop : uint8_t {
        stop_on_frame_num,
        stop_on_timeout,
        stop_on_user_frame_num,
        stop_on_any
    };

    class PipelineSyncer : public rs2::asynchronous_syncer
    {
    public:
        void operator()(rs2::frame f) const
        {
            invoke(std::move(f));
        }
    };

    class CPointcloudStitcher
    {
    public:
	    CPointcloudStitcher(const std::string& working_dir, const std::string& calibration_file);
        bool Init();
        bool Start();
        void CloseSensors();
        void StopSensors();
        void Run(window& app);

    private:
        bool OpenSensors(std::shared_ptr<rs2::device> dev);
        void frame_callback(rs2::frame frame, const string& serial);
        rs2_intrinsics create_intrinsics(const synthetic_frame& _virtual_frame, const float fov_x, const float fov_y);    // fov_x, fov_y in radians.
        void InitializeVirtualFrames();
        void CreateVirtualDevice();
        void ProjectFramesOnOtherDevice(rs2::frameset frames, const string& from_serial, const string& to_serial);
        void RecordButton(const ImVec2& window_size);
        void DrawTitles(const ImVec2& window_size);
        void StartRecording(const std::string& path);
        void StopRecording();
        void parse_calibration_file(const std::string& config_filename);
        void parse_virtual_device_config_file(const std::string& config_filename);

    private:
	    std::string _working_dir, _calibration_file;
        std::string _left_device;
        std::vector<std::shared_ptr<rs2::device> > _devices;
        std::map<std::string, std::vector<rs2::sensor> >    _active_sensors;
        std::map<std::string, rs2::software_sensor> _active_software_sensors;
        std::map<std::string, std::vector<stream_request> > _wanted_profiles;
        std::map<std::string, PipelineSyncer> _syncer;
        rs2::syncer _soft_sync;
        std::map<std::string, rs2::frame_queue>     _frames;
        std::mutex              _frames_mutex;
        std::condition_variable _frame_cv;
        frames_mosaic _frames_map;
        rs2::colorizer _colorizer;
        rs2::pointcloud _pc;
        std::map< std::string, std::map<std::string, rs2_extrinsics > > _ir_extrinsics;
        rs2::software_device _soft_dev; // Create software-only device
        std::string _serial_vir;
        synthetic_frame _virtual_depth_frame, _virtual_color_frame;
        int _frame_number;
        std::shared_ptr<rs2::recorder> _recorder;
        bool _is_recording;
        std::map<std::string, double> _virtual_dev_params;


        enum frame_id { COLOR1, COLOR_UNITED, COLOR2, DEPTH1, DEPTH_UNITED, DEPTH2};
    };
}
