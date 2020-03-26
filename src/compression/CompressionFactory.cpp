// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "CompressionFactory.h"
#include "JpegCompression.h"
#include "Lz4Compression.h"
#include "RvlCompression.h"

std::shared_ptr<ICompression> CompressionFactory::getObject(int t_width, int t_height, rs2_format t_format, rs2_stream t_streamType, int t_bpp)
{
    ZipMethod zipMeth;
    if(t_streamType == RS2_STREAM_COLOR || t_streamType == RS2_STREAM_INFRARED)
    {
        zipMeth = ZipMethod::jpeg;
    }
    else if(t_streamType == RS2_STREAM_DEPTH)
    {
        zipMeth = ZipMethod::lz;
    }
    if(!isCompressionSupported(t_format, t_streamType))
    {
        return nullptr;
    }

    switch(zipMeth)
    {
    case ZipMethod::rvl:
        return std::make_shared<RvlCompression>(t_width, t_height, t_format, t_bpp);
        break;
    case ZipMethod::jpeg:
        return std::make_shared<JpegCompression>(t_width, t_height, t_format, t_bpp);
        break;
    case ZipMethod::lz:
        return std::make_shared<Lz4Compression>(t_width, t_height, t_format, t_bpp);
        break;
    default:
        ERR << "unknown zip method";
        return nullptr;
    }
}

bool& CompressionFactory::getIsEnabled()
{
    static bool m_isEnabled = true;
    return m_isEnabled;
}

bool CompressionFactory::isCompressionSupported(rs2_format t_format, rs2_stream t_streamType)
{
    if(getIsEnabled() == 0)
    {
        return false;
    }

    if((t_streamType == RS2_STREAM_COLOR || t_streamType == RS2_STREAM_INFRARED) && (t_format != RS2_FORMAT_BGR8 && t_format != RS2_FORMAT_RGB8 && t_format != RS2_FORMAT_Y8 && t_format != RS2_FORMAT_YUYV && t_format != RS2_FORMAT_UYVY))
    {
        return false;
    }
    return true;
}
