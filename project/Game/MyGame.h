// ============================================================================
// ファイルの役割: 作品固有の初期シーン設定とゲーム起動構成を定義する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include "Engine.h"
#include "SceneFactory.h"
#include "SceneManager.h"

// MyGame はゲーム全体の進行役。
// Engine の初期化と SceneManager の更新・描画をつなぐ。
class MyGame {
public:
    void Initialize();
    void Update();
    void Draw();
    void Finalize();

    bool IsRunning();

private:
    Engine engine_;
    SceneFactory sceneFactory_;
    SceneManager sceneManager_;
};
