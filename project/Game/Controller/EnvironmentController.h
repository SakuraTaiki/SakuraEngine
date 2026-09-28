// ============================================================================
// ファイルの役割: 環境マップやライティング係数など、シーン環境設定を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <cstdint>
#include <memory>

#include "Skybox.h"

class DirectXCommon;
class TextureManager;

struct Matrix4x4;

class EnvironmentController {
public:
    void Initialize(
        DirectXCommon* dxCommon,
        TextureManager* textureManager
    );

    void Finalize();

    void Update(
        const Matrix4x4& view,
        const Matrix4x4& projection
    );

    void Draw();
    bool DrawImGui();

    uint32_t GetEnvironmentTextureHandle() const {
        return environmentTextureHandle_;
    }

    float GetEnvironmentCoefficient() const {
        return environmentCoefficient_;
    }

private:
    std::unique_ptr<Skybox> skybox_;

    uint32_t environmentTextureHandle_ = 0;
    float environmentCoefficient_ = 0.05f;

    bool enableSkybox_ = true;
};