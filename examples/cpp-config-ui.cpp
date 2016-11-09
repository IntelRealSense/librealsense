#include <librealsense/rs.hpp>
#include "example.hpp"

#include <cstdarg>
#include <thread>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <mutex>
#include "concurrency.hpp"
#include <atomic>
#include <map>


#pragma comment(lib, "opengl32.lib")

template<typename T> inline T MIN(T x, T y) { return x < y ? x : y; }
template<typename T> inline T MAX(T x, T y) { return x > y ? x : y; }

struct int2 { int x, y; };
struct rect
{
    int x0, y0, x1, y1;
    bool contains(const int2 & p) const { return x0 <= p.x && y0 <= p.y && p.x < x1 && p.y < y1; }
    rect shrink(int amt) const { return{ x0 + amt, y0 + amt, x1 - amt, y1 - amt }; }
};
struct color { float r, g, b; };

std::string find_and_replace(std::string source, std::string const& find, std::string const& replace)
{
    for (std::string::size_type i = 0; (i = source.find(find, i)) != std::string::npos;)
    {
        source.replace(i, find.length(), replace);
        i += replace.length();
    }
    return source;
}

struct gui
{
    int2 cursor, clicked_offset, scroll_vec;
    bool click, mouse_down;
    int clicked_id;

    gui() : scroll_vec({ 0, 0 }), click(), mouse_down(), clicked_id() {}

    void label(const int2 & p, const color& c, const char * format, ...)
    {
        va_list args;
        va_start(args, format);
        char buffer[2048];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);

