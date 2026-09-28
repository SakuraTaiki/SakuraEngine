// ============================================================================
// ファイルの役割: 個別3DオブジェクトのTransform、Material、Model、描画を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "Object3d.h"

#include "Object3dSkinning.h"
#include "TextureManager.h"
#include "D3DResourceHelper.h"

#include <cassert>


// Object3dSkinningの完全な定義が見える場所でデストラクタを生成する
Object3d::Object3d() = default;
Object3d::~Object3d() = default;

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void Object3d::Initialize(Object3dCommon* object3dCommon) {
    assert(object3dCommon);
    object3dCommon_ = object3dCommon;

    auto device = object3dCommon_->GetDxCommon()->GetDevice();

    camera_ = object3dCommon_->GetDefaultCamera();

    transformationResource_ =
        D3DResourceHelper::CreateUploadBuffer(
            device,
            D3DResourceHelper::AlignConstantBufferSize(
                sizeof(TransformationMatrix)
            )
        );

    transformationData_ =
        D3DResourceHelper::Map<TransformationMatrix>(
            transformationResource_.Get()
        );

    transformationData_->WVP = Math::MakeIdentity4x4();
    transformationData_->World = Math::MakeIdentity4x4();
    transformationData_->WorldInverseTranspose = Math::MakeIdentity4x4();
    worldMatrix_ = Math::MakeIdentity4x4();

    materialResource_ =
        D3DResourceHelper::CreateUploadBuffer(
            device,
            D3DResourceHelper::AlignConstantBufferSize(
                sizeof(Material)
            )
        );

    materialData_ =
        D3DResourceHelper::Map<Material>(
            materialResource_.Get()
        );

    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
    materialData_->enableLighting = 1;
    materialData_->environmentCoefficient = 0.05f;
    materialData_->shininess = 32.0f;
    materialData_->padding = 0.0f;
    materialData_->uvTransform = Math::MakeIdentity4x4();

    cameraResource_ =
        D3DResourceHelper::CreateUploadBuffer(
            device,
            D3DResourceHelper::AlignConstantBufferSize(
                sizeof(CameraForGPU)
            )
        );

    cameraData_ =
        D3DResourceHelper::Map<CameraForGPU>(
            cameraResource_.Get()
        );

    cameraData_->worldPosition = { 0.0f, 0.0f, 0.0f };
    cameraData_->padding = 0.0f;
}

// 処理概要: 外部から渡された値を、担当オブジェクトの状態へ反映する。
// 注意事項: 必要に応じて範囲制限や依存データの再計算も行う。
void Object3d::SetModel(Model* model) {
    model_ = model;

    skinning_.reset();

    if (!model_) {
        return;
    }

    skinning_ = std::make_unique<Object3dSkinning>();
    skinning_->Initialize(object3dCommon_, model_);
}

// 処理概要: 外部から渡された値を、担当オブジェクトの状態へ反映する。
// 注意事項: 必要に応じて範囲制限や依存データの再計算も行う。
void Object3d::SetSkeleton(const Skeleton& skeleton) {
    if (skinning_) {
        skinning_->SetSkeleton(skeleton);
    }
}

