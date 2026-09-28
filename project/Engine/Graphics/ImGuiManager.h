#pragma once
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "WinApp.h"

class ImGuiManager
{
public:
    void Initialize(
        DirectXCommon* dxCommon,
        SrvManager* srvManager,
        WinApp* winApp);

    void Finalize();

    void Begin();

    void End();

    void Draw();

    void UpdateGameViewTexture();

    uint32_t GetGameViewSrvIndex() const {
        return gameViewSrvIndex_;
    }

private:
    DirectXCommon* dxCommon_ = nullptr;
    SrvManager* srvManager_ = nullptr;

    uint32_t gameViewSrvIndex_ = 0;
};

// ============================================================================
// ファイルの役割: Dear ImGuiの初期化、フレーム開始、描画、終了処理を管理する。
// 構成上の位置付け: エンジンが提供するデバッグUI基盤の公開インターフェースを宣言する。
// 実装時の注意: Win32・DirectX 12バックエンドとディスクリプタの寿命をそろえる。
// ============================================================================
