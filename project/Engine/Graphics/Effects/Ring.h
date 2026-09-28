// ============================================================================
// ファイルの役割: リング形状の頂点生成と描画を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <cstdint>
#include <d3d12.h>
#include <list>
#include <wrl.h>

#include "MyMath.h"
#include "DirectXCommon.h"
#include "TextureManager.h"

class Ring
{
public:
    struct Settings
    {
        Vector4 color = {
            0.35f, 0.70f, 1.00f, 1.00f
        };

        Vector4 endColor = {
            0.10f, 0.25f, 1.00f, 0.0f
        };

        float intensity = 1.0f;

        float startScale = 0.45f;
        float endScale = 1.65f;

        // 外周半径に対する太さ
        float thickness = 0.18f;
        float endThickness = 0.05f;

        float lifeTime = 0.45f;
        float fadeInRatio = 0.1f;

        float easePower = 3.0f;
        float rotationSpeed = 0.0f;

        // 輪郭の柔らかさ
        float edgeSoftness = 0.03f;

        // 外側の発光
        float glowStrength = 0.25f;

        // 波打ち
        float distortionStrength = 0.0f;
        float distortionFrequency = 8.0f;
        float distortionSpeed = 1.0f;
    };

    struct VertexData
    {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    struct InstanceData
    {
        Matrix4x4 WVP;
        Vector4 color;

        // x: thickness
        // y: distortionStrength
        // z: distortionFrequency
        // w: distortionPhase
        Vector4 parameters0;

        // x: edgeSoftness
        // y: glowStrength
        Vector4 parameters1;
    };

    struct RingParticle
    {
        Transform transform;
        Vector4 color;

        float lifeTime = 0.0f;
        float maxTime = 1.0f;

        float currentThickness = 0.1f;
        float distortionPhase = 0.0f;

        // 発生時の設定を保存
        Settings settings{};
    };

    Settings& GetSettings()
    {
        return settings_;
    }

    const Settings& GetSettings() const
    {
        return settings_;
    }

    void SetThickness(float thickness);

    void Initialize(
        DirectXCommon* dxCommon,
        TextureManager* textureManager
    );

    void Update(
        const Matrix4x4& viewMatrix,
        const Matrix4x4& projectionMatrix
    );

    void Draw();

    void Emit(const Vector3& position);

    void SetIsActive(bool isActive)
    {
        isActive_ = isActive;
    }

    bool GetIsActive() const
    {
        return isActive_;
    }

private:
    void CreateRootSignature();
    void CreatePipelineState();
    void CreateMesh();

private:
    static const uint32_t kMaxRings = 64;

    DirectXCommon* dxCommon_ = nullptr;
    TextureManager* textureManager_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature>
        rootSignature_;

    Microsoft::WRL::ComPtr<ID3D12PipelineState>
        pipelineState_;

    Microsoft::WRL::ComPtr<ID3D12Resource>
        vertexBuffer_;

    D3D12_VERTEX_BUFFER_VIEW
        vertexBufferView_{};

    uint32_t vertexCount_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource>
        instancingBuffer_;

    D3D12_VERTEX_BUFFER_VIEW
        instancingBufferView_{};

    InstanceData* instancingDataMapped_ = nullptr;

    uint32_t textureHandle_ = 0;

    bool isActive_ = true;

    std::list<RingParticle> rings_;

    Settings settings_{};
};