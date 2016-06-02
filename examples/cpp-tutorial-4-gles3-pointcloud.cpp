// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

//////////////////////////////////////////////////////////////////
// librealsense tutorial #4 - OpenGL ES3 Point cloud generation //
//////////////////////////////////////////////////////////////////

// First include the librealsense C++ header file
#include <librealsense/rs.hpp>

#include <cmath>
#include <memory>

// Also include GLFW to allow for graphical display
#define GLFW_INCLUDE_ES3
#include <GLFW/glfw3.h>

#define SHADER(x) #x

namespace examples
{
    class matrix
    {
    public:
        matrix() { init_identity(); }
        matrix(const float (&rotation)[9], const float (&translation)[3])
        {
            init_identity();
            m_[0][0] = rotation[0];
            m_[0][1] = rotation[1];
            m_[0][2] = rotation[2];
            m_[1][0] = rotation[3];
            m_[1][1] = rotation[4];
            m_[1][2] = rotation[5];
            m_[2][0] = rotation[6];
            m_[2][1] = rotation[7];
            m_[2][2] = rotation[8];
            m_[3][0] = translation[0];
            m_[3][1] = translation[1];
            m_[3][2] = translation[2];
        }
        ~matrix() {}
        matrix(const matrix &) = default;

        void operator=(const matrix & other)
        {
            std::copy(&other.m_[0][0], &other.m_[3][3] + 1, &m_[0][0]);
        }

        const float * data() const { return &m_[0][0]; }

        void get3x3(float * m3x3) const
        {
            m3x3[0] = m_[0][0];
            m3x3[1] = m_[0][1];
            m3x3[2] = m_[0][2];
            m3x3[3] = m_[1][0];
            m3x3[4] = m_[1][1];
            m3x3[5] = m_[1][2];
            m3x3[6] = m_[2][0];
            m3x3[7] = m_[2][1];
            m3x3[8] = m_[2][2];
        }

        // |this| x |op|
        void matrix_multiply(const matrix & op)
        {
            matrix tmp;
            for(int i = 0; i < 4; i++)
            {
                tmp.m_[i][0] = (m_[i][0] * op.m_[0][0]) + (m_[i][1] * op.m_[1][0]) +
                               (m_[i][2] * op.m_[2][0]) + (m_[i][3] * op.m_[3][0]);

                tmp.m_[i][1] = (m_[i][0] * op.m_[0][1]) + (m_[i][1] * op.m_[1][1]) +
                               (m_[i][2] * op.m_[2][1]) + (m_[i][3] * op.m_[3][1]);

                tmp.m_[i][2] = (m_[i][0] * op.m_[0][2]) + (m_[i][1] * op.m_[1][2]) +
                               (m_[i][2] * op.m_[2][2]) + (m_[i][3] * op.m_[3][2]);

                tmp.m_[i][3] = (m_[i][0] * op.m_[0][3]) + (m_[i][1] * op.m_[1][3]) +
                               (m_[i][2] * op.m_[2][3]) + (m_[i][3] * op.m_[3][3]);
            }
            std::copy(&tmp.m_[0][0], &tmp.m_[3][3] + 1, &m_[0][0]);
        }

        void scale(float sx, float sy, float sz)
        {
            m_[0][0] *= sx;
            m_[0][1] *= sx;
            m_[0][2] *= sx;
            m_[0][3] *= sx;

            m_[1][0] *= sy;
            m_[1][1] *= sy;
            m_[1][2] *= sy;
            m_[1][3] *= sy;

            m_[2][0] *= sz;
            m_[2][1] *= sz;
            m_[2][2] *= sz;
            m_[2][3] *= sz;
        }

        void translate(float tx, float ty, float tz)
        {
            m_[3][0] += (m_[0][0] * tx + m_[1][0] * ty + m_[2][0] * tz);
            m_[3][1] += (m_[0][1] * tx + m_[1][1] * ty + m_[2][1] * tz);
            m_[3][2] += (m_[0][2] * tx + m_[1][2] * ty + m_[2][2] * tz);
            m_[3][3] += (m_[0][3] * tx + m_[1][3] * ty + m_[2][3] * tz);
        }

