// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <chrono>

class RsStatistics
{
public:
    static std::chrono::high_resolution_clock::time_point& getResetPacketStartTp()
    {
        static std::chrono::high_resolution_clock::time_point tpResetPacketStart = std::chrono::high_resolution_clock::now();
        return tpResetPacketStart;
    }
    static std::chrono::high_resolution_clock::time_point& getFirstPacketTp()
    {
        static std::chrono::high_resolution_clock::time_point tpFirstPacket = std::chrono::high_resolution_clock::now();
        return tpFirstPacket;
    }
    static std::chrono::high_resolution_clock::time_point& getSendPacketTp()
    {
        static std::chrono::high_resolution_clock::time_point tpSendPacket = std::chrono::high_resolution_clock::now();
        return tpSendPacket;
    }
    static std::chrono::high_resolution_clock::time_point& getScheduleTp()
    {
        static std::chrono::high_resolution_clock::time_point tpSchedule = std::chrono::high_resolution_clock::now();
        return tpSchedule;
    }
    static double& getPrevDiff()
    {
        static double prevDiff = 0;
        return prevDiff;
    }
    static double isJump()
    {
        double* prevDiff = &getPrevDiff();
        double diff = 1000 * std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - RsStatistics::getSendPacketTp()).count();
        double diffOfDiff = diff - *prevDiff;
        if(diffOfDiff > 5)
        {
            *prevDiff = diff;
            return diffOfDiff;
        }
        else
        {
            *prevDiff = diff;
            return 0;
        }
    }
};
