#ifndef DS4OSX_Util_h
#define DS4OSX_Util_h

#include <OpenGL/gl3.h>
#include "librealsense/Common.h"

#ifndef CHECK_GL_ERROR
#define CHECK_GL_ERROR() CheckGLError(__FILE__, __LINE__)
#endif

void convert_yuyv_rgb(const uint8_t *src, int width, int height, uint8_t *dst);

void CheckGLError(const char* file, int32_t line);

GLuint CreateGLProgram(const std::string & vertSource, const std::string & fragSource);

GLuint CreateTexture(int width, int height, GLint internalFmt);

void ConvertDepthToRGBUsingHistogram(uint8_t img[], const uint16_t depthImage[], int width, int height, const float nearHue, const float farHue);

std::array<double, 3> rgbToHsv(uint8_t r, uint8_t g, uint8_t b);

std::array<int, 3> hsvToRgb(double h, double s, double v);

void drawTexture(GLuint prog, GLuint vbo, GLuint texHandle, GLuint texId, void * pixels, int width, int height, GLint fmt, GLenum type);

#endif
