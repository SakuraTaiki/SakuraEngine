// ============================================================================
// ファイルの役割: シーンの生成、切り替え、更新、描画、破棄のライフサイクルを管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <memory>
#include <string>

#include "AbstractSceneFactory.h"

class EngineContext;

// Scene の所有と切り替えを担当する。
// 具体的な Scene の生成は AbstractSceneFactory に委譲する。
class SceneManager {
public:
    void Initialize(EngineContext* context, AbstractSceneFactory* sceneFactory);
    void Finalize();

    void Update();
    void Draw();

    // 指定名の Scene をファクトリーで生成し、現在の Scene と切り替える。
    // 未登録名や未初期化の場合は、現在の Scene を維持して false を返す。
    bool ChangeScene(const std::string& sceneName);

private:
    EngineContext* context_ = nullptr;

    // 所有権は呼び出し側が持つ。SceneManager より長く生存する必要がある。
    AbstractSceneFactory* sceneFactory_ = nullptr;

    std::unique_ptr<IScene> scene_;
};