        glColor3f(c.r, c.g, c.b);
        draw_text(p.x, p.y, buffer);
    }

    // extended label method for option lines
    // the purpose is to provide a little more visual context to the various options,
    // and make config-ui interface more human friendly
    void option_label(const int2& p, const color& c, rs::device& dev, rs::option opt, double max_width, bool enabled, double* value = nullptr)
    {
        auto name = find_and_replace(rs_option_to_string((rs_option)opt), "_", " "); // replacing _ with ' ' to reduce visual clutter
        std::string s(name);

        auto size = name.size(); // align the string to max. allowed width
        while (size > 0 && stb_easy_font_width((char*)s.c_str()) > max_width)
        {
            s = name.substr(0, size--) + "...";
        }

        // remove option prefixes converting them to visual hints through color:
        color newC = c;
#define STRING_CASE(S, C) std::string S = #S; if (s.compare(0, S.length(), S) == 0) { newC = C; s = find_and_replace(s, S + " ", ""); }
        color color1 = { 0.6f, 1.0f, 1.0f };
        color color2 = { 1.0f, 0.6f, 1.0f };
        color color3 = { 1.0f, 1.0f, 0.6f };
        color color4 = { 1.0f, 0.6f, 0.6f };
        color color5 = { 0.6f, 0.6f, 1.0f };
        color color6 = { 0.6f, 1.0f, 0.6f };
        STRING_CASE(ZR300, color1)
        STRING_CASE(F200, color2)
        STRING_CASE(SR300, color3)
        STRING_CASE(R200, color4)
        STRING_CASE(FISHEYE, color5)
        STRING_CASE(COLOR, color6)
        if (!enabled) newC = { 0.5f, 0.5f, 0.5f };

        auto w = stb_easy_font_width((char*)s.c_str());
        label(p, newC, s.c_str());
        // if value is required, append it at the end of the string
        if (value)
        {
            std::stringstream sstream;
            sstream << ": " << *value;
            int2 newP{ p.x + w, p.y };
            label(newP, c, sstream.str().c_str());
        }

        rect bbox{ p.x - 15, p.y - 10, p.x + w + 10, p.y + 5 };
        if (bbox.contains(cursor))
        {
            std::string hint = dev.get_option_description(opt);
            auto hint_w = stb_easy_font_width((char*)hint.c_str());
            fill_rect({ cursor.x - hint_w - 7, cursor.y + 5, cursor.x + 7, cursor.y - 17 }, { 1.0f, 1.0f, 1.0f });
            fill_rect({ cursor.x - hint_w - 6, cursor.y + 4, cursor.x + 6, cursor.y - 16 }, { 0.0f, 0.0f, 0.0f });
            label({ cursor.x - hint_w, cursor.y - 2 }, { 1.f, 1.f, 1.f }, hint.c_str());
        }
    }

    void fill_rect(const rect & r, const color & c)
    {
        glBegin(GL_QUADS);
        glColor3f(c.r, c.g, c.b);
        glVertex2i(r.x0, r.y0);
        glVertex2i(r.x0, r.y1);
        glVertex2i(r.x1, r.y1);
        glVertex2i(r.x1, r.y0);
        glEnd();
    }

    void outline_rect(const rect & r, const color & c)
    {
        glPushAttrib(GL_ENABLE_BIT);

        glLineStipple(1, 0xAAAA);
        glEnable(GL_LINE_STIPPLE);

        glBegin(GL_LINE_STRIP);
        glColor3f(c.r, c.g, c.b);
        glVertex2i(r.x0, r.y0);
        glVertex2i(r.x0, r.y1);
        glVertex2i(r.x1, r.y1);
        glVertex2i(r.x1, r.y0);
        glVertex2i(r.x0, r.y0);
        glEnd();

        glPopAttrib();
    }

    bool button(const rect & r, const std::string & label)
    {
        fill_rect(r, { 1, 1, 1 });
        fill_rect(r.shrink(2), r.contains(cursor) ? (mouse_down ? color{ 0.3f, 0.3f, 0.3f } : color{ 0.4f, 0.4f, 0.4f }) : color{ 0.5f, 0.5f, 0.5f });
        glColor3f(1, 1, 1);
        draw_text(r.x0 + 4, r.y1 - 8, label.c_str());
        return click && r.contains(cursor);
    }

    bool checkbox(const rect & r, bool & value)
    {
        bool changed = false;
        if (click && r.contains(cursor))
        {
            value = !value;
            changed = true;
        }
        fill_rect(r, { 1, 1, 1 });
        fill_rect(r.shrink(1), { 0.5, 0.5, 0.5 });
        if (value) fill_rect(r.shrink(3), { 1, 1, 1 });
        return changed;
    }

    bool slider(int id, const rect & r, double min, double max, double step, double & value, bool disable_dragger = false)
    {
        bool changed = false;
        const int w = r.x1 - r.x0, h = r.y1 - r.y0;
        double p = (w - h) * (value - min) / (max - min);
        if (mouse_down && clicked_id == id)
        {
            p = std::max(0.0, std::min<double>(cursor.x - clicked_offset.x - r.x0, w - h));
            double new_value = min + p * (max - min) / (w - h);
            if (step) new_value = std::round((new_value - min) / step) * step + min;
            changed = new_value != value;
            value = new_value;
            p = (w - h) * (value - min) / (max - min);
        }
        const rect dragger = { int(r.x0 + p), int(r.y0), int(r.x0 + p + h), int(r.y1) };
        if (click && dragger.contains(cursor) && !disable_dragger)
        {
            clicked_offset = { cursor.x - dragger.x0, cursor.y - dragger.y0 };
            clicked_id = id;
        }
        fill_rect(r, { 0.5, 0.5, 0.5 });

        if (!disable_dragger)
            fill_rect(dragger, { 1, 1, 1 });

        return changed;
    }

    void indicator(const rect & r, double min, double max, double value)
    {
        value = MAX(min, MIN(max, value));
        const int w = r.x1 - r.x0, h = r.y0 - r.y1;
        double p = (w)* (value - min) / (max - min);
        int Xdelta = 1;
        int Ydelta = -1;
        if (value == max)
        {
            Xdelta = 0;
            Ydelta = -2;
        }
        else if (value == min)
        {
            Xdelta = 2;
            Ydelta = 0;
        }

        const rect dragger = { int(r.x0 + p + Xdelta), int(r.y0), int(r.x0 + p + Ydelta), int(r.y1) };
        fill_rect(r, { 0.5, 0.5, 0.5 });
        fill_rect(dragger, { 1, 1, 1 });
        glColor3f(1, 1, 1);

        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed << min;
        auto delta = (min < 0) ? 40 : 22;
        draw_text(r.x0 - delta, r.y1 + abs(h / 2) + 3, oss.str().c_str());

        oss.str("");
        oss << std::setprecision(2) << std::fixed << max;
        draw_text(r.x1 + 6, r.y1 + abs(h / 2) + 3, oss.str().c_str());

        oss.str("");
        oss << std::setprecision(2) << std::fixed << value;
        draw_text(dragger.x0 - 1, dragger.y0 + 12, oss.str().c_str());
    }

    void vscroll(const rect & r, int client_height, int & offset)
    {
        if (r.contains(cursor)) offset -= scroll_vec.y * 20;
        offset = std::min(offset, client_height - (r.y1 - r.y0));
        offset = std::max(offset, 0);
        if (client_height <= r.y1 - r.y0) return;
        auto bar = r; bar.x0 = bar.x1 - 10;
        auto dragger = bar;
        dragger.y0 = bar.y0 + offset * (r.y1 - r.y0) / client_height;
        dragger.y1 = bar.y0 + (offset + r.y1 - r.y0) * (r.y1 - r.y0) / client_height;
        fill_rect(bar, { 0.5, 0.5, 0.5 });
        fill_rect(dragger, { 1, 1, 1 });
    }
};


