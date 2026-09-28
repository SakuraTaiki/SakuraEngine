// ============================================================================
// ファイルの役割: モデルファイルからアニメーションデータを読み込み、エンジン形式へ変換する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "AnimationLoader.h"

#include <cassert>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

Animation AnimationLoader::Load(
    const std::string& directoryPath,
    const std::string& filename
) {
    Animation animation{};

    Assimp::Importer importer;

    std::string filePath = directoryPath + "/" + filename;

    const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);

    assert(scene);
    assert(scene->mNumAnimations != 0);

    // 今は最初の Animation だけを読む。
    aiAnimation* animationAssimp = scene->mAnimations[0];

    float ticksPerSecond = static_cast<float>(animationAssimp->mTicksPerSecond);

    if (ticksPerSecond == 0.0f) {
        ticksPerSecond = 1.0f;
    }

    animation.duration =
        static_cast<float>(animationAssimp->mDuration / ticksPerSecond);

    // Node ごとの animation channel を読み込む。
    for (uint32_t channelIndex = 0;
        channelIndex < animationAssimp->mNumChannels;
        ++channelIndex)
    {
        aiNodeAnim* nodeAnimationAssimp =
            animationAssimp->mChannels[channelIndex];

        NodeAnimation& nodeAnimation =
            animation.nodeAnimations[nodeAnimationAssimp->mNodeName.C_Str()];

        // Translation keyframes.
        for (uint32_t keyIndex = 0;
            keyIndex < nodeAnimationAssimp->mNumPositionKeys;
            ++keyIndex)
        {
            aiVectorKey& keyAssimp =
                nodeAnimationAssimp->mPositionKeys[keyIndex];

            KeyframeVector3 keyframe{};
            keyframe.time =
                static_cast<float>(keyAssimp.mTime / ticksPerSecond);

            keyframe.value = {
                -keyAssimp.mValue.x,
                 keyAssimp.mValue.y,
                 keyAssimp.mValue.z
            };

            nodeAnimation.translate.push_back(keyframe);
        }

        // Rotation keyframes.
        for (uint32_t keyIndex = 0;
            keyIndex < nodeAnimationAssimp->mNumRotationKeys;
            ++keyIndex)
        {
            aiQuatKey& keyAssimp =
                nodeAnimationAssimp->mRotationKeys[keyIndex];

            KeyframeQuaternion keyframe{};
            keyframe.time =
                static_cast<float>(keyAssimp.mTime / ticksPerSecond);

            keyframe.value = {
                 keyAssimp.mValue.x,
                -keyAssimp.mValue.y,
                -keyAssimp.mValue.z,
                 keyAssimp.mValue.w
            };

            keyframe.value = Math::Normalize(keyframe.value);

            nodeAnimation.rotate.push_back(keyframe);
        }

        // Scale keyframes.
        for (uint32_t keyIndex = 0;
            keyIndex < nodeAnimationAssimp->mNumScalingKeys;
            ++keyIndex)
        {
            aiVectorKey& keyAssimp =
                nodeAnimationAssimp->mScalingKeys[keyIndex];

            KeyframeVector3 keyframe{};
            keyframe.time =
                static_cast<float>(keyAssimp.mTime / ticksPerSecond);

            keyframe.value = {
                keyAssimp.mValue.x,
                keyAssimp.mValue.y,
                keyAssimp.mValue.z
            };

            nodeAnimation.scale.push_back(keyframe);
        }
    }

    return animation;
}