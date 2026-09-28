// ============================================================================
// ファイルの役割: アニメーションの再生状態、骨表示、編集用UIを管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "AnimationDebugController.h"
#include "AnimationLoader.h"
#include "ModelManager.h"
#include "Object3dCommon.h"
#include "Input.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void AnimationDebugController::Initialize(
    Object3dCommon* object3dCommon,
    uint32_t environmentTextureHandle,
    float environmentCoefficient
) {
    Model* modelAnimated =
        ModelManager::Load("Resources/human", "walk.gltf");

    
    // 2種類のアニメーションを読み込む
    idleAnimation_ =
        AnimationLoader::Load(
            "Resources/human",
            "idle.gltf"
        );

    walkAnimation_ =
        AnimationLoader::Load(
            "Resources/human",
            "walk.gltf"
        );

    runAnimation_ =
        AnimationLoader::Load(
            "Resources/human",
            "run.gltf"
        );

    slideAnimation_ =
        AnimationLoader::Load(
            "Resources/human",
            "slide.gltf"
        );

    jumpAnimation_ =
        AnimationLoader::Load(
            "Resources/human",
            "jump.gltf"
        );


    // 初期状態はWalk
    animation_ = idleAnimation_;
    animationTime_ = 0.0f;
    animationPlaying_ = true;
    animationLoop_ = true;
    playerAnimationState_ = PlayerAnimationState::Idle;
    movementHoldTime_ = 0.0f;
    requireMovementRelease_ = false;
    isJumping_ = false;
    jumpVelocity_ = 0.0f;
    groundHeight_ = 0.0f;

    skeleton_ =
        CreateSkeleton(modelAnimated->GetRootNode());

    // Only nodes used by the skin cluster are actual deforming bones.
    skinJointMask_.assign(skeleton_.joints.size(), false);
    const auto& skinClusterData = modelAnimated->GetSkinClusterData();
    for (size_t i = 0; i < skeleton_.joints.size(); ++i) {
        skinJointMask_[i] =
            skinClusterData.find(skeleton_.joints[i].name) != skinClusterData.end();
    }

    initialPose_.resize(skeleton_.joints.size());
    for (size_t i = 0; i < skeleton_.joints.size(); ++i) {
        initialPose_[i] = skeleton_.joints[i].transform;
    }

    animatedObject_ = std::make_unique<Object3d>();
    animatedObject_->Initialize(object3dCommon);
    animatedObject_->SetModel(modelAnimated);
    animatedObject_->SetSkeleton(skeleton_);
   

    animatedObject_->SetPosition({ 0.0f, 0.0f, 0.0f });
    animatedObject_->SetRotation({ modelPitch_, modelYaw_, 0.0f });
    animatedObject_->SetScale({ 200.0f, 200.0f, 200.0f });

    animatedObject_->SetEnvironmentTexture(environmentTextureHandle);
    animatedObject_->SetEnvironmentCoefficient(environmentCoefficient);

    // The debug skeleton is parented to the animated model so both always use
    // exactly the same object/world transform.
    InitializeSkeletonDebug(
        object3dCommon,
        environmentTextureHandle,
        environmentCoefficient
    );
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void AnimationDebugController::Finalize() {
    animatedObject_.reset();
    skeletonDebugObjects_.clear();
    skeletonBoneObjects_.clear();
}

void AnimationDebugController::InitializeSkeletonDebug(
    Object3dCommon* object3dCommon,
    uint32_t environmentTextureHandle,
    float environmentCoefficient
) {
    skeletonDebugObjects_.clear();
    skeletonBoneObjects_.clear();

    Model* jointModel =
        ModelManager::Load("Resources/Editor", "ico_sphere.obj");
    Model* boneModel =
        ModelManager::Load("Resources/Editor", "bone_line.obj");

    for (size_t i = 0; i < skeleton_.joints.size(); ++i) {
        auto jointObj = std::make_unique<Object3d>();
        jointObj->Initialize(object3dCommon);
        jointObj->SetModel(jointModel);
        jointObj->SetScale({ jointDisplaySize_, jointDisplaySize_, jointDisplaySize_ });
        jointObj->SetRotation({ 0.0f, 0.0f, 0.0f });
        jointObj->SetPosition({ 0.0f, 0.0f, 0.0f });
        jointObj->SetEnvironmentTexture(environmentTextureHandle);
        jointObj->SetEnvironmentCoefficient(environmentCoefficient);
        jointObj->SetEnableLighting(false);
        jointObj->SetColor({ 1.0f, 0.55f, 0.10f, 1.0f });
        jointObj->SetParent(animatedObject_.get());

        skeletonDebugObjects_.push_back(std::move(jointObj));
    }

    for (const Joint& joint : skeleton_.joints) {
        if (!joint.parent) {
            continue;
        }

        auto boneObj = std::make_unique<Object3d>();
        boneObj->Initialize(object3dCommon);
        boneObj->SetModel(boneModel);
        boneObj->SetScale({ 1.0f, boneDisplayThickness_, boneDisplayThickness_ });
        boneObj->SetRotation({ 0.0f, 0.0f, 0.0f });
        boneObj->SetPosition({ 0.0f, 0.0f, 0.0f });
        boneObj->SetEnvironmentTexture(environmentTextureHandle);
        boneObj->SetEnvironmentCoefficient(environmentCoefficient);
        boneObj->SetEnableLighting(false);
        boneObj->SetColor({ 0.88f, 0.88f, 0.88f, 1.0f });
        boneObj->SetParent(animatedObject_.get());

        skeletonBoneObjects_.push_back(std::move(boneObj));
    }
}

void AnimationDebugController::Update(Input* input)
{
#ifndef USE_IMGUI
    // Release has no ImGui checkbox.  Let the evaluator toggle the skeleton
    // overlay directly while keeping all gameplay keys unchanged.
    if (input && input->TriggerKey(DIK_B)) {
        showSkeletonDebug_ = !showSkeletonDebug_;
    }
#endif

    if (editorMode_ == EditorMode::Play) {
        UpdateMovement(input);
        UpdateAnimationInput(input);
        UpdateAnimation();
    } else if (editorMode_ == EditorMode::Preview) {
        UpdateAnimation();
    } else {
        // Animation Edit mode keeps the pose edited by ImGui.
        UpdateSkelton(skeleton_);
    }

    SyncSkeletonToObject();

    if (animatedObject_) {
        animatedObject_->Update();
    }

    // Parent world matrix must be current before updating child debug objects.
    if (showSkeletonDebug_) {
        UpdateSkeletonDebug();
    }
}



void AnimationDebugController::UpdateAnimationInput(Input* input)
{

    if (!input) {
        return;
    }

    if (manualAnimationTest_) {
        return;
    }

    constexpr float deltaTime = 1.0f / 60.0f;
    constexpr float runStartSeconds = 2.0f;

    const bool movementPressed =
        input->PushKey(DIK_W) ||
        input->PushKey(DIK_A) ||
        input->PushKey(DIK_S) ||
        input->PushKey(DIK_D) ||
        std::abs(input->GetLeftStickX()) > 0.0f ||
        std::abs(input->GetLeftStickY()) > 0.0f;

    const bool slideTriggered =
        input->TriggerKey(DIK_LCONTROL) ||
        input->TriggerKey(DIK_RCONTROL);

    const bool jumpTriggered =
        input->TriggerKey(DIK_RETURN) ||
        input->TriggerGamepadButton(XINPUT_GAMEPAD_A);

    if (jumpTriggered && !isJumping_) {
        isJumping_ = true;
        jumpVelocity_ = jumpInitialVelocity_;
        ChangePlayerAnimation(PlayerAnimationState::Jump);
        return;
    }

    if (playerAnimationState_ == PlayerAnimationState::Slide || isJumping_) {
        return;
    }

    if (requireMovementRelease_) {
        if (movementPressed) {
            ChangePlayerAnimation(PlayerAnimationState::Idle);
            return;
        }
        requireMovementRelease_ = false;
    }

    // 1キー：Walk
    if (slideTriggered && movementPressed) {
        movementHoldTime_ = 0.0f;
        requireMovementRelease_ = true;
        ChangePlayerAnimation(PlayerAnimationState::Slide);
        return;
    }

    // 2キー：Sneak
    if (!movementPressed) {
        movementHoldTime_ = 0.0f;
        ChangePlayerAnimation(PlayerAnimationState::Idle);
        return;
    }

    movementHoldTime_ += deltaTime;

    if (movementHoldTime_ < runStartSeconds) {
        ChangePlayerAnimation(PlayerAnimationState::Walk);
    } else {
        ChangePlayerAnimation(PlayerAnimationState::Run);
    }
}


void AnimationDebugController::UpdateAnimation()
{
    constexpr float deltaTime =
        1.0f / 60.0f;

    if (isJumping_ && animatedObject_) {
        Vector3 position = animatedObject_->GetTransform().translate;
        jumpVelocity_ -= jumpGravity_ * deltaTime;
        position.y += jumpVelocity_ * deltaTime;

        if (position.y <= groundHeight_ && jumpVelocity_ < 0.0f) {
            position.y = groundHeight_;
            jumpVelocity_ = 0.0f;
            isJumping_ = false;
            ChangePlayerAnimation(PlayerAnimationState::Idle);
        }

        animatedObject_->SetPosition(position);
    }

    // アニメーション時間を進める
    if (animationPlaying_) {
        animationTime_ +=
            deltaTime * animationSpeed_;

        if (animation_.duration > 0.0f) {
            if (animationLoop_) {
                animationTime_ =
                    std::fmod(
                        animationTime_,
                        animation_.duration
                    );
            } else if (
                animationTime_ >=
                animation_.duration
                ) {
                animationTime_ =
                    animation_.duration;

                animationPlaying_ = false;
            }
        }
    }

    if (isBlending_ && hasBlendSourceAnimation_) {
        blendSourceAnimationTime_ += deltaTime * animationSpeed_;
        if (blendSourceAnimation_.duration > 0.0f) {
            if (blendSourceAnimationLoop_) {
                blendSourceAnimationTime_ = std::fmod(
                    blendSourceAnimationTime_,
                    blendSourceAnimation_.duration
                );
            } else {
                blendSourceAnimationTime_ = (std::min)(
                    blendSourceAnimationTime_,
                    blendSourceAnimation_.duration
                );
            }
        }
    }

    if (playerAnimationState_ == PlayerAnimationState::Slide &&
        !animationPlaying_) {
        playerAnimationState_ = PlayerAnimationState::Idle;
        StartAnimationTransition(idleAnimation_, 0.18f, true);
    }

    if (isBlending_) {
        // 新しいアニメーション側の姿勢を計算
        Skeleton sourceSkeleton = skeleton_;
        if (hasBlendSourceAnimation_) {
            ApplyAnimation(
                sourceSkeleton,
                blendSourceAnimation_,
                blendSourceAnimationTime_
            );
        }

        Skeleton targetSkeleton =
            skeleton_;

        ApplyAnimation(
            targetSkeleton,
            animation_,
            animationTime_
        );

        blendTime_ += deltaTime;

        float blend =
            std::clamp(
                blendTime_ /
                blendDuration_,
                0.0f,
                1.0f
            );

        // Linear cross-fade: (1 - t) * source + t * target.
        // 開始と終了の速度を滑らかにする
        const size_t jointCount =
            (std::min)(
                skeleton_.joints.size(),
                blendStartPose_.size()
            );

        for (size_t i = 0;
            i < jointCount;
            ++i) {

            const QuaternionTransform& start =
                hasBlendSourceAnimation_
                ? sourceSkeleton.joints[i].transform
                : blendStartPose_[i];

            const QuaternionTransform& target =
                targetSkeleton
                .joints[i]
                .transform;

            skeleton_
                .joints[i]
                .transform
                .translate =
                Math::Lerp(
                    start.translate,
                    target.translate,
                    blend
                );

            skeleton_
                .joints[i]
                .transform
                .rotate =
                Math::Slerp(
                    start.rotate,
                    target.rotate,
                    blend
                );

            skeleton_
                .joints[i]
                .transform
                .scale =
                Math::Lerp(
                    start.scale,
                    target.scale,
                    blend
                );
        }

        if (blendTime_ >= blendDuration_) {
            isBlending_ = false;
            hasBlendSourceAnimation_ = false;

            // 最後にターゲット姿勢を正確に反映
            ApplyAnimation(
                skeleton_,
                animation_,
                animationTime_
            );
        }
    } else if (animationPlaying_) {
        ApplyAnimation(
            skeleton_,
            animation_,
            animationTime_
        );
    }

    UpdateSkelton(skeleton_);
}


// 処理概要: AnimationDebugControllerが担当する「SyncSkeletonToObject」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void AnimationDebugController::SyncSkeletonToObject() {
    if (!animatedObject_) {
        return;
    }

    animatedObject_->SetSkeleton(skeleton_);
}

