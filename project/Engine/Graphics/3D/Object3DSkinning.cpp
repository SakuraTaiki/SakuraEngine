// ============================================================================
// ファイルの役割: スキニング済み3Dモデル専用の更新・描画処理を提供する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "Object3dSkinning.h"

#include "Object3dCommon.h"
#include "Model.h"

#include <cmath>

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void Object3dSkinning::Initialize(Object3dCommon* object3dCommon, Model* model) {
    object3dCommon_ = object3dCommon;
    model_ = model;

    hasSkinCluster_ = false;

    if (!object3dCommon_ || !model_) {
        return;
    }

    // Model の Node 階層から Skeleton を作成する。
    skeleton_ = CreateSkeleton(model_->GetRootNode());

    // Model に skin 情報があれば SkinCluster を作成する。
    hasSkinCluster_ = CreateSkinCluster(
        skinCluster_,
        object3dCommon_->GetDxCommon()->GetDevice(),
        object3dCommon_->GetSrvManager(),
        skeleton_,
        *model_
    );
}

// 処理概要: 外部から渡された値を、担当オブジェクトの状態へ反映する。
// 注意事項: 必要に応じて範囲制限や依存データの再計算も行う。
void Object3dSkinning::SetSkeleton(const Skeleton& skeleton) {
    skeleton_ = skeleton;
}

// 処理概要: 外部から渡された値を、担当オブジェクトの状態へ反映する。
// 注意事項: 必要に応じて範囲制限や依存データの再計算も行う。
void Object3dSkinning::SetAnimation(const Animation& animation) {
    animation_ = animation;
    useAnimation_ = true;
    animationTime_ = 0.0f;
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void Object3dSkinning::Update() {
    if (!model_ || !hasSkinCluster_) {
        return;
    }

    // Animation がある場合は時間を進めて Skeleton に反映する。
    if (useAnimation_) {
        animationTime_ += 1.0f / 60.0f;

        if (animation_.duration > 0.0f) {
            animationTime_ = std::fmod(animationTime_, animation_.duration);
        }

        ApplyAnimation(skeleton_, animation_, animationTime_);
    }

    UpdateSkelton(skeleton_);
    UpdateSkinCluster(skinCluster_, skeleton_);
}

// 処理概要: Object3dSkinningが担当する「DispatchComputeSkinning」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void Object3dSkinning::DispatchComputeSkinning() {
    if (!object3dCommon_ || !hasSkinCluster_ ||
        skinCluster_.vertexCount == 0 ||
        !skinCluster_.computeDispatchRequired) {
        return;
    }

    ID3D12GraphicsCommandList* commandList =
        object3dCommon_->GetDxCommon()->GetCommandList();

    if (skinCluster_.skinnedVertexIsReadyForDraw) {
        D3D12_RESOURCE_BARRIER toUav{};
        toUav.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        toUav.Transition.pResource =
            skinCluster_.skinnedVertexResource.Get();
        toUav.Transition.StateBefore =
            D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        toUav.Transition.StateAfter =
            D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        toUav.Transition.Subresource =
            D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        commandList->ResourceBarrier(1, &toUav);
    }

    commandList->SetComputeRootSignature(
        object3dCommon_->GetSkinningComputeRootSignature()
    );
    commandList->SetPipelineState(
        object3dCommon_->GetSkinningComputePipelineState()
    );
    commandList->SetComputeRootDescriptorTable(
        0,
        skinCluster_.sourceVertexSrvHandle
    );
    commandList->SetComputeRootDescriptorTable(
        1,
        skinCluster_.influenceSrvHandle
    );
    commandList->SetComputeRootDescriptorTable(
        2,
        skinCluster_.paletteSrvHandle
    );
    commandList->SetComputeRootDescriptorTable(
        3,
        skinCluster_.skinnedVertexUavHandle
    );
    commandList->SetComputeRoot32BitConstant(
        4,
        skinCluster_.vertexCount,
        0
    );
    commandList->Dispatch(
        (skinCluster_.vertexCount + 1023u) / 1024u,
        1,
        1
    );

    D3D12_RESOURCE_BARRIER uavBarrier{};
    uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    uavBarrier.UAV.pResource =
        skinCluster_.skinnedVertexResource.Get();
    commandList->ResourceBarrier(1, &uavBarrier);

    D3D12_RESOURCE_BARRIER toVertexBuffer{};
    toVertexBuffer.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    toVertexBuffer.Transition.pResource =
        skinCluster_.skinnedVertexResource.Get();
    toVertexBuffer.Transition.StateBefore =
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    toVertexBuffer.Transition.StateAfter =
        D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
    toVertexBuffer.Transition.Subresource =
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList->ResourceBarrier(1, &toVertexBuffer);
    skinCluster_.skinnedVertexIsReadyForDraw = true;
    skinCluster_.computeDispatchRequired = false;
}

// 処理概要: 外部から必要な状態またはリソース参照を取得する。
// 注意事項: 返す参照やポインターの寿命は所有オブジェクトに従う。
Matrix4x4 Object3dSkinning::GetRootLocalMatrix() const {
    if (!model_) {
        return Math::MakeIdentity4x4();
    }

    Matrix4x4 localMatrix = model_->GetRootNode().localMatrix;

    if (!useAnimation_) {
        return localMatrix;
    }

    auto it = animation_.nodeAnimations.find(model_->GetRootNode().name);

    if (it == animation_.nodeAnimations.end()) {
        return localMatrix;
    }

    const NodeAnimation& rootNodeAnimation = it->second;

    Vector3 translate =
        CalculateValue(rootNodeAnimation.translate, animationTime_);

    Quaternion rotate =
        CalculateValue(rootNodeAnimation.rotate, animationTime_);

    Vector3 scale =
        CalculateValue(rootNodeAnimation.scale, animationTime_);

    return Math::MakeAffineMatrix(scale, rotate, translate);
}
