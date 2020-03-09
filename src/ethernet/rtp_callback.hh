#pragma once

#include <time.h>
#include <queue>

#include <memory>
#include <mutex>

#ifdef _WIN64
#  define ssize_t __int64
#else
#  define ssize_t long
#endif

class rtp_callback
{
    public:
        
        void virtual on_frame(unsigned char*buffer, ssize_t size, struct timeval presentationTime)=0;
        
        //void virtual on_error(int code, std::string message)=0;
        
        //void virtual on_messege(std::string message)=0;
};


