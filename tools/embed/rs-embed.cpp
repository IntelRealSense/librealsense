// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <string>
#include <string.h> // memcpy
#include <fstream>
#include <vector>

#include "tclap/CmdLine.h"

#define STB_IMAGE_IMPLEMENTATION
#include "../third-party/stb_image.h"

#include <lz4.h>

#define RS_EMBED_VERSION "0.0.0.1"

struct float3
{
    float x, y, z;
};

struct short3
{
    uint16_t x, y, z;
};

using namespace std;
using namespace TCLAP;

bool file_exists(const std::string& name) {
    std::ifstream f(name.c_str());
    return f.good();
}

struct int6
{
    int x, y, z, a, b, c;
};

void uncompress(const uint8_t* ptr, int compressed_size, int original_size, int vertex_count, int index_count,
    std::vector<float3>& vertex_data, std::vector<float3>& normals, std::vector<short3>& index_data)
{
    std::vector<char> uncompressed(original_size, 0);
    LZ4_decompress_safe((const char*)ptr, uncompressed.data(), compressed_size, original_size);
    const int vertex_size = vertex_count * sizeof(float3);
    const int index_size = index_count * sizeof(short3);
    vertex_data.resize(vertex_count);
    memcpy(vertex_data.data(), uncompressed.data(), vertex_size);
    index_data.resize(index_count);
    memcpy(index_data.data(), uncompressed.data() + vertex_size, index_size);
    normals.resize(vertex_count);
    memcpy(normals.data(), uncompressed.data() + vertex_size + index_size, vertex_size);
}

bool ends_with(const std::string& s, const std::string& suffix)
{
    auto i = s.rbegin(), j = suffix.rbegin();
    for (; i != s.rend() && j != suffix.rend() && *i == *j;
        i++, j++);
    return j == suffix.rend();
}

