// ============================================================================
// ファイルの役割: パーティクルの生成、更新、描画、GPUリソースを管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "CylinderParticleSystem.h"

#include <algorithm>
#include <cmath>

#include "EffectMath.h"

void CylinderParticleSystem::Emit(
    const Vector3& position
) {
    if (particles_.size() >= kMaxParticles) {
        return;
    }

    Particle particle{};

    // 発生時の設定を保存
    particle.settings =
        settings_;

    particle.startPosition = {
        position.x +
            settings_.positionOffset.x,

        position.y +
            settings_.positionOffset.y,

        position.z +
            settings_.positionOffset.z
    };

    particle.transform.translate =
        particle.startPosition;

    particle.transform.scale = {
        settings_.radius,
        settings_.startHeight,
        settings_.radius
    };

    particle.transform.rotate = {
        0.0f, 0.0f, 0.0f
    };

    particle.color =
        settings_.color;

    particle.lifeTime = 0.0f;

    particle.maxTime =
        std::max(
            settings_.lifeTime,
            0.001f
        );

    particle.progress = 0.0f;
    particle.twistPhase = 0.0f;
    particle.noisePhase = 0.0f;

    particles_.push_back(particle);
}

void CylinderParticleSystem::Update()
{
    constexpr float deltaTime =
        1.0f / 60.0f;

    for (
        auto iterator = particles_.begin();
        iterator != particles_.end();
        ) {
        Particle& particle =
            *iterator;

        particle.lifeTime +=
            deltaTime;

        if (
            particle.lifeTime >=
            particle.maxTime
            ) {
            iterator =
                particles_.erase(iterator);

            continue;
        }

        const Settings& settings =
            particle.settings;

        const float time =
            std::clamp(
                particle.lifeTime /
                particle.maxTime,
                0.0f,
                1.0f
            );

        particle.progress =
            time;

        const float easedTime =
            EffectMath::EaseOut(
                time,
                settings.easePower
            );

        const float radius =
            settings.radius +
            (
                settings.endRadius -
                settings.radius
                ) *
            easedTime;

        const float height =
            settings.startHeight +
            (
                settings.endHeight -
                settings.startHeight
                ) *
            easedTime;

        particle.transform.scale = {
            radius,
            height,
            radius
        };

        particle.transform.translate =
            particle.startPosition;

        particle.transform.translate.y +=
            settings.riseDistance *
            easedTime;

        particle.twistPhase +=
            settings.twistSpeed *
            deltaTime;

        particle.noisePhase +=
            settings.noiseSpeed *
            deltaTime;

        const float fadeIn =
            EffectMath::FadeIn(
                time,
                settings.fadeInRatio
            );

        const float fadeOut =
            EffectMath::FadeOut(
                time,
                settings.fadePower
            );

        particle.color =
            EffectMath::MakeFadedColor(
                settings.color,
                settings.endColor,
                time,
                settings.intensity,
                fadeIn,
                fadeOut
            );

        ++iterator;
    }
}
