// ============================================================================
// ファイルの役割: 円柱形状または円柱パーティクルの頂点生成と描画を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <d3d12.h>
#include <wrl.h>
#include <cstdint>
#include <list>

#include "MyMath.h"
#include "DirectXCommon.h"
#include "TextureManager.h"
#include "CylinderParticleSystem.h"

class Cylinder
{

public:
    struct VertexData {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    struct InstanceData {
        Matrix4x4 WVP;
        Vector4 color;

        // x: bottomRadiusScale
        // y: topRadiusScale
        // z: twistAmount
        // w: twistPhase
        Vector4 shapeParameters;

        // x: progress
        // y: topFade
        // z: bottomFade
        Vector4 effectParameters;

        // x: noiseStrength
        // y: noiseFrequency
        // z: noisePhase
        Vector4 noiseParameters;
    };


    using Settings =
        CylinderParticleSystem::Settings;

    Settings& GetSettings()
    {
        return particleSystem_.GetSettings();
    }

    const Settings& GetSettings() const
    {
        return particleSystem_.GetSettings();
    }

    void Initialize(DirectXCommon* dxCommon, TextureManager* textureManager);
    void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);
    void Draw();

    void SetIsActive(bool isActive) { isActive_ = isActive; }
    bool GetIsActive() const { return isActive_; }

    void Emit(const Vector3& position);

private:
    void CreateRootSignature();
    void CreatePipelineState();
    void CreateMesh();

private:
    static const uint32_t kMaxCylinders = 64;

    DirectXCommon* dxCommon_ = nullptr;
    TextureManager* textureManager_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};
    uint32_t vertexCount_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> instancingBuffer_;
    D3D12_VERTEX_BUFFER_VIEW instancingBufferView_{};
    InstanceData* instancingDataMapped_ = nullptr;

    uint32_t textureHandle_ = 0;

    bool isActive_ = true;

    // Cylinder エフェクトの粒子管理。
    // Cylinder 本体は描画、ParticleSystem は動きを担当する。
    CylinderParticleSystem particleSystem_;

};

