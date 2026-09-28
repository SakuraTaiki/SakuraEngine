// ============================================================================
// ファイルの役割: CPU/GPUスキニングに必要なInfluence、Palette、SRV/UAVを管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include "Model.h"
#include "Skelton.h"
#include "SrvManager.h"
#include <array>
#include <vector>
#include <wrl.h>
#include <d3d12.h>

const uint32_t kNumMaxInfluence = 4;

struct VertexInfluence {
    std::array<float, kNumMaxInfluence> weights;
    std::array<int32_t, kNumMaxInfluence> jointIndices;
};

struct WellForGPU {
    Matrix4x4 skeletonSpaceMatrix;
    Matrix4x4 skeletonSpaceInverseTransposeMatrix;
};

struct SkinCluster {
    std::vector<Matrix4x4> inverseBindPoseMatrices;

    Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
    D3D12_VERTEX_BUFFER_VIEW influenceBufferView{};
    VertexInfluence* mappedInfluence = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
    WellForGPU* mappedPalette = nullptr;

    uint32_t paletteSrvIndex = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE paletteSrvHandle{};

    uint32_t sourceVertexSrvIndex = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE sourceVertexSrvHandle{};
    uint32_t influenceSrvIndex = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE influenceSrvHandle{};

    Microsoft::WRL::ComPtr<ID3D12Resource> skinnedVertexResource;
    D3D12_VERTEX_BUFFER_VIEW skinnedVertexBufferView{};
    uint32_t skinnedVertexUavIndex = 0;
    D3D12_GPU_DESCRIPTOR_HANDLE skinnedVertexUavHandle{};
    bool skinnedVertexIsReadyForDraw = false;
    bool computeDispatchRequired = true;
    uint32_t vertexCount = 0;
};

bool CreateSkinCluster(
    SkinCluster& skinCluster,
    ID3D12Device* device,
    SrvManager* srvManager,
    const Skeleton& skeleton,
    const Model& model
);

void UpdateSkinCluster(
    SkinCluster& skinCluster,
    const Skeleton& skeleton
);