std::mutex mm_mutex;
rs::motion_data m_gyro_data;
rs::motion_data m_acc_data;

void on_motion_event(rs::motion_data entry)
{
    std::lock_guard<std::mutex> lock(mm_mutex);
    if (entry.timestamp_data.source_id == RS_EVENT_IMU_ACCEL)
        m_acc_data = entry;
    if (entry.timestamp_data.source_id == RS_EVENT_IMU_GYRO)
        m_gyro_data = entry;
}

void on_timestamp_event(rs::timestamp_data entry)
{
}

struct user_data
{
    GLFWwindow* curr_window = nullptr;
    gui* g = nullptr;
};

void show_message(GLFWwindow* curr_window, const std::string& title, const std::string& message)
{
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    glfwWindowHint(GLFW_FLOATING, GL_TRUE);

    int xpos, ypos;
    glfwGetWindowPos(curr_window, &xpos, &ypos);

    int width, height;
    glfwGetWindowSize(curr_window, &width, &height);

    int close_win_width = 550;
    int close_win_height = 150;
    auto closeWin = glfwCreateWindow(close_win_width, close_win_height, title.c_str(), nullptr, nullptr);
    glfwMakeContextCurrent(closeWin);
    glfwSetWindowPos(closeWin, xpos + width / 2 - close_win_width / 2, ypos + height / 2 - close_win_height / 2);

    gui g;

    user_data data;
    data.curr_window = curr_window;
    data.g = &g;

    glfwSetWindowUserPointer(closeWin, &data);
    glfwSetWindowCloseCallback(closeWin, [](GLFWwindow* w) {
        glfwDestroyWindow(w);
    });

    glfwSetCursorPosCallback(closeWin, [](GLFWwindow * w, double cx, double cy) { reinterpret_cast<user_data *>(glfwGetWindowUserPointer(w))->g->cursor = { (int)cx, (int)cy }; });
    glfwSetScrollCallback(closeWin, [](GLFWwindow * w, double x, double y) { reinterpret_cast<user_data *>(glfwGetWindowUserPointer(w))->g->scroll_vec = { (int)x, (int)y }; });

    glfwSetMouseButtonCallback(closeWin, [](GLFWwindow * w, int button, int action, int mods)
    {
        auto data = reinterpret_cast<user_data *>(glfwGetWindowUserPointer(w));
        if (action == GLFW_PRESS)
        {
            data->g->clicked_id = 0;
            data->g->click = true;
        }
        data->g->mouse_down = action != GLFW_RELEASE;
    });


    while (!glfwWindowShouldClose(closeWin))
    {
        glfwPollEvents();
        int w, h;
        glfwGetFramebufferSize(closeWin, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwGetWindowSize(closeWin, &w, &h);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, +1);


        size_t line_lenght = 80;
        int2 p;
        p.x = 20;
        p.y = 30;
        if (message.size() < line_lenght)
        {
            g.label(p, { 1, 1, 1 }, message.c_str());
        }
        else
        {
            std::vector<std::string> str_vec;
            std::string temp_message = message;
            temp_message = find_and_replace(temp_message, "\n", " ");
            size_t index = 0;
            size_t string_size = temp_message.size();
            while (index < string_size)
            {
                if (index + line_lenght >= string_size)
                {
                    str_vec.push_back(temp_message.substr(index));
                    break;
                }

                auto curr_index = index + temp_message.substr(index, line_lenght).find_last_of(' ');

                str_vec.push_back(temp_message.substr(index, curr_index - index));
                index = curr_index;
            }

            for (auto& elem : str_vec)
            {
                g.label(p, { 1, 1, 1 }, elem.c_str());
                p.y += 15;
            }

            if (p.y > 100)
                glfwSetWindowSize(closeWin, close_win_width, p.y + 50);
        }

        if (g.button({ w / 2 - 40, h - 40, w / 2 + 40, h - 10 }, "      OK"))
        {
            glfwDestroyWindow(closeWin);
        }

        glfwSwapBuffers(closeWin);
        g.click = false;
        if (!g.mouse_down) g.clicked_id = 0;
    }
    glfwMakeContextCurrent(curr_window);
}

