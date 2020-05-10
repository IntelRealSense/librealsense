// License:    Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RsUsageEnvironment.h"

#include <types.h>

RSUsageEnvironment::RSUsageEnvironment(TaskScheduler& taskScheduler)
    : BasicUsageEnvironment(taskScheduler)
{}

RSUsageEnvironment::~RSUsageEnvironment()
{
    LOG_INFO("RealSense network logging closed");
}

RSUsageEnvironment* RSUsageEnvironment::createNew(TaskScheduler& taskScheduler)
{
    RSUsageEnvironment* env = new RSUsageEnvironment(taskScheduler);

    if(env)
    {
        env->ptr = env->buffer;
        LOG_INFO("RealSense network logging initialized");
    }

    return env;
}

void RSUsageEnvironment::flush()
{
    *ptr = '\0';
    LOG_INFO(buffer);
    ptr = buffer;
}

void RSUsageEnvironment::check()
{
    if((ptr - buffer) > (RS_MAX_LOG_MSG_SIZE - RS_MAX_LOG_MSG_THLD))
    {
        flush();
    }
}

UsageEnvironment& RSUsageEnvironment::operator<<(char const* str)
{
    int num = 0;

    if(str == NULL)
        str = "(NULL)"; // sanity check

    while(str[num] != '\0')
    {
        if(str[num] == '\n')
        {
            flush();
        }
        else
        {
            *ptr++ = str[num];
            check();
        }
        num++;
    }

    return *this;
}

UsageEnvironment& RSUsageEnvironment::operator<<(int i)
{
    ptr += sprintf(ptr, "%d", i);
    check();
    return *this;
}

UsageEnvironment& RSUsageEnvironment::operator<<(unsigned u)
{
    ptr += sprintf(ptr, "%u", u);
    check();
    return *this;
}

UsageEnvironment& RSUsageEnvironment::operator<<(double d)
{
    ptr += sprintf(ptr, "%f", d);
    check();
    return *this;
}

UsageEnvironment& RSUsageEnvironment::operator<<(void* p)
{
    ptr += sprintf(ptr, "%p", p);
    check();
    return *this;
}
