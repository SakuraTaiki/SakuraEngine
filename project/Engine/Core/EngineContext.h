// ============================================================================
// ファイルの役割: 各サブシステムが共有するエンジン依存オブジェクトへの参照をまとめる。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

// EngineContext は、各エンジン機能への参照を Scene に渡すための窓口。
// 各ポインタは所有しない。実体の所有は Engine / 各 System が担当する。

class WinApp;
class DirectXCommon;
class Input;
class SrvManager;
class TextureManager;
class SpriteCommon;
class Object3dCommon;
class ParticleManager;
class GPUParticleManager;
class ImGuiManager;
class Camera;

class EngineContext {
public:
    void SetWinApp(WinApp* winApp) { winApp_ = winApp; }
    void SetDxCommon(DirectXCommon* dxCommon) { dxCommon_ = dxCommon; }
    void SetInput(Input* input) { input_ = input; }
    void SetSrvManager(SrvManager* srvManager) { srvManager_ = srvManager; }
    void SetTextureManager(TextureManager* textureManager) { textureManager_ = textureManager; }
    void SetSpriteCommon(SpriteCommon* spriteCommon) { spriteCommon_ = spriteCommon; }
    void SetObject3dCommon(Object3dCommon* object3dCommon) { object3dCommon_ = object3dCommon; }
    void SetParticleManager(ParticleManager* particleManager) { particleManager_ = particleManager; }
    void SetGPUParticleManager(GPUParticleManager* gpuParticleManager) { gpuParticleManager_ = gpuParticleManager; }
    void SetImGuiManager(ImGuiManager* imGuiManager) { imGuiManager_ = imGuiManager; }
    void SetCamera(Camera* camera) { camera_ = camera; }

    WinApp* GetWinApp() const { return winApp_; }
    DirectXCommon* GetDxCommon() const { return dxCommon_; }
    Input* GetInput() const { return input_; }
    SrvManager* GetSrvManager() const { return srvManager_; }
    TextureManager* GetTextureManager() const { return textureManager_; }
    SpriteCommon* GetSpriteCommon() const { return spriteCommon_; }
    Object3dCommon* GetObject3dCommon() const { return object3dCommon_; }
    ParticleManager* GetParticleManager() const { return particleManager_; }
    GPUParticleManager* GetGPUParticleManager() const { return gpuParticleManager_; }
    ImGuiManager* GetImGuiManager() const { return imGuiManager_; }
    Camera* GetCamera() const { return camera_; }

private:
    // すべて非所有ポインタ。delete はしない。
    WinApp* winApp_ = nullptr;
    DirectXCommon* dxCommon_ = nullptr;
    Input* input_ = nullptr;
    SrvManager* srvManager_ = nullptr;
    TextureManager* textureManager_ = nullptr;
    SpriteCommon* spriteCommon_ = nullptr;
    Object3dCommon* object3dCommon_ = nullptr;
    ParticleManager* particleManager_ = nullptr;
    GPUParticleManager* gpuParticleManager_ = nullptr;
    ImGuiManager* imGuiManager_ = nullptr;
    Camera* camera_ = nullptr;
};
