// ============================================================================
// ファイルの役割: SRV・UAVディスクリプタヒープの確保とGPU/CPUハンドル管理を担当する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "SrvManager.h"
#include "DirectXCommon.h"
#include <cassert>

// 処理概要: 外部から必要な状態またはリソース参照を取得する。
// 注意事項: 返す参照やポインターの寿命は所有オブジェクトに従う。
SrvManager* SrvManager::GetInstance() {
    static SrvManager instance;
    return &instance;
}

const uint32_t SrvManager::kMaxSRVCount = 512;

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void SrvManager::Initialize(DirectXCommon* dxCommon) {
    assert(dxCommon);
    directXCommon_ = dxCommon;

    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors = kMaxSRVCount;
    desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    HRESULT hr = directXCommon_->GetDevice()->CreateDescriptorHeap(
        &desc,
        IID_PPV_ARGS(&descriptorHeap_)
    );
    assert(SUCCEEDED(hr));

    descriptorSize_ =
        directXCommon_->GetDevice()->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );
}


// 処理概要: SrvManagerが担当する「Allocate」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
uint32_t SrvManager::Allocate() {
    if (!freeIndices_.empty()) {
        uint32_t index = freeIndices_.back();
        freeIndices_.pop_back();
        return index;
    }

    assert(CheckCanAllocate());

    uint32_t index = useIndex_;
    useIndex_++;

    return index;
}


// 処理概要: SrvManagerが担当する「Free」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void SrvManager::Free(uint32_t index) {
    assert(index < useIndex_);
    freeIndices_.push_back(index);
}


// 処理概要: 外部から必要な状態またはリソース参照を取得する。
// 注意事項: 返す参照やポインターの寿命は所有オブジェクトに従う。
uint32_t SrvManager::GetDescriptorIndex(D3D12_CPU_DESCRIPTOR_HANDLE handle) const {
    D3D12_CPU_DESCRIPTOR_HANDLE start =
        descriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    assert(handle.ptr >= start.ptr);
    assert(descriptorSize_ != 0);

    SIZE_T diff = handle.ptr - start.ptr;
    assert(diff % descriptorSize_ == 0);

    uint32_t index = static_cast<uint32_t>(diff / descriptorSize_);
    assert(index < kMaxSRVCount);

    return index;
}


// 処理概要: 処理を続行できる条件やエラー状態を検査する。
// 注意事項: 失敗条件を呼び出し側が判断できる形で返す。
bool SrvManager::CheckCanAllocate() const {
    return useIndex_ < kMaxSRVCount;
}

// 処理概要: 外部から必要な状態またはリソース参照を取得する。
// 注意事項: 返す参照やポインターの寿命は所有オブジェクトに従う。
D3D12_CPU_DESCRIPTOR_HANDLE SrvManager::GetCPUDescriptorHandle(uint32_t index) {
    D3D12_CPU_DESCRIPTOR_HANDLE handle =
        descriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    handle.ptr += descriptorSize_ * index;
    return handle;
}

// 処理概要: 外部から必要な状態またはリソース参照を取得する。
// 注意事項: 返す参照やポインターの寿命は所有オブジェクトに従う。
D3D12_GPU_DESCRIPTOR_HANDLE SrvManager::GetGPUDescriptorHandle(uint32_t index) {
    D3D12_GPU_DESCRIPTOR_HANDLE handle =
        descriptorHeap_->GetGPUDescriptorHandleForHeapStart();

    handle.ptr += descriptorSize_ * index;
    return handle;
}

void SrvManager::CreateSRVForTexture2D(
    uint32_t srvIndex,
    ID3D12Resource* pResource,
    DXGI_FORMAT format,
    UINT mipLevels
) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = mipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    directXCommon_->GetDevice()->CreateShaderResourceView(
        pResource,
        &srvDesc,
        GetCPUDescriptorHandle(srvIndex)
    );
}

void SrvManager::CreateSRVForTextureCube(
    uint32_t srvIndex,
    ID3D12Resource* pResource,
    DXGI_FORMAT format,
    UINT mipLevels
) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = format;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
    srvDesc.TextureCube.MipLevels = mipLevels;
    srvDesc.TextureCube.MostDetailedMip = 0;
    srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;

    directXCommon_->GetDevice()->CreateShaderResourceView(
        pResource,
        &srvDesc,
        GetCPUDescriptorHandle(srvIndex)
    );
}

void SrvManager::CreateSRVForStructuredBuffer(
    uint32_t srvIndex,
    ID3D12Resource* pResource,
    UINT numElements,
    UINT structureByteStride
) {
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
    srvDesc.Buffer.NumElements = numElements;
    srvDesc.Buffer.StructureByteStride = structureByteStride;

    directXCommon_->GetDevice()->CreateShaderResourceView(
        pResource,
        &srvDesc,
        GetCPUDescriptorHandle(srvIndex)
    );
}

void SrvManager::CreateUAVForStructuredBuffer(
    uint32_t srvIndex,
    ID3D12Resource* pResource,
    UINT numElements,
    UINT structureByteStride
) {
    D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = numElements;
    uavDesc.Buffer.StructureByteStride = structureByteStride;
    uavDesc.Buffer.CounterOffsetInBytes = 0;
    uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

    directXCommon_->GetDevice()->CreateUnorderedAccessView(
        pResource,
        nullptr,
        &uavDesc,
        GetCPUDescriptorHandle(srvIndex)
    );
}

// 処理概要: SrvManagerが担当する「PreDraw」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void SrvManager::PreDraw() {
    ID3D12DescriptorHeap* heaps[] = {
        descriptorHeap_.Get()
    };

    directXCommon_->GetCommandList()->SetDescriptorHeaps(1, heaps);
}

// 処理概要: 外部から渡された値を、担当オブジェクトの状態へ反映する。
// 注意事項: 必要に応じて範囲制限や依存データの再計算も行う。
void SrvManager::SetGraphicsRootDescriptorTable(UINT rootParameterIndex, uint32_t srvIndex) {
    directXCommon_->GetCommandList()->SetGraphicsRootDescriptorTable(
        rootParameterIndex,
        GetGPUDescriptorHandle(srvIndex)
    );
}
