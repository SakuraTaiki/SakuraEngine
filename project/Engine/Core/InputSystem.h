// ============================================================================
// ファイルの役割: 入力機器の初期化とフレームごとの入力更新を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <memory>
#include "Input.h"

class WinApp;
class EngineContext;

// 入力機能を担当するクラス。
// Input の所有と毎フレーム更新を行う。
class InputSystem {
public:
    void Initialize(WinApp* winApp, EngineContext* context);
    void Finalize();
    void Update();

    Input* GetInput() const { return input_.get(); }

private:
    std::unique_ptr<Input> input_;
};