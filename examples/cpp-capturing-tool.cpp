// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

////////////////////////////////////////////////////////////
// librealsense motion-module sample - accessing IMU data //
////////////////////////////////////////////////////////////

// First include the librealsense C++ header file
#include <librealsense/rs.hpp>
#include <cstdio>
#include <mutex>
#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <thread>
#include <deque>
#include <algorithm>
#include <fstream>
#include <atomic>
#include <condition_variable>
#include<signal.h>
#include <functional>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

using namespace std;

#ifdef WIN32
#define DOUBLE_SLASH "\\" // Double slash for Windows convention 
#else
#define DOUBLE_SLASH "//" // Double slash for Linux convention 
#endif

const int LEADING_ZEROS = 5;
const char* OUTPUT_FOLDER = "output";
const char* FRAMES_FOLDER = "fisheye";

// Moveable concurrent queue
template <class T>
class concurrent_queue
{
public:
    void push_back_data(T data)
    {
        lock_guard<mutex> lock(mtx);

        data_queue.push_back(move(data));
    }

    bool pop_back_data(T& data)
    {
        lock_guard<mutex> lock(mtx);

        if (!data_queue.size())
            return false;

        data = move(data_queue.back());
        data_queue.pop_back();

        return true;
    }

    bool pop_front()
    {
        lock_guard<mutex> lock(mtx);

        if (!data_queue.size())
            return false;

        data_queue.pop_front();

        return true;
    }

    bool pop_front_data(T& data)
    {
        lock_guard<mutex> lock(mtx);

        if (!data_queue.size())
            return false;

        data = move(data_queue.front());
        data_queue.pop_front();

        return true;
    }

    size_t size()
    {
        lock_guard<mutex> lock(mtx);

        return data_queue.size();
    }

private:
    std::deque<T> data_queue;
    std::mutex mtx;
};

#ifdef WIN32
#include <Windows.h>
#endif

concurrent_queue<string> str_fisheye_ts_queue;

// Add frame data to 'str_fisheye_ts_queue'
std::function<void(string, double, unsigned long long int /*frame_number*/)> save_frame_data = [&](string filename, double ts, unsigned long long int /*frame_number*/){
    stringstream ts_data;
    ts_data << filename << /*((frame_number) ? string("," + to_string(frame_number) + ",") : "") <<*/ "," << setprecision(10) << ts << endl;
    str_fisheye_ts_queue.push_back_data(ts_data.str());
};

// Save frame to png file
std::function<void(string, rs::frame&)> save_frame = [&](string file_full_path, rs::frame& frame){
    stbi_write_png(file_full_path.data(),
        frame.get_width(), frame.get_height(),
        1,
        frame.get_data(),
        frame.get_width());
};

bool debug_mode = false;
atomic<int> frame_counter(0);
std::function<void(rs::frame&)> save_motion_data_and_frame = [&](rs::frame& frame){
    stringstream filename;
    stringstream path;
    stringstream file_full_path;
    path << OUTPUT_FOLDER << DOUBLE_SLASH << FRAMES_FOLDER << DOUBLE_SLASH;
    filename << "image_" << setw(LEADING_ZEROS) << setfill('0') << ++frame_counter;
    file_full_path << path.str() << filename.str() << ".png";

    auto frame_number = 0;
    if (debug_mode)
        frame_number = frame.get_frame_number();

    save_frame_data(filename.str(), frame.get_timestamp(), frame_number);
    save_frame(file_full_path.str(), frame);
};

struct ts_and_frame_counter{
    ts_and_frame_counter(){}
    ts_and_frame_counter(int cnt, double ts) : frame_counter(cnt), timestamp(ts) {}
    int frame_counter;
    double timestamp;
};

// record class provide preallocated memory
class record{
public:
    record(int num_of_buffers, int size_of_each_buffer_in_byte)
        :save_index(0), mem_cpy_index(0), each_buffer_size(size_of_each_buffer_in_byte), max_buffers(num_of_buffers)
    {
        buffers.resize(num_of_buffers);
        for (auto& elem : buffers)
        {
            elem.first = new char[size_of_each_buffer_in_byte];
        }

        //  save frame data to a png file
        thread grab([this](){
            while (true)
            {
                std::unique_lock<std::mutex> lk(this->mutex);
                rec_cv.wait(lk, [&]{return (action); });
                if (save_index == max_buffers)
                    save_index = 0;

                stringstream filename;
                stringstream path;
                stringstream file_full_path;
                path << OUTPUT_FOLDER << "/" << FRAMES_FOLDER << "/";
                filename << "image_" << setw(LEADING_ZEROS) << setfill('0') << ++frame_counter;
                file_full_path << path.str() << filename.str() << ".png";

                action = false;
                lk.unlock();
                save_frame_data(filename.str(), buffers[save_index].second.timestamp, buffers[save_index].second.frame_counter);
                stbi_write_png(file_full_path.str().data(),
                    640, 480,
                    1,
                    buffers[save_index++].first,
                    640);

                ++num_of_saved_frames;

            }
        });
        grab.detach();
    }

