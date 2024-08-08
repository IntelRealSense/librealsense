// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device D435


#include <unit-tests/test.h>
#include "../live-common.h"

using namespace rs2;

constexpr int RECEIVE_FRAMES_TIME = 10;
constexpr float LOW_FPS = 30;
constexpr float HIGH_FPS = 60;

TEST_CASE( "Syncer dynamic FPS - throughput test" )
{
    typedef enum configuration
    {
        IR_ONLY,
        IR_RGB_EXPOSURE,
        STOP
    }configuration;

    class ir_cfg
    {
    public:
        ir_cfg(sensor rgb_sensor,
            sensor ir_sensor,
            std::vector<rs2::stream_profile > rgb_stream_profile,
            std::vector<rs2::stream_profile > ir_stream_profile) : _rgb_sensor(rgb_sensor), _ir_sensor(ir_sensor),
            _rgb_stream_profile(rgb_stream_profile), _ir_stream_profile(ir_stream_profile) {}
         
        void test_configuration(configuration test)
        {
            std::cout << "==============================================" << std::endl;
            std::string cfg = test == IR_ONLY ? "IR Only" : "IR + RGB";
            std::cout << "Configuration: " << cfg << std::endl << std::endl;

            start_streaming(test);
            process_validate_frames(test);
            stop_streaming(test);
            // analysis
            analysis(test, _rgb_frames_arrival_info, "Color");
            analysis(test, _ir_frames_arrival_info);
        }

    private:
        void set_ir_exposure(float value = 18000.0f)
        {
            if (_ir_sensor.supports(RS2_OPTION_EXPOSURE))
                REQUIRE_NOTHROW(_ir_sensor.set_option(RS2_OPTION_EXPOSURE, value));
        }
        void start_streaming(configuration test)
        {
            _ir_sensor.set_option(RS2_OPTION_EXPOSURE, 1);
            _ir_sensor.open(_ir_stream_profile); // ir streams in all configurations
            _ir_sensor.start(_sync);
            if (test == IR_RGB_EXPOSURE)
            {
                _rgb_sensor.open(_rgb_stream_profile);
                _rgb_sensor.start(_sync);
            }
        }
        void stop_streaming(configuration test)
        {
            _ir_sensor.stop();
            _ir_sensor.close();
            if (test == IR_RGB_EXPOSURE)
            {
                _rgb_sensor.stop();
                _rgb_sensor.close();
            }
            set_ir_exposure(1.0f);
        }
        void process_validate_frames(configuration test)
        {
            auto t_start = std::chrono::system_clock::now();
            auto t_end = std::chrono::system_clock::now();

            bool exposure_set = false;
            auto process_frame = [&](const rs2::frame& f)
            {
                std::lock_guard<std::mutex> lock(_mutex);
                auto stream_type = std::string(f.get_profile().stream_name());
                // Only IR fps is relevant for this test, IR1 and IR2 have same fps so it is enough to get only one of them
                if (!(stream_type == "Infrared 1" || stream_type == "Color"))
                    return;
                if (!f.supports_frame_metadata(RS2_FRAME_METADATA_ACTUAL_FPS) || !f.supports_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP))
                    return;
                auto frame_num = f.get_frame_number();
                auto actual_fps = f.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_FPS) / 1000.f;
                auto frame_arrival = f.get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP); // usec
                if (stream_type == "Infrared 1")
                    _ir_frames_arrival_info.push_back({ (float)frame_arrival, actual_fps });
                if (stream_type == "Color")
                    _rgb_frames_arrival_info.push_back({ (float)frame_arrival, actual_fps });
            };

            float delta = (float)std::chrono::duration_cast<std::chrono::seconds>(t_end - t_start).count();
            while (delta < RECEIVE_FRAMES_TIME)
            {
                if (!exposure_set && delta > RECEIVE_FRAMES_TIME / 2)
                {
                    set_ir_exposure(); // set exposure to 18000 after 5 sec to get fps = 30, set to 60000 to get fps = 15
                    exposure_set = true;
                }
                auto fs = _sync.wait_for_frames();
                for (const rs2::frame& ff : fs)
                {
                    process_frame(ff);
                }
                t_end = std::chrono::system_clock::now();
                delta = (float)std::chrono::duration_cast<std::chrono::seconds>(t_end - t_start).count();
            }
        }
        void analysis(configuration test, std::vector <std::pair<float, float>>& frames_arrival_info, std::string stream_type = "Infrared 1")
        {
            if (frames_arrival_info.empty())
                return;
            std::vector<std::pair<float, float>>  vec[2]; // separate frames according to fps (before and after setting exposure)
            std::vector<std::pair<float, float>> extra_frames; // frames with unstable fps are received after changing exposure
            for (auto& frm : frames_arrival_info)
            { 
                // used ratio instead of precised fps because syncer doesn't change fps to exact value when running RGB
                if (frm.second / HIGH_FPS > 0.9)
                    vec[0].push_back(frm);
                else if (frm.second / LOW_FPS > 0.9)
                    vec[1].push_back(frm);
                else
                    extra_frames.push_back(frm);
            }

            int i = 0;
            for (auto& v : vec)
            {
                if (v.size() < 10)
                    continue;
                auto dt = (v.back().first - v.front().first)/ 1000000;
                auto actual_fps = v.front().second;
                float calc_fps = (float)v.size() / dt;
                float fps_ratio = fabs(1 - calc_fps / actual_fps);
                CAPTURE(stream_type, actual_fps, calc_fps, v.size());
                CHECK(fps_ratio < 0.1);
                check_frame_drops(v);
                i += 1;
            }
            CHECK(extra_frames.size() / frames_arrival_info.size() < 0.1);
        }
        void check_frame_drops(std::vector<std::pair<float, float>> frames)
        {
            float prev_frame_time = 0.0f;
            bool first = true;
            int count_drops = 0;
            for (auto& frame : frames)
            {
                auto actual_fps = frame.second;
                float expected_dt_ms = 1000 / actual_fps; // convert sec -> msec
                auto frame_time = frame.first;
                float calc_dt_msec = ((float)frame_time - prev_frame_time)/1000; //convert usec -> msec
                float dt_msec_ratio = calc_dt_msec / expected_dt_ms;
                CAPTURE(expected_dt_ms, calc_dt_msec, dt_msec_ratio, frame_time, prev_frame_time);
                if (!first && dt_msec_ratio > 1.5)
                    count_drops++;
                prev_frame_time = frame_time;
                first = false;
            }
            float count_drops_ratio = (float)count_drops / frames.size();
            CHECK(count_drops_ratio < 0.1);
        }

        sensor _rgb_sensor;
        sensor _ir_sensor;
        std::vector<rs2::stream_profile > _rgb_stream_profile;
        std::vector<rs2::stream_profile > _ir_stream_profile;
        std::vector <std::pair<float, float>> _ir_frames_arrival_info; //{HW timestamp, fps}
        std::vector <std::pair<float, float>> _rgb_frames_arrival_info; //{HW timestamp, fps}
        std::mutex _mutex;
        rs2::syncer _sync;
    };

    // Require at least one device to be plugged in
    auto dev = find_first_device_or_exit();
    auto sensors = dev.query_sensors();
    int width = 848;
    int height = 480;

    // extract IR/RGB sensors
    sensor rgb_sensor;
    sensor ir_sensor;
    std::vector<rs2::stream_profile > rgb_stream_profile;
    std::vector<rs2::stream_profile > ir_stream_profile;
    for (auto& s : sensors)
    {
        auto info = std::string(s.get_info(RS2_CAMERA_INFO_NAME));
        if( info == "RGB Camera" )
            rgb_sensor = s;
        if( info == "Stereo Module" )
            ir_sensor = s;
        auto stream_profiles = s.get_stream_profiles();
        for (auto& sp : stream_profiles)
        {
            auto vid = sp.as<rs2::video_stream_profile>();
            if (!(vid.width() == width && vid.height() == height && vid.fps() == 60)) continue;
            if (sp.stream_type() == RS2_STREAM_COLOR && sp.format() == RS2_FORMAT_RGB8)
                rgb_stream_profile.push_back(sp);
            if (sp.stream_type() == RS2_STREAM_INFRARED && sp.format() == RS2_FORMAT_Y8)
                ir_stream_profile.push_back(sp);
        }
        if (s.supports(RS2_OPTION_GLOBAL_TIME_ENABLED))
            s.set_option(RS2_OPTION_GLOBAL_TIME_ENABLED, 0);
    }
    REQUIRE( rgb_sensor );
    REQUIRE( ir_sensor );
    if (ir_sensor.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
        ir_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, false);
    if (rgb_sensor.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE))
        rgb_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, false);

    // test configurations
    configuration tests[2] = { IR_ONLY, IR_RGB_EXPOSURE }; // {cfg, exposure}
    for (auto& test : tests)
    {
        CAPTURE(test);
        ir_cfg test_cfg(rgb_sensor, ir_sensor, rgb_stream_profile, ir_stream_profile);
        test_cfg.test_configuration(test);
    }
}
