// ============================================================================
// ファイルの役割: DirectX 12描画基盤と描画関連マネージャーの初期化・終了を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "GraphicsSystem.h"

#include "EngineContext.h"
#include "WinApp.h"

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void GraphicsSystem::Initialize(WinApp* winApp, EngineContext* context) {
    // DirectX 本体を最初に初期化する。
    dxCommon_ = std::make_unique<DirectXCommon>();
    dxCommon_->Initialize(winApp);
    context->SetDxCommon(dxCommon_.get());

    // SRV は Texture や ImGui などから使われる。
    srvManager_ = std::make_unique<SrvManager>();
    srvManager_->Initialize(dxCommon_.get());
    context->SetSrvManager(srvManager_.get());

    // TextureManager は DirectX と SRV に依存する。
    textureManager_ = std::make_unique<TextureManager>();
    textureManager_->Initialize(dxCommon_.get(), srvManager_.get());
    context->SetTextureManager(textureManager_.get());

    // Dissolveマスクを読み込む
    const uint32_t dissolveMaskHandle =
        textureManager_->LoadTexture(
            "Resources/noise0.png"
        );

    dxCommon_->SetDissolveMaskSrv(
        textureManager_->GetSrvHandleCPU(
            dissolveMaskHandle
        )
    );

    // Sprite 用の共通描画設定。
    spriteCommon_ = std::make_unique<SpriteCommon>();
    spriteCommon_->SetTextureManager(textureManager_.get());
    spriteCommon_->Initialize(dxCommon_.get());
    context->SetSpriteCommon(spriteCommon_.get());

    // 3D Object 用の共通描画設定。
    object3dCommon_ = std::make_unique<Object3dCommon>();
    object3dCommon_->SetTextureManager(textureManager_.get());
    object3dCommon_->Initialize(dxCommon_.get());
    object3dCommon_->SetSrvManager(srvManager_.get());
    context->SetObject3dCommon(object3dCommon_.get());

    // デフォルトカメラを作成し、3D 描画側へ渡す。
    camera_ = std::make_unique<Camera>();

    camera_->SetAspectRatio(
        static_cast<float>(WinApp::kClientWidth) /
        static_cast<float>(WinApp::kClientHeight)
    );

    camera_->Update();

    object3dCommon_->SetDefaultCamera(camera_.get());
    context->SetCamera(camera_.get());

    // パーティクル描画。
    particleManager_ = std::make_unique<ParticleManager>();
    particleManager_->Initialize(dxCommon_.get(), textureManager_.get());
    context->SetParticleManager(particleManager_.get());

    gpuParticleManager_ = std::make_unique<GPUParticleManager>();
    gpuParticleManager_->Initialize(
        dxCommon_.get(),
        srvManager_.get(),
        textureManager_.get()
    );
    context->SetGPUParticleManager(gpuParticleManager_.get());

    // ImGui は DirectX / SRV / Window に依存するため最後の方で初期化する。
    imGuiManager_ = std::make_unique<ImGuiManager>();
    imGuiManager_->Initialize(dxCommon_.get(), srvManager_.get(), winApp);
    context->SetImGuiManager(imGuiManager_.get());
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void GraphicsSystem::Finalize() {
    if (imGuiManager_) {
        imGuiManager_->Finalize();
    }

    // 依存関係があるため、初期化と逆順に解放する。
    imGuiManager_.reset();
    gpuParticleManager_.reset();
    particleManager_.reset();
    camera_.reset();
    object3dCommon_.reset();
    spriteCommon_.reset();
    textureManager_.reset();
    srvManager_.reset();
    dxCommon_.reset();
}
