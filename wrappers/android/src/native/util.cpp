// License: Apache 2.0. See LICENSE file in root directory.
// meng.yu1@intel.com

#include "util.h"
#include "cde_log.h"

static void DrawRGB8(ANativeWindow_Buffer &buffer, const uint8_t *in, int in_width, int in_height, int in_stride_in_bytes) {
    uint8_t *out = static_cast<uint8_t *>(buffer.bits);
    for (int y = 0; y < buffer.height; ++y) {
        for (int x = 0; x < buffer.width; ++x) {
            out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_stride_in_bytes + x * 3 + 0];
            out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_stride_in_bytes + x * 3 + 1];
            out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_stride_in_bytes + x * 3 + 2];
            out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;
        }
    }
}

static void DrawBGR8(ANativeWindow_Buffer &buffer, const uint8_t *in, int in_width, int in_height, int in_stride_in_bytes) {
    uint8_t *out = static_cast<uint8_t *>(buffer.bits);
    for (int y = 0; y < buffer.height; ++y) {
        for (int x = 0; x < buffer.width; ++x) {
            out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_stride_in_bytes + x * 3 + 2];
            out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_stride_in_bytes + x * 3 + 1];
            out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_stride_in_bytes + x * 3 + 0];
            out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;
        }
    }
}

static void DrawRGBA8(ANativeWindow_Buffer &buffer, const uint8_t *in, int in_width, int in_height, int in_stride_in_bytes) {
    uint8_t *out = static_cast<uint8_t *>(buffer.bits);
    for (int y = 0; y < buffer.height; ++y) {
        for (int x = 0; x < buffer.width; ++x) {
            out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_stride_in_bytes + x * 4 + 0];
            out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_stride_in_bytes + x * 4 + 1];
            out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_stride_in_bytes + x * 4 + 2];
            out[y * buffer.stride * 4 + x * 4 + 3] = in[y * in_stride_in_bytes + x * 4 + 3];
        }
    }
}

static void DrawBGRA8(ANativeWindow_Buffer &buffer, const uint8_t *in, int in_width, int in_height, int in_stride_in_bytes) {
    uint8_t *out = static_cast<uint8_t *>(buffer.bits);
    for (int y = 0; y < buffer.height; ++y) {
        for (int x = 0; x < buffer.width; ++x) {
            out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_stride_in_bytes + x * 4 + 2];
            out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_stride_in_bytes + x * 4 + 1];
            out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_stride_in_bytes + x * 4 + 0];
            out[y * buffer.stride * 4 + x * 4 + 3] = in[y * in_stride_in_bytes + x * 4 + 3];
        }
    }
}

static void DrawY8(ANativeWindow_Buffer &buffer, const uint8_t *in, int in_width, int in_height, int in_stride_in_bytes) {
    uint8_t *out = static_cast<uint8_t *>(buffer.bits);
    for (int y = 0; y < buffer.height; ++y) {
        for (int x = 0; x < buffer.width; ++x) {
            out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_stride_in_bytes + x];
            out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_stride_in_bytes + x];
            out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_stride_in_bytes + x];
            out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;
        }
    }
}

static void DrawY16(ANativeWindow_Buffer &buffer, const uint8_t *in, int in_width, int in_height, int in_stride_in_bytes) {
    uint8_t *out = static_cast<uint8_t *>(buffer.bits);
    for (int y = 0; y < buffer.height; ++y) {
        for (int x = 0; x < buffer.width; ++x) {
            out[y * buffer.stride * 4 + x * 4 + 0] = in[y * in_stride_in_bytes + x * 2 + 1];
            out[y * buffer.stride * 4 + x * 4 + 1] = in[y * in_stride_in_bytes + x * 2 + 1];
            out[y * buffer.stride * 4 + x * 4 + 2] = in[y * in_stride_in_bytes + x * 2 + 1];
            out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;
        }
    }
}

static void DrawYUYV(ANativeWindow_Buffer &buffer, const uint8_t *in, int in_width, int in_height, int in_stride_in_bytes) {
    uint8_t *out = static_cast<uint8_t *>(buffer.bits);
    for (int y = 0; y < buffer.height; ++y) {
        for (int x = 0; x < buffer.width; x += 2) {
            int y1 = in[y * in_stride_in_bytes + x * 2 + 0];
            int u = in[y * in_stride_in_bytes + x * 2 + 1];
            int y2 = in[y * in_stride_in_bytes + x * 2 + 2];
            int v = in[y * in_stride_in_bytes + x * 2 + 3];

            int32_t c1 = y1 - 16;
            int32_t c2 = y2 - 16;
            int32_t d = u - 128;
            int32_t e = v - 128;

            int32_t t;
#define clamp(x)  ((t=(x)) > 255 ? 255 : t < 0 ? 0 : t)
            int r1 = clamp((298 * c1 + 409 * e + 128) >> 8);
            int g1 = clamp((298 * c1 - 100 * d - 208 * e + 128) >> 8);
            int b1 = clamp((298 * c1 + 516 * d + 128) >> 8);
            int r2 = clamp((298 * c2 + 409 * e + 128) >> 8);
            int g2 = clamp((298 * c2 - 100 * d - 208 * e + 128) >> 8);
            int b2 = clamp((298 * c2 + 516 * d + 128) >> 8);
#undef clamp

            out[y * buffer.stride * 4 + x * 4 + 0] = (uint8_t) r1;
            out[y * buffer.stride * 4 + x * 4 + 1] = (uint8_t) g1;
            out[y * buffer.stride * 4 + x * 4 + 2] = (uint8_t) b1;
            out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;

            out[y * buffer.stride * 4 + x * 4 + 4] = (uint8_t) r2;
            out[y * buffer.stride * 4 + x * 4 + 5] = (uint8_t) g2;
            out[y * buffer.stride * 4 + x * 4 + 6] = (uint8_t) b2;
            out[y * buffer.stride * 4 + x * 4 + 7] = 0xFF;
        }
    }
}