// 処理概要: 外部から渡された値を、担当オブジェクトの状態へ反映する。
// 注意事項: 必要に応じて範囲制限や依存データの再計算も行う。
void Object3d::SetAnimation(const Animation& animation) {
    if (skinning_) {
        skinning_->SetAnimation(animation);
    }
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void Object3d::Update() {
    worldMatrix_ = Math::MakeAffineMatrix(
        transform_.scale,
        transform_.rotate,
        transform_.translate
    );

    if (parent_) {
        worldMatrix_ = Math::Multiply(
            worldMatrix_,
            parent_->GetWorldMatrix()
        );
    }

    Matrix4x4 worldViewProjectionmatrix = worldMatrix_;

    if (camera_) {
        worldViewProjectionmatrix =
            Math::Multiply(worldMatrix_, camera_->GetViewProjectionMatrix());

        if (cameraData_) {
            cameraData_->worldPosition = camera_->GetTranslate();
        }
    }

    if (skinning_) {
        skinning_->Update();
    }

    if (model_) {
        Matrix4x4 localMatrix = model_->GetRootNode().localMatrix;

        if (skinning_) {
            localMatrix = skinning_->GetRootLocalMatrix();
        }

        Matrix4x4 worldWithRoot = Math::Multiply(localMatrix, worldMatrix_);
        Matrix4x4 wvpWithRoot = worldWithRoot;

        if (camera_) {
            wvpWithRoot =
                Math::Multiply(worldWithRoot, camera_->GetViewProjectionMatrix());
        }

        transformationData_->WVP = wvpWithRoot;
        transformationData_->World = worldWithRoot;
    } else {
        transformationData_->WVP = worldViewProjectionmatrix;
        transformationData_->World = worldMatrix_;
    }

    transformationData_->WorldInverseTranspose =
        Math::Transpose(Math::Inverse(transformationData_->World));
}

// 処理概要: 外部から渡された値を、担当オブジェクトの状態へ反映する。
// 注意事項: 必要に応じて範囲制限や依存データの再計算も行う。
void Object3d::SetUVTransform(const Transform& t) {
    if (materialData_) {
        Matrix4x4 w = Math::MakeAffineMatrix(t.scale, t.rotate, t.translate);
        materialData_->uvTransform = w;
    }
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void Object3d::Draw() {
    if (!model_) {
        return;
    }

    auto commandList = object3dCommon_->GetDxCommon()->GetCommandList();

    if (skinning_ && skinning_->HasSkinCluster()) {
        skinning_->DispatchComputeSkinning();
        // Compute dispatch changes both root signature and PSO.
        object3dCommon_->PreDraw();
    }

    commandList->SetGraphicsRootConstantBufferView(
        0,
        materialResource_->GetGPUVirtualAddress()
    );

    commandList->SetGraphicsRootConstantBufferView(
        1,
        transformationResource_->GetGPUVirtualAddress()
    );

    commandList->SetGraphicsRootConstantBufferView(
        2,
        object3dCommon_->GetLightGPUVirtualAddress()
    );

    commandList->SetGraphicsRootConstantBufferView(
        3,
        cameraResource_->GetGPUVirtualAddress()
    );

    commandList->SetGraphicsRootConstantBufferView(
        7,
        object3dCommon_->GetSpotLightGPUVirtualAddress()
    );


    commandList->SetGraphicsRootConstantBufferView(
        8,
        object3dCommon_->GetPointLightGPUVirtualAddress()
    );

    if (object3dCommon_->GetTextureManager()) {
       
        const uint32_t mainTextureHandle =
            useOverrideTexture_
            ? overrideTextureHandle_
            : model_->GetTextureHandle();

        auto gpuHandle =
            object3dCommon_->GetTextureManager()->GetSrvHandleGPU(
                mainTextureHandle
            );

        commandList->SetGraphicsRootDescriptorTable(
            4,
            gpuHandle
        );


        if (environmentTextureHandle_ != 0) {
            auto envHandle =
                object3dCommon_->GetTextureManager()->GetSrvHandleGPU(
                    environmentTextureHandle_
                );

            commandList->SetGraphicsRootDescriptorTable(
                5,
                envHandle
            );
        } else {
            auto envHandle =
                object3dCommon_->GetTextureManager()->GetSrvHandleGPU(
                    model_->GetTextureHandle()
                );

            commandList->SetGraphicsRootDescriptorTable(
                5,
                envHandle
            );
        }
    }

    if (skinning_ && skinning_->HasSkinCluster()) {
        model_->Draw(
            commandList,
            skinning_->GetSkinnedVertexBufferView()
        );
    } else {
        model_->Draw(commandList);
    }
}
