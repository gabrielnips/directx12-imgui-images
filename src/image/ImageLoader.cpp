#include "ImageLoader.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_DDS
#define STBI_NO_HDR
#include "vendor/stb/stb_image.h"

#include <d3d12.h>
#include <dxgi1_4.h>

#include "render/Dx12Renderer.h"
#include "render/Dx12Utils.h"

ImGuiDx12Texture::~ImGuiDx12Texture()
{

}

void ImGuiDx12Texture::Release(ExampleDescriptorHeapAllocator* srvAllocator)
{
    if (TextureResource)
    {
        TextureResource.Reset();
    }
    if (SrvCpuDescriptorHandle.ptr != 0 && srvAllocator)
    {
        srvAllocator->Free(SrvCpuDescriptorHandle, SrvGpuDescriptorHandle);
        SrvCpuDescriptorHandle = {};
        SrvGpuDescriptorHandle = {};
    }
    Width = 0;
    Height = 0;
}

ImGuiDx12Texture::ImGuiDx12Texture(ImGuiDx12Texture&& other) noexcept
    : TextureResource(std::move(other.TextureResource)),
    SrvCpuDescriptorHandle(other.SrvCpuDescriptorHandle),
    SrvGpuDescriptorHandle(other.SrvGpuDescriptorHandle),
    Width(other.Width),
    Height(other.Height)
{
    other.SrvCpuDescriptorHandle = {};
    other.SrvGpuDescriptorHandle = {};
    other.Width = 0;
    other.Height = 0;
}

ImGuiDx12Texture& ImGuiDx12Texture::operator=(ImGuiDx12Texture&& other) noexcept
{
    if (this != &other)
    {
        TextureResource = std::move(other.TextureResource);
        SrvCpuDescriptorHandle = other.SrvCpuDescriptorHandle;
        SrvGpuDescriptorHandle = other.SrvGpuDescriptorHandle;
        Width = other.Width;
        Height = other.Height;

        other.SrvCpuDescriptorHandle = {};
        other.SrvGpuDescriptorHandle = {};
        other.Width = 0;
        other.Height = 0;
    }
    return *this;
}


namespace ImageLoader
{
    bool LoadTextureFromFile(
        const std::string& filename,
        ID3D12Device* device,
        ID3D12CommandQueue* commandQueue,
        ExampleDescriptorHeapAllocator* srvAllocator,
        ImGuiDx12Texture& out_texture)
    {

        int image_width = 0;
        int image_height = 0;
        int components = 0; // RGBA
        unsigned char* image_data = stbi_load(filename.c_str(), &image_width, &image_height, &components, STBI_rgb_alpha);
        if (image_data == nullptr)
        {
            std::cerr << "Failed to load image: " << filename << std::endl;
            return false;
        }

        out_texture.Width = image_width;
        out_texture.Height = image_height;

        D3D12_HEAP_PROPERTIES heapProps = {};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resDesc = {};
        resDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        resDesc.Alignment = 0;
        resDesc.Width = image_width;
        resDesc.Height = image_height;
        resDesc.DepthOrArraySize = 1;
        resDesc.MipLevels = 1;
        resDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        resDesc.SampleDesc.Count = 1;
        resDesc.SampleDesc.Quality = 0;
        resDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&out_texture.TextureResource));

        if (FAILED(hr))
        {
            std::cerr << "Failed to create D3D12 texture resource. HRESULT: " << std::hex << hr << std::endl;
            stbi_image_free(image_data);
            return false;
        }

        if (!srvAllocator)
        {
            std::cerr << "Error: SRV descriptor allocator is null." << std::endl;
            stbi_image_free(image_data);
            return false;
        }
        srvAllocator->Alloc(&out_texture.SrvCpuDescriptorHandle, &out_texture.SrvGpuDescriptorHandle);

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = resDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MipLevels = resDesc.MipLevels;
        device->CreateShaderResourceView(out_texture.TextureResource.Get(), &srvDesc, out_texture.SrvCpuDescriptorHandle);

        UINT64 uploadBufferSize = Dx12Utils::GetRequiredIntermediateSize(out_texture.TextureResource.Get(), 0, 1);


        Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
        D3D12_HEAP_PROPERTIES uploadHeapProps = {};
        uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC uploadResDesc = {};
        uploadResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        uploadResDesc.Width = uploadBufferSize;
        uploadResDesc.Height = 1;
        uploadResDesc.DepthOrArraySize = 1;
        uploadResDesc.MipLevels = 1;
        uploadResDesc.Format = DXGI_FORMAT_UNKNOWN;
        uploadResDesc.SampleDesc.Count = 1;
        uploadResDesc.SampleDesc.Quality = 0;
        uploadResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        uploadResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        hr = device->CreateCommittedResource(
            &uploadHeapProps,
            D3D12_HEAP_FLAG_NONE,
            &uploadResDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&uploadBuffer));

        if (FAILED(hr))
        {
            std::cerr << "Failed to create upload buffer. HRESULT: " << std::hex << hr << std::endl;
            stbi_image_free(image_data);
            out_texture.Release(srvAllocator);
            return false;
        }

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = image_data;
        subresourceData.RowPitch = (LONG_PTR)image_width * 4; // 4 bytes por pixel (RGBA)
        subresourceData.SlicePitch = (LONG_PTR)subresourceData.RowPitch * image_height;

        Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
        device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));

        Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
        device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList));

        Dx12Utils::UpdateSubresources(commandList.Get(), out_texture.TextureResource.Get(), uploadBuffer.Get(), 0, 0, 1, &subresourceData);

        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = out_texture.TextureResource.Get();
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
        commandList->ResourceBarrier(1, &barrier);
        commandList->Close();

        ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
        commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        Microsoft::WRL::ComPtr<ID3D12Fence> fence;
        device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence));
        HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        commandQueue->Signal(fence.Get(), 1);
        fence->SetEventOnCompletion(1, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
        CloseHandle(fenceEvent);

        stbi_image_free(image_data);
        return true;
    }
}