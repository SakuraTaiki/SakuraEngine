// ============================================================================
// ファイルの役割: シーンの生成、切り替え、更新、描画、破棄のライフサイクルを管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "SceneManager.h"

#include <utility>

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void SceneManager::Initialize(EngineContext* context, AbstractSceneFactory* sceneFactory) {
    context_ = context;
    sceneFactory_ = sceneFactory;
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void SceneManager::Finalize() {
    if (scene_) {
        scene_->Finalize();
        scene_.reset();
    }

    sceneFactory_ = nullptr;
    context_ = nullptr;
}

// 処理概要: 現在の状態から次の状態へ安全に遷移させる。
// 注意事項: 遷移前後に必要な初期化と終了処理を漏らさない。
bool SceneManager::ChangeScene(const std::string& sceneName) {
    if (!context_ || !sceneFactory_) {
        return false;
    }

    std::unique_ptr<IScene> nextScene = sceneFactory_->CreateScene(sceneName);
    if (!nextScene) {
        return false;
    }

    if (scene_) {
        scene_->Finalize();
    }

    scene_ = std::move(nextScene);
    scene_->Initialize(context_);
    return true;
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void SceneManager::Update() {
    if (scene_) {
        scene_->Update();
    }
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void SceneManager::Draw() {
    if (scene_) {
        scene_->Draw();
    }
}
