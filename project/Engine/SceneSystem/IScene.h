// ============================================================================
// ファイルの役割: ISceneに関するデータ構造と処理を提供し、担当機能を他モジュールから分離する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

class EngineContext;

// Scene 共通インターフェース。
// SceneManager はこの型で Scene を保持する。
class IScene {
public:
    virtual ~IScene() = default;

    virtual void Initialize(EngineContext* context) = 0;
    virtual void Finalize() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;
};