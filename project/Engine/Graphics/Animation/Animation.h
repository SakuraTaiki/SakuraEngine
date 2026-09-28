// ============================================================================
// ファイルの役割: キーフレーム補間とアニメーション時間の評価に必要なデータと処理を提供する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <map>
#include <string>
#include <vector>

#include "MyMath.h"

// 1つのキー情報。
// Vector3 と Quaternion の両方で使うため template にしている。
template <typename TValue>
struct Keyframe {
    float time;
    TValue value;
};

using KeyframeVector3 = Keyframe<Vector3>;
using KeyframeQuaternion = Keyframe<Quaternion>;

// 1 Node 分の Animation。
// translate / rotate / scale を別々の keyframe として持つ。
struct NodeAnimation {
    std::vector<KeyframeVector3> translate;
    std::vector<KeyframeQuaternion> rotate;
    std::vector<KeyframeVector3> scale;
};

// Animation 全体のデータ。
// nodeAnimations は Node 名をキーにしている。
struct Animation {
    float duration = 0.0f;
    std::map<std::string, NodeAnimation> nodeAnimations;
};

// 指定時刻の Vector3 を補間して返す。
Vector3 CalculateValue(
    const std::vector<KeyframeVector3>& keyframes,
    float time
);

// 指定時刻の Quaternion を補間して返す。
Quaternion CalculateValue(
    const std::vector<KeyframeQuaternion>& keyframes,
    float time
);