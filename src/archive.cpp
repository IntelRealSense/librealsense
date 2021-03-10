// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "metadata-parser.h"
#include "archive.h"
#include <fstream>
#include "core/processing.h"
#include "core/video.h"
#include "frame-archive.h"

#define MIN_DISTANCE 1e-6

namespace librealsense
{
    std::shared_ptr<sensor_interface> frame::get_sensor() const
    {
        auto res = sensor.lock();
        if (!res)
        {
            auto archive = get_owner();
            if (archive) return archive->get_sensor();
        }
        return res;
    }
    void frame::set_sensor(std::shared_ptr<sensor_interface> s) { sensor = s; }

    float3* points::get_vertices()
    {
        get_frame_data(); // call GetData to ensure data is in main memory
        auto xyz = (float3*)data.data();
        return xyz;
    }

    std::tuple<uint8_t, uint8_t, uint8_t> get_texcolor(const frame_holder& texture, float u, float v)
    {
        auto ptr = dynamic_cast<video_frame*>(texture.frame);
        if (ptr == nullptr) {
            throw librealsense::invalid_value_exception("frame must be video frame");
        }
        const int w = ptr->get_width(), h = ptr->get_height();
        int x = std::min(std::max(int(u*w + .5f), 0), w - 1);
        int y = std::min(std::max(int(v*h + .5f), 0), h - 1);
        int idx = x * ptr->get_bpp() / 8 + y * ptr->get_stride();
        const auto texture_data = reinterpret_cast<const uint8_t*>(ptr->get_frame_data());
        return std::make_tuple(texture_data[idx], texture_data[idx + 1], texture_data[idx + 2]);
    }


    void points::export_to_ply(const std::string& fname, const frame_holder& texture)
    {
        auto stream_profile = get_stream().get();
        auto video_stream_profile = dynamic_cast<video_stream_profile_interface*>(stream_profile);
        if (!video_stream_profile)
            throw librealsense::invalid_value_exception("stream must be video stream");
        const auto vertices = get_vertices();
        const auto texcoords = get_texture_coordinates();
        std::vector<float3> new_vertices;
        std::vector<std::tuple<uint8_t, uint8_t, uint8_t>> new_tex;
        std::map<int, int> index2reducedIndex;

        new_vertices.reserve(get_vertex_count());
        new_tex.reserve(get_vertex_count());
        assert(get_vertex_count());
        for (size_t i = 0; i < get_vertex_count(); ++i)
            if (fabs(vertices[i].x) >= MIN_DISTANCE || fabs(vertices[i].y) >= MIN_DISTANCE ||
                fabs(vertices[i].z) >= MIN_DISTANCE)
            {
                index2reducedIndex[i] = new_vertices.size();
                new_vertices.push_back({ vertices[i].x,  -1*vertices[i].y, -1*vertices[i].z });
                if (texture)
                {
                    auto color = get_texcolor(texture, texcoords[i].x, texcoords[i].y);
                    new_tex.push_back(color);
                }
            }

        const auto threshold = 0.05f;
        auto width = video_stream_profile->get_width();
        std::vector<std::tuple<int, int, int>> faces;
        for (int x = 0; x < width - 1; ++x) {
            for (int y = 0; y < video_stream_profile->get_height() - 1; ++y) {
                auto a = y * width + x, b = y * width + x + 1, c = (y + 1)*width + x, d = (y + 1)*width + x + 1;
                if (vertices[a].z && vertices[b].z && vertices[c].z && vertices[d].z
                    && abs(vertices[a].z - vertices[b].z) < threshold && abs(vertices[a].z - vertices[c].z) < threshold
                    && abs(vertices[b].z - vertices[d].z) < threshold && abs(vertices[c].z - vertices[d].z) < threshold)
                {
                    if (index2reducedIndex.count(a) == 0 || index2reducedIndex.count(b) == 0 || index2reducedIndex.count(c) == 0 ||
                        index2reducedIndex.count(d) == 0)
                        continue;

                    faces.emplace_back(index2reducedIndex[a], index2reducedIndex[d], index2reducedIndex[b]);
                    faces.emplace_back(index2reducedIndex[d], index2reducedIndex[a], index2reducedIndex[c]);
                }
            }
        }

        std::ofstream out(fname);
        out << "ply\n";
        out << "format binary_little_endian 1.0\n";
        out << "comment pointcloud saved from Realsense Viewer\n";
        out << "element vertex " << new_vertices.size() << "\n";
        out << "property float" << sizeof(float) * 8 << " x\n";
        out << "property float" << sizeof(float) * 8 << " y\n";
        out << "property float" << sizeof(float) * 8 << " z\n";
        if (texture)
        {
            out << "property uchar red\n";
            out << "property uchar green\n";
            out << "property uchar blue\n";
        }
        out << "element face " << faces.size() << "\n";
        out << "property list uchar int vertex_indices\n";
        out << "end_header\n";
        out.close();

        out.open(fname, std::ios_base::app | std::ios_base::binary);
        for (int i = 0; i < new_vertices.size(); ++i)
        {
            // we assume little endian architecture on your device
            out.write(reinterpret_cast<const char*>(&(new_vertices[i].x)), sizeof(float));
            out.write(reinterpret_cast<const char*>(&(new_vertices[i].y)), sizeof(float));
            out.write(reinterpret_cast<const char*>(&(new_vertices[i].z)), sizeof(float));

            if (texture)
            {
                uint8_t x, y, z;
                std::tie(x, y, z) = new_tex[i];
                out.write(reinterpret_cast<const char*>(&x), sizeof(uint8_t));
                out.write(reinterpret_cast<const char*>(&y), sizeof(uint8_t));
                out.write(reinterpret_cast<const char*>(&z), sizeof(uint8_t));
            }
        }
        auto size = faces.size();
        for (int i = 0; i < size; ++i) {
            int three = 3;
            out.write(reinterpret_cast<const char*>(&three), sizeof(uint8_t));
            out.write(reinterpret_cast<const char*>(&(std::get<0>(faces[i]))), sizeof(int));
            out.write(reinterpret_cast<const char*>(&(std::get<1>(faces[i]))), sizeof(int));
            out.write(reinterpret_cast<const char*>(&(std::get<2>(faces[i]))), sizeof(int));
        }
    }

