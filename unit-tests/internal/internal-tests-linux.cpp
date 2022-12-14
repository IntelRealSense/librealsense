#include "catch/catch.hpp"
#include <thread>
#include <string>
#include <linux/backend-v4l2.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>

using namespace librealsense::platform;

TEST_CASE("named_mutex_threads", "[code]")
{
    class locker_test
    {
        std::mutex _m0, _m1;
        bool _go_0, _go_1;
        std::condition_variable _cv0, _cv1;
        bool _actual_test;

        std::string _device_path;
        public:
            locker_test(const std::string& device_path, bool actual_test):
            _go_0(false),
            _go_1(false),
            _actual_test(actual_test),
            _device_path(device_path)
            {
            };

            void func_0()
            {
                {
                    std::unique_lock<std::mutex> lk(_m0);
                    _cv0.wait(lk, [this]{return _go_0;});
                }
                named_mutex mutex0(_device_path, 0);
                if (_actual_test)
                {
                    mutex0.lock();
                }
                {
                    std::unique_lock<std::mutex> lk(_m0);
                    _go_0 = false;
                }
            }
            void func_1()
            {
                {
                    std::unique_lock<std::mutex> lk(_m1);
                    _cv1.wait(lk, [this]{return _go_1;});
                }
                named_mutex mutex1(_device_path, 0);
                mutex1.lock();
                {
                    std::lock_guard<std::mutex> lk_gm(_m1);
                    _go_1 = false;
                }
                _cv1.notify_all();
                {
                    std::unique_lock<std::mutex> lk(_m1);
                    _cv1.wait(lk, [this]{return _go_1;});
                }
            }

            void run_test()
            {
                bool test_ok(false);

                std::thread t0 = std::thread([this](){func_0();});
                std::thread t1 = std::thread([this](){func_1();});
                // Tell Thread 1 to lock named_mutex.
                {
                    std::lock_guard<std::mutex> lk_gm(_m1);
                    _go_1 = true;
                }
                _cv1.notify_all();
                // Wait for Thread 1 to acknowledge lock.
                {
                    std::unique_lock<std::mutex> lk(_m1);
                    _cv1.wait(lk, [this]{return !_go_1;});
                }
                // Tell Thread 0 to lock named_mutex.
                {
                    std::lock_guard<std::mutex> lk_gm(_m0);
                    _go_0 = true;
                }
                _cv0.notify_all();
                // Give Thread 2 seconds opportunity to lock.
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                {
                    std::lock_guard<std::mutex> lk_gm(_m0);
                    // test_ok if thread 0 didn't manage to change value of _go_0.
                    test_ok = (_go_0 == _actual_test);
                }
                // Tell thread 1 to finish and release named_mutex.
                {
                    std::lock_guard<std::mutex> lk_gm(_m1);
                    _go_1 = true;
                }
                _cv1.notify_all();
                t1.join();
                t0.join();
                REQUIRE(test_ok);
            }
    };
    std::string device_path("./named_mutex_test");
    int fid(-1);
    if( access( device_path.c_str(), F_OK ) == -1 ) 
    {
        fid = open(device_path.c_str(), O_CREAT | O_RDWR, 0666);
        close(fid);
    }
    bool actual_test;
    SECTION("self-validation")
    {
        actual_test = false;
    }
    SECTION("actual-test")
    {
        actual_test = true;
    }

    locker_test _test(device_path, actual_test);
    _test.run_test();
}

TEST_CASE("named_mutex_processes", "[code]")
{
    std::string device_path("./named_mutex_test");
    int fid(-1);
    if( access( device_path.c_str(), F_OK ) == -1 ) 
    {
        fid = open(device_path.c_str(), O_CREAT | O_RDWR, 0666);
        close(fid);
    }

    sem_unlink("test_semaphore1");
    sem_t *sem1 = sem_open("test_semaphore1", O_CREAT|O_EXCL, S_IRWXU, 0);
    CHECK_FALSE(sem1 == SEM_FAILED);
    sem_unlink("test_semaphore2");
    sem_t *sem2 = sem_open("test_semaphore2", O_CREAT|O_EXCL, S_IRWXU, 0);
    CHECK_FALSE(sem2 == SEM_FAILED);
    pid_t pid_0 = getpid();
    pid_t pid = fork();
    bool actual_test;
    SECTION("self-validation")
    {
        actual_test = false;
    }
    SECTION("actual-test")
    {
        actual_test = true;
    }
    if (pid == 0) // child
    {
        signal(SIGTERM, [](int signum) { exit(1); });

        sem_wait(sem1);
        sem_post(sem2);
        named_mutex mutex1(device_path, 0);
        if (actual_test)
            mutex1.lock();
        exit(0);
    }
    else
    {
        named_mutex mutex1(device_path, 0);
        mutex1.lock();
        CHECK_FALSE(sem_post(sem1) < 0);
        sem_wait(sem2);
        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
        int status;
        pid_t w = waitpid(pid, &status, WNOHANG);
        CHECK_FALSE(w == -1);
        bool child_alive(w == 0);
        if (child_alive) {
            int res = kill(pid,SIGTERM);
            pid_t w = waitpid(pid, &status, 0);
        }
        if (fid > 0)
        {
            remove(device_path.c_str());
        }
        sem_unlink("test_semaphore1");
        sem_close(sem1);
        sem_unlink("test_semaphore2");
        sem_close(sem2);
        REQUIRE(child_alive == actual_test);
    }
}

