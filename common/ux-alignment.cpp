#include "ux-alignment.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <vector>
#include <memory>

#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>

#include <Windows.h>
#endif

#define MAGIC 250

bool is_gui_aligned(GLFWwindow *win)
{
#ifdef _WIN32
    try
    {
        auto hwn = glfwGetWin32Window(win);
        if (hwn == nullptr)
            return true;

        auto flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus;

        ImGui::SetNextWindowPos({ 0, 0 });

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("is_gui_aligned", nullptr, flags);

        auto DrawList = ImGui::GetWindowDrawList();
        if (DrawList == nullptr)
            return true;

        DrawList->AddRectFilled({ 0,0 }, { 1,1 }, ImColor(MAGIC / 255.f, 0.f, 0.f, 1.f));

        ImGui::End();
        ImGui::PopStyleVar();

        ImGui::Render();

        glfwSwapBuffers(win);

        ImGui_ImplGlfw_NewFrame(1.f);

        SetFocus(hwn);

        int width = 1;
        int height = 1;

        HDC hdc = GetDC(hwn);
        if (hdc == nullptr)
            return true;

        std::shared_ptr<HDC> shared_hdc(&hdc, [&](HDC* hdc) {ReleaseDC(hwn, *hdc); DeleteDC(*hdc);});

        HDC hCaptureDC = CreateCompatibleDC(hdc);
        if (hCaptureDC == nullptr)
            return true;

        std::shared_ptr<HDC> shared_capture_hdc(&hCaptureDC, [&](HDC* hdc) {ReleaseDC(hwn, *hdc); DeleteDC(*hdc);});

        HBITMAP hCaptureBitmap = CreateCompatibleBitmap(hdc, width, height);
        if (hCaptureBitmap == nullptr)
            return true;

        std::shared_ptr<HBITMAP> shared_bmp(&hCaptureBitmap, [&](HBITMAP* bmp) {DeleteObject(bmp);});

        auto original = SelectObject(hCaptureDC, hCaptureBitmap);
        if (original == nullptr)
            return true;

        if (!BitBlt(hCaptureDC, 0, 0, width, height, hdc, 0, 0, SRCCOPY | CAPTUREBLT))
            return true;

        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
        bmi.bmiHeader.biWidth = width;
        bmi.bmiHeader.biHeight = height;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        std::vector<RGBQUAD> pPixels(width * height);

        auto res = GetDIBits(
            hCaptureDC,
            hCaptureBitmap,
            0,
            height,
            pPixels.data(),
            &bmi,
            DIB_RGB_COLORS
        );

        if (res <= 0 || res == ERROR_INVALID_PARAMETER)
            return true;

        auto ret = pPixels[0].rgbRed == MAGIC;
        return ret;
    }
    catch (...)
    {
        return true;
    }
#else
    return true;
#endif
}