    size_t points::get_vertex_count() const
    {
        return data.size() / (sizeof(float3) + sizeof(int2));
    }

    float2* points::get_texture_coordinates()
    {
        get_frame_data(); // call GetData to ensure data is in main memory
        auto xyz = (float3*)data.data();
        auto ijs = (float2*)(xyz + get_vertex_count());
        return ijs;
    }


    std::shared_ptr<archive_interface> make_archive(rs2_extension type,
        std::atomic<uint32_t>* in_max_frame_queue_size,
        std::shared_ptr<platform::time_service> ts,
        std::shared_ptr<metadata_parser_map> parsers)
    {
        switch (type)
        {
        case RS2_EXTENSION_VIDEO_FRAME:
            return std::make_shared<frame_archive<video_frame>>(in_max_frame_queue_size, ts, parsers);

        case RS2_EXTENSION_COMPOSITE_FRAME:
            return std::make_shared<frame_archive<composite_frame>>(in_max_frame_queue_size, ts, parsers);

        case RS2_EXTENSION_MOTION_FRAME:
            return std::make_shared<frame_archive<motion_frame>>(in_max_frame_queue_size, ts, parsers);

        case RS2_EXTENSION_POINTS:
            return std::make_shared<frame_archive<points>>(in_max_frame_queue_size, ts, parsers);

        case RS2_EXTENSION_DEPTH_FRAME:
            return std::make_shared<frame_archive<depth_frame>>(in_max_frame_queue_size, ts, parsers);

        case RS2_EXTENSION_POSE_FRAME:
            return std::make_shared<frame_archive<pose_frame>>(in_max_frame_queue_size, ts, parsers);

        case RS2_EXTENSION_DISPARITY_FRAME:
            return std::make_shared<frame_archive<disparity_frame>>(in_max_frame_queue_size, ts, parsers);

        default:
            throw std::runtime_error("Requested frame type is not supported!");
        }
    }