        void rotate(float angle, float x, float y, float z)
        {
            float mag = sqrt(x * x + y * y + z * z);
            if(mag > 0.0f)
            {
                float xx, yy, zz, xy, yz, zx, xs, ys, zs;
                matrix rot_mat;

                x /= mag;
                y /= mag;
                z /= mag;

                xx = x * x;
                yy = y * y;
                zz = z * z;
                xy = x * y;
                yz = y * z;
                zx = z * x;
                float sin_angle = sin(angle * M_PI / 180.0f);
                float cos_angle = cos(angle * M_PI / 180.0f);
                xs = x * sin_angle;
                ys = y * sin_angle;
                zs = z * sin_angle;
                float one_cos = 1.0f - cos_angle;

                rot_mat.m_[0][0] = (one_cos * xx) + cos_angle;
                rot_mat.m_[0][1] = (one_cos * xy) - zs;
                rot_mat.m_[0][2] = (one_cos * zx) + ys;
                rot_mat.m_[0][3] = 0.0F;

                rot_mat.m_[1][0] = (one_cos * xy) + zs;
                rot_mat.m_[1][1] = (one_cos * yy) + cos_angle;
                rot_mat.m_[1][2] = (one_cos * yz) - xs;
                rot_mat.m_[1][3] = 0.0F;

                rot_mat.m_[2][0] = (one_cos * zx) - ys;
                rot_mat.m_[2][1] = (one_cos * yz) + xs;
                rot_mat.m_[2][2] = (one_cos * zz) + cos_angle;
                rot_mat.m_[2][3] = 0.0F;

                rot_mat.m_[3][0] = 0.0F;
                rot_mat.m_[3][1] = 0.0F;
                rot_mat.m_[3][2] = 0.0F;
                rot_mat.m_[3][3] = 1.0F;

                rot_mat.matrix_multiply(*this);
                *this = rot_mat;
            }
        }

        // multiply matrix specified by result with a perspective matrix and
        // return new matrix in result
        void frustum(float left, float right, float bottom, float top, float nearZ, float farZ)
        {
            float deltaX = right - left;
            float deltaY = top - bottom;
            float deltaZ = farZ - nearZ;
            if((nearZ <= 0.0f) || (farZ <= 0.0f) || (deltaX <= 0.0f) || (deltaY <= 0.0f) ||
                (deltaZ <= 0.0f))
                return;

            matrix frust;
            frust.m_[0][0] = 2.0f * nearZ / deltaX;
            frust.m_[0][1] = frust.m_[0][2] = frust.m_[0][3] = 0.0f;

            frust.m_[1][1] = 2.0f * nearZ / deltaY;
            frust.m_[1][0] = frust.m_[1][2] = frust.m_[1][3] = 0.0f;

            frust.m_[2][0] = (right + left) / deltaX;
            frust.m_[2][1] = (top + bottom) / deltaY;
            frust.m_[2][2] = -(nearZ + farZ) / deltaZ;
            frust.m_[2][3] = -1.0f;

            frust.m_[3][2] = -2.0f * nearZ * farZ / deltaZ;
            frust.m_[3][0] = frust.m_[3][1] = frust.m_[3][3] = 0.0f;

            frust.matrix_multiply(*this);
            *this = frust;
        }

        /// multiply matrix specified by result with a perspective matrix and
        /// return new matrix in result
        void perspective(float fovy, float aspect, float nearZ, float farZ)
        {
            float frustumH = tanf(fovy / 360.0f * M_PI) * nearZ;
            float frustumW = frustumH * aspect;
            frustum(-frustumW, frustumW, -frustumH, frustumH, nearZ, farZ);
        }

