// ============================================================================
// ファイルの役割: ビュー行列と透視投影行列を生成し、3D描画で使用する視点を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include "MyMath.h"

// 3D 描画で使うカメラ。
// Transform から ViewMatrix を作り、
// Fov / AspectRatio / Near / Far から ProjectionMatrix を作る。
class Camera {
public:
    Camera();

    // Transform や Projection 設定から各種 Matrix を更新する。
    // カメラ値を変更した後は Update を呼ぶ。
    void Update();

    void SetRotate(const Vector3& rotate) { transform_.rotate = rotate; }
    void SetTranslate(const Vector3& translate) { transform_.translate = translate; }

    void SetFovY(float fovY) { fovY_ = fovY; }
    void SetAspectRatio(float aspectRatio) { aspectRatio_ = aspectRatio; }
    void SetNearClip(float nearClip) { nearClip_ = nearClip; }
    void SetFarClip(float farClip) { farClip_ = farClip; }

    const Matrix4x4& GetWorldMatrix() const { return worldMatrix_; }
    const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }
    const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix_; }
    const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix_; }

    const Vector3& GetRotate() const { return transform_.rotate; }
    const Vector3& GetTranslate() const { return transform_.translate; }

    float GetFovY() const { return fovY_; }
    float GetAspectRatio() const { return aspectRatio_; }
    float GetNearClip() const { return nearClip_; }
    float GetFarClip() const { return farClip_; }

private:
    // カメラの Transform。
    // scale は基本 1 のままで使う。
    Transform transform_;

    // GPU や描画処理から参照する行列。
    Matrix4x4 worldMatrix_;
    Matrix4x4 viewMatrix_;
    Matrix4x4 projectionMatrix_;
    Matrix4x4 viewProjectionMatrix_;

    // Projection 設定。
    float fovY_;
    float aspectRatio_;
    float nearClip_;
    float farClip_;
};