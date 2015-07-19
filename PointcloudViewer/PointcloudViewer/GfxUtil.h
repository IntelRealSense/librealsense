#ifndef DS4OSX_Util_h
#define DS4OSX_Util_h

#include <OpenGL/gl.h>
#include <OpenGL/gl3.h>

#ifndef CHECK_GL_ERROR
#define CHECK_GL_ERROR() CheckGLError(__FILE__, __LINE__)
#endif

template <typename T>
T max(T a, T b, T c)
{
    return std::max(a, std::max(b, c));
}

template <typename T>
T min(T a, T b, T c)
{
    return std::min(a, std::min(b, c));
}

template<class T> T
clamp(T a, T mn, T mx)
{
    return std::max(std::min(a, mx), mn);
}

template <typename T, bool clamp, int inputMin, int inputMax>
inline T remapInt(T value, float outputMin, float outputMax)
{
    T invVal = 1.0f / (inputMax - inputMin);
    T outVal = (invVal*(value - inputMin) * (outputMax - outputMin) + outputMin);
    if (clamp)
    {
        if (outputMax < outputMin)
        {
            if (outVal < outputMax) outVal = outputMax;
            else if (outVal > outputMin) outVal = outputMin;
        }
        else
        {
            if (outVal > outputMax) outVal = outputMax;
            else if (outVal < outputMin) outVal = outputMin;
        }
    }
    return outVal;
}

template <typename T>
inline T remap(T value, T inputMin, T inputMax, T outputMin, T outputMax, bool clamp)
{
    T outVal = ((value - inputMin) / (inputMax - inputMin) * (outputMax - outputMin) + outputMin);
    if (clamp)
    {
        if (outputMax < outputMin)
        {
            if (outVal < outputMax) outVal = outputMax;
            else if (outVal > outputMin) outVal = outputMin;
        }
        else
        {
            if (outVal > outputMax) outVal = outputMax;
            else if (outVal < outputMin) outVal = outputMin;
        }
    }
    return outVal;
}

inline uint8_t clampbyte(int v)
{
    return v < 0 ? 0 : v > 255 ? 255 : v;
}

void convert_yuyv_rgb(const uint8_t *src, int width, int height, uint8_t *dst);

void CheckGLError(const char* file, int32_t line);

GLuint CreateGLProgram(const std::string & vertSource, const std::string & fragSource);

GLuint CreateTexture(int width, int height, GLint internalFmt);

void ConvertDepthToRGBUsingHistogram(uint8_t img[], const uint16_t depthImage[], int width, int height, const float nearHue, const float farHue);

std::array<double, 3> rgbToHsv(uint8_t r, uint8_t g, uint8_t b);

std::array<int, 3> hsvToRgb(double h, double s, double v);

#endif
