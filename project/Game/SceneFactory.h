// ============================================================================
// ファイルの役割: 文字列で指定されたシーン名から対応するシーンインスタンスを生成する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include "AbstractSceneFactory.h"

// ゲームで使用する具体的な Scene の生成を担当する。
class SceneFactory final : public AbstractSceneFactory {
public:
    std::unique_ptr<IScene> CreateScene(const std::string& sceneName) override;
};
