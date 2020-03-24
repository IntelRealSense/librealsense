// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <NetdevLog.h>

#include <librealsense2/rs.hpp>

class ICompression
{
public:
    virtual int compressBuffer(unsigned char* t_buffer, int t_size, unsigned char* t_compressedBuf) = 0;
    virtual int decompressBuffer(unsigned char* t_buffer, int t_size, unsigned char* t_uncompressedBuf) = 0;

protected:
    int m_width, m_height, m_bpp;
    rs2_format m_format;
    int m_decompFrameCounter = 0, m_compFrameCounter = 0;
};
