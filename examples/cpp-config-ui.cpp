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

template<typename T> inline T MIN(T x, T y) { return x < y? x : y; }
template<typename T> inline T MAX(T x, T y) { return x > y? x : y; }

struct int2 { int x,y; };
struct rect 
{ 
    int x0,y0,x1,y1;
    bool contains(const int2 & p) const { return x0 <= p.x && y0 <= p.y && p.x < x1 && p.y < y1; } 
    rect shrink(int amt) const { return {x0+amt, y0+amt, x1-amt, y1-amt}; }
};
struct color { float r,g,b; };

struct gui
{
    int2 cursor, clicked_offset, scroll_vec;
    bool click, mouse_down;
    int clicked_id;

    gui() : scroll_vec({0,0}), click(), mouse_down(), clicked_id() {}

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
        fill_rect(r, {1,1,1});
        fill_rect(r.shrink(2), r.contains(cursor) ? (mouse_down ? color{0.3f,0.3f,0.3f} : color{0.4f,0.4f,0.4f}) : color{0.5f,0.5f,0.5f});
        glColor3f(1,1,1);
        draw_text(r.x0 + 4, r.y1 - 8, label.c_str());
        return click && r.contains(cursor);
    }

    bool checkbox(const rect & r, bool & value)
    {
        bool changed = false;
        if(click && r.contains(cursor))
        {
            value = !value;
            changed = true;
        }
        fill_rect(r, {1,1,1});
        fill_rect(r.shrink(1), {0.5,0.5,0.5});
        if(value) fill_rect(r.shrink(3), {1,1,1});
        return changed;
    }

    bool slider(int id, const rect & r, double min, double max, double step, double & value, bool disable_dragger = false)
    {
        bool changed = false;
        const int w = r.x1 - r.x0, h = r.y1 - r.y0;
        double p = (w - h) * (value - min) / (max - min);
        if(mouse_down && clicked_id == id)
        {
            p = std::max(0.0, std::min<double>(cursor.x - clicked_offset.x - r.x0, w-h));
            double new_value = min + p * (max - min) / (w - h);
            if(step) new_value = std::round((new_value - min) / step) * step + min;
            changed = new_value != value;
            value = new_value;
            p = (w - h) * (value - min) / (max - min);
        }
        const rect dragger = {int(r.x0+p), int(r.y0), int(r.x0+p+h), int(r.y1)};
        if(click && dragger.contains(cursor))
        {
            clicked_offset = {cursor.x - dragger.x0, cursor.y - dragger.y0};
            clicked_id = id;
        }
        fill_rect(r, {0.5,0.5,0.5});

        if (!disable_dragger)
            fill_rect(dragger, {1,1,1});

        return changed;
    }

    void indicator(const rect & r, double min, double max, double value)
    {
        value = MAX(min, MIN(max, value));
        const int w = r.x1 - r.x0, h = r.y0 - r.y1;
        double p = (w) * (value - min) / (max - min);
        int Xdelta = 1;
        int Ydelta = -1;
        if (value == max)
        {
            Xdelta =  0;
            Ydelta = -2;
        }
        else if (value == min)
        {
            Xdelta = 2;
            Ydelta = 0;
        }

        const rect dragger = {int(r.x0+p+Xdelta), int(r.y0), int(r.x0+p+Ydelta), int(r.y1)};
        fill_rect(r, {0.5,0.5,0.5});
        fill_rect(dragger, {1,1,1});
        glColor3f(1,1,1);

        std::ostringstream oss;
        oss << std::setprecision(2) << std::fixed << min;
        auto delta = (min<0)?40:22;
        draw_text(r.x0 - delta, r.y1 + abs(h/2) + 3, oss.str().c_str());

        oss.str("");
        oss << std::setprecision(2) << std::fixed << max;
        draw_text(r.x1 + 6, r.y1 + abs(h/2) + 3, oss.str().c_str());

        oss.str("");
        oss << std::setprecision(2) << std::fixed << value;
        draw_text(dragger.x0 - 1, dragger.y0 + 12, oss.str().c_str());
    }

    void vscroll(const rect & r, int client_height, int & offset)
    {
        if(r.contains(cursor)) offset -= scroll_vec.y * 20;
        offset = std::min(offset, client_height - (r.y1 - r.y0));
        offset = std::max(offset, 0);
        if(client_height <= r.y1 - r.y0) return;
        auto bar = r; bar.x0 = bar.x1 - 10;
        auto dragger = bar;
        dragger.y0 = bar.y0 + offset * (r.y1 - r.y0) / client_height;
        dragger.y1 = bar.y0 + (offset + r.y1 - r.y0) * (r.y1 - r.y0) / client_height;
        fill_rect(bar, {0.5,0.5,0.5});
        fill_rect(dragger, {1,1,1});
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

int main(int argc, char * argv[]) try
{
    rs::log_to_console(rs::log_severity::warn);
    //rs::log_to_file(rs::log_severity::debug, "librealsense.log");

    glfwInit();
    auto win = glfwCreateWindow(1550, 960, "CPP Configuration Example", nullptr, nullptr);

    glfwMakeContextCurrent(win);
    gui g;
    glfwSetWindowUserPointer(win, &g);
    glfwSetCursorPosCallback(win, [](GLFWwindow * w, double cx, double cy) { reinterpret_cast<gui *>(glfwGetWindowUserPointer(w))->cursor = {(int)cx, (int)cy}; });
    glfwSetScrollCallback(win, [](GLFWwindow * w, double x, double y) { reinterpret_cast<gui *>(glfwGetWindowUserPointer(w))->scroll_vec = {(int)x, (int)y}; });
    glfwSetMouseButtonCallback(win, [](GLFWwindow * w, int button, int action, int mods) 
    { 
        auto g = reinterpret_cast<gui *>(glfwGetWindowUserPointer(w));
        if(action == GLFW_PRESS)
        {
            g->clicked_id = 0;
            g->click = true;
        }
        g->mouse_down = action != GLFW_RELEASE; 
    });

    rs::context ctx;
    if(ctx.get_device_count() < 1) throw std::runtime_error("No device found. Is it plugged in?");

    struct resolution
    {
        int width;
        int height;
        rs::format format;
    };
    std::map<rs::stream, resolution> resolutions;

    rs::device * dev = ctx.get_device(0);
    const auto streams = 6;
    single_consumer_queue<rs::frame> frames_queue[streams];
    texture_buffer buffers[streams];
    std::atomic<bool> running(true);
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
    bool has_motion_module = supports_fish_eye || supports_motion_events;
    if(dev->supports(rs::capabilities::infrared2))
    {
        dev->enable_stream(rs::stream::infrared2, 0, 0, rs::format::y8, 60, rs::output_buffer_format::native);
        resolutions[rs::stream::infrared2] = { dev->get_stream_width(rs::stream::infrared2), dev->get_stream_height(rs::stream::infrared2), rs::format::y8 };
    }

    if(supports_fish_eye)
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
    struct option { rs::option opt; double min, max, step, value, def; };
    std::vector<option> options;
    for(int i=0; i<RS_OPTION_COUNT; ++i)
    {
        option o = {(rs::option)i};
        if(!dev->supports_option(o.opt)) continue;
        dev->get_option_range(o.opt, o.min, o.max, o.step, o.def);
        if(o.min == o.max) continue;
        try { o.value = dev->get_option(o.opt); } catch(...) {}
        options.push_back(o);
    }

    rs::format format[streams] = {};
    int frame_number[streams] = {};
    int fps[streams] = {};
    double dc_preset = 0, iv_preset = 0;
    int offset = 0, panel_height = 1;
    while(!glfwWindowShouldClose(win))
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

        g.vscroll({w-270, 0, w, h}, panel_height, offset);
        int y = 10 - offset;

        if(dev->is_streaming())
        {
            if(g.button({w-260, y, w-20, y+24}, "Stop Capture"))
            {
                running = false;
                for (auto i = 0; i < 5; i++) frames_queue[i].clear();
                dev->stop();

                if (has_motion_module)
                    dev->stop(rs::source::motion_data);
            }
        }
        else
        {
            if(g.button({w-260, y, w-20, y+24}, "Start Capture"))
            {
                running = true;
                dev->start();

                if (has_motion_module)
                    dev->start(rs::source::motion_data);
            }
        }
        y += 34;
        if(!dev->is_streaming())
        {
            for (int i = 0; i <= RS_CAPABILITIES_FISH_EYE ; ++i)
            {
                auto s = (rs::stream)i;
                auto cap = (rs::capabilities)i;

                if (dev->supports(cap))
                {
                    bool enable = dev->is_stream_enabled(s);

                    if(enable) dev->enable_stream(s, rs::preset::best_quality);
                    else dev->disable_stream(s);

                    if(g.checkbox({w-260, y, w-240, y+20}, enable))
                    {
                        if(enable) dev->enable_stream(s, rs::preset::best_quality);
                        else dev->disable_stream(s);
                    }
                    g.label({w-234, y+13}, {1,1,1}, "Enable %s", rs_stream_to_string((rs_stream)i));
                    y += 30;
                }
            }
        }

        bool auto_exposure;
        bool auto_white_balance;
        for(auto & o : options)
        {
            bool disable_dragger = false;
            if (o.opt == rs::option::color_enable_auto_exposure && o.value == 1)
            {
                auto_exposure = true;
            }
            else if (o.opt == rs::option::color_enable_auto_exposure && o.value == 0)
            {
                auto_exposure = false;
            }

            if (o.opt == rs::option::color_enable_auto_white_balance && o.value == 1)
            {
                auto_white_balance = true;
            }
            else if (o.opt == rs::option::color_enable_auto_white_balance && o.value == 0)
            {
                auto_white_balance = false;
            }

            if (o.opt == rs::option::color_exposure && auto_exposure)
            {
                disable_dragger = true;
            }

            if (o.opt == rs::option::color_white_balance && auto_white_balance)
            {
                disable_dragger = true;
            }

            std::ostringstream ss;
            ss << o.opt << ": ";

            if ((auto_exposure && o.opt == rs::option::color_exposure) ||
                    (auto_white_balance && o.opt == rs::option::color_white_balance))
                ss << "Auto";
            else
                ss << o.value;

            g.label({w-260,y+12}, {1,1,1}, ss.str().c_str());

            if(g.slider((int)o.opt + 1, {w-260,y+16,w-20,y+36}, o.min, o.max, o.step, o.value, disable_dragger))
            {
                dev->set_option(o.opt, o.value);
            }
            y += 38;
        }

        g.label({w-260,y+12}, {1,1,1}, "Depth control parameters preset: %g", dc_preset);
        if(g.slider(100, {w-260,y+16,w-20,y+36}, 0, 5, 1, dc_preset)) rs_apply_depth_control_preset((rs_device *)dev, static_cast<int>(dc_preset));
        y += 38;
        g.label({w-260,y+12}, {1,1,1}, "IVCAM options preset: %g", iv_preset);
        if(g.slider(101, {w-260,y+16,w-20,y+36}, 0, 10, 1, iv_preset)) rs_apply_ivcam_preset((rs_device *)dev, static_cast<rs_ivcam_preset>((int)iv_preset));
        y += 38;

        panel_height = y + 10 + offset;

        if(dev->is_streaming())
        {
            w += (has_motion_module ? 150 : -280);

            int scale_factor = (has_motion_module ? 3 : 2);
            int fWidth = w/scale_factor;
            int fHeight = h/scale_factor;

            static struct position{int rx, ry, rw, rh;} pos_vec[5];
            pos_vec[0] = position{fWidth, 0, fWidth, fHeight};
            pos_vec[1] = position{0, 0, fWidth, fHeight};
            pos_vec[2] = position{0, fHeight, fWidth, fHeight};
            pos_vec[3] = position{fWidth, fHeight, fWidth, fHeight};
            pos_vec[4] = position{0, 2*fHeight, fWidth, fHeight};

            for (auto i = 0; i < 5; i++)
            {
                if(!dev->is_stream_enabled((rs::stream)i))
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

            if (has_motion_module)
            {
              std::lock_guard<std::mutex> lock(mm_mutex);

              int x = w/3 + 10;
              int y = 2*h/3 + 5;
              //buffers[5].print(x, y, "MM (200 Hz)");

              auto rect_y0_pos = y+36;
              auto rect_y1_pos = y+28;
              auto indicator_width = 42;

              buffers[5].print(x, rect_y0_pos-10, "Gyro X: ");
              g.indicator({x + 100, rect_y0_pos , x + 300, rect_y1_pos}, -10, 10, m_gyro_data.axes[0]);
              rect_y0_pos+=indicator_width;
              rect_y1_pos+=indicator_width;

              buffers[5].print(x, rect_y0_pos-10, "Gyro Y: ");
              g.indicator({x + 100, rect_y0_pos , x + 300, rect_y1_pos}, -10, 10, m_gyro_data.axes[1]);
              rect_y0_pos+=indicator_width;
              rect_y1_pos+=indicator_width;

              buffers[5].print(x, rect_y0_pos-10, "Gyro Z: ");
              g.indicator({x + 100, rect_y0_pos , x + 300, rect_y1_pos}, -10, 10, m_gyro_data.axes[2]);
              rect_y0_pos+=indicator_width;
              rect_y1_pos+=indicator_width;

              buffers[5].print(x, rect_y0_pos-10, "Acc X: ");
              g.indicator({x + 100, rect_y0_pos , x + 300, rect_y1_pos}, -10, 10, m_acc_data.axes[0]);
              rect_y0_pos+=indicator_width;
              rect_y1_pos+=indicator_width;

              buffers[5].print(x, rect_y0_pos-10, "Acc Y: ");
              g.indicator({x + 100, rect_y0_pos , x + 300, rect_y1_pos}, -10, 10, m_acc_data.axes[1]);
              rect_y0_pos+=indicator_width;
              rect_y1_pos+=indicator_width;

              buffers[5].print(x, rect_y0_pos-10, "Acc Z: ");
              g.indicator({x + 100, rect_y0_pos , x + 300, rect_y1_pos}, -10, 10, m_acc_data.axes[2]);
          }
        }

        glfwSwapBuffers(win);
        g.scroll_vec = {0,0};
        g.click = false;
        if(!g.mouse_down) g.clicked_id = 0;
    }

    running = false;

    for (auto i = 0; i < streams; i++) frames_queue[i].clear();

    if(dev->is_streaming())
        dev->stop();

    for (auto i = 0; i < streams; i++)
    {
        if (dev->is_stream_enabled((rs::stream)i))
            dev->disable_stream((rs::stream)i);
    }

    glfwTerminate();
    return 0;
}
catch(const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
