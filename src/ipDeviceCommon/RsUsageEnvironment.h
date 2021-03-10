// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <BasicUsageEnvironment.hh>
#include <easylogging++.h>

#define RS_MAX_LOG_MSG_SIZE 1024
#define RS_MAX_LOG_MSG_THLD 128

class RSUsageEnvironment : public BasicUsageEnvironment
{
public:
    static RSUsageEnvironment* createNew(TaskScheduler& taskScheduler);

    virtual UsageEnvironment& operator<<(char const* str);
    virtual UsageEnvironment& operator<<(int i);
    virtual UsageEnvironment& operator<<(unsigned u);
    virtual UsageEnvironment& operator<<(double d);
    virtual UsageEnvironment& operator<<(void* p);

protected:
    RSUsageEnvironment(TaskScheduler& taskScheduler);
    // called only by "createNew()" (or subclass constructors)
    virtual ~RSUsageEnvironment();

private:
    void flush();
    void check();

    char buffer[RS_MAX_LOG_MSG_SIZE];
    char* ptr;

    el::Logger* netdev_log;
    el::Logger* lrs_log;
};
