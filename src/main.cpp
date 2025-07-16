#include "Stdafx.hpp"
#include "render/Dx12Renderer.h"
#include "manager/ImGuiManager.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGuiManager::Instance().HandleMessage(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			ImGuiManager::Instance().GetRenderer()->ResizeBuffers(LOWORD(lParam), HIWORD(lParam));
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU)
			return 0;
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProcW(hWnd, msg, wParam, lParam);
}

int run()
{
	ImGui_ImplWin32_EnableDpiAwareness();
	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));

	WNDCLASSEXW wc = {
		sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
		L"ImGuiImages-DirectX12", nullptr
	};
	RegisterClassExW(&wc);
	HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"ImGui Images DirectX12", WS_OVERLAPPEDWINDOW, 100, 100,
	                            GetSystemMetrics( SM_CXSCREEN ), GetSystemMetrics( SM_CYSCREEN ), nullptr, nullptr,
	                            wc.hInstance, nullptr);

	Dx12Renderer renderer;

	if (!renderer.Initialize(hwnd))
	{
		renderer.Shutdown();
		UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	if (!ImGuiManager::Instance().Initialize(hwnd, &renderer))
	{
		ImGuiManager::Instance().Shutdown();
		renderer.Shutdown();
		UnregisterClassW(wc.lpszClassName, wc.hInstance);
		return 1;
	}

	ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	UpdateWindow(hwnd);

	auto clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	bool done = false;
	while (!done)
	{
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		if (IsIconic(hwnd))
		{
			Sleep(10);
			continue;
		}

		ImGuiManager::Instance().NewFrame();
		ImGuiManager::Instance().Render();

		renderer.Render(ImGui::GetDrawData(), clear_color);
	}

	renderer.WaitForLastSubmittedFrame();
	ImGuiManager::Instance().Shutdown();
	renderer.Shutdown();

	DestroyWindow(hwnd);
	UnregisterClassW(wc.lpszClassName, wc.hInstance);

	return 0;
}

#ifdef NDEBUG
int __stdcall WinMain( HINSTANCE, HINSTANCE, LPSTR, int )
#else
int main(int, char*[])
#endif
{
	return run();
}
