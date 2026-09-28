// ============================================================================
// ファイルの役割: デバッグカメラの操作と通常カメラへの切り替えを管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "CameraDebugController.h"

#include "Camera.h"
#include "Input.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float kPi =
        3.14159265358979323846f;

    constexpr float kPitchLimit =
        kPi * 0.5f - 0.01f;
}


void CameraDebugController::InitializeFromCamera(
    Camera* camera
) {
    if (!camera) {
        return;
    }

    const Vector3& rotate =
        camera->GetRotate();

    yaw_ = rotate.y;
    pitch_ = rotate.x;

    Vector3 right;
    Vector3 up;
    Vector3 forward;

    CalculateCameraAxes(
        right,
        up,
        forward
    );

    const Vector3& position =
        camera->GetTranslate();

    distance_ = 10.0f;
    targetDistance_ = distance_;

    focusPoint_ = {
        position.x + forward.x * distance_,
        position.y + forward.y * distance_,
        position.z + forward.z * distance_
    };

    initialized_ = true;
}

void CameraDebugController::CalculateCameraAxes(
    Vector3& right,
    Vector3& up,
    Vector3& forward
) const {
    const Vector3 scale = {
        1.0f,
        1.0f,
        1.0f
    };

    const Vector3 rotate = {
        pitch_,
        yaw_,
        0.0f
    };

    const Vector3 translate = {
        0.0f,
        0.0f,
        0.0f
    };

    const Matrix4x4 rotationMatrix =
        Math::MakeAffineMatrix(
            scale,
            rotate,
            translate
        );

    right = {
        rotationMatrix.m[0][0],
        rotationMatrix.m[0][1],
        rotationMatrix.m[0][2]
    };

    up = {
        rotationMatrix.m[1][0],
        rotationMatrix.m[1][1],
        rotationMatrix.m[1][2]
    };

    forward = {
        rotationMatrix.m[2][0],
        rotationMatrix.m[2][1],
        rotationMatrix.m[2][2]
    };

    right = Math::Normalize(right);
    up = Math::Normalize(up);
    forward = Math::Normalize(forward);
}


void CameraDebugController::Update(
    Camera* camera,
    Input* input,
    bool allowMouseOperation
) {
    if (!camera || !input) {
        return;
    }

    if (!initialized_) {
        InitializeFromCamera(camera);
    }

    constexpr float deltaTime = 1.0f / 60.0f;
    const float mouseDeltaX = static_cast<float>(input->GetMouseDeltaX());
    const float mouseDeltaY = static_cast<float>(input->GetMouseDeltaY());

    // =========================================
    // Orbit
    // Alt + 左ドラッグ、または右ドラッグ
    // =========================================

    const bool orbitWithLeft =
        (input->PushKey(DIK_LALT) || input->PushKey(DIK_RALT)) &&
        input->PushMouseButton(0);

    const bool orbitWithRight =
        input->PushMouseButton(1);

    if (allowMouseOperation && (orbitWithLeft || orbitWithRight)) {
        yaw_ +=
            mouseDeltaX *
            rotateSensitivity_;

        pitch_ +=
            mouseDeltaY *
            rotateSensitivity_;

        pitch_ = std::clamp(
            pitch_,
            -kPitchLimit,
            kPitchLimit
        );
    }

    Vector3 right;
    Vector3 up;
    Vector3 forward;

    CalculateCameraAxes(
        right,
        up,
        forward
    );

    // =========================================
    // Pan
    // 中ドラッグ
    // =========================================

    if (allowMouseOperation && input->PushMouseButton(2)) {
        // 遠いほど大きく移動する
        const float panSpeed =
            (std::max)(
                distance_,
                1.0f
            ) * panSensitivity_;

        const float horizontal =
            mouseDeltaX *
            panSpeed;

        const float vertical =
            -mouseDeltaY *
            panSpeed;

        focusPoint_.x +=
            right.x * horizontal +
            up.x * vertical;

        focusPoint_.y +=
            right.y * horizontal +
            up.y * vertical;

        focusPoint_.z +=
            right.z * horizontal +
            up.z * vertical;
    }

    // =========================================
    // Smooth Zoom
    // =========================================

    // StageEditorのメニュー上ではホイールをImGuiだけに渡し、
    // ゲームビュー上にカーソルがある時だけステージをズームする。
    const float mouseWheel = allowMouseOperation
        ? static_cast<float>(input->GetMouseWheelDelta()) / WHEEL_DELTA
        : 0.0f;
    if (mouseWheel != 0.0f) {
        // 距離に対して割合でズームする
        // 遠距離では大きく、近距離では細かく動く
        targetDistance_ *=
            std::exp(
                -mouseWheel *
                zoomSensitivity_
            );

        targetDistance_ = std::clamp(
            targetDistance_,
            minimumDistance_,
            maximumDistance_
        );
    }

    // フレームレートに依存しにくい補間
    const float zoomBlend =
        1.0f -
        std::exp(
            -zoomSmoothness_ *
            deltaTime
        );

    distance_ +=
        (targetDistance_ - distance_) *
        zoomBlend;

    distance_ = std::clamp(
        distance_,
        minimumDistance_,
        maximumDistance_
    );

    // =========================================
    // Cameraへ反映
    // =========================================

    Vector3 cameraPosition = {
        focusPoint_.x -
            forward.x * distance_,

        focusPoint_.y -
            forward.y * distance_,

        focusPoint_.z -
            forward.z * distance_
    };

    camera->SetTranslate(
        cameraPosition
    );

    camera->SetRotate({
        pitch_,
        yaw_,
        0.0f
        });
}
