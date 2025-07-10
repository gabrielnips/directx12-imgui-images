#pragma once
#include "imgui.h"
#include "backends/imgui_impl_win32.h"
#include "backends/imgui_impl_dx12.h"

#include <windows.h>
#include <map>
#include <iostream>

#include "image/ImageLoader.h"
#include "render/Dx12Renderer.h"

class Dx12Renderer;

class ImGuiManager
{
public:
    ImGuiManager();
    ImGuiManager(const ImGuiManager&) = delete;
    ImGuiManager& operator=(const ImGuiManager&) = delete;

    ~ImGuiManager();

    static ImGuiManager& Instance();

    bool Initialize(HWND hWnd, Dx12Renderer* renderer);
    void Shutdown();

    void NewFrame();
    void Render();

    LRESULT HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

    Dx12Renderer* GetRenderer() const { return m_renderer; }

private:
    char IMAGE_PATH[256] = "C:\\blablabla.png";

    Dx12Renderer* m_renderer = nullptr;

    static Dx12Renderer* s_dx12Renderer;
    static std::map<std::string, ImGuiDx12Texture> s_loadedTextures;
};