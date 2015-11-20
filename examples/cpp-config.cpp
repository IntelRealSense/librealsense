#include <librealsense/rs.hpp>
#include "example.hpp"

#include <cstdarg>
#include <algorithm>
#pragma comment(lib, "opengl32.lib")

struct int2 { int x,y; };
struct rect 
{ 
    int x0,y0,x1,y1;
    bool contains(const int2 & p) const { return x0 <= p.x && y0 <= p.y && p.x < x1 && p.y < y1; } 
    rect shrink(int amt) const { return {x0+amt, y0+amt, x1-amt, y1-amt}; }
};
struct color { float r,g,b; };

void draw_rect(const rect & r, const color & c)
{
    glBegin(GL_QUADS);
    glColor3f(c.r, c.g, c.b);
    glVertex2i(r.x0, r.y0);
    glVertex2i(r.x0, r.y1);
    glVertex2i(r.x1, r.y1);
    glVertex2i(r.x1, r.y0);
    glEnd();
}

struct gui
{
    font fn;
    int2 cursor, clicked_offset;
    bool click, mouse_down;
    int clicked_id;

    void label(const int2 & p, const char * format, ...)
    {
        va_list args;
        va_start(args, format);
        char buffer[1024];
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        ttf_print(&fn, p.x, p.y, buffer);
    }

    bool checkbox(const rect & r, bool & value)
    {
        bool changed = false;
        if(click && r.contains(cursor))
        {
            value = !value;
            changed = true;
        }
        draw_rect(r, {1,1,1});
        draw_rect(r.shrink(1), {0.5,0.5,0.5});
        if(value) draw_rect(r.shrink(3), {1,1,1});
        return changed;
    }

    bool slider(int id, const rect & r, int min, int max, int & value)
    {
        bool changed = false;
        const int w = r.x1 - r.x0, h = r.y1 - r.y0;
        int p = (w - h) * (value - min) / (max - min);
        if(mouse_down && clicked_id == id)
        {
            p = std::max(0, std::min(cursor.x - clicked_offset.x - r.x0, w-h));
            const int new_value = min + p * (max - min) / (w - h);
            changed = new_value != value;
            value = new_value;
        }
        const rect dragger = {r.x0+p, r.y0, r.x0+p+h, r.y1};
        if(click && dragger.contains(cursor))
        {
            clicked_offset = {cursor.x - dragger.x0, cursor.y - dragger.y0};
            clicked_id = id;
        }
        draw_rect(r, {0.5,0.5,0.5});
        draw_rect(dragger, {1,1,1});
        return changed;
    }
};

int main(int argc, char * argv[])
{
    gui g;

    glfwInit();
    auto win = glfwCreateWindow(640, 480, "CPP Configuration Example", nullptr, nullptr);
    glfwSetWindowUserPointer(win, &g);
    glfwSetCursorPosCallback(win, [](GLFWwindow * w, double cx, double cy) { reinterpret_cast<gui *>(glfwGetWindowUserPointer(w))->cursor = {(int)cx, (int)cy}; });
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
    glfwMakeContextCurrent(win);

    if(auto f = find_file("examples/assets/Roboto-Bold.ttf", 3))
    {
        g.fn = ttf_create(f);
        fclose(f);
    }
    else throw std::runtime_error("Unable to open examples/assets/Roboto-Bold.ttf");

    int val = 20;
    bool enable = false;
    while(!glfwWindowShouldClose(win))
    {
        glfwPollEvents();

        int w, h;
        glfwGetFramebufferSize(win, &w, &h);
        glViewport(0, 0, w, h);
        glClear(GL_COLOR_BUFFER_BIT);

        glfwGetWindowSize(win, &w, &h);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, +1);

        glColor3f(1,1,1);
        g.label({10,20}, "Val: %d", val);
        g.slider(1, {10,30,100,50}, 10, 50, val);
        g.checkbox({10,60,30,80}, enable);

        glfwSwapBuffers(win);

        g.click = false;
    }

    glfwTerminate();
    return 0;
}