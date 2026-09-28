// ============================================================================
// ファイルの役割: 個別3DオブジェクトのTransform、Material、Model、描画を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <memory>

#include "Object3dCommon.h"
#include "Model.h"
#include "MyMath.h"
#include "camera.h"

class Object3dSkinning;

struct Animation;
struct Skeleton;

// GPU に送る Transform 情報。
struct TransformationMatrix {
    Matrix4x4 WVP;
    Matrix4x4 World;
    Matrix4x4 WorldInverseTranspose;
};

// GPU に送る Camera 情報。
struct CameraForGPU {
    Vector3 worldPosition;
    float padding;
};

// GPU に送る Material 情報。
struct Material {
    Vector4 color;
    int32_t enableLighting;

    float environmentCoefficient;
    float shininess;
    float padding;

    Matrix4x4 uvTransform;
};

// 3D Object 本体。
// Transform / Material / Model / Draw を担当する。
// Skinning / Animation は Object3dSkinning に任せる。
class Object3d {
public:

    // Object3dSkinningの完全な定義が見える.cpp側で生成・破棄する
    Object3d();
    // Object3dSkinningの完全な型が見える.cpp側で破棄する
    ~Object3d();

    void Initialize(Object3dCommon* object3dCommon);
    void Update();
    void Draw();

    void SetModel(Model* model);
    Model* GetModel() { return model_; }
    const Model* GetModel() const { return model_; }

    // Skinning 用。Object3dSkinning がある場合だけ反映する。
    void SetSkeleton(const Skeleton& skeleton);
    void SetAnimation(const Animation& animation);

    void SetPosition(const Vector3& position) { transform_.translate = position; }
    void SetRotation(const Vector3& rotation) { transform_.rotate = rotation; }
    void SetScale(const Vector3& scale) { transform_.scale = scale; }

    Transform& GetTransform() { return transform_; }
    const Transform& GetTransform() const { return transform_; }

    void SetParent(Object3d* parent) { parent_ = parent; }
    Object3d* GetParent() { return parent_; }
    const Object3d* GetParent() const { return parent_; }
    const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }

    void SetCamera(Camera* camera) { camera_ = camera; }

    void SetColor(const Vector4& color) {
        if (materialData_) {
            materialData_->color = color;
        }
    }

    void SetEnableLighting(bool enable) {
        if (materialData_) {
            materialData_->enableLighting = enable ? 1 : 0;
        }
    }

    void SetShininess(float shininess) {
        if (materialData_) {
            materialData_->shininess = shininess;
        }
    }

    void SetUVTransform(const Transform& uvTransform);

    void SetEnvironmentCoefficient(float value) {
        if (materialData_) {
            materialData_->environmentCoefficient = value;
        }
    }

    void SetEnvironmentTexture(uint32_t textureHandle) {
        environmentTextureHandle_ = textureHandle;
    }

    bool useOverrideTexture_ = false;
    uint32_t overrideTextureHandle_ = 0;

    void SetOverrideTexture(uint32_t textureHandle) {
        overrideTextureHandle_ = textureHandle;
        useOverrideTexture_ = true;
    }

    void ClearOverrideTexture() {
        useOverrideTexture_ = false;
        overrideTextureHandle_ = 0;
    }

    bool HasOverrideTexture() const {
        return useOverrideTexture_;
    }

    uint32_t GetOverrideTextureHandle() const {
        return overrideTextureHandle_;
    }


    float GetEnvironmentCoefficient() const {
        if (!materialData_) {
            return 0.0f;
        }

        return materialData_->environmentCoefficient;
    }

    Material* GetMaterial() {
        return materialData_;
    }

    const Material* GetMaterial() const {
        return materialData_;
    }

private:
    // 非所有。Object3dCommon は GraphicsSystem 側が持つ。
    Object3dCommon* object3dCommon_ = nullptr;

    // 非所有。ModelManager などが持つ Model を参照する。
    Model* model_ = nullptr;

    Transform transform_ = { {1, 1, 1}, {0, 0, 0}, {0, 0, 0} };
    Object3d* parent_ = nullptr;
    Matrix4x4 worldMatrix_{};

    // Transform 用 GPU Buffer。
    Microsoft::WRL::ComPtr<ID3D12Resource> transformationResource_;
    TransformationMatrix* transformationData_ = nullptr;

    // Material 用 GPU Buffer。
    Microsoft::WRL::ComPtr<ID3D12Resource> materialResource_;
    Material* materialData_ = nullptr;

    // 非所有。基本は Object3dCommon の defaultCamera を使う。
    Camera* camera_ = nullptr;

    // Camera 用 GPU Buffer。
    Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource_;
    CameraForGPU* cameraData_ = nullptr;

    uint32_t environmentTextureHandle_ = 0;

    // スキニングとアニメーション処理。
    std::unique_ptr<Object3dSkinning> skinning_;
};
