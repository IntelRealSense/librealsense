#include <cmath>
#include "unit-tests-common.h"
#include <librealsense2/hpp/rs_types.hpp>
#include <librealsense2/hpp/rs_frame.hpp>
#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <librealsense2/rsutil.h>
#include <sys/wait.h>
#include <semaphore.h>
#include <fcntl.h>

using namespace rs2;
using namespace std::chrono;

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

TEST_CASE("multicam_streaming", "[code][live]")
{
    // Test will start and stop streaming on 2 devices simultaneously for 10 times, thus testing the named_mutex mechnism.
    rs2::context ctx;
    rs2::device_list list(ctx.query_devices());
    REQUIRE(list.size() >= 2);
    std::vector<std::string> serials_numbers;
    for (auto&& dev : ctx.query_devices())
    {
        serials_numbers.push_back(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
    }
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