void AnimationDebugController::ChangePlayerAnimation(
    PlayerAnimationState nextState
)
{
    if (playerAnimationState_ == nextState) {
        return;
    }

    playerAnimationState_ = nextState;

    switch (nextState) {
    case PlayerAnimationState::Idle:
        StartAnimationTransition(idleAnimation_, 0.20f, true);
        break;
    case PlayerAnimationState::Walk:
        StartAnimationTransition(walkAnimation_, 0.20f, true);
        break;
    case PlayerAnimationState::Run:
        StartAnimationTransition(runAnimation_, 0.20f, true);
        break;
    case PlayerAnimationState::Slide:
        StartAnimationTransition(slideAnimation_, 0.12f, false);
        break;
    case PlayerAnimationState::Jump:
        StartAnimationTransition(jumpAnimation_, 0.08f, false);
        break;
    }
}

const char* AnimationDebugController::GetPlayerAnimationStateName() const
{
    switch (playerAnimationState_) {
    case PlayerAnimationState::Idle:  return "Idle";
    case PlayerAnimationState::Walk:  return "Walk";
    case PlayerAnimationState::Run:   return "Run";
    case PlayerAnimationState::Slide: return "Slide";
    case PlayerAnimationState::Jump:  return "Jump";
    }
    return "Unknown";
}

