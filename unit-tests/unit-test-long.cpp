#include <cmath>
#include "unit-tests-common.h"
#include <librealsense2/hpp/rs_types.hpp>
#include <librealsense2/hpp/rs_frame.hpp>
#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <librealsense2/rsutil.h>

using namespace rs2;
using namespace std::chrono;

#ifdef __linux__
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

bool stream(std::string serial_number, sem_t* sem2, bool do_query)
{
    signal(SIGTERM, [](int signum) { std::cout << "SIGTERM: " << getpid() << std::endl; exit(1);});

    rs2::context ctx;
    if (do_query)
    {
        rs2::device_list list(ctx.query_devices());
        bool found_sn(false);
        for (auto&& dev : ctx.query_devices())
        {
            if (dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) == serial_number)
            {
                found_sn = true;
            }
        }
        REQUIRE(found_sn);        
    }
    rs2::pipeline pipe(ctx);
    rs2::config cfg;
    cfg.enable_device(serial_number);
    std::cout << "pipe starting: " << serial_number << std::endl;
    cfg.disable_all_streams();
    cfg.enable_stream(RS2_STREAM_DEPTH, -1, 0, 0, RS2_FORMAT_Z16, 0);
    pipe.start(cfg);
    std::cout << "pipe started: " << serial_number << std::endl;

    double max_milli_between_frames(3000);
    double last_frame_time = duration<double, std::milli>(system_clock::now().time_since_epoch()).count();
    double crnt_time = duration<double, std::milli>(system_clock::now().time_since_epoch()).count();
    bool is_running(true);
    int sem_value;
    while (is_running && crnt_time-last_frame_time < max_milli_between_frames)
    {
        rs2::frameset fs;
        if (pipe.poll_for_frames(&fs))
        {
            last_frame_time = duration<double, std::milli>(system_clock::now().time_since_epoch()).count();            
        }
        crnt_time = duration<double, std::milli>(system_clock::now().time_since_epoch()).count();
        sem_getvalue(sem2, &sem_value);
        is_running = (sem_value == 0);
    }
    pipe.stop();
    return true; // TBD
}

void multiple_stream(std::string serial_number, sem_t* sem, bool do_query)
{
    size_t max_iterations(10);
    pid_t pid;
    std::stringstream sem_name;
    sem_name << "sem_" << serial_number << "_" << 0;
    bool is_running(true);
    int sem_value;
    for (size_t counter=0; counter<10 && is_running; counter++)
    {
        sem_unlink(sem_name.str().c_str());
        sem_t *sem2 = sem_open(sem_name.str().c_str(), O_CREAT|O_EXCL, S_IRWXU, 0);
        CHECK_FALSE(sem2 == SEM_FAILED);
        pid = fork();
        if (pid == 0) // child process
        {
            std::cout << "Start streaming: " << serial_number << " : (" << counter+1 << "/" << max_iterations << ")" << std::endl;
            stream(serial_number, sem2, do_query); //on normal behavior - should block
            break;
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            int status;
            pid_t w = waitpid(pid, &status, WNOHANG);
            bool child_alive(w == 0);
            if (child_alive) {
                sem_post(sem2);

                // Give 2 seconds to quit before kill:
                double start_time = duration<double, std::milli>(system_clock::now().time_since_epoch()).count();
                double crnt_time = duration<double, std::milli>(system_clock::now().time_since_epoch()).count();
                while (child_alive && (crnt_time - start_time < 2000))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    pid_t w = waitpid(pid, &status, WNOHANG);
                    child_alive = (w == 0);
                    crnt_time = duration<double, std::milli>(system_clock::now().time_since_epoch()).count();
                }
                if (child_alive)
                {
                    std::cout << "Failed to start streaming: " << serial_number << std::endl;
                    int res = kill(pid,SIGTERM);
                    pid_t w = waitpid(pid, &status, 0);
                    exit(2);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            else
            {
                std::cout << "Frames did not arrive: " << serial_number << std::endl;
                exit(1);
            }
        }
        sem_getvalue(sem, &sem_value);
        is_running = (sem_value == 0);
    }
    if (pid != 0)
    {
        sem_unlink(sem_name.str().c_str());
    }
    exit(0);
}

TEST_CASE("multicam_streaming", "[live][multicam]")
{
    // Test will start and stop streaming on 2 devices simultaneously for 10 times, thus testing the named_mutex mechnism.
    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<std::string> serials_numbers;
        for (auto&& dev : ctx.query_devices())
        {
            std::string serial(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
            std::string usb_type(dev.get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR));
            if ( usb_type != "3.2")
            {
                WARN("Device " << serial << " with usb_type " << usb_type << " is skipped.");
                continue;
            }
            serials_numbers.push_back(serial);
        }
        REQUIRE(serials_numbers.size() >= 2);
        std::vector<pid_t> pids;
        pid_t pid;

        bool do_query(true);
        std::vector<sem_t*> sems;
        for (size_t idx = 0; idx < serials_numbers.size(); idx++)
        {
            std::stringstream sem_name;
            sem_name << "sem_" << idx;
            sem_unlink(sem_name.str().c_str());
            sems.push_back(sem_open(sem_name.str().c_str(), O_CREAT|O_EXCL, S_IRWXU, 0));
            CHECK_FALSE(sems[idx] == SEM_FAILED);
            pid = fork();
            if (pid == 0) // child
            {
                multiple_stream(serials_numbers[idx], sems[idx], do_query); //on normal behavior - should block
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                pids.push_back(pid);
            }
        }
        if (pid != 0)
        {
            int status0;
            pid_t pid = wait(&status0);
            std::cout << "status0 = " << status0 << std::endl;
            for (auto sem : sems)
            {
                sem_post(sem);
            }
            for (auto pid : pids)
            {
                int status;
                pid_t w = waitpid(pid, &status, WNOHANG);
                std::cout << "status: " << pid << " : " << status << std::endl;
                bool child_alive(w == 0);

                double start_time = duration<double, std::milli>(system_clock::now().time_since_epoch()).count();
                double crnt_time = duration<double, std::milli>(system_clock::now().time_since_epoch()).count();
                while (child_alive && (crnt_time - start_time < 6000))
                {
                    std::cout << "waiting for: " << pid << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    pid_t w = waitpid(pid, &status, WNOHANG);
                    child_alive = (w == 0);
                    crnt_time = duration<double, std::milli>(system_clock::now().time_since_epoch()).count();
                }
                if (child_alive)
                {
                    std::cout << "kill: " << pid << std::endl;
                    int res = kill(pid,SIGTERM);
                    pid_t w = waitpid(pid, &status, 0);
                    std::cout << "status: " << status << ", " << w << std::endl;
                }
            }
            REQUIRE(status0 == 0);
        }
    }
}
#endif // __linux__