    ~record()
    {   
        if (!stop_called)
            stop();

        for (auto& elem : buffers)
        {
            delete[] elem.first;
        }
    }

    void stop()
    {
        atomic<unsigned long long> last_saved_frame_number;
        unsigned long long counter;
        while ((save_index+1) != mem_cpy_index)
        {
            counter = num_of_mem_cpy - num_of_saved_frames;
            if (counter != last_saved_frame_number)
            {
                printf("Saving frame %d.\n", counter);
                last_saved_frame_number = counter;
            }

            {
                std::lock_guard<std::mutex> lk(mutex);
                action = true;
            }
            rec_cv.notify_one();
        }
        stop_called = true;
    }

    bool mem_cpy(const void* src, int cnt, double ts)
    {
        std::unique_lock<std::mutex> lk(callback_mtx);

        ts_and_frame_counter data(cnt, ts);
        buffers[mem_cpy_index].second = data;
        memcpy(buffers[mem_cpy_index].first, src, each_buffer_size);
        {
            std::lock_guard<std::mutex> lk(mutex);
            action = true;
        }
        rec_cv.notify_one();
        ++num_of_mem_cpy;

        if ((++mem_cpy_index) >= max_buffers)
            mem_cpy_index = 0;

        return true;
    }

private:
    std::atomic<bool> keep_thread_allive = true;
    std::atomic<bool> stop_called = false;
    std::mutex callback_mtx;
    bool action = false;
    std::mutex mutex;
    std::condition_variable rec_cv;
    deque<std::pair<char*, ts_and_frame_counter>> buffers;
    int each_buffer_size;
    int max_buffers;
    unsigned long long mem_cpy_index;
    unsigned long long save_index;
    unsigned long long num_of_saved_frames = 0;
    unsigned long long num_of_mem_cpy = 0;
};

