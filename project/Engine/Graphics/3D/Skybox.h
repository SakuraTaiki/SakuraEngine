// ============================================================================
// ファイルの役割: キューブマップを使用した背景描画を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include "DirectXCommon.h"
#include "TextureManager.h"
#include "MyMath.h"
#include <wrl.h>
#include <string>
#include <cstdint>

class Skybox
{

public:
    void Initialize(DirectXCommon* dxCommon, TextureManager* textureManager, const std::string& texturePath);
    void Update();
    void Draw();

    void SetCamera(const Matrix4x4& view, const Matrix4x4& projection);
    void SetScale(const Vector3& scale) { transform_.scale = scale; }

private:
    struct VertexData {
        Vector4 position;
    };

    struct TransformationMatrix {
        Matrix4x4 WVP;
        Matrix4x4 World;
    };

    struct Material {
        Vector4 color;
    };

    void CreateRootSignature();
    void CreateGraphicsPipeline();
    void CreateVertexBuffer();
    void CreateConstantBuffers();

private:
    DirectXCommon* dxCommon_ = nullptr;
    TextureManager* textureManager_ = nullptr;

    uint32_t textureHandle_ = 0;

    Transform transform_ = {
        {100.0f, 100.0f, 100.0f},
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 0.0f}
    };

    Matrix4x4 viewMatrix_{};
    Matrix4x4 projectionMatrix_{};

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource_;
    TransformationMatrix* transformationData_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;

};

