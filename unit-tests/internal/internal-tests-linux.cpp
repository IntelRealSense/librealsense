#include "catch/catch.hpp"
#include <thread>
#include <string>
#include <linux/backend-v4l2.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <semaphore.h>

using namespace librealsense::platform;

TEST_CASE("named_mutex", "[code]")
{
    std::string device_path("./doron1");
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