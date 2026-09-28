// ============================================================================
// ファイルの役割: Compute Shaderを利用したGPUパーティクルの生成・更新・描画を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "GPUParticleManager.h"

#include <algorithm>
#include <cassert>
#include <cstring>

#include "D3DResourceHelper.h"

using Microsoft::WRL::ComPtr;


#include "GPUParticleManagerLifecycle.h"
#include "GPUParticleManagerUpdate.h"
#include "GPUParticleManagerDraw.h"
#include "GPUParticleManagerEmit.h"
#include "GPUParticleManagerResources.h"
#include "GPUParticleManagerGraphicsPipeline.h"
#include "GPUParticleManagerComputePipeline.h"
#include "GPUParticleManagerUtility.h"
