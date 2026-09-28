// ============================================================================
// ファイルの役割: ベクトル、行列、クォータニオンなどゲームで使用する数学処理を提供する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once
#include <cmath>

//=======================
// 型定義
//=======================
struct Vector2 {
    float x, y;
};
struct Vector3 {
    float x, y, z;
};
struct Vector4 {
    float x, y, z, w;
};
struct Matrix3x3 {
    float m[3][3];
};
struct Matrix4x4 {
    float m[4][4];
};

struct Transform {
    Vector3 scale;
    Vector3 rotate;
    Vector3 translate;
};

struct Quaternion {
    float x, y, z, w;
};

//=======================
// 数学関数群
//=======================
namespace Math {

    Matrix4x4 MakeIdentity4x4();

    // 拡大縮小行列
    Matrix4x4 Matrix4x4MakeScaleMatrix(const Vector3& s);

    // 回転行列
    Matrix4x4 MakeRotateXMatrix(float radian);
    Matrix4x4 MakeRotateYMatrix(float radian);
    Matrix4x4 MakeRotateZMatrix(float radian);

    // 平行移動行列
    Matrix4x4 MakeTranslateMatrix(const Vector3& translate);

    // 行列の積
    Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);

    // アフィン行列
    Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);

    // 逆行列 (引数を const Matrix4x4& に統一)
    Matrix4x4 Inverse(const Matrix4x4& m);
    Matrix4x4 MakeBillboardMatrix(const Matrix4x4& viewMatrix);

    // 透視投影行列
    Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspectRatio, float nearClip, float farClip);

    // 正射影行列 (Spriteで必要)
    Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);

    // ビューポート行列 (Spriteで必要)
    Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);

    Matrix4x4 Transpose(const Matrix4x4& m);

    // 正規化
    Vector3 Normalize(const Vector3& v);

    Vector3 Lerp(const Vector3& a, const Vector3& b, float t);

    Quaternion Normalize(const Quaternion& q);
    Quaternion Slerp(const Quaternion& q0, const Quaternion& q1, float t);

    Matrix4x4 MakeRotateMatrix(const Quaternion& q);
    Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate);
}
