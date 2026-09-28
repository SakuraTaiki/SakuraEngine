// ============================================================================
// ファイルの役割: エンジン全体の初期化、フレーム更新、終了処理を統括する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "Engine.h"

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void Engine::Initialize() {
    // Window は DirectX / Input の初期化に必要なので最初。
    windowSystem_.Initialize();
    context_.SetWinApp(windowSystem_.GetWinApp());

    // Input は Window に依存する。
    inputSystem_.Initialize(windowSystem_.GetWinApp(), &context_);

    // Graphics も Window に依存する。
    graphicsSystem_.Initialize(windowSystem_.GetWinApp(), &context_);
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void Engine::Finalize() {
    // 初期化と逆順で解放する。
    graphicsSystem_.Finalize();
    inputSystem_.Finalize();
    windowSystem_.Finalize();
}

// 処理概要: Engineが担当する「BeginFrame」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void Engine::BeginFrame() {
    // 入力は Scene 更新前に最新状態へしておく。
    inputSystem_.Update();
}

// 処理概要: 現在の状態が指定された条件を満たすか判定する。
// 注意事項: 状態を変更せず、判定結果だけを返す。
bool Engine::IsRunning() {
    return !windowSystem_.ProcessMessage();
}