struct global_time_test_meta_data : public internal_frame_additional_data
{
    double system_time;

    global_time_test_meta_data(const double &ts, const unsigned long long frame_num, const rs2_timestamp_domain& ts_domain, const rs2_stream& strm, const rs2_format& fmt, double& sts) :
        internal_frame_additional_data(ts, frame_num, ts_domain, strm, fmt)
    {
        system_time = sts;
    }
};

struct time_results
{
    double min_diff, max_diff;
};

void run_sensor(rs2::sensor subdevice, rs2::stream_profile profile, bool enable_gts, int iter, double& max_diff_system_global_time)
{
    const double msec_to_sec = 0.001;
    const int frames_for_fps_measure(profile.fps() * 1);  // max number of frames

    std::vector<global_time_test_meta_data> frames_additional_data;
    double start_time;
    std::condition_variable cv;
    std::mutex m;
    auto first = true;

    REQUIRE_NOTHROW(subdevice.open({ profile }));

    disable_sensitive_options_for(subdevice);
    if (enable_gts)
        REQUIRE_NOTHROW(subdevice.set_option(RS2_OPTION_GLOBAL_TIME_ENABLED, 1));

    REQUIRE_NOTHROW(subdevice.start([&](rs2::frame f)
    {
        double crnt_time(internal::get_time());
        if (first)
        {
            start_time = crnt_time;
        }
        first = false;

        if ((frames_additional_data.size() >= frames_for_fps_measure))
        {
            cv.notify_one();
        }

        if (frames_additional_data.size() < frames_for_fps_measure)
        {
            global_time_test_meta_data data{ f.get_timestamp(),
                f.get_frame_number(),
                f.get_frame_timestamp_domain(),
                f.get_profile().stream_type(),
                f.get_profile().format(),
                crnt_time
            };

            std::unique_lock<std::mutex> lock(m);
            frames_additional_data.push_back(data);
        }
    }));

    CAPTURE(frames_for_fps_measure);

    std::unique_lock<std::mutex> lock(m);
    cv.wait_for(lock, std::chrono::seconds(10), [&] {return ((frames_additional_data.size() >= frames_for_fps_measure)); });
    CAPTURE(frames_additional_data.size());

    auto end = internal::get_time();
    REQUIRE_NOTHROW(subdevice.stop());
    REQUIRE_NOTHROW(subdevice.close());

    lock.unlock();

    auto seconds = (end - start_time)*msec_to_sec;

    CAPTURE(start_time);
    CAPTURE(end);
    CAPTURE(seconds);

    REQUIRE(seconds > 0);

    if (frames_additional_data.size())
    {
        std::stringstream name;
        name << "test_results_" << iter << "_" << enable_gts << ".txt";
        std::ofstream fout(name.str());
        //std::ofstream fout("test_results.txt");
        for (auto data : frames_additional_data)
        {
            fout << std::fixed << std::setprecision(4) << data.system_time << " " << data.timestamp << " " << rs2_timestamp_domain_to_string(data.timestamp_domain) << std::endl;
        }
        fout.close();

        rs2_timestamp_domain first_timestamp_domain(frames_additional_data[0].timestamp_domain);
        auto actual_fps = (double)frames_additional_data.size() / (double)seconds;
        CAPTURE(actual_fps);
        max_diff_system_global_time = 0;
        for (int i = 1; i < frames_additional_data.size(); i++)
        {
            const global_time_test_meta_data& crnt_data = frames_additional_data[i];
            const global_time_test_meta_data& prev_data = frames_additional_data[i - 1];
            if ((crnt_data.timestamp_domain != first_timestamp_domain) ||
                (prev_data.timestamp_domain != first_timestamp_domain))
            {
                continue;
            }
            double system_ts_diff = crnt_data.system_time - prev_data.system_time;
            double ts_diff = crnt_data.timestamp - prev_data.timestamp;
            REQUIRE(system_ts_diff > 0);
            REQUIRE(ts_diff > 0);
            double crnt_diff(system_ts_diff - ts_diff); //big positive difference means system load. big negative means big global time correction.
            max_diff_system_global_time = std::max(max_diff_system_global_time, crnt_diff);
        }
    }
}

