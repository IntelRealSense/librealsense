// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include "rotation-transform.h"

#include "../include/librealsense2/hpp/rs_sensor.hpp"
#include "../include/librealsense2/hpp/rs_processing.hpp"
#include "context.h"
#include "image.h"
#include "stream.h"

namespace librealsense
{
    //// Unpacking routines ////
    template<size_t SIZE>
    void rotate_image_optimized(byte * const dest[], const byte * source, int width, int height, int actual_size)
    {
        auto width_out = height;
        auto height_out = width;

        auto out = dest[0];
        byte buffer[8][8 * SIZE]; // = { 0 };
        for (int i = 0; i <= height - 8; i = i + 8)
        {
            for (int j = 0; j <= width - 8; j = j + 8)
            {
                for (int ii = 0; ii < 8; ++ii)
                {
                    for (int jj = 0; jj < 8; ++jj)
                    {
                        auto source_index = ((j + jj) + (width * (i + ii))) * SIZE;
                        memcpy((void*)(&buffer[7 - jj][(7 - ii) * SIZE]), &source[source_index], SIZE);
                    }
                }

                for (int ii = 0; ii < 8; ++ii)
                {
                    auto out_index = (((height_out - 8 - j + 1) * width_out) - i - 8 + (ii)* width_out);
                    memcpy(&out[(out_index)* SIZE], &(buffer[ii]), 8 * SIZE);
                }
            }
        }
    }

    template<size_t SIZE>
    void rotate_image(byte * const dest[], const byte * source, int width, int height, int actual_size)
    {
        auto width_out = height;
        auto height_out = width;

        auto out = dest[0];
        for (int i = 0; i < height; ++i)
        {
            auto row_offset = i * width;
            for (int j = 0; j < width; ++j)
            {
                auto out_index = (((height_out - j) * width_out) - i - 1) * SIZE;
                librealsense::copy((void*)(&out[out_index]), &(source[(row_offset + j) * SIZE]), SIZE);
            }
        }
    }

    void rotate_confidence(byte * const dest[], const byte * source, int width, int height, int actual_size)
    {
#pragma pack (push, 1)
        struct lsb_msb
        {
            unsigned lsb : 4;
            unsigned msb : 4;
        };
#pragma pack(pop)

        rotate_image<1>(dest, source, width, height, actual_size);
        auto out = dest[0];
        for (int i = (width - 1), out_i = ((width - 1) * 2); i >= 0; --i, out_i -= 2)
        {
            auto row_offset = i * height;
            for (int j = 0; j < height; ++j)
            {
                auto val = *(reinterpret_cast<const lsb_msb*>(&out[(row_offset + j)]));
                auto out_index = out_i * height + j;
                out[out_index] = val.lsb << 4;
                out[out_index + height] = val.msb << 4;
            }
        }
    }

    //// Processing routines////
    rotation_transform::rotation_transform(rs2_format target_format, rs2_stream target_stream, rs2_extension extension_type)
        : rotation_transform("Rotation Transform", target_format, target_stream, extension_type)
    {}

    rotation_transform::rotation_transform(const char* name, rs2_format target_format, rs2_stream target_stream, rs2_extension extension_type)
        : functional_processing_block(name, target_format, target_stream, extension_type)
    {
        _stream_filter.format = _target_format;
        _stream_filter.stream = _target_stream;
    }

    void rotation_transform::init_profiles_info(const rs2::frame* f)
    {
        auto p = f->get_profile();
        if (p.get() != _source_stream_profile.get())
        {
            _source_stream_profile = p;
            _target_stream_profile = p.clone(p.stream_type(), p.stream_index(), _target_format);
            _target_bpp = get_image_bpp(_target_format) / 8;

            // Set the unique ID as the original frame.
            // The frames are piped through a syncer and must have the origin UID.
            auto target_spi = (stream_profile_interface*)_target_stream_profile.get()->profile;
            target_spi->set_unique_id(p.unique_id());
        }
    }

    void rotation_transform::process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size)
    {
        int rotated_width = height;
        int rotated_height = width;
        switch (_target_bpp)
        {
        case 1:
            rotate_image_optimized<1>(dest, source, rotated_width, rotated_height, actual_size);
            break;
        case 2:
            rotate_image_optimized<2>(dest, source, rotated_width, rotated_height, actual_size);
            break;
        default:
            LOG_ERROR("Rotation transform does not support format: " + std::string(rs2_format_to_string(_target_format)));
        }
    }

    confidence_rotation_transform::confidence_rotation_transform() :
        confidence_rotation_transform("Confidence Rotation Transform")
    {}

    confidence_rotation_transform::confidence_rotation_transform(const char * name)
        : rotation_transform(name, RS2_FORMAT_RAW8, RS2_STREAM_CONFIDENCE, RS2_EXTENSION_VIDEO_FRAME)
    {}

    void confidence_rotation_transform::process_function(byte * const dest[], const byte * source, int width, int height, int actual_size, int input_size)
    {
        int rotated_width = height;
        int rotated_height = width;

        // Workaround: the height is given by bytes and not by pixels.
        rotate_confidence(dest, source, rotated_width / 2, rotated_height, actual_size);
    }
}
