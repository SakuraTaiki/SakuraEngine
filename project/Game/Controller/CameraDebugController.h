// ============================================================================
// ファイルの役割: デバッグカメラの操作と通常カメラへの切り替えを管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include "MyMath.h"

class Camera;
class Input;

class CameraDebugController {
public:
    // allowMouseOperationがfalseの間は、カメラの回転・移動・ズーム入力を受け付けない。
    // ただし補間中のズームとカメラ座標の反映は継続し、表示が不自然に停止しないようにする。
    void Update(Camera* camera, Input* input, bool allowMouseOperation = true);

private:
    void InitializeFromCamera(Camera* camera);

    void CalculateCameraAxes(
        Vector3& right,
        Vector3& up,
        Vector3& forward
    ) const;


private:

    bool initialized_ = false;

    // カメラが注視する中心
    Vector3 focusPoint_ = {
        0.0f,
        0.0f,
        0.0f
    };

    // 回転
    float yaw_ = 0.0f;
    float pitch_ = 0.3f;

    // 現在距離と目標距離
    float distance_ = 10.0f;
    float targetDistance_ = 10.0f;

    // 操作感度
    float rotateSensitivity_ = 0.005f;
    float panSensitivity_ = 0.002f;
    float zoomSensitivity_ = 0.15f;

    // 大きいほど素早く目標距離へ近づく
    float zoomSmoothness_ = 12.0f;

    float minimumDistance_ = 0.1f;
    float maximumDistance_ = 500.0f;
};
