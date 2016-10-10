// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_STREAM_H
#define LIBREALSENSE_STREAM_H

#include "types.h"

#include <memory> // For shared_ptr

namespace rsimpl
{
    struct stream_interface : rs_stream_interface
    {
        stream_interface(calibration_validator in_validator, rs_stream in_stream) : stream(in_stream), validator(in_validator){};
                                                                 
        virtual rs_extrinsics                   get_extrinsics_to(const rs_stream_interface & other) const override;
        virtual rsimpl::pose                    get_pose() const = 0;
        virtual int                             get_mode_count() const override { return 0; }
        virtual void                            get_mode(int /*mode*/, int * /*w*/, int * /*h*/, rs_format * /*f*/, int * /*fps*/) const override { throw std::logic_error("no modes"); }
        virtual rs_stream                       get_stream_type()const override { return stream; }

        const rs_stream   stream;

    protected:
        calibration_validator validator;
    };
    
    class frame_archive;
    class syncronizing_archive;

    struct native_stream final : public stream_interface
    {
        const device_config &                   config;
        
        std::vector<subdevice_mode_selection>   modes;
        std::shared_ptr<syncronizing_archive>   archive;

                                                native_stream(device_config & config, rs_stream stream, calibration_validator in_validator);

        pose                                    get_pose() const override { return config.info.stream_poses[stream]; }
        float                                   get_depth_scale() const override { return config.depth_scale; }
        int                                     get_mode_count() const override { return (int)modes.size(); }
        void                                    get_mode(int mode, int * w, int * h, rs_format * f, int * fps) const override;

        bool                                    is_enabled() const override;
        subdevice_mode_selection                get_mode() const;
        rs_intrinsics                           get_intrinsics() const override;
        rs_intrinsics                           get_rectified_intrinsics() const override;
        rs_format                               get_format() const override { return get_mode().get_format(stream); }
        int                                     get_framerate() const override { return get_mode().get_framerate(); }

        double                                  get_frame_metadata(rs_frame_metadata frame_metadata) const override;
        bool                                    supports_frame_metadata(rs_frame_metadata frame_metadata) const override;
        unsigned long long                      get_frame_number() const override;
        double                                  get_frame_timestamp() const override;
        long long                               get_frame_system_time() const override;
        const uint8_t *                         get_frame_data() const override;

        int                                     get_frame_stride() const override;
        int                                     get_frame_bpp() const override;
    };

    class point_stream final : public stream_interface
    {
        const stream_interface &                source;
        mutable std::vector<uint8_t>            image;
        mutable unsigned long long              number;
    public:
        point_stream(const stream_interface & source) :stream_interface(calibration_validator(), RS_STREAM_POINTS), source(source), number() {}

        pose                                    get_pose() const override { return {{{1,0,0},{0,1,0},{0,0,1}}, source.get_pose().position}; }
        float                                   get_depth_scale() const override { return source.get_depth_scale(); }

        bool                                    is_enabled() const override { return source.is_enabled(); }
        rs_intrinsics                           get_intrinsics() const override { return source.get_intrinsics(); }
        rs_intrinsics                           get_rectified_intrinsics() const override { return source.get_rectified_intrinsics(); }
        rs_format                               get_format() const override { return RS_FORMAT_XYZ32F; }
        int                                     get_framerate() const override { return source.get_framerate(); }

        double                                  get_frame_metadata(rs_frame_metadata frame_metadata) const override { return source.get_frame_metadata(frame_metadata); }
        bool                                    supports_frame_metadata(rs_frame_metadata frame_metadata) const override { return source.supports_frame_metadata(frame_metadata); }
        unsigned long long                      get_frame_number() const override { return source.get_frame_number(); }
        double                                  get_frame_timestamp() const override{ return source.get_frame_timestamp(); }
        long long                               get_frame_system_time() const override { return source.get_frame_system_time(); }
        const uint8_t *                         get_frame_data() const override;

        int                                     get_frame_stride() const override { return source.get_frame_stride(); }
        int                                     get_frame_bpp() const override { return source.get_frame_bpp(); }
    };

    class rectified_stream final : public stream_interface
    {
        const stream_interface &                source;
        mutable std::vector<int>                table;
        mutable std::vector<uint8_t>            image;
        mutable unsigned long long              number;
    public:
        rectified_stream(const stream_interface & source) : stream_interface(calibration_validator(), RS_STREAM_RECTIFIED_COLOR), source(source), number() {}

        pose                                    get_pose() const override { return {{{1,0,0},{0,1,0},{0,0,1}}, source.get_pose().position}; }
        float                                   get_depth_scale() const override { return source.get_depth_scale(); }

        bool                                    is_enabled() const override { return source.is_enabled(); }
        rs_intrinsics                           get_intrinsics() const override { return source.get_rectified_intrinsics(); }
        rs_intrinsics                           get_rectified_intrinsics() const override { return source.get_rectified_intrinsics(); }
        rs_format                               get_format() const override { return source.get_format(); }
        int                                     get_framerate() const override { return source.get_framerate(); }

        double                                  get_frame_metadata(rs_frame_metadata frame_metadata) const override { return source.get_frame_metadata(frame_metadata); }
        bool                                    supports_frame_metadata(rs_frame_metadata frame_metadata) const override { return source.supports_frame_metadata(frame_metadata); }
        unsigned long long                      get_frame_number() const override { return source.get_frame_number(); }
        double                                  get_frame_timestamp() const override { return source.get_frame_timestamp(); }
        long long                               get_frame_system_time() const override { return source.get_frame_system_time(); }
        const uint8_t *                         get_frame_data() const override;

        int                                     get_frame_stride() const override { return source.get_frame_stride(); }
        int                                     get_frame_bpp() const override { return source.get_frame_bpp(); }
    };

    class aligned_stream final : public stream_interface
    {
        const stream_interface &                from, & to;
        mutable std::vector<uint8_t>            image;
        mutable unsigned long long              number;
    public:
        aligned_stream(const stream_interface & from, const stream_interface & to) :stream_interface(calibration_validator(), RS_STREAM_COLOR_ALIGNED_TO_DEPTH), from(from), to(to), number() {}

        pose                                    get_pose() const override { return to.get_pose(); }
        float                                   get_depth_scale() const override { return to.get_depth_scale(); }

        bool                                    is_enabled() const override { return from.is_enabled() && to.is_enabled(); }
        rs_intrinsics                           get_intrinsics() const override { return to.get_intrinsics(); }
        rs_intrinsics                           get_rectified_intrinsics() const override { return to.get_rectified_intrinsics(); }
        rs_format                               get_format() const override { return from.get_format(); }
        int                                     get_framerate() const override { return from.get_framerate(); }

        double                                  get_frame_metadata(rs_frame_metadata frame_metadata) const override { return from.get_frame_metadata(frame_metadata); }
        bool                                    supports_frame_metadata(rs_frame_metadata frame_metadata) const override { return from.supports_frame_metadata(frame_metadata); }
        unsigned long long                      get_frame_number() const override { return from.get_frame_number(); }
        double                                  get_frame_timestamp() const override { return from.get_frame_timestamp(); }
        long long                               get_frame_system_time() const override { return from.get_frame_system_time(); }
        const unsigned char *                   get_frame_data() const override;

        int                                     get_frame_stride() const override { return from.get_frame_stride(); }
        int                                     get_frame_bpp() const override { return from.get_frame_bpp(); }
    };
}

#endif
