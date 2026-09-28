// ============================================================================
// ファイルの役割: 作品固有の初期シーン設定とゲーム起動構成を定義する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "MyGame.h"

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void MyGame::Initialize() {
    // エンジン基盤を先に初期化する。
    engine_.Initialize();

    // Scene は EngineContext 経由で各システムへアクセスする。
    sceneManager_.Initialize(engine_.GetContext(), &sceneFactory_);
    sceneManager_.ChangeScene("GAME");
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void MyGame::Update() {
    // 入力など、Scene より前に必要な共通処理。
    engine_.BeginFrame();

    sceneManager_.Update();
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void MyGame::Draw() {
    sceneManager_.Draw();
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void MyGame::Finalize() {
    // Scene は Engine の機能を参照しているので先に破棄する。
    sceneManager_.Finalize();

    engine_.Finalize();
}

// 処理概要: 現在の状態が指定された条件を満たすか判定する。
// 注意事項: 状態を変更せず、判定結果だけを返す。
bool MyGame::IsRunning() {
    return engine_.IsRunning();
}
