// ============================================================================
// ファイルの役割: Windowsアプリケーションのエントリーポイントを提供し、ゲーム本体の起動と終了を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include <memory>
#include "MyGame.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    std::unique_ptr<MyGame> game = std::make_unique<MyGame>();

    game->Initialize();

    while (game->IsRunning()) {
        game->Update();
        game->Draw();
    }

    game->Finalize();

    return 0;
}