void AnimationDebugController::ForceIdleAnimation()
{
    ChangePlayerAnimation(PlayerAnimationState::Idle);
}

void AnimationDebugController::ForceWalkAnimation()
{
    ChangePlayerAnimation(PlayerAnimationState::Walk);
}

void AnimationDebugController::ForceRunAnimation()
{
    ChangePlayerAnimation(PlayerAnimationState::Run);
}

void AnimationDebugController::ForceSlideAnimation()
{
    ChangePlayerAnimation(PlayerAnimationState::Slide);
}

void AnimationDebugController::ForceJumpAnimation()
{
    if (!isJumping_) {
        isJumping_ = true;
        jumpVelocity_ = jumpInitialVelocity_;
    }
    ChangePlayerAnimation(PlayerAnimationState::Jump);
}

void AnimationDebugController::UpdateMovement(Input* input)
{
    if (!input || !animatedObject_ || manualAnimationTest_) {
        return;
    }

    constexpr float deltaTime = 1.0f / 60.0f;

    Vector3 direction = { 0.0f, 0.0f, 0.0f };
    if (input->PushKey(DIK_W)) {
        direction.z += 1.0f;
    }
    if (input->PushKey(DIK_S)) {
        direction.z -= 1.0f;
    }
    if (input->PushKey(DIK_A)) {
        direction.x -= 1.0f;
    }
    if (input->PushKey(DIK_D)) {
        direction.x += 1.0f;
    }

    direction.x += input->GetLeftStickX();
    direction.z += input->GetLeftStickY();

    const float length = std::sqrt(
        direction.x * direction.x +
        direction.z * direction.z
    );
    if (length <= 0.00001f) {
        return;
    }

    direction.x /= length;
    direction.z /= length;

    Vector3 position = animatedObject_->GetTransform().translate;
    position.x += direction.x * movementSpeed_ * deltaTime;
    position.z += direction.z * movementSpeed_ * deltaTime;
    animatedObject_->SetPosition(position);

    // +Z is the character's forward direction in this left-handed world.
    modelYaw_ = std::atan2(direction.x, direction.z);
    animatedObject_->SetRotation({ modelPitch_, modelYaw_, 0.0f });
}

