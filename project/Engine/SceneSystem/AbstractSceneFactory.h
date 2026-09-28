// ============================================================================
// ファイルの役割: AbstractSceneFactoryに関するデータ構造と処理を提供し、担当機能を他モジュールから分離する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <memory>
#include <string>

#include "IScene.h"

// Scene の生成を SceneManager から分離するための抽象ファクトリー。
class AbstractSceneFactory {
public:
    virtual ~AbstractSceneFactory() = default;

    virtual std::unique_ptr<IScene> CreateScene(const std::string& sceneName) = 0;
};
