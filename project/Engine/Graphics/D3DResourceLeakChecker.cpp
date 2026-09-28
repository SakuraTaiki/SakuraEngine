// ============================================================================
// ファイルの役割: DirectX 12リソースの生成補助またはリーク検出を担当する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "D3DResourceLeakChecker.h"
#include <dxgidebug.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

// 処理概要: D3DResourceLeakCheckerの生成・破棄に必要な初期状態を整える。
// 注意事項: 所有リソースの生成と解放が対になるように管理する。
D3DResourceLeakChecker::~D3DResourceLeakChecker() {

	//リソースリークチェック
	Microsoft::WRL::ComPtr<IDXGIDebug1> debug;
	// DXGIのデバッグインターフェースを取得
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		// DXGI全体のリソースチェック（アプリが作ったリソースがまだ残ってるか確認）
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
	}
}