const char* AnimationDebugController::GetEditorModeName() const
{
    switch (editorMode_) {
    case EditorMode::Play:          return "Play";
    case EditorMode::AnimationEdit: return "Animation Edit";
    case EditorMode::Preview:       return "Preview";
    }
    return "Unknown";
}

void AnimationDebugController::SetEditorMode(EditorMode mode)
{
    if (editorMode_ == mode) {
        return;
    }

    editorMode_ = mode;
    isBlending_ = false;
    isJumping_ = false;
    jumpVelocity_ = 0.0f;
    // The skeleton overlay is an Animation Edit tool.  Never carry it into
    // Play or Preview when changing modes.
    showSkeletonDebug_ = editorMode_ == EditorMode::AnimationEdit;

    if (animatedObject_) {
        Vector3 position = animatedObject_->GetTransform().translate;
        position.y = groundHeight_;
        animatedObject_->SetPosition(position);
    }

    if (editorMode_ == EditorMode::AnimationEdit) {
        animationPlaying_ = false;
        manualAnimationTest_ = true;
    } else if (editorMode_ == EditorMode::Preview) {
        animationTime_ = 0.0f;
        animationPlaying_ = true;
        manualAnimationTest_ = true;
    } else {
        manualAnimationTest_ = false;
        playerAnimationState_ = PlayerAnimationState::Idle;
        StartAnimationTransition(idleAnimation_, 0.15f, true);
    }
}