struct option { rs::option opt; double min, max, step, value, def; bool supports; };

static std::map<rs::option, std::vector<rs::option>> options_dependencies =
{
  { rs::option::color_exposure, { rs::option::color_enable_auto_exposure } },
  { rs::option::color_white_balance, { rs::option::color_enable_auto_white_balance } },
  { rs::option::r200_lr_gain, { rs::option::r200_lr_auto_exposure_enabled } },
  { rs::option::r200_lr_exposure, { rs::option::r200_lr_auto_exposure_enabled } },
  { rs::option::r200_lr_auto_exposure_enabled, { rs::option::r200_auto_exposure_mean_intensity_set_point,
                                                 rs::option::r200_auto_exposure_bright_ratio_set_point,
                                                 rs::option::r200_auto_exposure_kp_dark_threshold,
                                                 rs::option::r200_auto_exposure_kp_gain,
                                                 rs::option::r200_auto_exposure_kp_exposure,
                                                 rs::option::r200_auto_exposure_bottom_edge,
                                                 rs::option::r200_auto_exposure_top_edge,
                                                 rs::option::r200_auto_exposure_left_edge,
                                                 rs::option::r200_auto_exposure_right_edge,
                                               } },
  { rs::option::r200_auto_exposure_bottom_edge, { rs::option::r200_auto_exposure_top_edge } },
  { rs::option::r200_auto_exposure_top_edge, { rs::option::r200_auto_exposure_bottom_edge } },
  { rs::option::r200_auto_exposure_left_edge, { rs::option::r200_auto_exposure_right_edge } },
  { rs::option::r200_auto_exposure_right_edge, { rs::option::r200_auto_exposure_left_edge } },
};

void update_related_options(rs::device& dev, rs::option opt, std::vector<option>& options)
{
    auto it = options_dependencies.find(opt);
    if (it != options_dependencies.end())
    {
        for (auto& related : it->second)
        {
            auto opt_it = std::find_if(options.begin(), options.end(), [related](option o) { return related == o.opt; });
            if (opt_it != options.end())
            {
                try
                {
                    if (!dev.supports_option(opt_it->opt)) continue;
                    dev.get_option_range(opt_it->opt, opt_it->min, opt_it->max, opt_it->step, opt_it->def);
                    opt_it->value = dev.get_option(opt_it->opt);
                }
                catch (...) {}
            }
        }
    }
}

bool is_any_stream_enable(rs::device* dev)
{
    bool sts = false;
    for (auto i = 0; i < 5; i++)
    {
        if (dev->is_stream_enabled((rs::stream)i))
        {
            sts = true;
            break;
        }
    }

    return sts;
}

bool motion_tracking_enable = true;
void enable_stream(rs::device * dev, int stream, rs::format format, int w, int h, int fps, bool enable, std::stringstream& stream_name)
{
    stream_name.str("");
    if (stream == RS_CAPABILITIES_MOTION_EVENTS)
    {
        stream_name << "MOTION EVENTS";

        if (dev->is_motion_tracking_active())
            return;

        if (enable)
        {
            dev->enable_motion_tracking(on_motion_event, on_timestamp_event);
            motion_tracking_enable = true;
        }
        else
        {
            dev->disable_motion_tracking();
            motion_tracking_enable = false;
        }
    }
    else
    {
        stream_name << rs_stream_to_string((rs_stream)stream);
        if (enable)
        {
            if (!dev->is_stream_enabled((rs::stream)stream))
                dev->enable_stream((rs::stream)stream, w, h, format, fps);
        }
        else
        {
            if (dev->is_stream_enabled((rs::stream)stream))
                dev->disable_stream((rs::stream)stream);
        }
    }
}

