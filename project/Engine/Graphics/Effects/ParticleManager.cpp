// ============================================================================
// ファイルの役割: パーティクルの生成、更新、描画、GPUリソースを管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "ParticleManager.h"

#include<cmath>
#include <cassert>
#include <random>
#include<numbers>
#include <algorithm>

#include "D3DResourceHelper.h"

using namespace Microsoft::WRL;


static std::random_device seed_gen;
static std::mt19937_64 engine(seed_gen());


#include "ParticleManagerLifecycle.h"
#include "ParticleManagerUpdate.h"
#include "ParticleManagerDraw.h"
#include "ParticleManagerEmit.h"
#include "ParticleManagerPipeline.h"