int main(int argc, char* argv[]) try
{
    // Handle user arguments
    std::vector<std::pair<string, string>> args;
    for (int i = 1; i < argc; ++i)
    {
        string temp(argv[i]);
        auto index = temp.find("=");
        auto k = temp.substr(0, index);
        auto v = temp.substr(index + 1, temp.length());
        args.push_back(std::make_pair(k, v));
    }


    // Create a context object. This object owns the handles to all connected realsense devices.
    rs::context ctx;

    std::cout << "There are "<< ctx.get_device_count() << " connected RealSense devices.\n";

    if (ctx.get_device_count() == 0) return EXIT_FAILURE;

    // This tutorial will access only a single device, but it is trivial to extend to multiple devices
    rs::device * dev = ctx.get_device(0);
    printf("\nUsing device 0, an %s\n", dev->get_name());
	

    const unsigned max_frame_queue_size = 18;

    atomic<int> action(1);
    concurrent_queue<rs::motion_data> motion_data_queue;
    concurrent_queue<pair<rs_event_source,string>> str_motion_data_queue;

    concurrent_queue<rs::timestamp_data> timestamp_data_queue;
    concurrent_queue<string> str_timestamp_data_queue;


    std::mutex take_frame_mutex;
    std::condition_variable take_frame_cv;

    int take_frame = 0;
    concurrent_queue<rs::frame> frames_queue;
    atomic<int> stop_streaming(0);

    auto motion_callback = [&](rs::motion_data entry)
    {
        if (action && !debug_mode)
            motion_data_queue.push_back_data(entry);
        else if (debug_mode && !stop_streaming)
            motion_data_queue.push_back_data(entry);
    };


    auto timestamp_callback = [&](rs::timestamp_data entry)
    {
        if (action && !debug_mode && entry.source_id == RS_EVENT_IMU_MOTION_CAM)
            timestamp_data_queue.push_back_data(entry);
        else if (debug_mode && !stop_streaming)
            timestamp_data_queue.push_back_data(entry);

    };


    // Make motion-tracking available
    if (dev->supports(rs::capabilities::motion_events))
    {
        dev->enable_motion_tracking(motion_callback, timestamp_callback);
    }

    record rec(6000, 640 * 480); // preallocated moveable class


    std::mutex callback_mutex;
    if (dev->supports(rs::capabilities::fish_eye))
    {
        dev->set_frame_callback(rs::stream::fisheye, [&](rs::frame frame)
        {
            std::lock_guard<std::mutex> lk(callback_mutex);
            if (frames_queue.size() == max_frame_queue_size)
                frames_queue.pop_front();

            if (action && !debug_mode)
            {
                frames_queue.push_back_data(std::move(frame));
            }
            else if (debug_mode && !stop_streaming)
            {
                rec.mem_cpy(frame.get_data(), frame.get_frame_number(), frame.get_timestamp());
            }
        });

        dev->enable_stream(rs::stream::fisheye, 640, 480, rs::format::raw8, 60);
    }

    if (dev->supports_option(rs::option::fisheye_strobe))
    {
        dev->set_option(rs::option::fisheye_strobe, 1);
    }

    // set exposure and gain from the prompt
    for (auto& kv : args)
    {
        if (!kv.first.compare("e"))
        {
            std::cout << "exposure value: " << kv.second << std::endl;
            dev->set_option(rs::option::fisheye_exposure, stoi(kv.second));
        }
        else if (!kv.first.compare("g"))
        {
            std::cout << "gain value: " << kv.second << std::endl;
            dev->set_option(rs::option::fisheye_gain, stoi(kv.second));
        }
        else if (!kv.first.compare("debug"))
        {
            debug_mode = true;
        }

    }



    stringstream ss;
    ss << "mkdir " << OUTPUT_FOLDER;
    system(ss.str().c_str());
    ss << DOUBLE_SLASH << FRAMES_FOLDER;
    system(ss.str().c_str());

    // Pop 'frame' from the 'frames_queue' and save it into a png file
    // this thread works only in regular mode
    thread thread_frame_to_png_file([&](){
        if (debug_mode)
            return;

        while (true)
        {
            rs::frame frame;
            std::unique_lock<std::mutex> lk(take_frame_mutex);
            bool sts = false;
            take_frame_cv.wait(lk, [&]{return ((!action) || (take_frame && (sts = frames_queue.pop_back_data(frame)))); });

            if (!action && frames_queue.size() == 0)
                return;

            if (!action && frames_queue.size())
            {
                while (frames_queue.size())
                {
                    frames_queue.pop_front();
                }

                lk.unlock();
                break;
            }

            if (!sts)
            {
                lk.unlock();
                continue;
            }


            save_motion_data_and_frame(frame);
                        
            --take_frame;
            lk.unlock();
        }
    });

    // Pop 'timestamp_data' from 'timestamp_data_queue' and push a formated string into 'str_timestamp_data_queue'
    // Operates only in debug_mode!
    thread thread_timestamp_data_to_str([&](){
        if (!debug_mode)
            return;

        while (action)
        {
            std::stringstream ss;
            rs::timestamp_data timestamp_data;
            auto sts = timestamp_data_queue.pop_front_data(timestamp_data);

            if (sts /*&& motion_data.is_valid*/)
            {
                ss << timestamp_data.frame_number << "," << timestamp_data.timestamp << endl;
                str_timestamp_data_queue.push_back_data(ss.str());
            }
        }
    });

    // Pop 'motion_data' from 'motion_data_queue' and push a formated string to 'str_motion_data_queue'
    thread thread_motion_data_to_str([&](){
        while (action)
        {
            std::stringstream ss;
            rs::motion_data motion_data;
            auto sts = motion_data_queue.pop_front_data(motion_data);

            if (sts /*&& motion_data.is_valid*/)
            {
                if (motion_data.timestamp_data.source_id == RS_EVENT_IMU_ACCEL ||
                    motion_data.timestamp_data.source_id == RS_EVENT_IMU_GYRO)
                {
                    ss << std::setprecision(10) << (motion_data.timestamp_data.timestamp) << "," << motion_data.axes[0] <<
                        "," << motion_data.axes[1] << "," << motion_data.axes[2] << endl;
                    str_motion_data_queue.push_back_data(make_pair(motion_data.timestamp_data.source_id, ss.str()));
                }
            }
        }
    });

    ss.str("");
    fstream accel;
    ss << OUTPUT_FOLDER << "/accel.txt";
    accel.open(ss.str(), std::fstream::out);
    fstream gyro;
    ss.str("");
    ss << OUTPUT_FOLDER << "/gyro.txt";
    gyro.open(ss.str(), std::fstream::out);
    // Pop the formated string of 'motion_data' from 'str_motion_data_queue' and save it into accel.txt/gyro.txt
    thread thread_str_motion_data_to_file([&](){
        while (action)
        {
            pair<rs_event_source, string> p;
            auto sts = str_motion_data_queue.pop_front_data(p);

            if (sts)
            {
                if (p.first == RS_EVENT_IMU_GYRO)
                {
                    gyro << p.second;
                }
                else if (p.first == RS_EVENT_IMU_ACCEL)
                {
                    accel << p.second;
                }
            }
        }
    });


    fstream fe;
    if (debug_mode)
    {
        ss.str("");
        ss << OUTPUT_FOLDER << "/fe_events_debug.txt";
        fe.open(ss.str(), std::fstream::out);
    }
    // Pop the formated string of 'timestamp_data' from 'str_timestamp_data_queue' and save it into 'fe_events_debug.txt'
    // Operates only in debug_mode!
    thread thread_str_fe_data_to_file([&](){
        if (!debug_mode)
            return;

        while (action)
        {
            string str;
            auto sts = str_timestamp_data_queue.pop_front_data(str);

            if (sts)
            {
                fe << str;
            }
        }
    });

    fstream fisheye_ts;
    ss.str("");
    ss << OUTPUT_FOLDER << "/fisheye_timestamps.txt";
    fisheye_ts.open(ss.str(), std::fstream::out);
    // Pop the formated string of fisheye frame timestamp from 'str_fisheye_ts_queue' and save it into 'fisheye_timestamps.txt'
    thread thread_str_fisheye_ts_to_file([&](){
        while (action)
        {
            string p;
            auto sts = str_fisheye_ts_queue.pop_front_data(p);

            if (sts)
            {
                fisheye_ts << p;
            }
        }
    });

#ifdef WIN32
    auto takeFrameEvent = CreateEvent(NULL, false, false, L"TakeFrame");
#endif
    // Take frame by request (by signaling 'takeFrameEvent' handle) and notify 'thread_frame_to_png_file' thread
    // takeFrameEvent handle is signaled by an extern application (takeFrame.exe)
    // this thread works only in regular mode
    thread thread_take_frame([&](){
        if (debug_mode)
            return;

        while (action)
        {
#ifdef WIN32
            WaitForSingleObject(takeFrameEvent, INFINITE);
#else
            return;
#endif
            if (!action)
                break;

            {
                std::lock_guard<std::mutex> lk(take_frame_mutex);
                ++take_frame;
            }
            take_frame_cv.notify_one();
            cout << "New frame has been saved.\n";
        }
    });

#ifdef WIN32
    auto exitEvent = CreateEvent(NULL, FALSE, FALSE, L"Exit");
    auto exitedEvent = CreateEvent(NULL, FALSE, FALSE, L"Exited");
#else
    std::mutex exit_mutex;
    std::condition_variable exit_cv;
    int need_exit = 0;
#endif


    // Close the application by signal exitEvent handle
    // exitEvent handle is signaled by an extern application (exit.exe)
    thread thread_exit([&](){
#ifdef WIN32
            WaitForSingleObject(exitEvent, INFINITE);
#else
        std::unique_lock<std::mutex> lk(exit_mutex);
        exit_cv.wait(lk, [&]{return need_exit; });
        lk.unlock();
#endif
            cout << "\nExiting please wait...\n";

            if (debug_mode)
            {
                stop_streaming = 1;
                rec.stop();
            }
            else
            {
                {
                    std::lock_guard<std::mutex> lk(take_frame_mutex);
                    action = 0;
                }
                take_frame_cv.notify_one();
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }


            gyro.close();
            accel.close();
            if (debug_mode)
                fe.close();

            fisheye_ts.close();
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
#ifdef WIN32
            SetEvent(exitedEvent);
#endif
            exit(0);
    });

    // Start generating motion-tracking data
    dev->start(rs::source::all_sources);

    if (!debug_mode)
        cout << "\nPress RETURN for taking fish-eye frame or type 'exit' for quit.\n\n";
    else
        cout << "\nDEBUG MODE! type 'exit' for quit.\n\n"; // Video Mode

    while (true)
    {
        fflush(NULL);
        string line;
        getline(std::cin, line);

        if (!debug_mode && !line.compare(""))
        {
            {
                std::lock_guard<std::mutex> lk(take_frame_mutex);
                ++take_frame;
            }
            take_frame_cv.notify_one();
            cout << "New frame has been saved.\n";
        }
        else if (!line.compare("exit"))
        {
#ifdef WIN32
            SetEvent(exitEvent);
#else
            {
                std::lock_guard<std::mutex> lk(exit_mutex);
                need_exit = 1;
            }
            exit_cv.notify_one();
#endif
            break;
        }
    }


    thread_exit.join();

    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    // Method calls against librealsense objects may throw exceptions of type rs::error
    printf("rs::error was thrown when calling %s(%s):\n", e.get_failed_function().c_str(), e.get_failed_args().c_str());
    printf("    %s\n", e.what());
    return EXIT_FAILURE;
}
catch(...)
{
    printf("Unhandled excepton occured'n");
}

