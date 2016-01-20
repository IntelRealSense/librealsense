// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>

#include <sstream>
#include <vector>

inline void make_depth_histogram(uint8_t rgb_image[640*480*3], const uint16_t depth_image[], int width, int height)
{
    static uint32_t histogram[0x10000];
    int i, d, f;
    memset(histogram, 0, sizeof(histogram));

    for(i = 0; i < width*height; ++i) ++histogram[depth_image[i]];
    for(i = 2; i < 0x10000; ++i) histogram[i] += histogram[i-1]; // Build a cumulative histogram for the indices in [1,0xFFFF]
    for(i = 0; i < width*height; ++i)
    {
        if(d = depth_image[i])
        {
            f = histogram[d] * 255 / histogram[0xFFFF]; // 0-255 based on histogram location
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

#define STB_TRUETYPE_IMPLEMENTATION
#include "third_party/stb_truetype.h"

class gl_font
{
    GLuint tex;
    stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyph
public:
    gl_font(float height) : tex()
    {
        FILE * f = 0;
        std::string path = "examples/assets/Roboto-Bold.ttf";
        for(int i=0; i<=3; ++i)
        {
            if(f = fopen(path.c_str(), "rb")) break;
            path = "../" + path;
        }
        if(!f) throw std::runtime_error("Could not find font file!");

        int buffer_size;
        void * buffer;
        unsigned char temp_bitmap[512*512];
    
        fseek(f, 0, SEEK_END);
        buffer_size = ftell(f);
    
        buffer = malloc(buffer_size);
        fseek(f, 0, SEEK_SET);
        fread(buffer, 1, buffer_size, f);
        fclose(f);
    
        stbtt_BakeFontBitmap((unsigned char*)buffer,0, height, temp_bitmap, 512,512, 32,96, cdata);
        free(buffer);
    
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 512,512, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    }

    float width(const char * text)
    {
        float x=0, y=0;
        for(; *text; ++text)
        {
            if (*text >= 32 && *text < 128)
            {
                stbtt_aligned_quad q;
                stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1);
            }
        }
        return x;
    }

    void print(float x, float y, const char * text)
    {
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, tex);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glBegin(GL_QUADS);
        for(; *text; ++text)
        {
            if (*text >= 32 && *text < 128)
            {
                stbtt_aligned_quad q;
                stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1);
                glTexCoord2f(q.s0,q.t0); glVertex2f(q.x0,q.y0);
                glTexCoord2f(q.s1,q.t0); glVertex2f(q.x1,q.y0);
                glTexCoord2f(q.s1,q.t1); glVertex2f(q.x1,q.y1);
                glTexCoord2f(q.s0,q.t1); glVertex2f(q.x0,q.y1);
            }
        }
        glEnd();
        glPopAttrib();    
    }
};

////////////////////////
// Image display code //
////////////////////////

class texture_buffer
{
    GLuint texture;
    int last_timestamp;
    std::vector<uint8_t> rgb;

    int fps, num_frames, next_time;
public:
    texture_buffer() : texture(), last_timestamp(-1), fps(), num_frames(), next_time(1000) {}

    GLuint get_gl_handle() const { return texture; }

    void upload(const void * data, int width, int height, rs::format format)
    {
        // If the frame timestamp has changed since the last time show(...) was called, re-upload the texture
        if(!texture) glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        switch(format)
        {
        case rs::format::z16:
        case rs::format::disparity16:
            rgb.resize(width * height * 3);
            make_depth_histogram(rgb.data(), reinterpret_cast<const uint16_t *>(data), width, height);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
            break;
        case rs::format::yuyv: // Display YUYV by showing the luminance channel and packing chrominance into ignored alpha channel
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, data); 
            break;
        case rs::format::rgb8: case rs::format::bgr8: // Display both RGB and BGR by interpreting them RGB, to show the flipped byte ordering. Obviously, GL_BGR could be used on OpenGL 1.2+
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            break;
        case rs::format::rgba8: case rs::format::bgra8: // Display both RGBA and BGRA by interpreting them RGBA, to show the flipped byte ordering. Obviously, GL_BGRA could be used on OpenGL 1.2+
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
            break;
        case rs::format::y8:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, data);
            break;
        case rs::format::y16:
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_SHORT, data);
            break;
        case rs::format::raw10:
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
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width/2, height/2, 0, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
            break;
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void upload(rs::device & dev, rs::stream stream)
    {
        assert(dev.is_stream_enabled(stream));

        const int timestamp = dev.get_frame_timestamp(stream);
        if(timestamp != last_timestamp)
        {
            upload(dev.get_frame_data(stream), dev.get_stream_width(stream), dev.get_stream_height(stream), dev.get_stream_format(stream));
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

    void show(float rx, float ry, float rw, float rh) const
    {
        glBindTexture(GL_TEXTURE_2D, texture);
        glEnable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(rx,    ry   );
        glTexCoord2f(1, 0); glVertex2f(rx+rw, ry   );
        glTexCoord2f(1, 1); glVertex2f(rx+rw, ry+rh);
        glTexCoord2f(0, 1); glVertex2f(rx,    ry+rh);
        glEnd();    
        glDisable(GL_TEXTURE_2D);    
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void show(rs::device & dev, rs::stream stream, int rx, int ry, int rw, int rh, gl_font & font)
    {
        if(!dev.is_stream_enabled(stream)) return;

        upload(dev, stream);
        
        int width = dev.get_stream_width(stream), height = dev.get_stream_height(stream);
        float h = (float)rh, w = (float)rh * width / height;
        if(w > rw)
        {
            float scale = rw/w;
            w *= scale;
            h *= scale;
        }

        show(rx + (rw - w)/2, ry + (rh - h)/2, w, h);

        std::ostringstream ss; ss << stream << ": " << width << " x " << height << " " << dev.get_stream_format(stream) << " (" << fps << "/" << dev.get_stream_framerate(stream) << ")";
        glColor3f(0,0,0);
        font.print(rx+9.0f, ry+17.0f, ss.str().c_str());
        glColor3f(1,1,1);
        font.print(rx+8.0f, ry+16.0f, ss.str().c_str());
    }

    void show(const void * data, int width, int height, rs::format format, const std::string & caption, int rx, int ry, int rw, int rh, gl_font & font)
    {
        if(!data) return;

        upload(data, width, height, format);
        
        float h = (float)rh, w = (float)rh * width / height;
        if(w > rw)
        {
            float scale = rw/w;
            w *= scale;
            h *= scale;
        }

        show(rx + (rw - w)/2, ry + (rh - h)/2, w, h);

        std::ostringstream ss; ss << caption << ": " << width << " x " << height << " " << format;
        glColor3f(0,0,0);
        font.print(rx+9.0f, ry+17.0f, ss.str().c_str());
        glColor3f(1,1,1);
        font.print(rx+8.0f, ry+16.0f, ss.str().c_str());
    }
};

inline void draw_depth_histogram(const uint16_t depth_image[], int width, int height)
{
    static uint8_t rgb_image[640*480*3];
    make_depth_histogram(rgb_image, depth_image, width, height);
    glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, rgb_image);
}
