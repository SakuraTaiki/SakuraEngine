// ============================================================================
// ファイルの役割: DirectX 12のデバイス、コマンド、スワップチェーン、描画フレームを統括する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "DirectXCommon.h"
#include "WinApp.h"
#include <vector>
#include <cassert>
#include <format>
#include <thread>
#include "MyMath.h"
#include <algorithm>

using namespace Microsoft::WRL;

#include "DirectXCommonInitialize.h"
#include "DirectXCommonRenderTexture.h"
#include "DirectXCommonDevice.h"
#include "DirectXCommonFrame.h"
#include "DirectXCommonPipeline.h"
#include "DirectXCommonUtility.h"
