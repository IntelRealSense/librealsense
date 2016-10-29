// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <vector>
#include <algorithm>
#include <cstring>

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


inline float clamp(float x, float min, float max)
{
    return std::max(std::min(max, x), min);
}

inline float smoothstep(float x, float min, float max)
{
    x = clamp((x - min) / (max - min), 0.0, 1.0);
    return x*x*(3 - 2 * x);
}

inline float lerp(float a, float b, float t)
{
    return b * t + a * (1 - t);
}

struct float2
{
    float x, y;
};
struct rect
{
    float x, y;
    float w, h;

    bool operator==(const rect& other) const
    {
        return x == other.x && y == other.y && w == other.w && h == other.h;
    }

    rect center() const
    {
        return{ x + w / 2, y + h / 2, 0, 0 };
    }

    rect lerp(float t, const rect& other) const
    {
        return{
            ::lerp(x, other.x, t), ::lerp(y, other.y, t),
            ::lerp(w, other.w, t), ::lerp(h, other.h, t),
        };
    }

    rect adjust_ratio(float2 size) const
    {
        auto H = static_cast<float>(h), W = static_cast<float>(h) * size.x / size.y;
        if (W > w)
        {
            auto scale = w / W;
            W *= scale;
            H *= scale;
        }

        return{ x + (w - W) / 2, y + (h - H) / 2, W, H };
    }
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

    void upload(const void * data, int width, int height, rs_format format, int stride = 0)
    {
        // If the frame timestamp has changed since the last time show(...) was called, re-upload the texture
        if(!texture) glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        stride = stride == 0 ? width : stride;
        //glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
        switch(format)
        {
        case RS_FORMAT_ANY:
        throw std::runtime_error("not a valid format");
        case RS_FORMAT_Z16:
        case RS_FORMAT_DISPARITY16:
            rgb.resize(640 * 480 * 4);
            make_depth_histogram(rgb.data(), reinterpret_cast<const uint16_t *>(data), width, height);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 640, 480, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
            
            break;
        case RS_FORMAT_XYZ32F:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_FLOAT, data);
            break;
        case RS_FORMAT_YUYV: // Display YUYV by showing the luminance channel and packing chrominance into ignored alpha channel
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_RGB8: case RS_FORMAT_BGR8: // Display both RGB and BGR by interpreting them RGB, to show the flipped byte ordering. Obviously, GL_BGR could be used on OpenGL 1.2+
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_RGBA8: case RS_FORMAT_BGRA8: // Display both RGBA and BGRA by interpreting them RGBA, to show the flipped byte ordering. Obviously, GL_BGRA could be used on OpenGL 1.2+
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_Y8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB,  width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_Y16:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, data);
            break;
        case RS_FORMAT_RAW8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_RAW10:
            {
                // Visualize Raw10 by performing a naive downsample. Each 2x2 block contains one red pixel, two green pixels, and one blue pixel, so combine them into a single RGB triple.
                rgb.clear(); rgb.resize(width/2 * height/2 * 3);
                auto out = rgb.data(); auto in0 = reinterpret_cast<const uint8_t *>(data), in1 = in0 + width*5/4;
                for(auto y=0; y<height; y+=2)
                {
                    for(auto x=0; x<width; x+=4)
                    {
                        *out++ = in0[0]; *out++ = (in0[1] + in1[0]) / 2; *out++ = in1[1]; // RGRG -> RGB RGB
                        *out++ = in0[2]; *out++ = (in0[3] + in1[2]) / 2; *out++ = in1[3]; // GBGB
                        in0 += 5; in1 += 5;
                    }
                    in0 = in1; in1 += width*5/4;
                }
                glPixelStorei(GL_UNPACK_ROW_LENGTH, width / 2);        // Update row stride to reflect post-downsampling dimensions of the target texture
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width/2, height/2, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
            }
            break;
        default:
            throw std::runtime_error("The requested format is not provided by demo");
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void upload(rs::frame& frame)
    {
        upload(frame.get_data(), frame.get_width(), frame.get_height(), frame.get_format(), 
            (frame.get_stride_in_bytes() * 8) / frame.get_bits_per_pixel());
    }

    void show(const rect& r, float alpha = 1.0f) const
    {
        glEnable(GL_BLEND);

        glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
        glBegin(GL_QUADS);
        glColor4f(1.0f, 1.0f, 1.0f, 1 - alpha);
        glVertex2f(r.x, r.y);
        glVertex2f(r.x + r.w, r.y);
        glVertex2f(r.x + r.w, r.y + r.h);
        glVertex2f(r.x, r.y + r.h);
        glEnd();

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

        glDisable(GL_BLEND);
    }
};

inline void draw_depth_histogram(const uint16_t depth_image[], int width, int height)
{
    static uint8_t rgb_image[640*480*3];
    make_depth_histogram(rgb_image, depth_image, width, height);
    glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb_image);
}
