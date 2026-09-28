// ============================================================================
// ファイルの役割: ゲーム固有処理とエンジンの実行順序を接続し、毎フレームの進行を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "GameSystem.h"

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void GameSystem::Initialize() {
    winApp_ = std::make_unique<WinApp>();
    winApp_->Initialize();

    dxCommon_ = std::make_unique<DirectXCommon>();
    dxCommon_->Initialize(winApp_.get());

    input_ = std::make_unique<Input>();
    input_->Initialize(winApp_.get());

    srvManager_ = std::make_unique<SrvManager>();
    srvManager_->Initialize(dxCommon_.get());

    textureManager_ = std::make_unique<TextureManager>();
    textureManager_->Initialize(dxCommon_.get(), srvManager_.get());

    spriteCommon_ = std::make_unique<SpriteCommon>();
    spriteCommon_->SetTextureManager(textureManager_.get());
    spriteCommon_->Initialize(dxCommon_.get());

    object3dCommon_ = std::make_unique<Object3dCommon>();
    object3dCommon_->SetTextureManager(textureManager_.get());
    object3dCommon_->Initialize(dxCommon_.get());

    object3dCommon_->SetSrvManager(srvManager_.get());

    camera_ = std::make_unique<Camera>();

    camera_->Update();

    object3dCommon_->SetDefaultCamera(camera_.get());

    particleManager_ = std::make_unique<ParticleManager>();
    particleManager_->Initialize(dxCommon_.get(), textureManager_.get());

    imGuiManager_ = std::make_unique<ImGuiManager>();
    imGuiManager_->Initialize(dxCommon_.get(), srvManager_.get(), winApp_.get());
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void GameSystem::Finalize() {
    if (imGuiManager_) {
        imGuiManager_->Finalize();
    }

    imGuiManager_.reset();
    particleManager_.reset();
    camera_.reset();
    object3dCommon_.reset();
    spriteCommon_.reset();
    textureManager_.reset();
    srvManager_.reset();
    input_.reset();
    dxCommon_.reset();
    winApp_.reset();
}

// 処理概要: 現在の状態が指定された条件を満たすか判定する。
// 注意事項: 状態を変更せず、判定結果だけを返す。
bool GameSystem::IsRunning() const {
    return !winApp_->ProcessMessage();
}