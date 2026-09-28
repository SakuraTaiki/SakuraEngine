// ============================================================================
// ファイルの役割: デバッグ・演出用プリミティブの生成、更新、描画を管理する。
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

class Primitive
{
public:
    static const uint32_t kMaxParticles = 1024;

    struct Settings
    {
        Vector4 color = {
            1.0f, 0.85f, 0.35f, 1.0f
        };

        Vector4 endColor = {
            1.0f, 0.10f, 0.01f, 0.0f
        };

        int count = 8;
        float intensity = 1.0f;

        float width = 0.05f;
        float widthRandomness = 0.25f;

        float minLength = 0.4f;
        float maxLength = 1.5f;

        float minLifeTime = 0.4f;
        float maxLifeTime = 0.8f;

        float moveSpeed = 0.0f;
        float moveSpeedRandomness = 0.25f;

        float rotationSpeed = 0.0f;
        float rotationSpeedRandomness = 0.0f;

        // ラジアン。0で上方向
        float directionAngle = 0.0f;

        // ラジアン。PIで全方向
        float directionSpread = 3.14159265f;

        float spawnRadius = 0.0f;

        Vector3 acceleration = {
            0.0f,
            0.0f,
            0.0f
        };

        float endWidthScale = 1.0f;
        float endLengthScale = 1.0f;

        float scaleEasePower = 1.0f;

        float fadeInRatio = 0.05f;
        float fadePower = 1.0f;
    };

    struct Particle
    {
        Transform transform;

        Vector3 initialScale;
        Vector3 velocity;

        Vector4 color;

        float angularVelocity = 0.0f;
        float lifeTime = 0.0f;
        float maxTime = 1.0f;

        // 発生時の設定を保存する
        Settings settings{};
    };

    struct InstanceData
    {
        Matrix4x4 WVP;
        Vector4 color;
    };

    struct VertexData
    {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    Settings& GetSettings()
    {
        return settings_;
    }

    const Settings& GetSettings() const
    {
        return settings_;
    }

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

    void Emit(
        const Vector3& position,
        uint32_t count
    );

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

    Microsoft::WRL::ComPtr<ID3D12Resource>
        instancingBuffer_;

    D3D12_VERTEX_BUFFER_VIEW
        instancingBufferView_{};

    InstanceData* instancingDataMapped_ = nullptr;

    uint32_t textureHandle_ = 0;

    std::list<Particle> particles_;

    bool isActive_ = true;

    Settings settings_{};
};