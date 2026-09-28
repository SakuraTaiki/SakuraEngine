// ============================================================================
// ファイルの役割: ビュー行列と透視投影行列を生成し、3D描画で使用する視点を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "Camera.h"

Camera::Camera()
    : transform_({
        { 1.0f, 1.0f, 1.0f },
        { 0.3f, 0.0f, 0.0f },
        { 0.0f, 7.0f, -30.0f }
        })
    , worldMatrix_(Math::MakeIdentity4x4())
    , viewMatrix_(Math::MakeIdentity4x4())
    , projectionMatrix_(Math::MakeIdentity4x4())
    , viewProjectionMatrix_(Math::MakeIdentity4x4())
    , fovY_(0.45f)
    , aspectRatio_(16.0f / 9.0f)
    , nearClip_(0.1f)
    , farClip_(100.0f) {}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void Camera::Update() {
    // Camera の WorldMatrix を作る。
    worldMatrix_ = Math::MakeAffineMatrix(
        transform_.scale,
        transform_.rotate,
        transform_.translate
    );

    // Camera の WorldMatrix の逆行列が ViewMatrix。
    viewMatrix_ = Math::Inverse(worldMatrix_);

    // ProjectionMatrix を作る。
    projectionMatrix_ = Math::MakePerspectiveFovMatrix(
        fovY_,
        aspectRatio_,
        nearClip_,
        farClip_
    );

    // 描画でよく使う ViewProjection を先に作っておく。
    viewProjectionMatrix_ = Math::Multiply(viewMatrix_, projectionMatrix_);
}
