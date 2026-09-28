// ============================================================================
// ファイルの役割: テクスチャの読み込み、GPU転送、SRV割り当て、再利用を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "TextureManager.h"
#include <vector>
#include <cassert>
#include <format>
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"

using namespace Microsoft::WRL;

// 文字列変換用
static std::wstring ConvertString(const std::string& str) {
    if (str.empty()) {
        return std::wstring();
    }
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

// GPUが使用するDEFAULTヒープ上にテクスチャリソースを作成する。
[[nodiscard]]
ComPtr<ID3D12Resource> CreateTextureResource(
    ID3D12Device* device,
    const DirectX::TexMetadata& metadata) {

    D3D12_RESOURCE_DESC textureDesc{};
    textureDesc.Width = UINT(metadata.width);
    textureDesc.Height = UINT(metadata.height);
    textureDesc.MipLevels = UINT16(metadata.mipLevels);
    textureDesc.DepthOrArraySize = UINT16(metadata.arraySize);
    textureDesc.Format = metadata.format;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.Dimension = D3D12_RESOURCE_DIMENSION(metadata.dimension);

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

    ComPtr<ID3D12Resource> resource;
    HRESULT hr = device->CreateCommittedResource(
        &heapProps, D3D12_HEAP_FLAG_NONE, &textureDesc,
        D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&resource));
    assert(SUCCEEDED(hr));

    return resource;
}

// IntermediateResourceを経由して全サブリソースのコピー命令を積む。
// 戻り値はGPUによるコピー完了まで呼び出し側で保持する必要がある。
[[nodiscard]]
ComPtr<ID3D12Resource> UploadTextureData(
    ID3D12Resource* texture,
    const DirectX::ScratchImage& mipImages,
    ID3D12Device* device,
    ID3D12GraphicsCommandList* commandList) {

    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    HRESULT hr = DirectX::PrepareUpload(
        device,
        mipImages.GetImages(),
        mipImages.GetImageCount(),
        mipImages.GetMetadata(),
        subresources
    );
    assert(SUCCEEDED(hr));

    const UINT subresourceCount = static_cast<UINT>(subresources.size());
    const UINT64 intermediateSize = GetRequiredIntermediateSize(
        texture,
        0,
        subresourceCount
    );

    D3D12_HEAP_PROPERTIES uploadHeapProps{};
    uploadHeapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC bufferDesc{};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = intermediateSize;
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

    ComPtr<ID3D12Resource> intermediateResource;
    hr = device->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&intermediateResource)
    );
    assert(SUCCEEDED(hr));

    UpdateSubresources(
        commandList,
        texture,
        intermediateResource.Get(),
        0,
        0,
        subresourceCount,
        subresources.data()
    );

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = texture;
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_GENERIC_READ;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &barrier);

    return intermediateResource;
}

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void TextureManager::Initialize(DirectXCommon* dxCommon,SrvManager*srvManager) {
    dxCommon_ = dxCommon;
    srvManager_ = srvManager;

    assert(dxCommon_);
    assert(srvManager_);
}

// 処理概要: 外部データを読み込み、実行時に扱える形式へ変換する。
// 注意事項: 読込失敗時に既存の有効な状態を不必要に破壊しない。
uint32_t TextureManager::LoadTexture(const std::string& filePath) {
    if (fileMap_.contains(filePath)) {
        return fileMap_[filePath];
    }

    assert(textures_.size() < kMaxTextures);

    DirectX::ScratchImage image;
    DirectX::ScratchImage mipImages;

    std::wstring wFilePath = ConvertString(filePath);
    HRESULT hr;

    if (filePath.ends_with(".dds")) {
        hr = DirectX::LoadFromDDSFile(
            wFilePath.c_str(),
            DirectX::DDS_FLAGS_NONE,
            nullptr,
            image);
    } else {
        hr = DirectX::LoadFromWICFile(
            wFilePath.c_str(),
            DirectX::WIC_FLAGS_FORCE_SRGB,
            nullptr,
            image);
    }

    if (FAILED(hr)) {
        std::string message = "Failed to load texture: " + filePath + "\n";
        OutputDebugStringA(message.c_str());
        assert(false);
        return 0;
    }

    const DirectX::TexMetadata& metadata = image.GetMetadata();

    if (DirectX::IsCompressed(metadata.format)) {
        mipImages = std::move(image);
    } else {
        hr = DirectX::GenerateMipMaps(
            image.GetImages(),
            image.GetImageCount(),
            image.GetMetadata(),
            DirectX::TEX_FILTER_SRGB,
            0,
            mipImages);

        assert(SUCCEEDED(hr));
    }

    const DirectX::TexMetadata& finalMetadata = mipImages.GetMetadata();

    ComPtr<ID3D12Resource> textureResource =
        CreateTextureResource(dxCommon_->GetDevice(), finalMetadata);

    ComPtr<ID3D12Resource> intermediateResource =
        UploadTextureData(
            textureResource.Get(),
            mipImages,
            dxCommon_->GetDevice(),
            dxCommon_->GetCommandList()
        );

    TextureData data{};
    data.resource = textureResource;
    data.intermediateResource = intermediateResource;
    data.resourceDesc = textureResource->GetDesc();

    uint32_t index = static_cast<uint32_t>(textures_.size());

    assert(srvManager_->CheckCanAllocate());

    data.srvIndex = srvManager_->Allocate();
    data.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(data.srvIndex);
    data.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(data.srvIndex);

    if (finalMetadata.IsCubemap()) {
        srvManager_->CreateSRVForTextureCube(
            data.srvIndex,
            data.resource.Get(),
            data.resourceDesc.Format,
            UINT(finalMetadata.mipLevels)
        );
    } else {
        srvManager_->CreateSRVForTexture2D(
            data.srvIndex,
            data.resource.Get(),
            data.resourceDesc.Format,
            UINT(finalMetadata.mipLevels)
        );
    }

    textures_.push_back(data);

    fileMap_[filePath] = index;

    return index;
}
// 処理概要: 外部から必要な状態またはリソース参照を取得する。
// 注意事項: 返す参照やポインターの寿命は所有オブジェクトに従う。
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureHandle) {
    return textures_[textureHandle].srvHandleGPU;
}

// 処理概要: 外部から必要な状態またはリソース参照を取得する。
// 注意事項: 返す参照やポインターの寿命は所有オブジェクトに従う。
const D3D12_RESOURCE_DESC& TextureManager::GetResourceDesc(uint32_t textureHandle) {
    return textures_[textureHandle].resourceDesc;
}

D3D12_CPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleCPU(uint32_t textureHandle)
{
    assert(textureHandle < textures_.size());

    return textures_[textureHandle]
        .srvHandleCPU;
}
