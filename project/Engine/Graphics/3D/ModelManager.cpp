// ============================================================================
// ファイルの役割: 読み込み済み3Dモデルをキャッシュし、重複ロードを防いで共有する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "ModelManager.h"
#include <cassert>

Object3dCommon* ModelManager::common_ = nullptr;
std::unordered_map<std::string, std::unique_ptr<Model>> ModelManager::models_;

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void ModelManager::Initialize(Object3dCommon* common) {
    assert(common);
    common_ = common;
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void ModelManager::Finalize() {
    models_.clear();
    common_ = nullptr;
}

// 処理概要: 外部データを読み込み、実行時に扱える形式へ変換する。
// 注意事項: 読込失敗時に既存の有効な状態を不必要に破壊しない。
Model* ModelManager::Load(const std::string& modelName) {
    auto it = models_.find(modelName);
    if (it != models_.end()) {
        return it->second.get();
    }

    std::unique_ptr<Model> model(
        Model::CreateFromOBJ(
            common_->GetDxCommon(),
            "Resources",
            modelName,
            common_->GetTextureManager()
        )
    );

    Model* result = model.get();
    models_[modelName] = std::move(model);

    return result;
}

Model* ModelManager::Load(const std::string& directoryPath, const std::string& modelName)
{
    std::string key = directoryPath + "/" + modelName;

    auto it = models_.find(key);
    if (it != models_.end()) {
        return it->second.get();
    }

    std::unique_ptr<Model> model(
        Model::CreateFromOBJ(
            common_->GetDxCommon(),
            directoryPath,
            modelName,
            common_->GetTextureManager()
        )
    );

    Model* result = model.get();
    models_[key] = std::move(model);

    return result;
}

// 処理概要: 条件に一致するデータまたはリソースを検索する。
// 注意事項: 見つからない場合を正常な結果として扱えるようにする。
Model* ModelManager::Find(const std::string& modelName) {
    auto it = models_.find(modelName);
    if (it == models_.end()) {
        return nullptr;
    }

    return it->second.get();
}