        // http://stackoverflow.com/questions/19785313/rotation-moving-camera-using-gles20
        // Generate a transformation matrix from eye position, look at and up vectors
        // Returns transformation matrix
        void look_at(float eyeX, float eyeY, float eyeZ, float targetX, float targetY,
            float targetZ, float upX, float upY, float upZ)
        {
            float axisX[3], axisY[3], axisZ[3];
            float length;

            // axisZ = target - eye
            axisZ[0] = targetX - eyeX;
            axisZ[1] = targetY - eyeY;
            axisZ[2] = targetZ - eyeZ;

            // normalize axisZ
            length = sqrtf(axisZ[0] * axisZ[0] + axisZ[1] * axisZ[1] + axisZ[2] * axisZ[2]);

            if(length != 0.0f)
            {
                axisZ[0] /= length;
                axisZ[1] /= length;
                axisZ[2] /= length;
            }

            // axisX = up X axisZ
            axisX[0] = upY * axisZ[2] - upZ * axisZ[1];
            axisX[1] = upZ * axisZ[0] - upX * axisZ[2];
            axisX[2] = upX * axisZ[1] - upY * axisZ[0];

            // normalize axisX
            length = sqrtf(axisX[0] * axisX[0] + axisX[1] * axisX[1] + axisX[2] * axisX[2]);

            if(length != 0.0f)
            {
                axisX[0] /= length;
                axisX[1] /= length;
                axisX[2] /= length;
            }

            // axisY = axisZ x axisX
            axisY[0] = axisZ[1] * axisX[2] - axisZ[2] * axisX[1];
            axisY[1] = axisZ[2] * axisX[0] - axisZ[0] * axisX[2];
            axisY[2] = axisZ[0] * axisX[1] - axisZ[1] * axisX[0];

            // normalize axisY
            length = sqrtf(axisY[0] * axisY[0] + axisY[1] * axisY[1] + axisY[2] * axisY[2]);

            if(length != 0.0f)
            {
                axisY[0] /= length;
                axisY[1] /= length;
                axisY[2] /= length;
            }

            init_identity();
            m_[0][0] = -axisX[0];
            m_[0][1] = axisY[0];
            m_[0][2] = -axisZ[0];

            m_[1][0] = -axisX[1];
            m_[1][1] = axisY[1];
            m_[1][2] = -axisZ[1];

            m_[2][0] = -axisX[2];
            m_[2][1] = axisY[2];
            m_[2][2] = -axisZ[2];

            // translate (-eyeX, -eyeY, -eyeZ)
            m_[3][0] = axisX[0] * eyeX + axisX[1] * eyeY + axisX[2] * eyeZ;
            m_[3][1] = -axisY[0] * eyeX - axisY[1] * eyeY - axisY[2] * eyeZ;
            m_[3][2] = axisZ[0] * eyeX + axisZ[1] * eyeY + axisZ[2] * eyeZ;
            m_[3][3] = 1.0f;
        }

        // |point| x |this|
        void map(const float (&point)[3], float (&out)[3])
        {
            for(int i = 0; i <= 2; i++)
            {
                out[i] = (m_[0][i] * point[0]) + (m_[1][i] * point[1]) + (m_[2][i] * point[2]) +
                         m_[3][i];
            }
        }

    private:
        void init_identity()
        {
            std::memset(m_, 0, sizeof m_);
            m_[0][0] = 1.0f;
            m_[1][1] = 1.0f;
            m_[2][2] = 1.0f;
            m_[3][3] = 1.0f;
        }

        float m_[4][4];
    };

    class point_cloud;

    // Because glfw API doesn't hold a user data.
    point_cloud * g_point_cloud = nullptr;

    class point_cloud
    {
    public:
        point_cloud() : dev_(nullptr), win_(nullptr) { g_point_cloud = this; }
        ~point_cloud()
        {
            dev_->stop();

            glDeleteTextures(1, &color_texture_);
            glDeleteTextures(1, &depth_texture_);
            glDeleteProgram(program_);
        }
        point_cloud(const point_cloud &) = delete;
        void operator=(const point_cloud &) = delete;

