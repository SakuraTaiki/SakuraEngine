// ============================================================================
// ファイルの役割: モデルの階層構造からスケルトンと関節行列を構築・更新する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "Skelton.h"
#include <cassert>

int32_t CreateJoint(
    const Node& node,
    const std::optional<int32_t>& parent,
    std::vector<Joint>& joints)
{
    Joint joint{};

    joint.name = node.name;
    joint.localMatrix = node.localMatrix;
    joint.skeletonSpaceMatrix = Math::MakeIdentity4x4();
    joint.transform = node.transform;
    joint.index = static_cast<int32_t>(joints.size());
    joint.parent = parent;

    joints.push_back(joint);

    for (const Node& child : node.children) {
        int32_t childIndex =
            CreateJoint(child, joint.index, joints);

        joints[joint.index].children.push_back(childIndex);
    }

    return joint.index;
}

Skeleton CreateSkeleton(const Node& rootNode) {
    Skeleton skeleton{};

    skeleton.root = CreateJoint(rootNode, std::nullopt, skeleton.joints);

    for (const Joint& joint : skeleton.joints) {
        skeleton.jointMap.emplace(joint.name, joint.index);
    }

    UpdateSkelton(skeleton);

    return skeleton;
}

void UpdateSkelton(Skeleton& skeleton) {
    for (Joint& joint : skeleton.joints) {
        joint.localMatrix =
            Math::MakeAffineMatrix(
                joint.transform.scale,
                joint.transform.rotate,
                joint.transform.translate);

        if (joint.parent) {
            joint.skeletonSpaceMatrix =
                Math::Multiply(
                    joint.localMatrix,
                    skeleton.joints[*joint.parent].skeletonSpaceMatrix);
        } else {
            joint.skeletonSpaceMatrix = joint.localMatrix;
        }
    }
}

void ApplyAnimation(
    Skeleton& skeleton,
    const Animation& animation,
    float animationTime)
{
    for (Joint& joint : skeleton.joints) {

        auto it = animation.nodeAnimations.find(joint.name);

        if (it == animation.nodeAnimations.end()) {
            continue;
        }

        const NodeAnimation& nodeAnimation = it->second;

        if (!nodeAnimation.translate.empty()) {
            joint.transform.translate =
                CalculateValue(nodeAnimation.translate, animationTime);
        }

        if (!nodeAnimation.rotate.empty()) {
            joint.transform.rotate =
                CalculateValue(nodeAnimation.rotate, animationTime);
        }

        if (!nodeAnimation.scale.empty()) {
            joint.transform.scale =
                CalculateValue(nodeAnimation.scale, animationTime);
        }
    }
}