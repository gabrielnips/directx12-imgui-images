#pragma once

#include <d3d12.h> 
#include <vector>  

namespace Dx12Utils
{
    UINT64 GetRequiredIntermediateSize(
        ID3D12Resource* pDestinationResource,
        UINT FirstSubresource,
        UINT NumSubresources);

    UINT64 UpdateSubresources(
        ID3D12GraphicsCommandList* pCmdList,
        ID3D12Resource* pDestinationResource,
        ID3D12Resource* pIntermediate,
        UINT64 IntermediateOffset,
        UINT FirstSubresource,
        UINT NumSubresources,
        D3D12_SUBRESOURCE_DATA* pSrcData);
}