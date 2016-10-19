// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <sstream>
#include <vector>

inline void make_depth_histogram(uint8_t rgb_image[640*480*3], const uint16_t depth_image[], int width, int height)
{
    static uint32_t histogram[0x10000];
    memset(histogram, 0, sizeof(histogram));

    for(int i = 0; i < width*height; ++i) ++histogram[depth_image[i]];
    for(int i = 2; i < 0x10000; ++i) histogram[i] += histogram[i-1]; // Build a cumulative histogram for the indices in [1,0xFFFF]
    for(int i = 0; i < 640*480; ++i)
    {
        if(uint16_t d = depth_image[i])
        {
            int f = histogram[d] * 255 / histogram[0xFFFF]; // 0-255 based on histogram location
            rgb_image[i*3 + 0] = 255 - f;
            rgb_image[i*3 + 1] = 0;
            rgb_image[i*3 + 2] = f;
        }
        else
        {
            rgb_image[i*3 + 0] = 20;
            rgb_image[i*3 + 1] = 5;
            rgb_image[i*3 + 2] = 0;
        }
    }
}

//////////////////////////////
// Simple font loading code //
//////////////////////////////

#include "third_party/stb_easy_font.h"

inline int get_text_width(const char * text)
{
    return stb_easy_font_width((char *)text);
}

inline void draw_text(int x, int y, const char * text)
{
    std::vector<char> buffer(60000); // ~300 chars
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, buffer.data());
    glDrawArrays(GL_QUADS, 0, 4*stb_easy_font_print((float)x, (float)(y-7), (char *)text, nullptr, buffer.data(), (int)buffer.size()));
    glDisableClientState(GL_VERTEX_ARRAY);
}

////////////////////////
// Image display code //
////////////////////////

class texture_buffer
{
    GLuint texture;
    double last_timestamp;
    std::vector<uint8_t> rgb;

    int fps, num_frames;
    double next_time;

public:
    texture_buffer() : texture(), last_timestamp(-1), fps(), num_frames(), next_time(1000) {}

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
        case RS_FORMAT_RGB8: RS_FORMAT_BGR8: // Display both RGB and BGR by interpreting them RGB, to show the flipped byte ordering. Obviously, GL_BGR could be used on OpenGL 1.2+
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            break;
        case RS_FORMAT_BGRA8: RS_FORMAT_BGRA8: // Display both RGBA and BGRA by interpreting them RGBA, to show the flipped byte ordering. Obviously, GL_BGRA could be used on OpenGL 1.2+
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
                for(int y=0; y<height; y+=2)
                {
                    for(int x=0; x<width; x+=4)
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
        const double timestamp = frame.get_timestamp();
        //if(timestamp != last_timestamp)
        {
            upload(frame.get_data(), frame.get_width(), frame.get_height(), frame.get_format(), (frame.get_stride_in_bytes() * 8) / frame.get_bits_per_pixel());
            last_timestamp = timestamp;

            ++num_frames;
            if(timestamp >= next_time)
            {
                fps = num_frames;
                num_frames = 0;
                next_time += 1000;
            }
        }
    }

    void show(float rx, float ry, float rw, float rh, float alpha) const
    {


        glEnable(GL_BLEND);

        glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);
        glBegin(GL_QUADS);
        glColor4f(1.0f, 1.0f, 1.0f, 1 - alpha);
        glVertex2f(rx, ry);
        glVertex2f(rx + rw, ry);
        glVertex2f(rx + rw, ry + rh);
        glVertex2f(rx, ry + rh);
        glEnd();

        glBindTexture(GL_TEXTURE_2D, texture);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(rx, ry);
        glTexCoord2f(1, 0); glVertex2f(rx + rw, ry);
        glTexCoord2f(1, 1); glVertex2f(rx + rw, ry + rh);
        glTexCoord2f(0, 1); glVertex2f(rx, ry + rh);
        glEnd();
        glDisable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, 0);


        glDisable(GL_BLEND);
    }

    void print(int x, int y, const char * text)
    {
        std::vector<char> buffer(20000); // ~100 chars

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 16, buffer.data());
        glDrawArrays(GL_QUADS, 0, 4 * stb_easy_font_print((float)x, (float)y, (char *)text, nullptr, buffer.data(), (int)buffer.size()));
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    void show(rs::frame& frame, int rx, int ry, int rw, int rh)
    {
        upload(frame);
        
        int width = frame.get_width(), height = frame.get_height();
        float h = (float)rh, w = (float)rh * width / height;
        if(w > rw)
        {
            float scale = rw/w;
            w *= scale;
            h *= scale;
        }

        show(rx + (rw - w)/2, ry + (rh - h)/2, w, h, 1.0f);
        //std::ostringstream ss; ss << stream << ": " << width << " x " << height << " " << dev.get_stream_format(stream) << " (" << fps << "/" << dev.get_stream_framerate(stream) << ")" << ", F#: " << dev.get_frame_number(stream);
        //glColor3f(0,0,0);
        //draw_text(rx+9, ry+17, ss.str().c_str());
        //glColor3f(1,1,1);
        //draw_text(rx+8, ry+16, ss.str().c_str());
    }

    void show(int rx, int ry, int rw, int rh, int width, int height, float alpha)
    {
        float h = (float)rh, w = (float)rh * width / height;
        if (w > rw)
        {
            float scale = rw / w;
            w *= scale;
            h *= scale;
        }

        show(rx + (rw - w) / 2, ry + (rh - h) / 2, w, h, alpha);
    }
};

inline void draw_depth_histogram(const uint16_t depth_image[], int width, int height)
{
    static uint8_t rgb_image[640*480*3];
    make_depth_histogram(rgb_image, depth_image, width, height);
    glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb_image);
}
