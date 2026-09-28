// ============================================================================
// ファイルの役割: SRV・UAVディスクリプタヒープの確保とGPU/CPUハンドル管理を担当する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once
#include <cstdint>
#include <d3d12.h>
#include <wrl.h>
#include<vector>

class DirectXCommon;

class SrvManager {
public:
    static const uint32_t kMaxSRVCount;

    void Initialize(DirectXCommon* dxCommon);

    uint32_t Allocate();
    bool CheckCanAllocate() const;

    D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t index);
    D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t index);

    void CreateSRVForTexture2D(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT mipLevels);
    void CreateSRVForTextureCube(uint32_t srvIndex, ID3D12Resource* pResource, DXGI_FORMAT format, UINT mipLevels);

    void CreateSRVForStructuredBuffer(
        uint32_t srvIndex,
        ID3D12Resource* pResource,
        UINT numElements,
        UINT structureByteStride
    );

    void CreateUAVForStructuredBuffer(
        uint32_t srvIndex,
        ID3D12Resource* pResource,
        UINT numElements,
        UINT structureByteStride
    );

    void PreDraw();
    void SetGraphicsRootDescriptorTable(UINT rootParameterIndex, uint32_t srvIndex);

    static SrvManager* GetInstance();

    ID3D12DescriptorHeap* GetDescriptorHeap() const {
        return descriptorHeap_.Get();
    }

    void Free(uint32_t index);
    uint32_t GetDescriptorIndex(D3D12_CPU_DESCRIPTOR_HANDLE handle) const;

private:
    DirectXCommon* directXCommon_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap_;
    uint32_t descriptorSize_ = 0;
    uint32_t useIndex_ = 0;

    std::vector<uint32_t> freeIndices_;
};
