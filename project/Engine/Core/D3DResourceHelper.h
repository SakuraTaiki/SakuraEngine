// ============================================================================
// ファイルの役割: DirectX 12リソースの生成補助またはリーク検出を担当する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <cassert>
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

namespace D3DResourceHelper {

    inline D3D12_HEAP_PROPERTIES MakeUploadHeapProperties()
    {
        return {
            D3D12_HEAP_TYPE_UPLOAD,
            D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
            D3D12_MEMORY_POOL_UNKNOWN,
            1,
            1
        };
    }

    inline D3D12_RESOURCE_DESC MakeBufferResourceDesc(uint64_t size)
    {
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = size;
        desc.Height = 1;
        desc.DepthOrArraySize = 1;
        desc.MipLevels = 1;
        desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        desc.SampleDesc.Count = 1;
        return desc;
    }

    inline Microsoft::WRL::ComPtr<ID3D12Resource> CreateUploadBuffer(
        ID3D12Device* device,
        uint64_t size
    ) {
        assert(device);

        D3D12_HEAP_PROPERTIES heapProperties =
            MakeUploadHeapProperties();

        D3D12_RESOURCE_DESC resourceDescription =
            MakeBufferResourceDesc(size);

        Microsoft::WRL::ComPtr<ID3D12Resource> resource;

        HRESULT result =
            device->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &resourceDescription,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&resource)
            );

        assert(SUCCEEDED(result));
        return resource;
    }

    template <class T>
    inline T* Map(ID3D12Resource* resource)
    {
        assert(resource);

        T* mappedData = nullptr;
        resource->Map(
            0,
            nullptr,
            reinterpret_cast<void**>(&mappedData)
        );
        return mappedData;
    }

    inline uint64_t AlignConstantBufferSize(uint64_t size)
    {
        return (size + 0xff) & ~0xff;
    }

    inline void SetAdditiveBlendState(
        D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineDescription
    ) {
        auto& blend =
            pipelineDescription.BlendState.RenderTarget[0];

        blend.BlendEnable = TRUE;
        blend.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        blend.DestBlend = D3D12_BLEND_ONE;
        blend.BlendOp = D3D12_BLEND_OP_ADD;
        blend.SrcBlendAlpha = D3D12_BLEND_ONE;
        blend.DestBlendAlpha = D3D12_BLEND_ZERO;
        blend.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blend.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    inline void SetParticlePipelineDefaults(
        D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineDescription
    ) {
        SetAdditiveBlendState(pipelineDescription);

        pipelineDescription.RasterizerState.CullMode =
            D3D12_CULL_MODE_NONE;

        pipelineDescription.RasterizerState.FillMode =
            D3D12_FILL_MODE_SOLID;

        pipelineDescription.DepthStencilState.DepthEnable =
            TRUE;

        pipelineDescription.DepthStencilState.DepthWriteMask =
            D3D12_DEPTH_WRITE_MASK_ZERO;

        pipelineDescription.DepthStencilState.DepthFunc =
            D3D12_COMPARISON_FUNC_LESS_EQUAL;

        pipelineDescription.NumRenderTargets = 1;
        pipelineDescription.RTVFormats[0] =
            DXGI_FORMAT_R8G8B8A8_UNORM;

        pipelineDescription.DSVFormat =
            DXGI_FORMAT_D24_UNORM_S8_UINT;

        pipelineDescription.SampleMask =
            D3D12_DEFAULT_SAMPLE_MASK;

        pipelineDescription.PrimitiveTopologyType =
            D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        pipelineDescription.SampleDesc.Count = 1;
    }
}
