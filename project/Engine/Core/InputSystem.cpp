// ============================================================================
// ファイルの役割: 入力機器の初期化とフレームごとの入力更新を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "InputSystem.h"

#include "EngineContext.h"
#include "WinApp.h"

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void InputSystem::Initialize(WinApp* winApp, EngineContext* context) {
    input_ = std::make_unique<Input>();
    input_->Initialize(winApp);

    // Scene から入力を取得できるように Context へ登録。
    context->SetInput(input_.get());
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void InputSystem::Finalize() {
    input_.reset();
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void InputSystem::Update() {
    input_->Update();
}