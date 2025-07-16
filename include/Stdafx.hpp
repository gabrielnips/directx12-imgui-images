#pragma once

// --- Padrão C++ ---
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <cassert>

// --- Windows e DirectX 12 ---
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl/client.h>

// ImGui
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

// --- Headers ---
#include "render/Dx12Utils.h"
