// See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <vector>
#include <algorithm>
#include <cstring>
#include <ctype.h>
#include <memory>
#include <string>
#include <sstream>


inline void make_depth_histogram(uint8_t rgb_image[], const uint16_t depth_image[], int width, int height)
{
    static uint32_t histogram[0x10000];
    memset(histogram, 0, sizeof(histogram));

    for (auto i = 0; i < width*height; ++i) ++histogram[depth_image[i]];
    for (auto i = 2; i < 0x10000; ++i) histogram[i] += histogram[i - 1]; // Build a cumulative histogram for the indices in [1,0xFFFF]
    for (auto i = 0; i < width*height; ++i)
    {
        if (auto d = depth_image[i])
        {
            int f = histogram[d] * 255 / histogram[0xFFFF]; // 0-255 based on histogram location
            rgb_image[i * 3 + 0] = 255 - f;
            rgb_image[i * 3 + 1] = 0;
            rgb_image[i * 3 + 2] = f;
        }
        else
        {
            rgb_image[i * 3 + 0] = 20;
            rgb_image[i * 3 + 1] = 5;
            rgb_image[i * 3 + 2] = 0;
        }
    }
}

struct rect
{
    float x, y;
    float w, h;
};



////////////////////////
// Image display code //
////////////////////////

class texture_buffer
{
    GLuint texture;
    std::vector<uint8_t> rgb;

public:
    texture_buffer() : texture() {}

    GLuint get_gl_handle() const { return texture; }

    void upload(const void* data, int width, int height, rs2_format format)
    {
        // If the frame timestamp has changed since the last time show(...) was called, re-upload the texture
        if (!texture)
            glGenTextures(1, &texture);

        glBindTexture(GL_TEXTURE_2D, texture);
        //glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);

        switch (format)
        {
        case RS2_FORMAT_ANY:
            throw std::runtime_error("not a valid format");
        case RS2_FORMAT_Z16:
        case RS2_FORMAT_DISPARITY16:
            rgb.resize(width * height * 4);
            make_depth_histogram(rgb.data(), reinterpret_cast<const uint16_t *>(data), width, height);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());

            break;
        case RS2_FORMAT_XYZ32F:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, data);
            break;
        case RS2_FORMAT_YUYV: // Display YUYV by showing the luminance channel and packing chrominance into ignored alpha channel
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);
            break;
        case RS2_FORMAT_RGB8: case RS2_FORMAT_BGR8: // Display both RGB and BGR by interpreting them RGB, to show the flipped byte ordering. Obviously, GL_BGR could be used on OpenGL 1.2+
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            break;
        case RS2_FORMAT_RGBA8: case RS2_FORMAT_BGRA8: // Display both RGBA and BGRA by interpreting them RGBA, to show the flipped byte ordering. Obviously, GL_BGRA could be used on OpenGL 1.2+
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            break;
        case RS2_FORMAT_Y8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
            break;
        case RS2_FORMAT_Y16:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, data);
            break;
        case RS2_FORMAT_RAW8:
        case RS2_FORMAT_MOTION_RAW:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
            break;
        case RS2_FORMAT_RAW10:
        {
            // Visualize Raw10 by performing a naive downsample. Each 2x2 block contains one red pixel, two green pixels, and one blue pixel, so combine them into a single RGB triple.
            rgb.clear(); rgb.resize(width / 2 * height / 2 * 3);
            auto out = rgb.data(); auto in0 = reinterpret_cast<const uint8_t *>(data), in1 = in0 + width * 5 / 4;
            for (auto y = 0; y<height; y += 2)
            {
                for (auto x = 0; x<width; x += 4)
                {
                    *out++ = in0[0]; *out++ = (in0[1] + in1[0]) / 2; *out++ = in1[1]; // RGRG -> RGB RGB
                    *out++ = in0[2]; *out++ = (in0[3] + in1[2]) / 2; *out++ = in1[3]; // GBGB
                    in0 += 5; in1 += 5;
                }
                in0 = in1; in1 += width * 5 / 4;
            }
            glPixelStorei(GL_UNPACK_ROW_LENGTH, width / 2);        // Update row stride to reflect post-downsampling dimensions of the target texture
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width / 2, height / 2, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
        }
        break;
        default:
            throw std::runtime_error("The requested format is not suported for rendering");
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }


    void upload(rs2::frame& frame)
    {
        upload(frame.get_data(), frame.get_width(), frame.get_height(), frame.get_format());
    }

    void show(const rect& r) const
    {
        glBindTexture(GL_TEXTURE_2D, texture);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(r.x, r.y);
        glTexCoord2f(1, 0); glVertex2f(r.x + r.w, r.y);
        glTexCoord2f(1, 1); glVertex2f(r.x + r.w, r.y + r.h);
        glTexCoord2f(0, 1); glVertex2f(r.x, r.y + r.h);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

#ifdef _WINDOWS
#include <Windows.h>
#else
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

inline std::vector<std::string> enumerate_dir(const std::string& directory)
{
    using namespace std;
    vector<std::string> out;
#ifdef _WINDOWS
    HANDLE dir;
    WIN32_FIND_DATAA file_data;

    if ((dir = FindFirstFileA((directory + "/*").c_str(), &file_data)) == INVALID_HANDLE_VALUE)
        throw std::runtime_error("INVALID_HANDLE_VALUE");

    do {
        const std::string file_name = file_data.cFileName;
        const auto full_file_name = directory + "/" + file_name;
        const auto is_directory = (file_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;

        if (file_name[0] == '.')
            continue;

        if (is_directory)
            continue;

        out.push_back(full_file_name);
    } while (FindNextFileA(dir, &file_data));

    FindClose(dir);
#else
    DIR *dir;
    class dirent *ent;
    class stat st;

    dir = opendir(directory.c_str());
    if (dir == nullptr) throw std::runtime_error("Directory not found!");

    while ((ent = readdir(dir)) != NULL) {
        const string file_name = ent->d_name;
        const string full_file_name = directory + "/" + file_name;

        if (file_name[0] == '.')
            continue;

        if (stat(full_file_name.c_str(), &st) == -1)
            continue;

        const bool is_directory = (st.st_mode & S_IFDIR) != 0;

        if (is_directory)
            continue;

        out.push_back(full_file_name);
    }
    closedir(dir);
#endif

    return out;
} // GetFilesInDirectory
