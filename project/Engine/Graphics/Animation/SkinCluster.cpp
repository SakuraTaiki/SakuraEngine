// ============================================================================
// ファイルの役割: CPU/GPUスキニングに必要なInfluence、Palette、SRV/UAVを管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "SkinCluster.h"
#include <cassert>
#include <algorithm>
#include <cstring>

namespace {

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(
        ID3D12Device* device,
        size_t sizeInBytes
    ) {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = sizeInBytes;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

        HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&resource)
        );

        assert(SUCCEEDED(hr));
        return resource;
    }

    Microsoft::WRL::ComPtr<ID3D12Resource> CreateUavBufferResource(
        ID3D12Device* device,
        size_t sizeInBytes
    ) {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;

        D3D12_HEAP_PROPERTIES heapProps{};
        heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;

        D3D12_RESOURCE_DESC resourceDesc{};
        resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        resourceDesc.Width = sizeInBytes;
        resourceDesc.Height = 1;
        resourceDesc.DepthOrArraySize = 1;
        resourceDesc.MipLevels = 1;
        resourceDesc.SampleDesc.Count = 1;
        resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        const HRESULT hr = device->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &resourceDesc,
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
            nullptr,
            IID_PPV_ARGS(&resource)
        );
        assert(SUCCEEDED(hr));
        return resource;
    }

}