void update_mm_data(texture_buffer* buffers, int w, int h, gui& g)
{
    int x = w / 3 + 10;
    int y = 2 * h / 3 + 5;
    auto rect_y0_pos = y + 36;
    auto rect_y1_pos = y + 28;
    auto indicator_width = 42;

    buffers[5].print(x, rect_y0_pos - 10, "Gyro X: ");
    g.indicator({ x + 100, rect_y0_pos, x + 300, rect_y1_pos }, -10, 10, m_gyro_data.axes[0]);
    rect_y0_pos += indicator_width;
    rect_y1_pos += indicator_width;

    buffers[5].print(x, rect_y0_pos - 10, "Gyro Y: ");
    g.indicator({ x + 100, rect_y0_pos, x + 300, rect_y1_pos }, -10, 10, m_gyro_data.axes[1]);
    rect_y0_pos += indicator_width;
    rect_y1_pos += indicator_width;

    buffers[5].print(x, rect_y0_pos - 10, "Gyro Z: ");
    g.indicator({ x + 100, rect_y0_pos, x + 300, rect_y1_pos }, -10, 10, m_gyro_data.axes[2]);
    rect_y0_pos += indicator_width;
    rect_y1_pos += indicator_width;

    buffers[5].print(x, rect_y0_pos - 10, "Acc X: ");
    g.indicator({ x + 100, rect_y0_pos, x + 300, rect_y1_pos }, -10, 10, m_acc_data.axes[0]);
    rect_y0_pos += indicator_width;
    rect_y1_pos += indicator_width;

    buffers[5].print(x, rect_y0_pos - 10, "Acc Y: ");
    g.indicator({ x + 100, rect_y0_pos, x + 300, rect_y1_pos }, -10, 10, m_acc_data.axes[1]);
    rect_y0_pos += indicator_width;
    rect_y1_pos += indicator_width;

    buffers[5].print(x, rect_y0_pos - 10, "Acc Z: ");
    g.indicator({ x + 100, rect_y0_pos, x + 300, rect_y1_pos }, -10, 10, m_acc_data.axes[2]);
}

double find_option_value(const std::vector<option>& options, rs::option opt)
{
    auto it = find_if(options.begin(), options.end(), [opt](option o) { return o.opt == opt; });
    if (it == options.end()) return 0.0;
    return it->value;
}

void draw_autoexposure_roi_boundary(rs::stream s, const std::vector<option>& options, rs::device* dev, gui& g, int x, int y, double w, double h)
{
    if ((s == rs::stream::depth || s == rs::stream::infrared) &&
        find_option_value(options, rs::option::r200_lr_auto_exposure_enabled) > 0)
    {
        auto intrinsics = dev->get_stream_intrinsics(s);
        auto width = intrinsics.width;
        auto height = intrinsics.height;

        auto left =     find_option_value(options, rs::option::r200_auto_exposure_left_edge) / width;
        auto right =    find_option_value(options, rs::option::r200_auto_exposure_right_edge) / width;
        auto top =      find_option_value(options, rs::option::r200_auto_exposure_top_edge) / height;
        auto bottom =   find_option_value(options, rs::option::r200_auto_exposure_bottom_edge) / height;

        left = x + left * w;
        right = x + right * w;
        top = y + top * h;
        bottom = y + bottom * h;

        g.outline_rect({ (int)left + 1, (int)top + 1, (int)right - 1, (int)bottom - 1 }, { 1.0f, 1.0f, 1.0f });
        g.label({ (int)left + 4, (int)bottom - 6 }, { 1.0f, 1.0f, 1.0f }, "AE ROI");
    }
}

