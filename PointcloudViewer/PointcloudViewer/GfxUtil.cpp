#include "GfxUtil.h"

void convert_yuyv_rgb(const uint8_t *src, int width, int height, uint8_t *dst)
{
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; x += 2)
        {
            int y0 = src[0], u0 = src[1], y1 = src[2], v0 = src[3];
            
            int c = y0 - 16, d = u0 - 128, e = v0 - 128;
            dst[0] = clampbyte((298 * c + 409 * e + 128) >> 8);           // r
            dst[1] = clampbyte((298 * c - 100 * d - 208 * e + 128) >> 8); // g
            dst[2] = clampbyte((298 * c + 516 * d + 128) >> 8);           // b
            
            c = y1 - 16;
            dst[3] = clampbyte((298 * c + 409 * e + 128) >> 8);           // r
            dst[4] = clampbyte((298 * c - 100 * d - 208 * e + 128) >> 8); // g
            dst[5] = clampbyte((298 * c + 516 * d + 128) >> 8);           // b
            
            src += 4;
            dst += 6;
        }
    }
}

void CheckGLError(const char* file, int32_t line)
{
    GLint error = glGetError();
    if (error)
    {
        const char * errorStr = 0;
        switch (error)
        {
            case GL_INVALID_ENUM: errorStr = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE: errorStr = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: errorStr = "GL_INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY: errorStr = "GL_OUT_OF_MEMORY"; break;
            default: errorStr = "unknown error"; break;
        }
        printf("GL error : %s, line %d : %s\n", file, line, errorStr);
        error = 0;
    }
}

void AddShader(GLuint program, GLenum type, const char * source)
{
    auto shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint status, length;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if(status == GL_FALSE)
    {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        std::vector<GLchar> buffer(length);
        glGetShaderInfoLog(shader, buffer.size(), nullptr, buffer.data());
        glDeleteShader(shader);
        throw std::runtime_error(std::string("GLSL compile error: ") + buffer.data());
    }
    glAttachShader(program, shader);
    glDeleteShader(shader);
}

GLuint CreateGLProgram(const std::string & vertSource, const std::string & fragSource)
{
    GLuint program = glCreateProgram();
    AddShader(program, GL_VERTEX_SHADER, vertSource.c_str());
    AddShader(program, GL_FRAGMENT_SHADER, fragSource.c_str());
    
    glLinkProgram(program);
    GLint status, length;
    
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    
    if(status == GL_FALSE)
    {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        std::vector<GLchar> buffer(length);
        glGetProgramInfoLog(program, buffer.size(), nullptr, buffer.data());
        throw std::runtime_error(std::string("GLSL link error: ") + buffer.data());
    }
    
    return program;
    
}

GLuint CreateTexture(int width, int height, GLint internalFmt)
{
    GLuint newTexId;
    
    glGenTextures(1, &newTexId);
    
    glBindTexture(GL_TEXTURE_2D, newTexId);
    
    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, width, height, 0, internalFmt, GL_UNSIGNED_BYTE, 0);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    return newTexId;
}

void ConvertDepthToRGBUsingHistogram(uint8_t img[], const uint16_t depthImage[], int width, int height, const float nearHue, const float farHue)
{
    int histogram[256 * 256] = { 1 };
    
    for (int i = 0; i < width * height; ++i)
        if (auto d = depthImage[i]) ++histogram[d];
    
    for (int i = 1; i < 256 * 256; i++)
        histogram[i] += histogram[i - 1];

    for (int i = 1; i < 256 * 256; i++)
        histogram[i] = (histogram[i] << 8) / histogram[256 * 256 - 1];
    
    auto rgb = img;
    for (int i = 0; i < width * height; i++)
    {
        if (uint16_t d = depthImage[i])
        {
            auto t = histogram[d]; // Use the histogram entry (in the range of [0-256]) to interpolate between nearColor and farColor
            std::array<int, 3> returnRGB = { 0, 0, 0 };
            returnRGB = hsvToRgb(remapInt<float, true, 0, 255>(t,nearHue, farHue), 1, 1);
            *rgb++ = returnRGB[0];
            *rgb++ = returnRGB[1];
            *rgb++ = returnRGB[2];
        }
        else
        {
            *rgb++ = 0;
            *rgb++ = 0;
            *rgb++ = 0;
        }
    }
}

std::array<double, 3> rgbToHsv(uint8_t r, uint8_t g, uint8_t b)
{
    std::array<double, 3> hsv;
    double rd = (double)r / 255;
    double gd = (double)g / 255;
    double bd = (double)b / 255;
    
    double max = rs_max<double>(rd, gd, bd);
    double min = rs_min<double>(rd, gd, bd);
    double h, s, v = max;
    double d = max - min;
    
    s = (max == 0) ? 0 : (d / max);
    
    if (max == min)
    {
        h = 0; // achromatic
    }
    else
    {
        if (max == rd)
        {
            h = (gd - bd) / d + (gd < bd ? 6 : 0);
        }
        else if (max == gd)
        {
            h = (bd - rd) / d + 2;
        }
        else if (max == bd)
        {
            h = (rd - gd) / d + 4;
        }
        h /= 6;
    }
    
    hsv[0] = h;
    hsv[1] = s;
    hsv[2] = v;
    return hsv;
}

std::array<int, 3> hsvToRgb(double h, double s, double v) {
    
    std::array<int, 3> rgb;
    double r, g, b;
    int i = int(h * 6);
    double f = h * 6 - i;
    double p = v * (1 - s);
    double q = v * (1 - f * s);
    double t = v * (1 - (1 - f) * s);
    
    switch (i % 6)
    {
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
    }
    rgb[0] = uint8_t(rs_clamp((float) r * 255.0f, 0.0f, 255.0f));
    rgb[1] = uint8_t(rs_clamp((float) g * 255.0f, 0.0f, 255.0f));
    rgb[2] = uint8_t(rs_clamp((float) b * 255.0f, 0.0f, 255.0f));
    return rgb;
}

void drawTexture(GLuint prog, GLuint vbo, GLuint texHandle, GLuint texId, void * pixels, int width, int height, GLint fmt, GLenum type)
{
    CHECK_GL_ERROR();
    
    glUseProgram(prog);
    
    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexImage2D(GL_TEXTURE_2D, 0, fmt, width, height, 0, fmt, type, pixels);
    glUniform1i(texHandle, 0); // Set our "u_image" sampler to user Texture Unit 0
    
    CHECK_GL_ERROR();
    
    // 1st attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(
                          0,                  // attribute 0 (layout location 1)
                          3,                  // size
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );
    
    CHECK_GL_ERROR();
    
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glDisableVertexAttribArray(0);
    
    glUseProgram(0);
}