void AnimationDebugController::ResetSelectedJointPose()
{
    if (selectedJointIndex_ < 0 ||
        static_cast<size_t>(selectedJointIndex_) >= skeleton_.joints.size() ||
        static_cast<size_t>(selectedJointIndex_) >= initialPose_.size()) {
        return;
    }

    skeleton_.joints[selectedJointIndex_].transform =
        initialPose_[selectedJointIndex_];
    UpdateSkelton(skeleton_);
}

void AnimationDebugController::ResetAllJointPoses()
{
    const size_t count = (std::min)(skeleton_.joints.size(), initialPose_.size());
    for (size_t i = 0; i < count; ++i) {
        skeleton_.joints[i].transform = initialPose_[i];
    }
    UpdateSkelton(skeleton_);
}


void AnimationDebugController::StartAnimationTransition(
    const Animation& nextAnimation,
    float blendDuration,
    bool loop
)
{
    hasBlendSourceAnimation_ =
        !isBlending_ &&
        !animation_.nodeAnimations.empty();
    if (hasBlendSourceAnimation_) {
        blendSourceAnimation_ = animation_;
        blendSourceAnimationTime_ = animationTime_;
        blendSourceAnimationLoop_ = animationLoop_;
    }

    // 現在画面に出ている姿勢を保存
    blendStartPose_.resize(
        skeleton_.joints.size()
    );

    for (size_t i = 0;
        i < skeleton_.joints.size();
        ++i) {

        blendStartPose_[i] =
            skeleton_.joints[i].transform;
    }

    // 次のアニメーションへ変更
    animation_ = nextAnimation;
    animationTime_ = 0.0f;

    blendTime_ = 0.0f;

    blendDuration_ =
        (std::max)(
            blendDuration,
            0.001f
        );

    isBlending_ = true;
    animationPlaying_ = true;
    animationLoop_ = loop;

}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void AnimationDebugController::UpdateSkeletonDebug() {
    if (skeletonDebugObjects_.size() != skeleton_.joints.size()) {
        return;
    }

    // Object3d renders the skinned mesh with:
    //   rootNode.localMatrix * objectWorld
    // Apply that same root-node matrix to debug joint coordinates. This keeps
    // the course Skeleton/Joint/SkinCluster structure unchanged while placing
    // markers in exactly the same mesh space as the rendered model.
    Matrix4x4 modelRootMatrix = Math::MakeIdentity4x4();
    if (animatedObject_ && animatedObject_->GetModel()) {
        modelRootMatrix = animatedObject_->GetModel()->GetRootNode().localMatrix;
    }

    // Debug objects are children of animatedObject_. Cancel only the parent's
    // scale for marker size/thickness. Their positions and bone lengths stay in
    // model local space so they follow the exact same transform as the mesh.
    Vector3 inverseParentScale = { 1.0f, 1.0f, 1.0f };
    if (animatedObject_) {
        const Vector3& parentScale = animatedObject_->GetTransform().scale;
        if (std::abs(parentScale.x) > 0.00001f) {
            inverseParentScale.x = 1.0f / std::abs(parentScale.x);
        }
        if (std::abs(parentScale.y) > 0.00001f) {
            inverseParentScale.y = 1.0f / std::abs(parentScale.y);
        }
        if (std::abs(parentScale.z) > 0.00001f) {
            inverseParentScale.z = 1.0f / std::abs(parentScale.z);
        }
    }

    for (size_t i = 0; i < skeleton_.joints.size(); ++i) {
        const Matrix4x4 mat = Math::Multiply(
            skeleton_.joints[i].skeletonSpaceMatrix,
            modelRootMatrix
        );

        Vector3 jointPosition = {
            mat.m[3][0],
            mat.m[3][1],
            mat.m[3][2]
        };

        skeletonDebugObjects_[i]->SetPosition(jointPosition);
        skeletonDebugObjects_[i]->SetRotation({ 0.0f, 0.0f, 0.0f });
        const bool isSkinJoint =
            i < skinJointMask_.size() && skinJointMask_[i];
        const float displaySize = isSkinJoint ? jointDisplaySize_ : 0.0f;
        skeletonDebugObjects_[i]->SetScale({
            displaySize * inverseParentScale.x,
            displaySize * inverseParentScale.y,
            displaySize * inverseParentScale.z
        });
        if (static_cast<int>(i) == selectedJointIndex_) {
            skeletonDebugObjects_[i]->SetColor({ 1.0f, 0.95f, 0.15f, 1.0f });
        } else {
            skeletonDebugObjects_[i]->SetColor({ 1.0f, 0.55f, 0.10f, 1.0f });
        }
        skeletonDebugObjects_[i]->Update();
    }

    size_t boneIndex = 0;

    for (const Joint& joint : skeleton_.joints) {
        if (!joint.parent) {
            continue;
        }

        if (boneIndex >= skeletonBoneObjects_.size()) {
            break;
        }

        const bool childIsSkinJoint =
            static_cast<size_t>(joint.index) < skinJointMask_.size() &&
            skinJointMask_[joint.index];
        const bool parentIsSkinJoint =
            static_cast<size_t>(*joint.parent) < skinJointMask_.size() &&
            skinJointMask_[*joint.parent];

        if (!childIsSkinJoint || !parentIsSkinJoint) {
            skeletonBoneObjects_[boneIndex]->SetScale({ 0.0f, 0.0f, 0.0f });
            skeletonBoneObjects_[boneIndex]->Update();
            ++boneIndex;
            continue;
        }

        const Matrix4x4 childMat = Math::Multiply(
            joint.skeletonSpaceMatrix,
            modelRootMatrix
        );

        const Matrix4x4 parentMat = Math::Multiply(
            skeleton_.joints[*joint.parent].skeletonSpaceMatrix,
            modelRootMatrix
        );

        Vector3 childPos = {
            childMat.m[3][0],
            childMat.m[3][1],
            childMat.m[3][2]
        };

        Vector3 parentPos = {
            parentMat.m[3][0],
            parentMat.m[3][1],
            parentMat.m[3][2]
        };

        Vector3 center = {
            (childPos.x + parentPos.x) * 0.5f,
            (childPos.y + parentPos.y) * 0.5f,
            (childPos.z + parentPos.z) * 0.5f
        };

        Vector3 direction = {
            childPos.x - parentPos.x,
            childPos.y - parentPos.y,
            childPos.z - parentPos.z
        };
        const float length = std::sqrt(
            direction.x * direction.x +
            direction.y * direction.y +
            direction.z * direction.z
        );

        Vector3 rotation = { 0.0f, 0.0f, 0.0f };
        if (length > 0.00001f) {
            direction.x /= length;
            direction.y /= length;
            direction.z /= length;
            rotation.y = -std::asin(std::clamp(direction.z, -1.0f, 1.0f));
            rotation.z = std::atan2(direction.y, direction.x);
        }

        skeletonBoneObjects_[boneIndex]->SetPosition(center);
        skeletonBoneObjects_[boneIndex]->SetRotation(rotation);
        skeletonBoneObjects_[boneIndex]->SetScale({
            length,
            boneDisplayThickness_ * inverseParentScale.y,
            boneDisplayThickness_ * inverseParentScale.z
        });
        skeletonBoneObjects_[boneIndex]->Update();

        ++boneIndex;
    }
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void AnimationDebugController::Draw() {
    if (!showSkeletonDebug_) {
        if (animatedObject_) {
            animatedObject_->SetColor({ 1.0f, 1.0f, 1.0f, 1.0f });
            animatedObject_->Draw();
        }
        return;
    }

    // Draw bones first. The transparent model is blended over them afterwards,
    // so joints inside the body remain visible without changing skinning.
    for (auto& debugObject : skeletonDebugObjects_) {
        debugObject->Draw();
    }

    for (auto& boneObject : skeletonBoneObjects_) {
        boneObject->Draw();
    }

    if (animatedObject_) {
        animatedObject_->SetColor({
            1.0f,
            1.0f,
            1.0f,
            std::clamp(modelDebugOpacity_, 0.0f, 1.0f)
        });
        animatedObject_->Draw();
    }
}

// 処理概要: 外部から必要な状態またはリソース参照を取得する。
// 注意事項: 返す参照やポインターの寿命は所有オブジェクトに従う。
int AnimationDebugController::GetJointCount() const {
    return static_cast<int>(skeleton_.joints.size());
}

void AnimationDebugController::SetEnvironmentCoefficient(
    float environmentCoefficient
) {
    if (animatedObject_) {
        animatedObject_->SetEnvironmentCoefficient(environmentCoefficient);
    }

    for (auto& object : skeletonDebugObjects_) {
        object->SetEnvironmentCoefficient(environmentCoefficient);
    }

    for (auto& object : skeletonBoneObjects_) {
        object->SetEnvironmentCoefficient(environmentCoefficient);
    }
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void AnimationDebugController::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::Text("Mode");
    const char* modes[] = { "Play", "Animation Edit", "Preview" };
    int modeIndex = static_cast<int>(editorMode_);
    if (ImGui::Combo("##AnimationEditorMode", &modeIndex, modes, 3)) {
        SetEditorMode(static_cast<EditorMode>(modeIndex));
    }
    ImGui::Text("Current Mode : %s", GetEditorModeName());
    ImGui::Separator();

    ImGui::Text("Animation Control");
    ImGui::Separator();

    if (editorMode_ != EditorMode::AnimationEdit) {

    if (ImGui::Button(
        animationPlaying_ ? "Pause" : "Play",
        ImVec2(100.0f, 28.0f)
    )) {
        animationPlaying_ = !animationPlaying_;
    }

    ImGui::SameLine();

    if (ImGui::Button("Reset", ImVec2(100.0f, 28.0f))) {
        animationTime_ = 0.0f;

        ApplyAnimation(
            skeleton_,
            animation_,
            animationTime_
        );

        UpdateSkelton(skeleton_);
    }

    ImGui::Checkbox("Loop", &animationLoop_);
    ImGui::Checkbox("Auto Pause On Joint Edit", &autoPauseOnJointEdit_);
    ImGui::Checkbox("Show Skeleton Debug", &showSkeletonDebug_);

    ImGui::DragFloat(
        "Speed",
        &animationSpeed_,
        0.01f,
        0.0f,
        5.0f
    );

    if (animation_.duration > 0.0f) {
        ImGui::SliderFloat(
            "Time",
            &animationTime_,
            0.0f,
            animation_.duration
        );

        if (!animationPlaying_) {
            ApplyAnimation(
                skeleton_,
                animation_,
                animationTime_
            );

            UpdateSkelton(skeleton_);
        }
    }
    } else {
        ImGui::TextWrapped(
            "Playback is locked while editing. Select a joint below and edit its local transform."
        );
        ImGui::Checkbox("Show Skeleton Debug", &showSkeletonDebug_);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Skeleton");

    ImGui::DragFloat(
        "Joint Box Size",
        &jointDisplaySize_,
        0.002f,
        0.005f,
        0.5f
    );
    ImGui::DragFloat(
        "Bone Line Thickness",
        &boneDisplayThickness_,
        0.001f,
        0.002f,
        0.2f
    );
    ImGui::SliderFloat(
        "Model Opacity",
        &modelDebugOpacity_,
        0.05f,
        1.0f,
        "%.2f"
    );

    const int jointCount =
        static_cast<int>(skeleton_.joints.size());

    ImGui::Text("Joint Count : %d", jointCount);

    if (jointCount == 0) {
        return;
    }

    selectedJointIndex_ =
        std::clamp(
            selectedJointIndex_,
            0,
            jointCount - 1
        );

    const char* previewName =
        skeleton_.joints[selectedJointIndex_].name.c_str();

    if (ImGui::BeginCombo("Selected Joint", previewName)) {
        for (int i = 0; i < jointCount; ++i) {
            const bool isSelected =
                selectedJointIndex_ == i;

            std::string label =
                std::to_string(i) +
                " : " +
                skeleton_.joints[i].name;

            if (ImGui::Selectable(label.c_str(), isSelected)) {
                selectedJointIndex_ = i;
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    if (ImGui::TreeNode("Bone Hierarchy")) {
        std::function<void(int32_t)> drawJointTree;
        drawJointTree = [&](int32_t jointIndex) {
            if (jointIndex < 0 || jointIndex >= jointCount) {
                return;
            }

            const Joint& treeJoint = skeleton_.joints[jointIndex];
            ImGui::PushID(jointIndex);
            const bool hasChildren = !treeJoint.children.empty();
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanAvailWidth;
            if (!hasChildren) {
                flags |= ImGuiTreeNodeFlags_Leaf |
                    ImGuiTreeNodeFlags_NoTreePushOnOpen;
            }
            if (selectedJointIndex_ == jointIndex) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            const bool open = ImGui::TreeNodeEx("##joint", flags, "%s", treeJoint.name.c_str());
            if (ImGui::IsItemClicked()) {
                selectedJointIndex_ = jointIndex;
            }
            if (hasChildren && open) {
                for (int32_t childIndex : treeJoint.children) {
                    drawJointTree(childIndex);
                }
                ImGui::TreePop();
            }
            ImGui::PopID();
        };

        drawJointTree(skeleton_.root);
        ImGui::TreePop();
    }

    Joint& joint =
        skeleton_.joints[selectedJointIndex_];

    ImGui::Spacing();
    ImGui::Text("Joint Detail");
    ImGui::Separator();

    ImGui::Text("Name   : %s", joint.name.c_str());
    ImGui::Text("Index  : %d", joint.index);

    if (joint.parent) {
        ImGui::Text("Parent : %d", *joint.parent);
    } else {
        ImGui::Text("Parent : none");
    }

    Vector3 translate = joint.transform.translate;
    Vector3 scale = joint.transform.scale;
    Quaternion rotate = joint.transform.rotate;

    bool edited = false;

    if (editorMode_ == EditorMode::AnimationEdit) {

    edited |= ImGui::DragFloat3(
        "Local Translate",
        &translate.x,
        0.01f,
        -100.0f,
        100.0f
    );

    edited |= ImGui::DragFloat3(
        "Local Scale",
        &scale.x,
        0.01f,
        0.001f,
        100.0f
    );

    edited |= ImGui::DragFloat4(
        "Local Rotate Quaternion",
        &rotate.x,
        0.01f,
        -1.0f,
        1.0f
    );

    if (ImGui::Button("Reset Selected Joint")) {
        ResetSelectedJointPose();
        translate = joint.transform.translate;
        scale = joint.transform.scale;
        rotate = joint.transform.rotate;
    }

    ImGui::SameLine();
    if (ImGui::Button("Reset All Joints")) {
        ResetAllJointPoses();
        translate = joint.transform.translate;
        scale = joint.transform.scale;
        rotate = joint.transform.rotate;
    }

    if (ImGui::Button(
        "Normalize Quaternion",
        ImVec2(180.0f, 28.0f)
    )) {
        rotate = Math::Normalize(rotate);
        edited = true;
    }

    if (edited) {
        if (autoPauseOnJointEdit_) {
            animationPlaying_ = false;
        }

        joint.transform.translate = translate;
        joint.transform.scale = scale;
        joint.transform.rotate = Math::Normalize(rotate);

        UpdateSkelton(skeleton_);
    }
    } else {
        ImGui::TextDisabled("Switch to Animation Edit mode to modify this joint.");
        ImGui::Text(
            "Local Translate : %.3f, %.3f, %.3f",
            translate.x, translate.y, translate.z
        );
        ImGui::Text(
            "Local Scale : %.3f, %.3f, %.3f",
            scale.x, scale.y, scale.z
        );
        ImGui::Text(
            "Local Rotate : %.3f, %.3f, %.3f, %.3f",
            rotate.x, rotate.y, rotate.z, rotate.w
        );
    }

    const Matrix4x4& skeletonMatrix =
        joint.skeletonSpaceMatrix;

    Vector3 skeletonPosition = {
        skeletonMatrix.m[3][0],
        skeletonMatrix.m[3][1],
        skeletonMatrix.m[3][2]
    };

    ImGui::Spacing();
    ImGui::Text("Skeleton Space");
    ImGui::BulletText(
        "Position : %.3f, %.3f, %.3f",
        skeletonPosition.x,
        skeletonPosition.y,
        skeletonPosition.z
    );

    if (ImGui::TreeNode("Children")) {
        for (int32_t childIndex : joint.children) {
            const Joint& child =
                skeleton_.joints[childIndex];

            ImGui::BulletText(
                "%d : %s",
                childIndex,
                child.name.c_str()
            );
        }

        ImGui::TreePop();
    }
#endif
}
