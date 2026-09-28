// ============================================================================
// ファイルの役割: スプライト描画で共有するルートシグネチャとPSOを管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once
#include "DirectXCommon.h"
#include "TextureManager.h" // 追加
#include <wrl.h>
#include <d3d12.h>
#include <memory>

class SpriteCommon {
public:
    void Initialize(DirectXCommon* dxCommon);

    // 共通描画設定（ルートシグネチャ設定など）
    void PreDraw();

    DirectXCommon* GetDxCommon() const { return dxCommon_; }
    TextureManager* GetTextureManager() { return textureManager_; }
    ID3D12RootSignature* GetRootSignature() { return rootSignature_.Get(); }
    ID3D12PipelineState* GetPipelineState() { return pipelineState_.Get(); }

    void SetTextureManager(TextureManager* textureManager) {
        textureManager_ = textureManager;
    }

private:
    void CreateRootSignature();
    void CreateGraphicsPipeline();

private:
    DirectXCommon* dxCommon_ = nullptr;
    TextureManager* textureManager_ = nullptr;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_ = nullptr;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_ = nullptr;
};