int main(int argc, char** argv) try
{
    // Parse command line arguments
    CmdLine cmd("librealsense rs-embed tool", ' ', RS_EMBED_VERSION);
    ValueArg<string> inputFilename("i", "input", "Input filename", true, "", "input-file");
    ValueArg<string> outputFilename("o", "output", "Output filename", false, "", "output-file");
    ValueArg<string> objectName("n", "name", "Name", false, "", "object-name");

    cmd.add(inputFilename);
    cmd.add(outputFilename);
    cmd.add(objectName);
    cmd.parse(argc, argv);

    auto input = inputFilename.getValue();
    auto output = outputFilename.getValue();
    auto name = objectName.getValue();

    if (ends_with(input, ".obj"))
    {
        std::vector<float3> vertex_data;
        std::vector<float3> normals_raw_data;
        std::vector<float3> normals_data;
        std::vector<int6> index_raw_data;
        std::vector<short3> index_data;

        if (file_exists(input))
        {
            std::ifstream file(input);
            std::string str;
            while (std::getline(file, str))
            {
                if (str.size())
                {
                    if (str[0] == 'v')
                    {
                        float a, b, c;
                        if (str[1] == 'n')
                        {
                            sscanf(str.c_str(), "vn %f %f %f", &a, &b, &c);
                            normals_raw_data.push_back({ a, b, c });
                        }
                        else
                        {
                            sscanf(str.c_str(), "v %f %f %f", &a, &b, &c);
                            vertex_data.push_back({ a, b, c });
                        }
                    }
                    if (str[0] == 'f')
                    {
                        int x, y, z, a = 0, b = 0, c = 0; // TODO check about a,b,c values
                        sscanf(str.c_str(), "f %d %d %d", &x, &y, &z);
                        index_raw_data.push_back({ x, y, z, a, b, c });
                    }
                }
            }
        }

        normals_data.resize(vertex_data.size());
        for (auto& idx : index_raw_data)
        {
            // normals_data[idx.x] = normals_raw_data[idx.a];
            // normals_data[idx.y] = normals_raw_data[idx.b];
            // normals_data[idx.z] = normals_raw_data[idx.c];
            index_data.push_back({ static_cast<uint16_t>(idx.x - 1),
                                    static_cast<uint16_t>(idx.y - 1),
                                    static_cast<uint16_t>(idx.z - 1) });
        }

        size_t vertex_data_size = vertex_data.size() * sizeof(float3);
        size_t index_data_size = index_data.size() * sizeof(short3);
        //size_t normals_data_size = normals_data.size() * sizeof(float3);

        std::vector<uint8_t> data(vertex_data_size + index_data_size, 0);
        memcpy(data.data(), vertex_data.data(), vertex_data_size);
        memcpy(data.data() + vertex_data_size, index_data.data(), index_data_size);
        //memcpy(data.data() + vertex_data_size + index_data_size, normals_data.data(), normals_data_size);

        // compress szSource into pchCompressed
        auto rawDataSize = (int)data.size();
        auto compressBufSize = LZ4_compressBound(rawDataSize);
        char* pchCompressed = new char[compressBufSize];
        memset(pchCompressed, compressBufSize, 0);
        int nCompressedSize = LZ4_compress_default((const char*)data.data(), pchCompressed, rawDataSize, compressBufSize);

        ofstream myfile;
        myfile.open(output);
        myfile << "// License: Apache 2.0. See LICENSE file in root directory.\n";
        myfile << "// Copyright(c) 2018 Intel Corporation. All Rights Reserved.\n\n";
        myfile << "// This file is auto-generated from " << name << ".obj\n";
        myfile << "#pragma once\n";
        myfile << "static uint32_t " << name << "_obj_data [] { ";

        auto nAllignedCompressedSize = nCompressedSize;
        auto leftover = nCompressedSize % 4;
        if (leftover % 4 != 0) nAllignedCompressedSize += (4 - leftover);

        for (int i = 0; i < nAllignedCompressedSize; i += 4)
        {
            uint32_t* ptr = (uint32_t*)(pchCompressed + i);
            myfile << "0x" << std::hex << (int)(*ptr);
            if (i < nAllignedCompressedSize - 1) myfile << ",";
        }

        myfile << "};\n";

        myfile << "#include <lz4.h>\n";
        myfile << "#include <vector>\n";

        myfile << "inline void uncompress_" << name << "_obj(std::vector<float3>& vertex_data, std::vector<float3>& normals, std::vector<short3>& index_data)\n";
        myfile << "{\n";
        myfile << "    std::vector<char> uncompressed(0x" << std::hex << rawDataSize << ", 0);\n";
        myfile << "    LZ4_decompress_safe((const char*)" << name << "_obj_data, uncompressed.data(), 0x" << std::hex << nCompressedSize << ", 0x" << std::hex << rawDataSize << ");\n";
        myfile << "    const int vertex_size = 0x" << std::hex << vertex_data.size() << " * sizeof(float3);\n";
        myfile << "    const int index_size = 0x" << std::hex << index_data.size() << " * sizeof(short3);\n";
        myfile << "    vertex_data.resize(0x" << std::hex << vertex_data.size() << ");\n";
        myfile << "    memcpy(vertex_data.data(), uncompressed.data(), vertex_size);\n";
        myfile << "    index_data.resize(0x" << std::hex << index_data.size() << ");\n";
        myfile << "    memcpy(index_data.data(), uncompressed.data() + vertex_size, index_size);\n";
        myfile << "    //normals.resize(0x" << std::hex << vertex_data.size() << ");\n";
        myfile << "    //memcpy(normals.data(), uncompressed.data() + vertex_size + index_size, vertex_size);\n";
        myfile << "}\n";

        myfile.close();
    }

    if (ends_with(input, ".png"))
    {
        ifstream ifs(input, ios::binary | ios::ate);
        ifstream::pos_type pos = ifs.tellg();

        std::vector<char> buffer(pos);

        ifs.seekg(0, ios::beg);
        ifs.read(&buffer[0], pos);

        ofstream myfile;
        myfile.open(output);
        myfile << "// License: Apache 2.0. See LICENSE file in root directory.\n";
        myfile << "// Copyright(c) 2018 Intel Corporation. All Rights Reserved.\n\n";
        myfile << "// This file is auto-generated from " << name << ".png\n";

        myfile << "static uint32_t " << name << "_png_size = 0x" << std::hex << buffer.size() << ";\n";

        myfile << "static uint8_t " << name << "_png_data [] { ";
        for (int i = 0; i < buffer.size(); i++)
        {
            uint8_t byte = buffer[i];
            myfile << "0x" << std::hex << (int)byte;
            if (i < buffer.size() - 1) myfile << ",";
        }
        myfile << "};\n";

        myfile.close();
    }


    //uncompress_d435_obj(vertex_data, normals_data, index_data);

    return EXIT_SUCCESS;
}
catch (const exception& e)
{
    cerr << e.what() << endl;
    return EXIT_FAILURE;
}
catch (...)
{
    cerr << "some error" << endl;
    return EXIT_FAILURE;
}