int main(int argc, char * argv[])
{
    rs::context ctx;
    GLFWwindow* win = nullptr;
    const auto streams = (unsigned short)rs::stream::fisheye + 1;       // Use camera-supported native streams
    gui g = {};
    rs::device * dev = nullptr;
    std::atomic<bool> running(true);
    bool has_motion_module = false;
    single_consumer_queue<rs::frame> frames_queue[streams];
    std::vector<option> options;
    texture_buffer buffers[streams];
    struct resolution
    {
        int width;
        int height;
        rs::format format;
    };
    std::map<rs::stream, resolution> resolutions;

	std::vector<int> req_fps = { 30, 30, 30, 30, 60, 30 };
    struct w_h { int width, height; };
    std::vector<rs::format> formats = { rs::format::z16,    rs::format::rgb8,   rs::format::y8,         rs::format::y8,        rs::format::raw8,    rs::format::any };
    std::vector<w_h>        wh      = { { 0,0 },            { 640,480 },        { 0,0 },                { 0,0 },               { 640,480 },         {0,0}};

    try {
        rs::log_to_console(rs::log_severity::warn);
        //rs::log_to_file(rs::log_severity::debug, "librealsense.log");

        glfwInit();
        glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
        win = glfwCreateWindow(1550, 960, "CPP Configuration Example", nullptr, nullptr);

        glfwMakeContextCurrent(win);

        glfwSetWindowUserPointer(win, &g);
        glfwSetCursorPosCallback(win, [](GLFWwindow * w, double cx, double cy) { reinterpret_cast<gui *>(glfwGetWindowUserPointer(w))->cursor = { (int)cx, (int)cy }; });
        glfwSetScrollCallback(win, [](GLFWwindow * w, double x, double y) { reinterpret_cast<gui *>(glfwGetWindowUserPointer(w))->scroll_vec = { (int)x, (int)y }; });
        glfwSetMouseButtonCallback(win, [](GLFWwindow * w, int button, int action, int mods)
        {
            auto g = reinterpret_cast<gui *>(glfwGetWindowUserPointer(w));
            if (action == GLFW_PRESS)
            {
                g->clicked_id = 0;
                g->click = true;
            }
            g->mouse_down = action != GLFW_RELEASE;
        });


        if (ctx.get_device_count() < 1) throw std::runtime_error("No device found. Is it plugged in?");

        dev = ctx.get_device(0);

        for (int stream = 0; stream < streams; stream++)
        {
            if (dev->supports((rs::capabilities)stream))
            {
                dev->enable_stream((rs::stream)stream, wh[stream].width, wh[stream].height, formats[stream], req_fps[stream]);
                resolutions[(rs::stream)stream] = { dev->get_stream_width((rs::stream)stream), dev->get_stream_height((rs::stream)stream), formats[stream] };
            }
        }

        has_motion_module = dev->supports(rs::capabilities::fish_eye) || dev->supports(rs::capabilities::motion_events);

        if (has_motion_module)
        {
            if (dev->supports(rs::capabilities::motion_events))
            {
                dev->enable_motion_tracking(on_motion_event, on_timestamp_event);
            }

            glfwSetWindowSize(win, 1100, 960);
        }

        for (int i = 0; i < RS_OPTION_COUNT; ++i)
        {
            option o = { (rs::option)i };
            try {
                o.supports = dev->supports_option(o.opt);
                if (o.supports)
                {
                    dev->get_option_range(o.opt, o.min, o.max, o.step, o.def);
                    o.value = dev->get_option(o.opt);
                }
                options.push_back(o);
            }
            catch (...) {}
        }
    }
    catch (const rs::error & e)
    {
        std::stringstream ss;
        ss << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
        std::cerr << ss.str();
        show_message(win, "Exception", ss.str());
        return 1;
    }
    catch (const std::exception & e)
    {
        std::cerr << e.what() << std::endl;
        show_message(win, "Exception", e.what());
        return 1;
    }
    catch (...)
    {
        std::cerr << "Unresolved error type during camera initialization" << std::endl;
        show_message(win, "Exception", "Unresolved error type during camera initialization");
        return 1;
    }

    rs::format format[streams] = {};
    unsigned long long  frame_number[streams] = {};
    double frame_timestamp[streams] = {};
    int fps[streams] = {};
    double dc_preset = 0, iv_preset = 0;
    int offset = 0, panel_height = 1;
    int gui_click_flag = 0;

    while (true)
    {
        try
        {
            while (!glfwWindowShouldClose(win))
            {
                glfwPollEvents();
                rs::frame frame;

                int w, h;
                glfwGetFramebufferSize(win, &w, &h);
                glViewport(0, 0, w, h);
                glClear(GL_COLOR_BUFFER_BIT);

                glfwGetWindowSize(win, &w, &h);
                glLoadIdentity();
                glOrtho(0, w, h, 0, -1, +1);

                g.vscroll({ w - 270, 0, w, h }, panel_height, offset);
                int y = 10 - offset;

                if (dev->is_streaming() || dev->is_motion_tracking_active())
                {
                    if (g.button({ w - 260, y, w - 20, y + 24 }, "Stop Capture"))
                    {
                        if (is_any_stream_enable(dev))
                        {
                            running = false;
                            for (auto i = 0; i < 5; i++) frames_queue[i].clear();
                            dev->stop();
                        }

                        if (has_motion_module && motion_tracking_enable)
                        {
                            running = false;
                            dev->stop(rs::source::motion_data);
                        }
                    }
                }
                else
                {
                    if (g.button({ w - 260, y, w - 20, y + 24 }, "Start Capture"))
                    {
                        std::vector<rs::stream> supported_streams;
                        for (int i = (int)rs::capabilities::depth; i <= (int)rs::capabilities::fish_eye; i++)
                            if (dev->supports((rs::capabilities)i))
                                supported_streams.push_back((rs::stream)i);
                        for (auto & stream : supported_streams)
                        {
                            if (!dev->is_stream_enabled(stream)) continue;
                            auto intrin = dev->get_stream_intrinsics(stream);
                            std::cout << "Capturing " << stream << " at " << intrin.width << " x " << intrin.height;
                            std::cout << std::setprecision(1) << std::fixed << ", fov = " << intrin.hfov() << " x " << intrin.vfov() << ", distortion = " << intrin.model() << std::endl;
                        }

                        if (has_motion_module && motion_tracking_enable)
                        {
                            running = true;
                            dev->start(rs::source::motion_data);
                        }

                        if (is_any_stream_enable(dev))
                        {
                            running = true;
                            dev->start();
                        }
                    }
                }
                y += 34;
                if (!dev->is_streaming())
                {
                    for (int i = 0; i <= RS_CAPABILITIES_MOTION_EVENTS; ++i)
                    {
                        auto s = (rs::stream)i;
                        auto cap = (rs::capabilities)i;
                        std::stringstream stream_name;

                        if (dev->supports(cap))
                        {
                            static bool is_callback_set = false;
                            bool enable;
                            if (i == RS_CAPABILITIES_MOTION_EVENTS)
                                enable = motion_tracking_enable;
                            else
                                enable = dev->is_stream_enabled(s);

                            enable_stream(dev, i, formats[i], wh[i].width, wh[i].height, req_fps[i], enable, stream_name);

                            if (!is_callback_set || g.checkbox({ w - 260, y, w - 240, y + 20 }, enable))
                            {
                                enable_stream(dev, i, formats[i], wh[i].width, wh[i].height, req_fps[i], enable, stream_name);

                                if (enable)
                                {
                                    static const auto max_queue_size = 2;
                                    for (auto i = 0; i < 5; i++)
                                    {
                                        dev->set_frame_callback((rs::stream)i, [dev, &buffers, &running, &frames_queue, &resolutions, i](rs::frame frame)
                                        {
                                            if (running && frames_queue[i].size() < max_queue_size)
                                            {
                                                frames_queue[i].enqueue(std::move(frame));
                                            }
                                        });
                                    }
                                }
                                is_callback_set = true;
                            }
                            g.label({ w - 234, y + 13 }, { 1, 1, 1 }, "Enable %s", stream_name.str().c_str());
                            y += 30;
                        }
                    }
                }

                if (dev->is_streaming() || dev->is_motion_tracking_active())
                {
                    auto new_w = w + (has_motion_module ? 150 : -280);

                    int scale_factor = (has_motion_module ? 3 : 2);
                    int fWidth = new_w / scale_factor;
                    int fHeight = h / scale_factor;

                    static struct position { int rx, ry, rw, rh; } pos_vec[5];
                    pos_vec[0] = position{ fWidth, 0, fWidth, fHeight };
                    pos_vec[1] = position{ 0, 0, fWidth, fHeight };
                    pos_vec[2] = position{ 0, fHeight, fWidth, fHeight };
                    pos_vec[3] = position{ fWidth, fHeight, fWidth, fHeight };
                    pos_vec[4] = position{ 0, 2 * fHeight, fWidth, fHeight };
                    position center_position = position{ 0, 0, fWidth * 2, fHeight * 2 };
                    position prev_pos;
                    bool g_clicked = g.click;
                    static int frame_clicked[5] = {};

                    for (auto i = 0; i < 5; i++)
                    {
                        if (!dev->is_stream_enabled((rs::stream)i))
                            continue;

                        if (frames_queue[i].try_dequeue(&frame))
                        {
                            buffers[i].upload(frame);
                            format[i] = frame.get_format();
                            frame_number[i] = frame.get_frame_number();
                            frame_timestamp[i] = frame.get_timestamp();
                            fps[i] = frame.get_framerate();
                        }

                        if (g_clicked && gui_click_flag &&
                            g.cursor.x >= center_position.rx && g.cursor.x <= (center_position.rw + center_position.rx) &&
                            g.cursor.y >= center_position.ry && g.cursor.y <= (center_position.rh + center_position.ry))
                        {
                            pos_vec[i] = prev_pos;
                            gui_click_flag = !gui_click_flag;
                            for (int j = 0; j < 5; ++j)
                                frame_clicked[j] = false;

                            g_clicked = false;
                        }
                        else if (g_clicked && !gui_click_flag &&
                            g.cursor.x >= pos_vec[i].rx && g.cursor.x <= (pos_vec[i].rw + pos_vec[i].rx) &&
                            g.cursor.y >= pos_vec[i].ry && g.cursor.y <= (pos_vec[i].rh + pos_vec[i].ry))
                        {
                            gui_click_flag = !gui_click_flag;
                            frame_clicked[i] = gui_click_flag;
                            g_clicked = false;
                        }

                        if (frame_clicked[i])
                        {
                            prev_pos = pos_vec[i];
                            pos_vec[i] = center_position;
                            buffers[i].show((rs::stream)i, format[i], fps[i], frame_number[i], frame_timestamp[i], pos_vec[i].rx, pos_vec[i].ry, pos_vec[i].rw, pos_vec[i].rh, resolutions[(rs::stream)i].width, resolutions[(rs::stream)i].height);
                        }
                        else if (!gui_click_flag)
                            buffers[i].show((rs::stream)i, format[i], fps[i], frame_number[i], frame_timestamp[i], pos_vec[i].rx, pos_vec[i].ry, pos_vec[i].rw, pos_vec[i].rh, resolutions[(rs::stream)i].width, resolutions[(rs::stream)i].height);

                        if (frame_clicked[i])
                            draw_autoexposure_roi_boundary((rs::stream)i, options, dev, g, center_position.rx, center_position.ry, center_position.rw, center_position.rh);
                        else if (!gui_click_flag)
                            draw_autoexposure_roi_boundary((rs::stream)i, options, dev, g, pos_vec[i].rx, pos_vec[i].ry, fWidth, fHeight);
                    }

                    if (has_motion_module && motion_tracking_enable)
                    {
                        std::lock_guard<std::mutex> lock(mm_mutex);
                        update_mm_data(buffers, new_w, h, g);
                    }
                }

                for (auto & o : options)
                {
                    if (!dev->supports_option(o.opt))
                    {
                        o.supports = false;
                        continue;
                    }
                    if (!o.supports)
                    {
                        try {
                            dev->get_option_range(o.opt, o.min, o.max, o.step, o.def);
                            o.value = dev->get_option(o.opt);
                        }
                        catch (...) {}
                        o.supports = true;
                    }

                    auto is_checkbox = (o.min == 0) && (o.max == 1) && (o.step == 1);
                    auto is_checked = o.value > 0;

                    if (is_checkbox ?
                        g.checkbox({ w - 260, y + 10, w - 240, y + 30 }, is_checked) :
                        g.slider((int)o.opt + 1, { w - 260, y + 16, w - 20, y + 36 }, o.min, o.max, o.step, o.value))
                    {
                        if (is_checkbox) dev->set_option(o.opt, is_checked ? 1 : 0);
                        else dev->set_option(o.opt, o.value);

                        update_related_options(*dev, o.opt, options);

                        o.value = dev->get_option(o.opt);
                    }

                    if (is_checkbox) g.option_label({ w - 230, y + 24 }, { 1, 1, 1 }, *dev, o.opt, 210, true);
                    else g.option_label({ w - 260, y + 12 }, { 1, 1, 1 }, *dev, o.opt, 240, true, &o.value);

                    y += 38;
                }

                g.label({ w - 260, y + 12 }, { 1, 1, 1 }, "Depth control parameters preset: %g", dc_preset);
                if (g.slider(100, { w - 260, y + 16, w - 20, y + 36 }, 0, 5, 1, dc_preset)) rs_apply_depth_control_preset((rs_device *)dev, static_cast<int>(dc_preset));
                y += 38;
                g.label({ w - 260, y + 12 }, { 1, 1, 1 }, "IVCAM options preset: %g", iv_preset);
                if (g.slider(101, { w - 260, y + 16, w - 20, y + 36 }, 0, 10, 1, iv_preset)) rs_apply_ivcam_preset((rs_device *)dev, static_cast<rs_ivcam_preset>((int)iv_preset));
                y += 38;

                panel_height = y + 10 + offset;

                glfwSwapBuffers(win);
                g.scroll_vec = { 0, 0 };
                g.click = false;
                if (!g.mouse_down) g.clicked_id = 0;
            }

            running = false;

            for (auto i = 0; i < streams; i++) frames_queue[i].clear();

            if (dev->is_streaming())
                dev->stop();

            for (auto i = 0; i < streams; i++)
            {
                if (dev->is_stream_enabled((rs::stream)i))
                    dev->disable_stream((rs::stream)i);
            }

            glfwTerminate();
            return 0;
        }
        catch (const rs::error & e)
        {
            std::stringstream ss;
            ss << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
            std::cerr << ss.str();
            show_message(win, "Exception", ss.str());
        }
        catch (const std::exception & e)
        {
            std::cerr << e.what() << std::endl;
            show_message(win, "Exception", e.what());
        }
    }
}