bool CreateSkinCluster(
    SkinCluster& skinCluster,
    ID3D12Device* device,
    SrvManager* srvManager,
    const Skeleton& skeleton,
    const Model& model
) {
    assert(device);
    assert(srvManager);

    if (skeleton.joints.empty()) {
        return false;
    }

    const auto& skinClusterData = model.GetSkinClusterData();
    // OBJ files and other rigid models still have a root node, but they do
    // not need any skinning resources. Allocating palette/SRV/UAV entries for
    // every rigid object quickly exhausts the shared descriptor heap.
    if (skinClusterData.empty() || model.GetVertexCount() == 0) {
        return false;
    }

    skinCluster.inverseBindPoseMatrices.resize(skeleton.joints.size());
    std::generate(
        skinCluster.inverseBindPoseMatrices.begin(),
        skinCluster.inverseBindPoseMatrices.end(),
        Math::MakeIdentity4x4
    );

    skinCluster.paletteResource = CreateBufferResource(
        device,
        sizeof(WellForGPU) * skeleton.joints.size()
    );

    skinCluster.paletteResource->Map(
        0,
        nullptr,
        reinterpret_cast<void**>(&skinCluster.mappedPalette)
    );

    skinCluster.paletteSrvIndex = srvManager->Allocate();

    srvManager->CreateSRVForStructuredBuffer(
        skinCluster.paletteSrvIndex,
        skinCluster.paletteResource.Get(),
        static_cast<UINT>(skeleton.joints.size()),
        sizeof(WellForGPU)
    );

    skinCluster.paletteSrvHandle =
        srvManager->GetGPUDescriptorHandle(skinCluster.paletteSrvIndex);

    const size_t vertexCount = model.GetVertexCount();
    skinCluster.vertexCount = static_cast<uint32_t>(vertexCount);

    skinCluster.influenceResource = CreateBufferResource(
        device,
        sizeof(VertexInfluence) * vertexCount
    );

    skinCluster.influenceResource->Map(
        0,
        nullptr,
        reinterpret_cast<void**>(&skinCluster.mappedInfluence)
    );

    std::memset(
        skinCluster.mappedInfluence,
        0,
        sizeof(VertexInfluence) * vertexCount
    );

   /* for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
        skinCluster.mappedInfluence[vertexIndex].weights[0] = 1.0f;
        skinCluster.mappedInfluence[vertexIndex].jointIndices[0] = 0;
    }*/

    skinCluster.influenceBufferView.BufferLocation =
        skinCluster.influenceResource->GetGPUVirtualAddress();
    skinCluster.influenceBufferView.SizeInBytes =
        static_cast<UINT>(sizeof(VertexInfluence) * vertexCount);
    skinCluster.influenceBufferView.StrideInBytes =
        sizeof(VertexInfluence);

    skinCluster.sourceVertexSrvIndex = srvManager->Allocate();
    srvManager->CreateSRVForStructuredBuffer(
        skinCluster.sourceVertexSrvIndex,
        model.GetVertexBufferResource(),
        static_cast<UINT>(vertexCount),
        sizeof(ModelVertexData)
    );
    skinCluster.sourceVertexSrvHandle =
        srvManager->GetGPUDescriptorHandle(
            skinCluster.sourceVertexSrvIndex
        );

    skinCluster.influenceSrvIndex = srvManager->Allocate();
    srvManager->CreateSRVForStructuredBuffer(
        skinCluster.influenceSrvIndex,
        skinCluster.influenceResource.Get(),
        static_cast<UINT>(vertexCount),
        sizeof(VertexInfluence)
    );
    skinCluster.influenceSrvHandle =
        srvManager->GetGPUDescriptorHandle(
            skinCluster.influenceSrvIndex
        );

    const size_t skinnedVertexBufferSize =
        sizeof(ModelVertexData) * vertexCount;
    skinCluster.skinnedVertexResource = CreateUavBufferResource(
        device,
        skinnedVertexBufferSize
    );
    skinCluster.skinnedVertexUavIndex = srvManager->Allocate();
    srvManager->CreateUAVForStructuredBuffer(
        skinCluster.skinnedVertexUavIndex,
        skinCluster.skinnedVertexResource.Get(),
        static_cast<UINT>(vertexCount),
        sizeof(ModelVertexData)
    );
    skinCluster.skinnedVertexUavHandle =
        srvManager->GetGPUDescriptorHandle(
            skinCluster.skinnedVertexUavIndex
        );
    skinCluster.skinnedVertexBufferView.BufferLocation =
        skinCluster.skinnedVertexResource->GetGPUVirtualAddress();
    skinCluster.skinnedVertexBufferView.SizeInBytes =
        static_cast<UINT>(skinnedVertexBufferSize);
    skinCluster.skinnedVertexBufferView.StrideInBytes =
        sizeof(ModelVertexData);

    for (const auto& jointWeight : skinClusterData) {
        auto it = skeleton.jointMap.find(jointWeight.first);
        if (it == skeleton.jointMap.end()) {
            continue;
        }

        int32_t jointIndex = it->second;
        skinCluster.inverseBindPoseMatrices[jointIndex] =
            jointWeight.second.inverseBindPoseMatrix;

        for (const VertexWeightData& vertexWeight :
            jointWeight.second.vertexWeights) {

            VertexInfluence& currentInfluence =
                skinCluster.mappedInfluence[vertexWeight.vertexIndex];

            for (uint32_t index = 0; index < kNumMaxInfluence; ++index) {
                if (currentInfluence.weights[index] == 0.0f) {
                    currentInfluence.weights[index] = vertexWeight.weight;
                    currentInfluence.jointIndices[index] = jointIndex;
                    break;
                }
            }
        }
    }

    // ボーンウェイトがない頂点だけ、ルートボーンに割り当てる
    for (size_t vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
        VertexInfluence& influence =
            skinCluster.mappedInfluence[vertexIndex];

        float totalWeight = 0.0f;

        for (uint32_t i = 0; i < kNumMaxInfluence; ++i) {
            totalWeight += influence.weights[i];
        }

        if (totalWeight == 0.0f) {
            influence.weights[0] = 1.0f;
            influence.jointIndices[0] = 0;
            continue;
        }

        // 読み込んだウェイトを合計1.0へ正規化
        for (uint32_t i = 0; i < kNumMaxInfluence; ++i) {
            influence.weights[i] /= totalWeight;
        }
    }

    return true;
}

void UpdateSkinCluster(
    SkinCluster& skinCluster,
    const Skeleton& skeleton
) {
    for (size_t jointIndex = 0;
        jointIndex < skeleton.joints.size();
        ++jointIndex) {

        Matrix4x4 skeletonSpaceMatrix = Math::Multiply(
            skinCluster.inverseBindPoseMatrices[jointIndex],
            skeleton.joints[jointIndex].skeletonSpaceMatrix
        );

        skinCluster.mappedPalette[jointIndex].skeletonSpaceMatrix =
            skeletonSpaceMatrix;

        skinCluster.mappedPalette[jointIndex]
            .skeletonSpaceInverseTransposeMatrix =
            Math::Transpose(Math::Inverse(skeletonSpaceMatrix));
    }

    // The palette changed, so the cached skinned vertex buffer must be
    // refreshed once before the next draw. Additional passes can reuse it.
    skinCluster.computeDispatchRequired = true;
}
