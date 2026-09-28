// ============================================================================
// ファイルの役割: 読み込み済み3Dモデルをキャッシュし、重複ロードを防いで共有する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once
#include <string>
#include <unordered_map>
#include <memory>

#include "Model.h"
#include "Object3dCommon.h"

class ModelManager {
public:
    static void Initialize(Object3dCommon* common);
    static void Finalize();

    static Model* Load(const std::string& modelName);
    static Model* Load(const std::string& directoryPath, const std::string& modelName);
    static Model* Find(const std::string& modelName);

private:
    static Object3dCommon* common_;
    static std::unordered_map<std::string, std::unique_ptr<Model>> models_;
};