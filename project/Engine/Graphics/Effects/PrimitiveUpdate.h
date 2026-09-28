// ============================================================================
// ファイルの役割: デバッグ・演出用プリミティブの生成、更新、描画を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void Primitive::Update(
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projectionMatrix
) {
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

        particle.velocity.x +=
            settings.acceleration.x *
            deltaTime;

        particle.velocity.y +=
            settings.acceleration.y *
            deltaTime;

        particle.velocity.z +=
            settings.acceleration.z *
            deltaTime;

       
        particle.transform.translate.x +=
            particle.velocity.x *
            deltaTime;

        particle.transform.translate.y +=
            particle.velocity.y *
            deltaTime;

        particle.transform.translate.z +=
            particle.velocity.z *
            deltaTime;

        particle.transform.rotate.z +=
            particle.angularVelocity *
            deltaTime;

        const float time =
            std::clamp(
                particle.lifeTime /
                particle.maxTime,
                0.0f,
                1.0f
            );

        const float scaleTime =
            EffectMath::EaseOut(
                time,
                settings.scaleEasePower
            );

        particle.transform.scale.x =
            particle.initialScale.x *
            (
                1.0f +
                (
                    settings.endWidthScale -
                    1.0f
                    ) *
                scaleTime
                );

        particle.transform.scale.y =
            particle.initialScale.y *
            (
                1.0f +
                (
                    settings.endLengthScale -
                    1.0f
                    ) *
                scaleTime
                );

       
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

    uint32_t index = 0;

    Matrix4x4 billboardMatrix =
        Math::MakeBillboardMatrix(viewMatrix);

    const Matrix4x4 viewProjectionMatrix =
        Math::Multiply(
            viewMatrix,
            projectionMatrix
        );

    for (const Particle& particle : particles_) {
        if (index >= kMaxParticles) {
            break;
        }

        const Matrix4x4 scaleMatrix =
            Math::Matrix4x4MakeScaleMatrix(
                particle.transform.scale
            );

        const Matrix4x4 rotateMatrix =
            Math::MakeRotateZMatrix(
                particle.transform.rotate.z
            );

        const Matrix4x4 translateMatrix =
            Math::MakeTranslateMatrix(
                particle.transform.translate
            );

        const Matrix4x4 worldMatrix =
            Math::Multiply(
                Math::Multiply(
                    scaleMatrix,
                    rotateMatrix
                ),
                Math::Multiply(
                    billboardMatrix,
                    translateMatrix
                )
            );

        instancingDataMapped_[index].WVP =
            Math::Multiply(
                worldMatrix,
                viewProjectionMatrix
            );

        instancingDataMapped_[index].color =
            particle.color;

        ++index;
    }
}