        bool initialize()
        {
            // Turn on logging. We can separately enable logging to console or to file, and use
            // different severity
            // filters for each.
            rs::log_to_console(rs::log_severity::warn);
            // rs::log_to_file(rs::log_severity::debug, "librealsense.log");

            printf("There are %d connected RealSense devices.\n", ctx_.get_device_count());
            if(ctx_.get_device_count() == 0) return EXIT_FAILURE;

            // This tutorial will access only a single device, but it is trivial to extend to
            // multiple devices
            dev_ = ctx_.get_device(0);
            printf("\nUsing device 0, an %s\n", dev_->get_name());
            printf("    Serial number: %s\n", dev_->get_serial());
            printf("    Firmware version: %s\n", dev_->get_firmware_version());

            // Configure depth and color to run with the device's preferred settings
            dev_->enable_stream(rs::stream::depth, rs::preset::best_quality);
            dev_->enable_stream(rs::stream::color, rs::preset::best_quality);
            dev_->start();

            // Open a GLFW window to display our output
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
            win_ = glfwCreateWindow(display_size_.width, display_size_.height,
                "librealsense tutorial #4", nullptr, nullptr);
            glfwSetCursorPosCallback(win_, on_cursor_pos);
            glfwSetMouseButtonCallback(win_, on_mouse_button);
            glfwMakeContextCurrent(win_);

            // Need to do the first mode setting before page flip.
            if(!initialize_gl()) return false;
            return true;
        }

        void run()
        {
            while(!glfwWindowShouldClose(win_))
            {
                // Wait for new frame data
                glfwPollEvents();
                dev_->wait_for_frames();
                draw();
                glfwSwapBuffers(win_);
            }
        }

    private:
        bool initialize_gl()
        {
            if(!initialize_gl_program()) return false;

            GLuint textures[2] = {0};
            glGenTextures(2, &textures[0]);
            color_texture_ = textures[0];
            depth_texture_ = textures[1];

            rs::intrinsics color_intrin = dev_->get_stream_intrinsics(rs::stream::color);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, color_texture_);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, color_intrin.width, color_intrin.height, 0,
                GL_RGB, GL_UNSIGNED_BYTE, nullptr);