TEST_CASE("global-time-start", "[live]") {
    //Require at least one device to be plugged in
    // This test checks if there are delays caused by GlobalTimeStampReader:
    // The test finds the maximal system time difference and frame time difference between every 2 consecutive frames.
    // It then checks that the maximal difference found between system time and frame time is about the same 
    // for runs with global time stamp on and runs with global time stamp off.

    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<sensor> list;
        REQUIRE_NOTHROW(list = ctx.query_all_sensors());
        REQUIRE(list.size() > 0);

        const int frames_before_start_measure = 10;
        const double msec_to_sec = 0.001;
        const int num_of_profiles_for_each_subdevice = 2;
        const float max_diff_between_real_and_metadata_fps = 1.0f;

        // Find profile with greatest fps:
        int max_fps(0);
        for (auto && subdevice : list) {
            if (!subdevice.supports(RS2_OPTION_GLOBAL_TIME_ENABLED))
                continue;
            std::vector<rs2::stream_profile> modes;
            REQUIRE_NOTHROW(modes = subdevice.get_stream_profiles());
            REQUIRE(modes.size() > 0);
            for (auto profile : modes)
            {
                if (max_fps < profile.fps())
                {
                    max_fps = profile.fps();
                }
            }
        }

        for (auto && subdevice : list) {
            if (!subdevice.supports(RS2_OPTION_GLOBAL_TIME_ENABLED))
                continue;
            std::vector<rs2::stream_profile> modes;
            REQUIRE_NOTHROW(modes = subdevice.get_stream_profiles());

            REQUIRE(modes.size() > 0);
            CAPTURE(subdevice.get_info(RS2_CAMERA_INFO_NAME));

            //the test will be done only on the profile with maximal fps:
            for (auto profile : modes)
            {
                if (profile.fps() < max_fps)
                    continue;

                CAPTURE(profile.format());
                CAPTURE(profile.fps());
                CAPTURE(profile.stream_type());
                CAPTURE(profile.stream_index());
                if (auto video = profile.as<video_stream_profile>())
                {
                    CAPTURE(video.width());
                    CAPTURE(video.height());
                }

                double max_diff_system_global_time;
                std::vector<double> all_results_gts_on;
                std::vector<double> all_results_gts_off;
                const int num_of_runs(30);
                for (int i = 0; i < num_of_runs; i++)
                {
                    run_sensor(subdevice, profile, true, i, max_diff_system_global_time);
                    all_results_gts_on.push_back(max_diff_system_global_time);
                    std::cout << "Ran iteration " << i << "/" << num_of_runs << " - gts-ON: max_diff_system_global_time=" << max_diff_system_global_time << std::endl;

                    run_sensor(subdevice, profile, false, i, max_diff_system_global_time);
                    all_results_gts_off.push_back(max_diff_system_global_time);
                    std::cout << "Ran iteration " << i << "/" << num_of_runs << " - gts-OFF: max_diff_system_global_time=" << max_diff_system_global_time << std::endl;
                }
                std::nth_element(all_results_gts_on.begin(), all_results_gts_on.begin() + all_results_gts_on.size() / 2, all_results_gts_on.end());
                double median_diff_gts_on = all_results_gts_on[all_results_gts_on.size() / 2];
                std::nth_element(all_results_gts_off.begin(), all_results_gts_off.begin() + all_results_gts_off.size() / 2, all_results_gts_off.end());
                double median_diff_gts_off = all_results_gts_off[all_results_gts_off.size() / 2];
                CAPTURE(median_diff_gts_on);
                CAPTURE(median_diff_gts_off);

                REQUIRE(median_diff_gts_on > 0.5*median_diff_gts_off);
                REQUIRE(median_diff_gts_on < 2.0*median_diff_gts_off);
                
                break; // Check 1 profile only.
            }
        }
    }
}

