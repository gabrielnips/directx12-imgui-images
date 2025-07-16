#include "Stdafx.hpp"
#include "manager/ImGuiManager.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

Dx12Renderer* ImGuiManager::s_dx12Renderer = nullptr;
std::map<std::string, ImGuiDx12Texture> ImGuiManager::s_loadedTextures;

ImGuiManager::ImGuiManager() : m_renderer(nullptr)
{
}

ImGuiManager::~ImGuiManager()
{
}

ImGuiManager& ImGuiManager::Instance()
{
	static ImGuiManager instance;
	return instance;
}

bool ImGuiManager::Initialize(HWND hWnd, Dx12Renderer* renderer)
{
	m_renderer = renderer;
	s_dx12Renderer = renderer;
	if (!m_renderer)
		return false;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsDark();

	ImGui_ImplWin32_EnableDpiAwareness();
	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));
	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);
	style.FontScaleDpi = main_scale;

	ImGui_ImplWin32_Init(hWnd);

	if (!s_dx12Renderer)
	{
		std::cerr << "Error: Dx12Renderer not defined for ImGuiManager!" << std::endl;
		IM_ASSERT(false && "Dx12Renderer pointer not set or is null in ImGuiManager::Initialize!");
		return false;
	}

	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = s_dx12Renderer->GetDevice();
	init_info.CommandQueue = s_dx12Renderer->GetCommandQueue();
	init_info.NumFramesInFlight = s_dx12Renderer->GetNumFramesInFlight();
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
	init_info.SrvDescriptorHeap = s_dx12Renderer->GetSrvDescriptorHeap();

	if (!init_info.Device || !init_info.CommandQueue || !init_info.SrvDescriptorHeap)
	{
		std::cerr << "Error: One or more DX12 resources are null during ImGui_ImplDX12 initialization!" << std::endl;
		IM_ASSERT(false && "DX12 resources are null during ImGui_ImplDX12_Init!");
		return false;
	}

	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_handle,
	                                    D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_handle)
	{
		if (s_dx12Renderer && s_dx12Renderer->GetSrvDescriptorHeapAllocator())
		{
			s_dx12Renderer->GetSrvDescriptorHeapAllocator()->Alloc(out_cpu_handle, out_gpu_handle);
		}
		else
		{
			std::cerr << "Error: SRV descriptor allocator is null when allocating ImGui texture!" << std::endl;
			IM_ASSERT(false && "SRV Descriptor Allocator is null!");
			*out_cpu_handle = {};
			*out_gpu_handle = {};
		}
	};
	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo*, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle,
	                                   D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle)
	{
		if (s_dx12Renderer && s_dx12Renderer->GetSrvDescriptorHeapAllocator())
		{
			s_dx12Renderer->GetSrvDescriptorHeapAllocator()->Free(cpu_handle, gpu_handle);
		}
		else
		{
			std::cerr << "Error: SRV descriptor allocator is null when freeing ImGui texture!" << std::endl;
			IM_ASSERT(false && "SRV Descriptor Allocator is null!");
		}
	};
	ImGui_ImplDX12_Init(&init_info);

	return true;
}

void ImGuiManager::Shutdown()
{
	if (s_dx12Renderer && s_dx12Renderer->GetSrvDescriptorHeapAllocator())
	{
		for (auto& pair : s_loadedTextures)
		{
			pair.second.Release(s_dx12Renderer->GetSrvDescriptorHeapAllocator());
		}
		s_loadedTextures.clear();
	}

	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}


void ImGuiManager::NewFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGuiIO& io = ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Once);
	ImGui::Begin("##fps", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
	ImGui::End();

	ImGui::Begin("Images");
	float contentWidth = ImGui::GetContentRegionAvail().x;
	float itemSpacing = ImGui::GetStyle().ItemSpacing.x;
	float desiredWidthPerItem = (contentWidth - itemSpacing) / 2.0f;

	ImGui::Text("Image Path:");
	ImGui::SetNextItemWidth(desiredWidthPerItem);
	ImGui::InputText("##image_path", IMAGE_PATH, IM_ARRAYSIZE(IMAGE_PATH));
	ImGui::SameLine();

	if (ImGui::Button("Load Image", ImVec2(-1, 0)))
	{
		std::string path_str(IMAGE_PATH);
		if (!path_str.empty())
		{
			if (!s_loadedTextures.contains(path_str))
			{
				ImGuiDx12Texture newTexture;
				if (s_dx12Renderer && s_dx12Renderer->GetDevice() &&
					s_dx12Renderer->GetCommandQueue() &&
					s_dx12Renderer->GetSrvDescriptorHeapAllocator())
				{
					if (ImageLoader::LoadTextureFromFile(
						path_str,
						s_dx12Renderer->GetDevice(),
						s_dx12Renderer->GetCommandQueue(),
						s_dx12Renderer->GetSrvDescriptorHeapAllocator(),
						newTexture))
					{
						s_loadedTextures[path_str] = std::move(newTexture);
						std::cout << "Image '" << path_str << "' loaded successfully!" << std::endl;
					}
					else
					{
						std::cerr << "Failed to load image: " << path_str << std::endl;
					}
				}
				else
				{
					std::cerr << "DX12 resources not available to load image." << std::endl;
				}
			}
			else
			{
				std::cout << "Image '" << path_str << "' is already loaded." << std::endl;
			}
		}
	}

	ImGui::End();

	constexpr float MAX_IMAGE_SIZE = 400.0f;

	for (const auto& pair : s_loadedTextures)
	{
		const std::string& imagePath = pair.first;
		const ImGuiDx12Texture& texture = pair.second;

		ImGui::PushID(imagePath.c_str());
		if (ImGui::Begin(imagePath.c_str()))
		{
			ImGui::Text("Path: %s", imagePath.c_str());
			ImGui::Text("Original Size: %dx%d", texture.Width, texture.Height);

			if (texture.TextureResource && texture.SrvGpuDescriptorHandle.ptr != 0)
			{
				auto displaySize = ImVec2(static_cast<float>(texture.Width), static_cast<float>(texture.Height));

				if (displaySize.x > MAX_IMAGE_SIZE || displaySize.y > MAX_IMAGE_SIZE)
				{
					if (displaySize.x > displaySize.y)
					{
						displaySize.y = displaySize.y / displaySize.x * MAX_IMAGE_SIZE;
						displaySize.x = MAX_IMAGE_SIZE;
					}
					else
					{
						displaySize.x = displaySize.x / displaySize.y * MAX_IMAGE_SIZE;
						displaySize.y = MAX_IMAGE_SIZE;
					}
				}

				ImGui::Image(texture.SrvGpuDescriptorHandle.ptr, displaySize);
			}
			else
			{
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid or unloaded image resource.");
			}

			ImGui::End();
		}
		ImGui::PopID();
	}
}

void ImGuiManager::Render()
{
	ImGui::Render();
}

LRESULT ImGuiManager::HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam);
}
