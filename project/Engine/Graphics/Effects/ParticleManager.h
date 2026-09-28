// ============================================================================
// ファイルの役割: パーティクルの生成、更新、描画、GPUリソースを管理する。
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

struct Particle
{
    Transform transform;

    Vector3 startScale;
    Vector3 velocity;
    Vector3 acceleration;

    Vector4 color;

    float angularVelocity = 0.0f;
    float lifeTime = 0.0f;
    float maxTime = 1.0f;

    // 0: 火花
    // 1: 中心閃光
    // 2: 残光
    // 3: 炎
    // 4: 桜の花弁
    // 5: 桜の中心閃光
    uint32_t effectType = 0;
};

class ParticleManager
{
public:
    static const uint32_t kMaxParticles = 1024;

    struct SakuraSettings
    {
        Vector4 color = {
            1.0f,
            0.35f,
            0.65f,
            1.0f
        };

        Vector4 subColor = {
            1.0f,
            0.75f,
            0.88f,
            1.0f
        };

        int petalCount = 48;

        float minSize = 0.08f;
        float maxSize = 0.22f;

        float spawnRadius = 0.25f;
        float spreadSpeed = 0.065f;
        float upwardSpeed = 0.025f;

        float minLifeTime = 0.55f;
        float maxLifeTime = 1.10f;

        float gravity = 0.0012f;
        float rotationSpeed = 0.12f;

        float flashSize = 1.6f;
        float flashLifeTime = 0.28f;
    };

    struct InstanceData
    {
        Matrix4x4 WVP;
        Vector4 color;

        float effectType;
        float padding[3];
    };

    struct VertexData
    {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    void Initialize(
        DirectXCommon* dxCommon,
        TextureManager* textureManager
    );

    void Update(
        const Matrix4x4& viewMatrix,
        const Matrix4x4& projectionMatrix
    );

    void Draw();

    void Emit(
        const Vector3& position,
        uint32_t count
    );

    void EmitSakura(
        const Vector3& position,
        float sizeMultiplier = 1.0f
    );

    SakuraSettings& GetSakuraSettings()
    {
        return sakuraSettings_;
    }

    const SakuraSettings& GetSakuraSettings() const
    {
        return sakuraSettings_;
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

    SakuraSettings sakuraSettings_{};
};