std::shared_ptr<std::map<rs2_stream, int>> count_streams_frames(const context& ctx, const sensor& sub, int num_of_frames)
{
    std::shared_ptr<std::condition_variable> cv = std::make_shared<std::condition_variable>();
    std::shared_ptr<std::mutex> m = std::make_shared<std::mutex>();
    std::shared_ptr<std::map<rs2_stream, int>> streams_frames = std::make_shared<std::map<rs2_stream, int>>();

    std::shared_ptr<std::function<void(rs2::frame fref)>>  func;

    std::vector<rs2::stream_profile> modes;
    REQUIRE_NOTHROW(modes = sub.get_stream_profiles());

    auto streaming = false;

    for (auto p : modes)
    {
        if (auto video = p.as<video_stream_profile>())
        {
            {
                if ((video.stream_type() == RS2_STREAM_DEPTH && video.format() == RS2_FORMAT_Z16))
                {
                    streaming = true;

                    REQUIRE_NOTHROW(sub.open(p));

                    func = std::make_shared< std::function<void(frame fref)>>([m, streams_frames, cv](frame fref) mutable
                    {
                        std::unique_lock<std::mutex> lock(*m);
                        auto stream = fref.get_profile().stream_type();
                        if (streams_frames->find(stream) == streams_frames->end())
                            (*streams_frames)[stream] = 0;
                        else
                            (*streams_frames)[stream]++;
                            
                        cv->notify_one();
                    });
                    REQUIRE_NOTHROW(sub.start(*func));
                    break;
                }
            }
        }
    }
    REQUIRE(streaming);


    std::unique_lock<std::mutex> lock(*m);
    cv->wait_for(lock, std::chrono::seconds(30), [&]
    {
        for (auto f : (*streams_frames))
        {
            if (f.second > num_of_frames)
            {
                return true;
            }
        }
        return false;
    });

    lock.unlock();
    if (streaming)
    {
        REQUIRE_NOTHROW(sub.stop());
        REQUIRE_NOTHROW(sub.close());

    }

    return streams_frames;
}

TEST_CASE("test-depth-only", "[live]") {
    //Require at least one device to be plugged in
    // This test checks if once depth is the only profile we ask, it's the only type of frames we get.

    rs2::context ctx;
    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        std::vector<rs2::device> list;
        REQUIRE_NOTHROW(list = ctx.query_devices());
        REQUIRE(list.size() > 0);

        auto dev = std::make_shared<device>(list.front());
        auto sensors = dev->query_sensors();

        sensor d_sensor;
        for (sensor& elem : sensors)
        {
            if (elem.is<depth_sensor>())
            {
                d_sensor = elem;                
                break;
            }
        }
        REQUIRE(d_sensor);

        std::shared_ptr<std::map<rs2_stream, int>> streams_frames = count_streams_frames(ctx, d_sensor, 10);
        std::cout << "streams_frames.size: " << streams_frames->size() << std::endl;
        for (auto stream_num : (*streams_frames))
        {
            std::cout << "For stream " << stream_num.first << " got " << stream_num.second << " frames." << std::endl;
        }
        REQUIRE(streams_frames->find(RS2_STREAM_DEPTH) != streams_frames->end());
        REQUIRE(streams_frames->size() == 1);
    }
}
