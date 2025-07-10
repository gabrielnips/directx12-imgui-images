#pragma once

#include <d3d12.h>
#include <dxgi1_4.h>

#include "imgui.h"
#include "backends/imgui_impl_dx12.h"

#include <tchar.h>

#ifdef _DEBUG
#define DX12_ENABLE_DEBUG_LAYER
#endif

static const int APP_NUM_FRAMES_IN_FLIGHT = 2;
static const int APP_NUM_BACK_BUFFERS     = 2;
static const int APP_SRV_HEAP_SIZE        = 64;

struct FrameContext
{
    ID3D12CommandAllocator* CommandAllocator;
    UINT64                  FenceValue;
};

struct ExampleDescriptorHeapAllocator
{
    ID3D12DescriptorHeap*       Heap     = nullptr;
    D3D12_DESCRIPTOR_HEAP_TYPE  HeapType = D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES;
    D3D12_CPU_DESCRIPTOR_HANDLE HeapStartCpu;
    D3D12_GPU_DESCRIPTOR_HANDLE HeapStartGpu;
    UINT                        HeapHandleIncrement;
    ImVector<int>               FreeIndices;

    void Create(ID3D12Device* device, ID3D12DescriptorHeap* heap);
    void Destroy();
    void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE* out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE* out_gpu_desc_handle);
    void Free(D3D12_CPU_DESCRIPTOR_HANDLE out_cpu_desc_handle, D3D12_GPU_DESCRIPTOR_HANDLE out_gpu_desc_handle);
};

class Dx12Renderer
{
public:
    Dx12Renderer();
    ~Dx12Renderer();

    bool Initialize(HWND hWnd);

    void Shutdown();
    void NewFrame();
    void Render(ImDrawData* draw_data, ImVec4 clear_color);
    void WaitForLastSubmittedFrame();
    void ResizeBuffers(int width, int height);

    int GetNumFramesInFlight() const { return APP_NUM_FRAMES_IN_FLIGHT; } 

    ID3D12Device*                   GetDevice() const { return g_pd3dDevice; }
    ID3D12CommandQueue*             GetCommandQueue() const { return g_pd3dCommandQueue; }
    ID3D12DescriptorHeap*           GetSrvDescriptorHeap() const { return g_pd3dSrvDescHeap; }
    ExampleDescriptorHeapAllocator* GetSrvDescriptorHeapAllocator() { return &g_pd3dSrvDescHeapAlloc; }

private:
    FrameContext                   g_frameContext[APP_NUM_FRAMES_IN_FLIGHT] = {};
    UINT                           g_frameIndex               = 0;
    ID3D12Device*                  g_pd3dDevice               = nullptr;
    ID3D12DescriptorHeap*          g_pd3dRtvDescHeap          = nullptr;
    ID3D12DescriptorHeap*          g_pd3dSrvDescHeap          = nullptr;
    ExampleDescriptorHeapAllocator g_pd3dSrvDescHeapAlloc;
    ID3D12CommandQueue*            g_pd3dCommandQueue         = nullptr;
    ID3D12GraphicsCommandList*     g_pd3dCommandList          = nullptr;
    ID3D12Fence*                   g_fence                    = nullptr;
    HANDLE                         g_fenceEvent               = nullptr;
    UINT64                         g_fenceLastSignaledValue   = 0;
    IDXGISwapChain3*               g_pSwapChain               = nullptr;
    bool                           g_SwapChainOccluded        = false;
    HANDLE                         g_hSwapChainWaitableObject = nullptr;
    ID3D12Resource*                g_mainRenderTargetResource[APP_NUM_BACK_BUFFERS] = {};
    D3D12_CPU_DESCRIPTOR_HANDLE    g_mainRenderTargetDescriptor[APP_NUM_BACK_BUFFERS] = {};

    bool CreateDeviceD3D(HWND hWnd);
    void CleanupDeviceD3D();
    void CreateRenderTarget();
    void CleanupRenderTarget();
    FrameContext* WaitForNextFrameResources();
};