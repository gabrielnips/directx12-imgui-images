#include "Stdafx.hpp"

namespace Dx12Utils
{
	UINT64 GetRequiredIntermediateSize(
		ID3D12Resource* pDestinationResource,
		UINT FirstSubresource,
		UINT NumSubresources)
	{
		D3D12_RESOURCE_DESC Desc = pDestinationResource->GetDesc();
		UINT64 RequiredSize = 0;

		Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
		HRESULT hr = pDestinationResource->GetDevice(IID_PPV_ARGS(&pDevice));
		if (FAILED(hr))
		{
			assert(false && "Failed to get D3D12Device from resource.");
			return 0;
		}

		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> Layouts(NumSubresources);
		std::vector<UINT> NumRows(NumSubresources);
		std::vector<UINT64> RowSizesInBytes(NumSubresources);

		pDevice->GetCopyableFootprints(
			&Desc,
			FirstSubresource,
			NumSubresources,
			0,
			Layouts.data(),
			NumRows.data(),
			RowSizesInBytes.data(),
			&RequiredSize);

		// pDevice.Reset(); 

		return RequiredSize;
	}

	UINT64 UpdateSubresources(
		ID3D12GraphicsCommandList* pCmdList,
		ID3D12Resource* pDestinationResource,
		ID3D12Resource* pIntermediate,
		UINT64 IntermediateOffset,
		UINT FirstSubresource,
		UINT NumSubresources,
		D3D12_SUBRESOURCE_DATA* pSrcData)
	{
		UINT64 RequiredSize = 0;
		D3D12_RESOURCE_DESC DestDesc = pDestinationResource->GetDesc();

		Microsoft::WRL::ComPtr<ID3D12Device> pDevice;
		HRESULT hr = pDestinationResource->GetDevice(IID_PPV_ARGS(&pDevice));
		if (FAILED(hr))
		{
			assert(false && "Failed to get D3D12Device from resource in UpdateSubresources.");
			return 0;
		}

		std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> Layouts(NumSubresources);
		std::vector<UINT> NumRows(NumSubresources);
		std::vector<UINT64> RowSizesInBytes(NumSubresources);

		pDevice->GetCopyableFootprints(&DestDesc, FirstSubresource, NumSubresources, IntermediateOffset, Layouts.data(),
		                               NumRows.data(), RowSizesInBytes.data(), &RequiredSize);


		void* pData;
		hr = pIntermediate->Map(0, nullptr, &pData);
		if (FAILED(hr))
		{
			assert(false && "Failed to map intermediate buffer.");
			return 0;
		}

		auto pDest = reinterpret_cast<BYTE*>(pData);

		for (UINT i = 0; i < NumSubresources; ++i)
		{
			const D3D12_SUBRESOURCE_DATA& SrcData = pSrcData[i];
			const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& Layout = Layouts[i];
			const UINT64 RowSizeInBytes = RowSizesInBytes[i];
			const UINT NumRow = NumRows[i];

			BYTE* pDestSubresource = pDest + Layout.Offset;
			auto pSrcSubresource = reinterpret_cast<const BYTE*>(SrcData.pData);

			for (UINT row = 0; row < NumRow; ++row)
			{
				memcpy(pDestSubresource + row * Layout.Footprint.RowPitch,
				       pSrcSubresource + row * SrcData.RowPitch,
				       RowSizeInBytes);
			}
		}
		pIntermediate->Unmap(0, nullptr);

		for (UINT i = 0; i < NumSubresources; ++i)
		{
			D3D12_TEXTURE_COPY_LOCATION Dst = {};
			Dst.pResource = pDestinationResource;
			Dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			Dst.SubresourceIndex = FirstSubresource + i;

			D3D12_TEXTURE_COPY_LOCATION Src = {};
			Src.pResource = pIntermediate;
			Src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			Src.PlacedFootprint = Layouts[i];

			pCmdList->CopyTextureRegion(&Dst, 0, 0, 0, &Src, nullptr);
		}

		return RequiredSize;
	}
}
