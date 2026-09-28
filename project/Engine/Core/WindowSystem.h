// ============================================================================
// ファイルの役割: ゲームウィンドウの生成、メッセージ処理、破棄を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <memory>
#include "WinApp.h"

// ウィンドウ生成とメッセージ処理を担当するクラス。
// WinApp の所有者。
class WindowSystem {
public:
    void Initialize();
    void Finalize();

    // true が返ったら終了メッセージあり。
    bool ProcessMessage();

    WinApp* GetWinApp() const { return winApp_.get(); }

private:
    std::unique_ptr<WinApp> winApp_;
};