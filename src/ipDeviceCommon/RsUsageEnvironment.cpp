// License:    Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "RsUsageEnvironment.h"

#ifdef BUILD_EASYLOGGINGPP
#ifdef BUILD_SHARED_LIBS
INITIALIZE_EASYLOGGINGPP
#endif
#endif

RSUsageEnvironment::RSUsageEnvironment(TaskScheduler& taskScheduler)
    : BasicUsageEnvironment(taskScheduler)
{}

RSUsageEnvironment::~RSUsageEnvironment()
{
    CLOG(INFO, "netdev") << "RealSense network logging closed";

    el::Loggers::unregisterLogger("librealsense");
    el::Loggers::unregisterLogger("netdev");
}

RSUsageEnvironment* RSUsageEnvironment::createNew(TaskScheduler& taskScheduler)
{
    RSUsageEnvironment* env = new RSUsageEnvironment(taskScheduler);

    if(env)
    {
        env->ptr = env->buffer;
        env->netdev_log = el::Loggers::getLogger("netdev");
        env->lrs_log = el::Loggers::getLogger("librealsense");

        el::Loggers::reconfigureAllLoggers(el::Level::Global, el::ConfigurationType::Format, "%datetime{%y%M%d%H%m%s.%g} [%logger]\t%levshort: %msg");
        el::Loggers::reconfigureAllLoggers(el::Level::Debug, el::ConfigurationType::Enabled, "false");
        el::Loggers::reconfigureAllLoggers(el::Level::Global, el::ConfigurationType::ToStandardOutput, "false");
        el::Loggers::reconfigureAllLoggers(el::Level::Global, el::ConfigurationType::ToFile, "true");

        CLOG(INFO, "netdev") << "RealSense network logging initialized";
    }

    return env;
}

void RSUsageEnvironment::flush()
{
    *ptr = '\0';
    CLOG(INFO, "netdev") << buffer;
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
