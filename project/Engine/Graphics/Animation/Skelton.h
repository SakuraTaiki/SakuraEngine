// ============================================================================
// ファイルの役割: モデルの階層構造からスケルトンと関節行列を構築・更新する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once
#include <vector>
#include <map>
#include <optional>
#include "Engine/Graphics/3D/Model.h"
#include "Animation.h"


struct Joint {

    QuaternionTransform transform;

    Matrix4x4 localMatrix;
    Matrix4x4 skeletonSpaceMatrix;

    std::string name;

    std::vector<int32_t> children;

    int32_t index;

    std::optional<int32_t> parent;
};

struct Skeleton {

    int32_t root;

    std::map<std::string, int32_t> jointMap;

    std::vector<Joint> joints;
};

int32_t CreateJoint(
    const Node& node,
    const std::optional<int32_t>& parent,
    std::vector<Joint>& joints);

Skeleton CreateSkeleton(const Node& rootNode);

void UpdateSkelton(Skeleton& skeleton);

void ApplyAnimation(
    Skeleton& skeleton,
    const Animation& animation,
    float animationTime);
