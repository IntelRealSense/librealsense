// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_EXPORT_HPP
#define LIBREALSENSE_RS2_EXPORT_HPP

#include <map>
#include <fstream>
#include "rs_processing.hpp"

namespace rs2
{
    class save_to_ply : public processing_block
    {
    public:
        save_to_ply(std::string filename = "RealSense Pointcloud ", pointcloud pc = pointcloud())
            : processing_block([this](rs2::frame f, rs2::frame_source& s) { func(f, s); }),
            _pc(std::move(pc)), fname(filename) {}

    private:
        void func(rs2::frame data, rs2::frame_source& source)
        {
            frame depth, color;
            if (auto fs = data.as<frameset>()) {
                depth = fs.first(RS2_STREAM_DEPTH);
                color = fs.first(RS2_STREAM_COLOR);
            } else {
                depth = data.as<depth_frame>();
            }

            if (!depth) throw std::runtime_error("Need depth data to save PLY");
            if (!depth.is<points>()) {
                if (color) _pc.map_to(color);
                depth = _pc.calculate(depth);
            }

            export_to_ply(depth, color);
        }

        void export_to_ply(points p, video_frame color) {
            auto profile = p.get_profile().as<video_stream_profile>();
            const auto verts = p.get_vertices();
            const auto texcoords = p.get_texture_coordinates();
            std::vector<rs2::vertex> new_verts;
            std::vector<std::array<uint8_t, 3>> new_tex;
            std::map<int, int> idx_map;

            new_verts.reserve(p.size());
            if (color) new_tex.reserve(p.size());

            static const auto min_distance = 1e-6;

            for (size_t i = 0; i < p.size(); ++i) {
                if (fabs(verts[i].x) >= min_distance || fabs(verts[i].y) >= min_distance ||
                    fabs(verts[i].z) >= min_distance)
                {
                    idx_map[i] = new_verts.size();
                    new_verts.push_back(verts[i]);
                    if (color)
                    {
                        auto rgb = get_texcolor(color, texcoords[i].u, texcoords[i].v);
                        new_tex.push_back(rgb);
                    }
                }
            }

            static const auto threshold = 0.05f;
            auto width = profile.width(), height = profile.height();
            std::vector<std::array<int, 3>> faces;
            for (int x = 0; x < width - 1; ++x) {
                for (int y = 0; y < height - 1; ++y) {
                    auto a = y * width + x, b = y * width + x + 1, c = (y + 1)*width + x, d = (y + 1)*width + x + 1;
                    if (verts[a].z && verts[b].z && verts[c].z && verts[d].z
                        && abs(verts[a].z - verts[b].z) < threshold && abs(verts[a].z - verts[c].z) < threshold
                        && abs(verts[b].z - verts[d].z) < threshold && abs(verts[c].z - verts[d].z) < threshold)
                    {
                        if (idx_map.count(a) == 0 || idx_map.count(b) == 0 || idx_map.count(c) == 0 ||
                            idx_map.count(d) == 0)
                            continue;
                        faces.emplace_back(idx_map[a], idx_map[b], idx_map[d]);
                        faces.emplace_back(idx_map[d], idx_map[c], idx_map[a]);
                    }
                }
            }

            std::ofstream out(fname + std::to_string(p.get_frame_number()) + ".ply");
            out << "ply\n";
            out << "format binary_little_endian 1.0\n" /*"format ascii 1.0\n"*/;
            out << "comment pointcloud saved from Realsense Viewer\n";
            out << "element vertex " << new_verts.size() << "\n";
            out << "property float" << sizeof(float) * 8 << " x\n";
            out << "property float" << sizeof(float) * 8 << " y\n";
            out << "property float" << sizeof(float) * 8 << " z\n";
            if (color)
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
            for (int i = 0; i < new_verts.size(); ++i)
            {
                // we assume little endian architecture on your device
                out.write(reinterpret_cast<const char*>(&(new_verts[i].x)), sizeof(float));
                out.write(reinterpret_cast<const char*>(&(new_verts[i].y)), sizeof(float));
                out.write(reinterpret_cast<const char*>(&(new_verts[i].z)), sizeof(float));

                if (color)
                {
                    uint8_t x = [0], y = new_tex[i][1], z = new_tex[i][2];
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

        // TODO: get_texcolor, options API

        std::string fname;
        pointcloud _pc;
    };
}

#endif