    void frame::release()
    {
        if (ref_count.fetch_sub(1) == 1)
        {
            unpublish();
            on_release();
            owner->unpublish_frame(this);
        }
    }

    void frame::keep()
    {
        if (!_kept.exchange(true))
        {
            owner->keep_frame(this);
        }
    }

    frame_interface* frame::publish(std::shared_ptr<archive_interface> new_owner)
    {
        owner = new_owner;
        _kept = false;
        return owner->publish_frame(this);
    }

    rs2_metadata_type frame::get_frame_metadata(const rs2_frame_metadata_value& frame_metadata) const
    {
        if (!metadata_parsers)
            throw invalid_value_exception(to_string() << "metadata not available for "
                << get_string(get_stream()->get_stream_type()) << " stream");

        auto it = metadata_parsers.get()->find(frame_metadata);
        if (it == metadata_parsers.get()->end())          // Possible user error - md attribute is not supported by this frame type
            throw invalid_value_exception(to_string() << get_string(frame_metadata)
                << " attribute is not applicable for "
                << get_string(get_stream()->get_stream_type()) << " stream ");

        // Proceed to parse and extract the required data attribute
        return it->second->get(*this);
    }

    bool frame::supports_frame_metadata(const rs2_frame_metadata_value& frame_metadata) const
    {
        // verify preconditions
        if (!metadata_parsers)
            return false;                         // No parsers are available or no metadata was attached

        auto it = metadata_parsers.get()->find(frame_metadata);
        if (it == metadata_parsers.get()->end())          // Possible user error - md attribute is not supported by this frame type
            return false;

        return it->second->supports(*this);
    }

    int frame::get_frame_data_size() const
    {
        return data.size();
    }

    const byte* frame::get_frame_data() const
    {
        const byte* frame_data = data.data();

        if (on_release.get_data())
        {
            frame_data = static_cast<const byte*>(on_release.get_data());
        }

        return frame_data;
    }

    rs2_timestamp_domain frame::get_frame_timestamp_domain() const
    {
        return additional_data.timestamp_domain;
    }

    rs2_time_t frame::get_frame_timestamp() const
    {
        return additional_data.timestamp;
    }

    unsigned long long frame::get_frame_number() const
    {
        return additional_data.frame_number;
    }

    rs2_time_t frame::get_frame_system_time() const
    {
        return additional_data.system_time;
    }

    void frame::update_frame_callback_start_ts(rs2_time_t ts)
    {
        additional_data.frame_callback_started = ts;
    }

    rs2_time_t frame::get_frame_callback_start_time_point() const
    {
        return additional_data.frame_callback_started;
    }

    void frame::log_callback_start(rs2_time_t timestamp)
    {
        update_frame_callback_start_ts(timestamp);
        LOG_DEBUG("CallbackStarted," << std::dec << librealsense::get_string(get_stream()->get_stream_type()) << "," << get_frame_number() << ",DispatchedAt," << std::fixed << timestamp);
    }

    void frame::log_callback_end(rs2_time_t timestamp) const
    {
        auto callback_warning_duration = 1000.f / (get_stream()->get_framerate() + 1);
        auto callback_duration = timestamp - get_frame_callback_start_time_point();

        LOG_DEBUG("CallbackFinished," << librealsense::get_string(get_stream()->get_stream_type()) << ","
                    << std::dec << get_frame_number() << ",DispatchedAt," << std::fixed << timestamp);

        if (callback_duration > callback_warning_duration)
        {
            LOG_INFO("Frame Callback " << librealsense::get_string(get_stream()->get_stream_type())
                << "#" << std::dec << get_frame_number()
                << "overdue. (Duration: " << callback_duration
                << "ms, FPS: " << get_stream()->get_framerate() << ", Max Duration: " << callback_warning_duration << "ms)");
        }
    }
}