            rs::intrinsics depth_intrin = dev_->get_stream_intrinsics(rs::stream::depth);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, depth_texture_);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, depth_intrin.width, depth_intrin.height, 0,
                GL_RED_INTEGER, GL_UNSIGNED_SHORT, nullptr);

            glViewport(0, 0, display_size_.width, display_size_.height);
            glClearColor(0.0, 0.0, 0.0, 1.0);
            glClear(GL_COLOR_BUFFER_BIT);

            GLenum error = glGetError();
            if(error == GL_NO_ERROR) return true;

            printf("Fail to initialize GL resources. code:%x \n", error);
            return false;
        }

        bool initialize_gl_program()
        {
            // clang-format off
            static const char* vertex_shader_source =
              "#version 300 es\n"
              "#define RS_DISTORTION_NONE 0\n"
              "#define RS_DISTORTION_MODIFIED_BROWN_CONRADY 1\n"
              "#define RS_DISTORTION_INVERSE_BROWN_CONRADY 2\n"
              SHADER(
                  uniform mat4 u_mvp;
                  uniform vec2 u_color_size;
                  uniform vec2 u_depth_size;
                  uniform highp usampler2D s_depth_texture;

                  uniform float u_depth_scale_in_meter;
                  uniform mat4 u_depth_to_color;

                  uniform vec2 u_color_offset;
                  uniform vec2 u_color_focal_length;
                  uniform float u_color_coeffs[5];
                  uniform int u_color_model;

                  uniform vec2 u_depth_offset;
                  uniform vec2 u_depth_focal_length;
                  uniform float u_depth_coeffs[5];
                  uniform int u_depth_model;

                  out vec2 v_tex;

                  vec3 depth_deproject(vec2 pixel, float depth)
                  {
                      vec2 point = (pixel - u_depth_offset) / u_depth_focal_length;
                      if(u_depth_model == RS_DISTORTION_INVERSE_BROWN_CONRADY)
                      {
                          float r2 = dot(point, point);
                          float f = 1.0 + u_depth_coeffs[0] * r2 + u_depth_coeffs[1] * r2 * r2 +
                                    u_depth_coeffs[4] * r2 * r2 * r2;
                          float ux = point.x * f + 2.0 * u_depth_coeffs[2] * point.x * point.y +
                                     u_depth_coeffs[3] * (r2 + 2.0 * point.x * point.x);
                          float uy = point.y * f + 2.0 * u_depth_coeffs[3] * point.x * point.y +
                                     u_depth_coeffs[2] * (r2 + 2.0 * point.y * point.y);
                          point = vec2(ux, uy);
                      }
                      return vec3(point * depth, depth);
                  }

                  vec2 color_project(vec3 point)
                  {
                      vec2 pixel = point.xy / point.z;
                      if(u_color_model == RS_DISTORTION_MODIFIED_BROWN_CONRADY)
                      {
                          float r2 = dot(pixel, pixel);
                          float f = 1.0 + u_color_coeffs[0] * r2 + u_color_coeffs[1] * r2 * r2 +
                                    u_color_coeffs[4] * r2 * r2 * r2;
                          pixel = pixel * f;
                          float dx = pixel.x + 2.0 * u_color_coeffs[2] * pixel.x * pixel.y +
                                     u_color_coeffs[3] * (r2 + 2.0 * pixel.x * pixel.x);
                          float dy = pixel.y + 2.0 * u_color_coeffs[3] * pixel.x * pixel.y +
                                     u_color_coeffs[2] * (r2 + 2.0 * pixel.y * pixel.y);
                          pixel = vec2(dx, dy);
                      }
                      return pixel * u_color_focal_length + u_color_offset;
                  }

                  void main()
                  {
                      vec2 depth_pixel;
                      // generate lattice pos; (0, 0) (1, 0) (2, 0) ... (w-1, h-1)
                      depth_pixel.x = mod(float(gl_VertexID), u_depth_size.x);
                      depth_pixel.y = clamp(floor(float(gl_VertexID) / u_depth_size.y), 0.0, u_depth_size.y);

                      // get depth
                      vec2 depth_tex_pos = depth_pixel / u_depth_size;
                      uint depth = texture(s_depth_texture, depth_tex_pos).r;
                      float depth_in_meter = float(depth) * u_depth_scale_in_meter;

                      vec3 depth_point = depth_deproject(depth_pixel, depth_in_meter);
                      vec4 color_point = u_depth_to_color * vec4(depth_point, 1.0);
                      vec2 color_pixel = color_project(color_point.xyz);

                      // map [0, w) to [0, 1]
                      v_tex = color_pixel / u_color_size;

                      // http://stackoverflow.com/questions/24593939/matrix-multiplication-with-vector-in-glsl
                      gl_Position = u_mvp * vec4(depth_point, 1.0);
                      gl_PointSize = 2.0;
                  }
              );

            static const char* fragment_shader_source =
              "#version 300 es\n"
              SHADER(
                  precision highp float;
                  uniform sampler2D s_color_texture;
                  in vec2 v_tex;
                  layout(location = 0) out vec4 fragColor;
                  void main()
                  {
                      fragColor = texture(s_color_texture, v_tex);
                  }
              );
            // clang-format on

            GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);

            glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
            glCompileShader(vertex_shader);

            GLint ret = 0;
            glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &ret);
            if(!ret)
            {
                printf("vertex shader compilation failed!:\n");
                glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &ret);
                if(ret > 1)
                {
                    char * log = static_cast<char *>(malloc(ret));
                    glGetShaderInfoLog(vertex_shader, ret, NULL, log);
                    printf("%s\n", log);
                    free(log);
                }
                return false;
            }

            GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

            glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
            glCompileShader(fragment_shader);

            glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &ret);
            if(!ret)
            {
                printf("fragment shader compilation failed!:\n");
                glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &ret);
                if(ret > 1)
                {
                    char * log = static_cast<char *>(malloc(ret));
                    glGetShaderInfoLog(fragment_shader, ret, NULL, log);
                    printf("%s\n", log);
                    free(log);
                }
                return false;
            }

            program_ = glCreateProgram();

            glAttachShader(program_, vertex_shader);
            glAttachShader(program_, fragment_shader);

            glBindAttribLocation(program_, 0, "in_position");

            glLinkProgram(program_);

            glGetProgramiv(program_, GL_LINK_STATUS, &ret);
            if(!ret)
            {
                printf("program linking failed!:\n");
                glGetProgramiv(program_, GL_INFO_LOG_LENGTH, &ret);
                if(ret > 1)
                {
                    char * log = static_cast<char *>(malloc(ret));
                    glGetProgramInfoLog(program_, ret, NULL, log);
                    printf("%s\n", log);
                    free(log);
                }
                return false;
            }

            glDeleteShader(vertex_shader);
            glDeleteShader(fragment_shader);
            glUseProgram(program_);

            u_model_view_projection_matrix_ = glGetUniformLocation(program_, "u_mvp");
            u_color_size_ = glGetUniformLocation(program_, "u_color_size");
            u_depth_size_ = glGetUniformLocation(program_, "u_depth_size");
            u_color_sampler_ = glGetUniformLocation(program_, "s_color_texture");
            u_depth_sampler_ = glGetUniformLocation(program_, "s_depth_texture");

            u_depth_scale_in_meter_ = glGetUniformLocation(program_, "u_depth_scale_in_meter");
            u_depth_to_color_ = glGetUniformLocation(program_, "u_depth_to_color");

            u_color_offset_ = glGetUniformLocation(program_, "u_color_offset");
            u_color_focal_length_ = glGetUniformLocation(program_, "u_color_focal_length");
            u_color_coeffs_ = glGetUniformLocation(program_, "u_color_coeffs");
            u_color_model_ = glGetUniformLocation(program_, "u_color_model");
            u_depth_offset_ = glGetUniformLocation(program_, "u_depth_offset");
            u_depth_focal_length_ = glGetUniformLocation(program_, "u_depth_focal_length");
            u_depth_coeffs_ = glGetUniformLocation(program_, "u_depth_coeffs");
            u_depth_model_ = glGetUniformLocation(program_, "u_depth_model");
            return true;
        }

        static void on_mouse_button(GLFWwindow * win, int button, int action, int mods)
        {
            assert(g_point_cloud->win_ == win);
            g_point_cloud->mouse_button(button, action);
        }
        void mouse_button(int button, int action)
        {
            if(button == GLFW_MOUSE_BUTTON_LEFT) is_pressed_ = (action == GLFW_PRESS);
        }

        static void on_cursor_pos(GLFWwindow * win, double x, double y)
        {
            assert(g_point_cloud->win_ == win);
            g_point_cloud->cursor_pos(x, y);
        }
        double clamp(double val, double lo, double hi) { return std::max(lo, std::min(hi, val)); }
        void cursor_pos(double x, double y)
        {
            if(is_pressed_)
            {
                yaw_ = clamp(yaw_ - (x - lastX_), -120, 120);
                pitch_ = clamp(pitch_ + (y - lastY_), -80, 80);
            }
            lastX_ = x;
            lastY_ = y;
        }

        void draw()
        {
            // Set up a perspective transform in a space that we can rotate by clicking and dragging
            // the mouse
            glUniformMatrix4fv(u_model_view_projection_matrix_, 1, GL_FALSE, get_mvp().data());

            // Retrieve camera parameters for mapping between depth and color
            rs::intrinsics depth_intrin = dev_->get_stream_intrinsics(rs::stream::depth);
            rs::extrinsics depth_to_color =
                dev_->get_extrinsics(rs::stream::depth, rs::stream::color);
            rs::intrinsics color_intrin = dev_->get_stream_intrinsics(rs::stream::color);
            float scale = dev_->get_depth_scale();

            glUniform2f(u_color_size_, (float)color_intrin.width, (float)color_intrin.height);
            glUniform2f(u_depth_size_, (float)depth_intrin.width, (float)depth_intrin.height);

            // Set camera intrinsic and extrinsic to program
            glUniform1f(u_depth_scale_in_meter_, scale);
            matrix depth_to_color_matrix(depth_to_color.rotation, depth_to_color.translation);
            glUniformMatrix4fv(u_depth_to_color_, 1, GL_FALSE, depth_to_color_matrix.data());

            glUniform2f(u_color_offset_, color_intrin.ppx, color_intrin.ppy);
            glUniform2f(u_color_focal_length_, color_intrin.fx, color_intrin.fy);
            glUniform1fv(u_color_coeffs_, 5, &color_intrin.coeffs[0]);
            glUniform1i(u_color_model_, (int)color_intrin.model());

            glUniform2f(u_depth_offset_, depth_intrin.ppx, depth_intrin.ppy);
            glUniform2f(u_depth_focal_length_, depth_intrin.fx, depth_intrin.fy);
            glUniform1fv(u_depth_coeffs_, 5, &depth_intrin.coeffs[0]);
            glUniform1i(u_depth_model_, (int)depth_intrin.model());

            // Retrieve our images
            const uint16_t * depth_image =
                static_cast<const uint16_t *>(dev_->get_frame_data(rs::stream::depth));
            const uint8_t * color_image =
                static_cast<const uint8_t *>(dev_->get_frame_data(rs::stream::color));

            glUniform1i(u_color_sampler_, 0);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, color_texture_);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, color_intrin.width, color_intrin.height, GL_RGB,
                GL_UNSIGNED_BYTE, color_image);

            glUniform1i(u_depth_sampler_, 1);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, depth_texture_);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, depth_intrin.width, depth_intrin.height,
                GL_RED_INTEGER, GL_UNSIGNED_SHORT, depth_image);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDrawArrays(GL_POINTS, 0, depth_intrin.width * depth_intrin.height);
        }

        matrix get_mvp()
        {
            matrix model;
            model.translate(0, 0, 0.5f);
            model.rotate(pitch_, 1, 0, 0);
            model.rotate(yaw_, 0, 1, 0);
            model.translate(0, 0, -0.5f);

            matrix view;
            view.look_at(0, 0, 0, // eye
                0, 0, 1,          // target
                0, -1, 0);        // up


            matrix modelview = model;
            modelview.matrix_multiply(view);

            matrix projection;
            GLfloat aspect = (GLfloat)(display_size_.width) / (GLfloat)(display_size_.height);
            float field_of_view = 60.f;
            projection.perspective(field_of_view, aspect, 0.1f, 20.0f);

            matrix modelviewprojection = modelview;
            modelviewprojection.matrix_multiply(projection);
            return modelviewprojection;
        }

        // This object owns the handles to all connected realsense devices.
        rs::context ctx_;
        rs::device * dev_;
        GLFWwindow * win_;
        struct size
        {
            int width;
            int height;
        };
        size display_size_ = {1280, 960};
        GLuint program_ = 0;
        GLint u_model_view_projection_matrix_ = 0;
        GLint u_color_size_ = 0;
        GLint u_depth_size_ = 0;
        GLint u_color_sampler_ = 0;
        GLint u_depth_sampler_ = 0;

        GLint u_depth_scale_in_meter_ = 0;
        GLint u_depth_to_color_ = 0;
        GLint u_color_offset_ = 0;
        GLint u_color_focal_length_ = 0;
        GLint u_color_coeffs_ = 0;
        GLint u_color_model_ = 0;
        GLint u_depth_offset_ = 0;
        GLint u_depth_focal_length_ = 0;
        GLint u_depth_coeffs_ = 0;
        GLint u_depth_model_ = 0;

        GLuint color_texture_ = 0;
        GLuint depth_texture_ = 0;

        double yaw_ = 0.;
        double pitch_ = 0.;
        double lastX_ = 0.;
        double lastY_ = 0.;
        bool is_pressed_ = false;
    };

} // namespace examples

int main() try
{
    std::unique_ptr<examples::point_cloud> point_cloud(new examples::point_cloud());
    if(!point_cloud->initialize())
    {
        fprintf(stderr, "failed to initialize point_cloud.\n");
        return -1;
    }

    point_cloud->run();

    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    // Method calls against librealsense objects may throw exceptions of type rs::error
    printf("rs::error was thrown when calling %s(%s):\n", e.get_failed_function().c_str(),
        e.get_failed_args().c_str());
    printf("    %s\n", e.what());
    return EXIT_FAILURE;
}
