// ============================================================================
// ファイルの役割: アニメーションの再生状態、骨表示、編集用UIを管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <memory>
#include <vector>

#include "Animation.h"
#include "Object3d.h"
#include "Skelton.h"

class Object3dCommon;
class Input;

class AnimationDebugController
{
public:
    enum class EditorMode {
        Play,
        AnimationEdit,
        Preview
    };

private:
    enum class PlayerAnimationState {
        Idle,
        Walk,
        Run,
        Slide,
        Jump
    };

public:
    void Initialize(
        Object3dCommon* object3dCommon,
        uint32_t environmentTextureHandle,
        float environmentCoefficient
    );

    void Finalize();

    void Update(Input* input);
    void Draw();
    void DrawImGui();

    int GetJointCount() const;
    float GetAnimationTime() const {
        return animationTime_;
    }

    const char* GetPlayerAnimationStateName() const;
    float GetMovementHoldTime() const { return movementHoldTime_; }
    float GetCurrentAnimationDuration() const { return animation_.duration; }
    bool IsAnimationPlaying() const { return animationPlaying_; }
    bool IsAnimationLoop() const { return animationLoop_; }
    bool IsAnimationBlending() const { return isBlending_; }
    bool IsManualAnimationTest() const { return manualAnimationTest_; }
    void SetManualAnimationTest(bool enabled) { manualAnimationTest_ = enabled; }

    void ForceIdleAnimation();
    void ForceWalkAnimation();
    void ForceRunAnimation();
    void ForceSlideAnimation();
    void ForceJumpAnimation();

    EditorMode GetEditorMode() const { return editorMode_; }
    const char* GetEditorModeName() const;
    void SetEditorMode(EditorMode mode);
    void ResetSelectedJointPose();
    void ResetAllJointPoses();

    bool& ShowSkeletonDebug() {
        return showSkeletonDebug_;
    }

    void SetEnvironmentCoefficient(
        float environmentCoefficient
    );

private:
    void InitializeSkeletonDebug(
        Object3dCommon* object3dCommon,
        uint32_t environmentTextureHandle,
        float environmentCoefficient
    );

    void UpdateAnimationInput(Input* input);
    void UpdateMovement(Input* input);
    void UpdateAnimation();
    void UpdateSkeletonDebug();
    void SyncSkeletonToObject();

    void StartAnimationTransition(
        const Animation& nextAnimation,
        float blendDuration,
        bool loop
    );

    void ChangePlayerAnimation(PlayerAnimationState nextState);

private:
    std::unique_ptr<Object3d> animatedObject_;

    std::vector<std::unique_ptr<Object3d>>
        skeletonDebugObjects_;

    std::vector<std::unique_ptr<Object3d>>
        skeletonBoneObjects_;

    // 読み込んだアニメーション
    Animation idleAnimation_;
    Animation walkAnimation_;
    Animation runAnimation_;
    Animation slideAnimation_;
    Animation jumpAnimation_;

    // 現在再生中のアニメーション
    Animation animation_;

    Skeleton skeleton_;
    std::vector<QuaternionTransform> initialPose_;
    std::vector<bool> skinJointMask_;

    float animationTime_ = 0.0f;

    bool showSkeletonDebug_ = false;
    bool animationPlaying_ = true;
    bool animationLoop_ = true;
    float animationSpeed_ = 1.0f;

    // クロスフェード開始時の姿勢
    std::vector<QuaternionTransform> blendStartPose_;
    Animation blendSourceAnimation_;
    float blendSourceAnimationTime_ = 0.0f;
    bool blendSourceAnimationLoop_ = true;
    bool hasBlendSourceAnimation_ = false;

    bool isBlending_ = false;
    float blendTime_ = 0.0f;
    float blendDuration_ = 0.25f;

    PlayerAnimationState playerAnimationState_ =
        PlayerAnimationState::Idle;
    float movementHoldTime_ = 0.0f;
    bool requireMovementRelease_ = false;
    bool manualAnimationTest_ = false;
    EditorMode editorMode_ = EditorMode::Play;

    // Jump is moved in world space. The glTF only controls the body pose.
    bool isJumping_ = false;
    float jumpVelocity_ = 0.0f;
    float groundHeight_ = 0.0f;
    float jumpInitialVelocity_ = 15.0f;
    float jumpGravity_ = 30.0f;
    float movementSpeed_ = 3.0f;
    float modelPitch_ = -1.57079632679f;
    float modelYaw_ = 0.0f;

    float jointDisplaySize_ = 0.06f;
    float boneDisplayThickness_ = 0.055f;
    float modelDebugOpacity_ = 0.25f;

    int selectedJointIndex_ = 0;
    bool autoPauseOnJointEdit_ = true;
};
