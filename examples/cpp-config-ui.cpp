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

struct gui
{
    int2 cursor, clicked_offset, scroll_vec;
    bool click, mouse_down;
    int clicked_id;

    gui() : scroll_vec({ 0, 0 }), click(), mouse_down(), clicked_id() {}

    void label(const int2 & p, const color & c, const char * format, ...)
    {
        va_list args;
        va_start(args, format);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        glColor3f(c.r, c.g, c.b);
        draw_text(p.x, p.y, buffer);
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
        if (click && dragger.contains(cursor))
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
        auto delta = (min<0) ? 40 : 22;
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

void find_and_replace(std::string& source, std::string const& find, std::string const& replace)
{
    for (std::string::size_type i = 0; (i = source.find(find, i)) != std::string::npos;)
    {
        source.replace(i, find.length(), replace);
        i += replace.length();
    }
}

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
    glfwSetWindowCloseCallback(closeWin, [](GLFWwindow* w){
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


        int2 p;
        p.x = 20;
        p.y = 30;
        if (message.size() < 80)
        {
            g.label(p, { 1, 1, 1 }, message.c_str());
        }
        else
        {
            std::vector<std::string> str_vec;
            std::string temp_message = message;
            find_and_replace(temp_message, "\n", " ");
            size_t index = 0;
            while (index < temp_message.size())
            {
                auto curr_index = index + temp_message.substr(index, 80).find_last_of(' ');
                if (curr_index == 0)
                {
                    str_vec.push_back(temp_message.substr(index));
                    break;
                }

                str_vec.push_back(temp_message.substr(index, index + curr_index));
                index += curr_index;
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

void update_auto_manual_str(std::ostringstream& ss, bool is_auto)
{
    ss << ((is_auto) ? "Auto" : "Manual");
}

struct option { rs::option opt; double min, max, step, value, def; };
bool auto_exposure = false;
bool auto_white_balance = false;
bool lr_auto_exposure = false;
struct auto_options
{
    auto_options(rs::option opt, std::vector<rs::option> options) : auto_option(opt), relate_options(options) {}
    rs::option auto_option;
    std::vector<rs::option> relate_options;
};
auto_options exp_option(rs::option::color_enable_auto_exposure, { rs::option::color_exposure });
auto_options wb_option(rs::option::color_enable_auto_white_balance, { rs::option::color_white_balance });
auto_options lr_option(rs::option::r200_lr_auto_exposure_enabled, { rs::option::r200_lr_gain, rs::option::r200_lr_exposure });
static std::vector<auto_options> auto_options_vec = { exp_option, wb_option, lr_option };
void update_auto_option(rs::option opt, std::vector<option>& options)
{
    auto it = find_if(auto_options_vec.begin(), auto_options_vec.end(),
        [&](const auto_options& element) {
        for (auto& o : element.relate_options)
        {
            if (opt == o)
                return true;
        }
        return false;
    });

    if (it != auto_options_vec.end())
    {
        auto auto_opt = it->auto_option;
        switch (auto_opt)
        {
            case rs::option::color_enable_auto_exposure:
                auto_exposure = false;
            break;

            case rs::option::color_enable_auto_white_balance:
                auto_white_balance = false;
            break;

            case rs::option::r200_lr_auto_exposure_enabled:
                lr_auto_exposure = false;
            break;
        }

        auto it = find_if(options.begin(), options.end(),
            [&](const option& element) {
            return (element.opt == auto_opt);
        });

        if (it != options.end())
        {
            it->value = 0;
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
void enable_stream(rs::device * dev, int stream, bool enable, std::stringstream& stream_name)
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
                dev->enable_stream((rs::stream)stream, rs::preset::best_quality);
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



int main(int argc, char * argv[])
{
    rs::context ctx;
    GLFWwindow* win = nullptr;
    const auto streams = 6;
    gui g;
    rs::device * dev = nullptr;
    std::atomic<bool> running(true);
    bool has_motion_module;
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

    try{
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

        dev->enable_stream(rs::stream::depth, 0, 0, rs::format::z16, 60, rs::output_buffer_format::native);
        dev->enable_stream(rs::stream::color, 640, 480, rs::format::rgb8, 60, rs::output_buffer_format::native);
        dev->enable_stream(rs::stream::infrared, 0, 0, rs::format::y8, 60, rs::output_buffer_format::native);

        resolutions[rs::stream::depth] = { dev->get_stream_width(rs::stream::depth), dev->get_stream_height(rs::stream::depth), rs::format::z16 };
        resolutions[rs::stream::color] = { dev->get_stream_width(rs::stream::color), dev->get_stream_height(rs::stream::color), rs::format::rgb8 };
        resolutions[rs::stream::infrared] = { dev->get_stream_width(rs::stream::infrared), dev->get_stream_height(rs::stream::infrared), rs::format::y8 };

        bool supports_fish_eye = dev->supports(rs::capabilities::fish_eye);
        bool supports_motion_events = dev->supports(rs::capabilities::motion_events);
        has_motion_module = supports_fish_eye || supports_motion_events;
        if (dev->supports(rs::capabilities::infrared2))
        {
            dev->enable_stream(rs::stream::infrared2, 0, 0, rs::format::y8, 60, rs::output_buffer_format::native);
            resolutions[rs::stream::infrared2] = { dev->get_stream_width(rs::stream::infrared2), dev->get_stream_height(rs::stream::infrared2), rs::format::y8 };
        }

        if (supports_fish_eye)
        {
            dev->enable_stream(rs::stream::fisheye, 640, 480, rs::format::raw8, 60, rs::output_buffer_format::native);
            resolutions[rs::stream::fisheye] = { dev->get_stream_width(rs::stream::fisheye), dev->get_stream_height(rs::stream::fisheye), rs::format::raw8 };
        }

        if (has_motion_module)
        {
            if (supports_motion_events)
            {
                dev->enable_motion_tracking(on_motion_event, on_timestamp_event);
            }

            glfwSetWindowSize(win, 1100, 960);
        }

        //std::this_thread::sleep_for(std::chrono::seconds(1));
        for (int i = 0; i < RS_OPTION_COUNT; ++i)
        {
            option o = { (rs::option)i };
            if (!dev->supports_option(o.opt)) continue;
            dev->get_option_range(o.opt, o.min, o.max, o.step, o.def);
            if (o.min == o.max) continue;
            try { o.value = dev->get_option(o.opt); }
            catch (...) {}
            options.push_back(o);
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

        rs::format format[streams] = {};
        int frame_number[streams] = {};
        int fps[streams] = {};
        double dc_preset = 0, iv_preset = 0;
        int offset = 0, panel_height = 1;

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

                            if (has_motion_module)
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
                            if (is_any_stream_enable(dev))
                            {
                                running = true;
                                dev->start();
                            }

                            if (has_motion_module)
                            {
                                running = true;
                                dev->start(rs::source::motion_data);
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
                                bool enable;
                                if (i == RS_CAPABILITIES_MOTION_EVENTS)
                                    enable = motion_tracking_enable;
                                else
                                    enable = dev->is_stream_enabled(s);

                                enable_stream(dev, i, enable, stream_name);

                                if (g.checkbox({ w - 260, y, w - 240, y + 20 }, enable))
                                {
                                    enable_stream(dev, i, enable, stream_name);
                                }
                                g.label({ w - 234, y + 13 }, { 1, 1, 1 }, "Enable %s", stream_name.str().c_str());
                                y += 30;
                            }
                        }
                    }


                    for (auto & o : options)
                    {
                        bool disable_dragger = false;
                        std::ostringstream ss;
                        ss << o.opt << ": ";

                        if ((o.opt == rs::option::color_enable_auto_exposure))
                        {
                            auto_exposure = (o.value == 1);
                            update_auto_manual_str(ss, auto_exposure);
                        }
                        else if ((o.opt == rs::option::color_enable_auto_white_balance))
                        {
                            auto_white_balance = (o.value == 1);
                            update_auto_manual_str(ss, auto_white_balance);
                        }
                        else if ((o.opt == rs::option::r200_lr_auto_exposure_enabled))
                        {
                            lr_auto_exposure = (o.value == 1);
                            update_auto_manual_str(ss, lr_auto_exposure);
                        }
                        else
                            ss << o.value;


                        g.label({ w - 260, y + 12 }, { 1, 1, 1 }, ss.str().c_str());

                        if (g.slider((int)o.opt + 1, { w - 260, y + 16, w - 20, y + 36 }, o.min, o.max, o.step, o.value, disable_dragger))
                        {
                            dev->set_option(o.opt, o.value);
                            update_auto_option(o.opt, options);
                        }
                        y += 38;
                    }

                    g.label({ w - 260, y + 12 }, { 1, 1, 1 }, "Depth control parameters preset: %g", dc_preset);
                    if (g.slider(100, { w - 260, y + 16, w - 20, y + 36 }, 0, 5, 1, dc_preset)) rs_apply_depth_control_preset((rs_device *)dev, static_cast<int>(dc_preset));
                    y += 38;
                    g.label({ w - 260, y + 12 }, { 1, 1, 1 }, "IVCAM options preset: %g", iv_preset);
                    if (g.slider(101, { w - 260, y + 16, w - 20, y + 36 }, 0, 10, 1, iv_preset)) rs_apply_ivcam_preset((rs_device *)dev, static_cast<rs_ivcam_preset>((int)iv_preset));
                    y += 38;

                    panel_height = y + 10 + offset;

                    if (dev->is_streaming() || dev->is_motion_tracking_active())
                    {
                        w += (has_motion_module ? 150 : -280);

                        int scale_factor = (has_motion_module ? 3 : 2);
                        int fWidth = w / scale_factor;
                        int fHeight = h / scale_factor;

                        static struct position{ int rx, ry, rw, rh; } pos_vec[5];
                        pos_vec[0] = position{ fWidth, 0, fWidth, fHeight };
                        pos_vec[1] = position{ 0, 0, fWidth, fHeight };
                        pos_vec[2] = position{ 0, fHeight, fWidth, fHeight };
                        pos_vec[3] = position{ fWidth, fHeight, fWidth, fHeight };
                        pos_vec[4] = position{ 0, 2 * fHeight, fWidth, fHeight };

                        for (auto i = 0; i < 5; i++)
                        {
                            if (!dev->is_stream_enabled((rs::stream)i))
                                continue;

                            if (frames_queue[i].try_dequeue(&frame))
                            {
                                buffers[i].upload(frame);
                                format[i] = frame.get_format();
                                frame_number[i] = frame.get_frame_number();
                                fps[i] = frame.get_framerate();
                            }

                            buffers[i].show((rs::stream)i, format[i], fps[i], frame_number[i], pos_vec[i].rx, pos_vec[i].ry, pos_vec[i].rw, pos_vec[i].rh, resolutions[(rs::stream)i].width, resolutions[(rs::stream)i].height);
                        }

                        if (has_motion_module && motion_tracking_enable)
                        {
                            std::lock_guard<std::mutex> lock(mm_mutex);
                            update_mm_data(buffers, w, h, g);
                        }
                    }

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