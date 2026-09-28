// ============================================================================
// ファイルの役割: スキニング済み3Dモデル専用の更新・描画処理を提供する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include "Animation.h"
#include "Skelton.h"
#include "SkinCluster.h"

class Object3dCommon;
class Model;

// Object3d のうち、スキニングとアニメーションだけを担当するクラス。
// Object3d 本体から分けることで、通常描画とスキニング処理を分離する。
class Object3dSkinning {
public:
    void Initialize(Object3dCommon* object3dCommon, Model* model);

    // 外部で編集した Skeleton を反映する。
    void SetSkeleton(const Skeleton& skeleton);

    // Animation をセットすると、このクラス内で再生時間を進める。
    void SetAnimation(const Animation& animation);

    void Update();
    void DispatchComputeSkinning();

    bool HasSkinCluster() const { return hasSkinCluster_; }

    const D3D12_VERTEX_BUFFER_VIEW* GetSkinnedVertexBufferView() const {
        return &skinCluster_.skinnedVertexBufferView;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetPaletteSrvHandle() const {
        return skinCluster_.paletteSrvHandle;
    }

    // RootNode の localMatrix を返す。
    // Animation がある場合は、現在時刻の RootNode 行列を返す。
    Matrix4x4 GetRootLocalMatrix() const;

private:
    // 非所有。Object3d 側が持つ共通描画情報と Model を参照する。
    Object3dCommon* object3dCommon_ = nullptr;
    Model* model_ = nullptr;

    Animation animation_;
    bool useAnimation_ = false;
    float animationTime_ = 0.0f;

    Skeleton skeleton_{};
    SkinCluster skinCluster_{};
    bool hasSkinCluster_ = false;
};
