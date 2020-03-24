// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "time.h"
#include <chrono>
#include <map>
#include <queue>

class StreamStatistic
{
public:
    std::queue<std::chrono::system_clock::time_point> m_clockBeginVec;
    std::chrono::system_clock::time_point m_prevClockBegin, m_compressionBegin, m_decompressionBegin;
    std::chrono::duration<double> m_processingTime, m_getFrameDiffTime, m_compressionTime, m_decompressionTime;
    int m_frameCounter = 0, m_compressionFrameCounter = 0, m_decompressionFrameCounter = 0;
    double m_avgProcessingTime = 0, m_avgGettingTime = 0, m_avgCompressionTime = 0, m_avgDecompressionTime = 0;
    long long m_decompressedSizeSum = 0, m_compressedSizeSum = 0;
};

class Statistic
{
public:
    static std::map<int, StreamStatistic*>& getStatisticStreams() //todo:change to uid instead of type
    {
        static std::map<int, StreamStatistic*> m_streamStatistic;
        return m_streamStatistic;
    };
};
