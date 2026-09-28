// ============================================================================
// ファイルの役割: デバッグ・演出用プリミティブの生成、更新、描画を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "Primitive.h"

#include <cassert>
#include <cstring>
#include <numbers>
#include <random>

#include <algorithm>
#include <cmath>

#include "D3DResourceHelper.h"
#include "EffectMath.h"

using namespace Microsoft::WRL;

namespace
{
    std::random_device primitiveSeedGenerator;

    std::mt19937_64 primitiveRandomEngine(
        primitiveSeedGenerator()
    );
}


#include "PrimitiveLifecycle.h"
#include "PrimitiveUpdate.h"
#include "PrimitiveDraw.h"
#include "PrimitiveEmit.h"
#include "PrimitivePipeline.h"
