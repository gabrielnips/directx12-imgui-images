#pragma once
#include "render/Dx12Renderer.h"


struct ExampleDescriptorHeapAllocator;

struct ImGuiDx12Texture
{
	Microsoft::WRL::ComPtr<ID3D12Resource> TextureResource;
	D3D12_CPU_DESCRIPTOR_HANDLE SrvCpuDescriptorHandle = {};
	D3D12_GPU_DESCRIPTOR_HANDLE SrvGpuDescriptorHandle = {};
	int Width = 0;
	int Height = 0;

	~ImGuiDx12Texture();

	void Release(ExampleDescriptorHeapAllocator* srvAllocator);

	ImGuiDx12Texture(ImGuiDx12Texture&& other) noexcept;
	ImGuiDx12Texture& operator=(ImGuiDx12Texture&& other) noexcept;

	ImGuiDx12Texture() = default;

	ImGuiDx12Texture(const ImGuiDx12Texture&) = delete;
	ImGuiDx12Texture& operator=(const ImGuiDx12Texture&) = delete;
};

namespace ImageLoader
{
	bool LoadTextureFromFile(
		const std::string& filename,
		ID3D12Device* device,
		ID3D12CommandQueue* commandQueue,
		ExampleDescriptorHeapAllocator* srvAllocator,
		ImGuiDx12Texture& out_texture);
}
