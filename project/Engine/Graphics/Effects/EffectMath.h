// ============================================================================
// ファイルの役割: EffectMathに関するデータ構造と処理を提供し、担当機能を他モジュールから分離する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <algorithm>
#include <cmath>

#include "MyMath.h"

namespace EffectMath {

    inline float EaseOut(float time, float power)
    {
        return 1.0f - std::pow(
            1.0f - time,
            (std::max)(power, 0.01f)
        );
    }

    inline float FadeIn(float time, float ratio)
    {
        if (ratio <= 0.0f) {
            return 1.0f;
        }

        return std::clamp(
            time / ratio,
            0.0f,
            1.0f
        );
    }

    inline float FadeOut(float time, float power)
    {
        return std::pow(
            1.0f - time,
            (std::max)(power, 0.01f)
        );
    }

    inline Vector4 LerpColor(
        const Vector4& start,
        const Vector4& end,
        float time
    ) {
        return {
            start.x + (end.x - start.x) * time,
            start.y + (end.y - start.y) * time,
            start.z + (end.z - start.z) * time,
            start.w + (end.w - start.w) * time
        };
    }

    inline Vector4 MakeFadedColor(
        const Vector4& start,
        const Vector4& end,
        float time,
        float intensity,
        float fadeIn,
        float fadeOut
    ) {
        Vector4 color =
            LerpColor(start, end, time);

        return {
            color.x * intensity,
            color.y * intensity,
            color.z * intensity,
            color.w * fadeIn * fadeOut
        };
    }
}