static void DrawUYVY(ANativeWindow_Buffer &buffer, const uint8_t *in, int in_width, int in_height, int in_stride_in_bytes) {
    uint8_t *out = static_cast<uint8_t *>(buffer.bits);
    for (int y = 0; y < buffer.height; ++y) {
        for (int x = 0; x < buffer.width; x += 2) {
            int y1 = in[y * in_stride_in_bytes + x * 2 + 1];
            int u = in[y * in_stride_in_bytes + x * 2 + 0];
            int y2 = in[y * in_stride_in_bytes + x * 2 + 3];
            int v = in[y * in_stride_in_bytes + x * 2 + 2];

            int32_t c1 = y1 - 16;
            int32_t c2 = y2 - 16;
            int32_t d = u - 128;
            int32_t e = v - 128;

            int32_t t;
#define clamp(x)  ((t=(x)) > 255 ? 255 : t < 0 ? 0 : t)
            int r1 = clamp((298 * c1 + 409 * e + 128) >> 8);
            int g1 = clamp((298 * c1 - 100 * d - 208 * e + 128) >> 8);
            int b1 = clamp((298 * c1 + 516 * d + 128) >> 8);
            int r2 = clamp((298 * c2 + 409 * e + 128) >> 8);
            int g2 = clamp((298 * c2 - 100 * d - 208 * e + 128) >> 8);
            int b2 = clamp((298 * c2 + 516 * d + 128) >> 8);
#undef clamp

            out[y * buffer.stride * 4 + x * 4 + 0] = (uint8_t) r1;
            out[y * buffer.stride * 4 + x * 4 + 1] = (uint8_t) g1;
            out[y * buffer.stride * 4 + x * 4 + 2] = (uint8_t) b1;
            out[y * buffer.stride * 4 + x * 4 + 3] = 0xFF;

            out[y * buffer.stride * 4 + x * 4 + 4] = (uint8_t) r2;
            out[y * buffer.stride * 4 + x * 4 + 5] = (uint8_t) g2;
            out[y * buffer.stride * 4 + x * 4 + 6] = (uint8_t) b2;
            out[y * buffer.stride * 4 + x * 4 + 7] = 0xFF;
        }
    }
}

void DrawRSFrame(ANativeWindow *window, rs2::frame &frame) {
    CHECK(window != nullptr);

    static int iCount = 0;
    auto vframe = frame.as<rs2::video_frame>();
    auto profile = vframe.get_profile().as<rs2::video_stream_profile>();
    ANativeWindow_Buffer buffer = {};
    ARect rect = {0, 0, profile.width(), profile.height()};
    if (ANativeWindow_lock(window, &buffer, &rect) == 0) {
        if (profile.format() == rs2_format::RS2_FORMAT_RGB8) {
            DrawRGB8(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
        } else if (profile.format() == rs2_format::RS2_FORMAT_BGR8) {
            DrawBGR8(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
        } else if (profile.format() == rs2_format::RS2_FORMAT_RGBA8) {
            DrawRGBA8(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
        } else if (profile.format() == rs2_format::RS2_FORMAT_BGRA8) {
            DrawBGRA8(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
        } else if (profile.format() == rs2_format::RS2_FORMAT_Y8) {
            DrawY8(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
        } else if (profile.format() == rs2_format::RS2_FORMAT_Y16) {
            DrawY16(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
        } else if (profile.format() == rs2_format::RS2_FORMAT_YUYV) {
            DrawYUYV(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
        } else if (profile.format() == rs2_format::RS2_FORMAT_UYVY) {
            DrawUYVY(buffer, static_cast<const uint8_t *>(frame.get_data()), profile.width(), profile.height(), vframe.get_stride_in_bytes());
        } else {
            LOGD("format not support");
        }
        ANativeWindow_unlockAndPost(window);
    } else {
        LOGD("ANativeWindow_lock FAILED");
    }
}
