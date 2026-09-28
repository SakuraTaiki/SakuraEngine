// ============================================================================
// ファイルの役割: Compute Shaderを利用したGPUパーティクルの生成・更新・描画を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <cstdint>
#include <d3d12.h>
#include <wrl.h>

#include "DirectXCommon.h"
#include "MyMath.h"
#include "SrvManager.h"
#include "TextureManager.h"

class GPUParticleManager
{
public:
    static const uint32_t kMaxParticles = 1024;
    static const uint32_t kThreadCount = 1024;

    struct Settings
    {
        bool enabled = true;

        Vector4 fireMainColor = {
            1.0f,
            0.15f,
            0.01f,
            1.0f
        };

        Vector4 fireSubColor = {
            1.0f,
            0.65f,
            0.05f,
            1.0f
        };

        Vector4 sakuraMainColor = {
            1.0f,
            0.40f,
            0.72f,
            1.0f
        };

        Vector4 sakuraSubColor = {
            1.0f,
            0.82f,
            0.92f,
            1.0f
        };

        int fireCount = 128;
        int sakuraCount = 80;

        float particleScale = 1.0f;
        float spawnRadius = 0.6f;
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
        SrvManager* srvManager,
        TextureManager* textureManager
    );

    void Update(
        const Matrix4x4& viewMatrix,
        const Matrix4x4& projectionMatrix
    );

    void Draw();

    void Emit(
        const Vector3& position,
        uint32_t count,
        float sizeMultiplier = 1.0f
    );

    void EmitSakura(
        const Vector3& position,
        uint32_t count,
        float sizeMultiplier = 1.0f
    );

private:
    struct ParticleData
    {
        Vector3 translate;
        float lifeTime;
        Vector3 scale;
        float maxTime;
        Vector3 startScale;
        float angularVelocity;
        Vector3 velocity;
        float effectType;
        Vector3 acceleration;
        float isAlive;
        Vector4 color;
        float rotateZ;
        float pad[3];
    };

    struct VertexData
    {
        Vector4 position;
        Vector2 texcoord;
        Vector3 normal;
    };

    struct ViewProjectionData
    {
        Matrix4x4 billboard;
        Matrix4x4 viewProjection;
    };

    struct UpdateData
    {
        float deltaTime;
        float totalTime;
        uint32_t particleCount;
        float pad;
    };

    struct EmitterSphere
    {
        Vector3 translate;
        float radius;

        uint32_t count;
        float frequency;
        float frequencyTime;
        uint32_t emit;

        float effectType;
        float sizeMultiplier;
        float pad[2];

        Vector4 mainColor;
        Vector4 subColor;
    };

    struct PerFrame
    {
        float time;
        float deltaTime;
        float pad[2];
    };

    

    void CreateBuffers();
    void CreateDescriptors();
    void CreateGraphicsRootSignature();
    void CreateGraphicsPipelineState();
    void CreateComputeRootSignature();
    void CreateComputePipelineState();
    void CreateMesh();
    void InitializeParticlesOnGPU();
    void TransitionParticleResource(D3D12_RESOURCE_STATES afterState);
    void DispatchEmit();

private:
    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;
    TextureManager* textureManager_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> graphicsRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature> computeRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> computePipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> initializePipelineState_;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> particleBuffer_;
    D3D12_RESOURCE_STATES particleResourceState_ = D3D12_RESOURCE_STATE_COPY_DEST;


    Microsoft::WRL::ComPtr<ID3D12Resource> viewProjectionBuffer_;
    ViewProjectionData* viewProjectionData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> updateBuffer_;
    UpdateData* updateData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> emitPipelineState_;

    Microsoft::WRL::ComPtr<ID3D12Resource> emitterBuffer_;
    EmitterSphere* emitterData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> perFrameBuffer_;
    PerFrame* perFrameData_ = nullptr;

    // ===== 追加：FreeListの末尾位置。-1なら空きParticleなし =====
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListIndexBuffer_;

    // ===== 追加：空いているParticle番号を格納する配列 =====
    Microsoft::WRL::ComPtr<ID3D12Resource> freeListBuffer_;

    uint32_t particleSrvIndex_ = 0;
    uint32_t particleUavIndex_ = 0;
    uint32_t textureHandle_ = 0;

    // ===== 追加：それぞれのUAVディスクリプタ番号 =====
    uint32_t freeListIndexUavIndex_ = 0;
    uint32_t freeListUavIndex_ = 0;

    bool emitRequested_ = false;
    float totalTime_ = 0.0f;

    Settings settings_{};
};
