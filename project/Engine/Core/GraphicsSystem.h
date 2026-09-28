// ============================================================================
// ファイルの役割: DirectX 12描画基盤と描画関連マネージャーの初期化・終了を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <memory>

#include "DirectXCommon.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "ParticleManager.h"
#include "GPUParticleManager.h"
#include "ImGuiManager.h"
#include "camera.h"

class WinApp;
class EngineContext;

// 描画に必要な機能をまとめて所有するクラス。
// DirectX、SRV、Texture、2D/3D共通処理、Particle、ImGui、Camera を管理する。
class GraphicsSystem {
public:
    void Initialize(WinApp* winApp, EngineContext* context);
    void Finalize();

    DirectXCommon* GetDxCommon() const { return dxCommon_.get(); }
    SrvManager* GetSrvManager() const { return srvManager_.get(); }
    TextureManager* GetTextureManager() const { return textureManager_.get(); }
    SpriteCommon* GetSpriteCommon() const { return spriteCommon_.get(); }
    Object3dCommon* GetObject3dCommon() const { return object3dCommon_.get(); }
    ParticleManager* GetParticleManager() const { return particleManager_.get(); }
    GPUParticleManager* GetGPUParticleManager() const { return gpuParticleManager_.get(); }
    ImGuiManager* GetImGuiManager() const { return imGuiManager_.get(); }
    Camera* GetCamera() const { return camera_.get(); }

private:
    // 初期化順と解放順が重要。
    // Finalize ではこの逆順で reset する。
    std::unique_ptr<DirectXCommon> dxCommon_;
    std::unique_ptr<SrvManager> srvManager_;
    std::unique_ptr<TextureManager> textureManager_;
    std::unique_ptr<SpriteCommon> spriteCommon_;
    std::unique_ptr<Object3dCommon> object3dCommon_;
    std::unique_ptr<ParticleManager> particleManager_;
    std::unique_ptr<GPUParticleManager> gpuParticleManager_;
    std::unique_ptr<ImGuiManager> imGuiManager_;
    std::unique_ptr<Camera> camera_;
};
