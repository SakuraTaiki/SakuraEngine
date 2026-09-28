// ============================================================================
// ファイルの役割: パーティクルの生成、更新、描画、GPUリソースを管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <cstdint>
#include <list>

#include "MyMath.h"

class CylinderParticleSystem
{
public:
    struct Settings
    {
        Vector4 color = {
            0.4f, 0.7f, 1.0f, 0.6f
        };

        Vector4 endColor = {
            0.1f, 0.3f, 1.0f, 0.0f
        };

        float intensity = 1.0f;

        float radius = 0.8f;
        float endRadius = 1.2f;

        float startHeight = 0.1f;
        float endHeight = 2.5f;

        // 円柱下側・上側の太さ
        float bottomRadiusScale = 1.0f;
        float topRadiusScale = 0.25f;

        float lifeTime = 0.7f;
        float riseDistance = 0.0f;

        float easePower = 1.0f;
        float fadeInRatio = 0.05f;
        float fadePower = 1.0f;

        // 円柱全体のねじれ
        float twistAmount = 0.0f;
        float twistSpeed = 0.0f;

        // シェーダー内の揺らぎ
        float noiseStrength = 0.1f;
        float noiseFrequency = 6.0f;
        float noiseSpeed = 1.0f;

        // 上下の透明化範囲
        float topFade = 0.2f;
        float bottomFade = 0.08f;

        Vector3 positionOffset = {
            0.0f, 0.0f, 0.0f
        };
    };

    struct Particle
    {
        Transform transform;
        Vector3 startPosition;

        Vector4 color;

        float lifeTime = 0.0f;
        float maxTime = 1.0f;

        float progress = 0.0f;
        float twistPhase = 0.0f;
        float noisePhase = 0.0f;

        // 発生時設定を保存する
        Settings settings{};
    };

    Settings& GetSettings()
    {
        return settings_;
    }

    const Settings& GetSettings() const
    {
        return settings_;
    }

    void Emit(const Vector3& position);
    void Update();

    const std::list<Particle>& GetParticles() const
    {
        return particles_;
    }

    bool IsEmpty() const
    {
        return particles_.empty();
    }

private:
    static const uint32_t kMaxParticles = 64;

    std::list<Particle> particles_;
    Settings settings_{};
};