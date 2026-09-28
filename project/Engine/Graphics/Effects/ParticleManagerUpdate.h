// ============================================================================
// ファイルの役割: パーティクルの生成、更新、描画、GPUリソースを管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void ParticleManager::Update(
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projectionMatrix
) {
    constexpr float deltaTime =
        1.0f / 60.0f;

    for (
        auto iterator = particles_.begin();
        iterator != particles_.end();
        ) {
        Particle& particle = *iterator;

        particle.lifeTime += deltaTime;

        if (particle.lifeTime >= particle.maxTime) {
            iterator = particles_.erase(iterator);
            continue;
        }

        particle.velocity.x += particle.acceleration.x;
        particle.velocity.y += particle.acceleration.y;
        particle.velocity.z += particle.acceleration.z;

        particle.transform.translate.x +=
            particle.velocity.x;

        particle.transform.translate.y +=
            particle.velocity.y;

        particle.transform.translate.z +=
            particle.velocity.z;

        const float time =
            particle.lifeTime /
            particle.maxTime;

        if (particle.effectType == 0) {
            particle.transform.scale.x =
                particle.startScale.x *
                (1.0f - time);

            particle.transform.scale.y =
                particle.startScale.y *
                (1.0f - 0.45f * time);

            particle.color.w =
                1.0f - time;
        } else if (particle.effectType == 1) {
            const float scale =
                0.35f + 1.65f * time;

            particle.transform.scale = {
                particle.startScale.x * scale,
                particle.startScale.y * scale,
                1.0f
            };

            particle.color.w =
                (1.0f - time) *
                (1.0f - time);
        } else if (particle.effectType == 2) {
            const float scale =
                0.7f + 0.8f * time;

            particle.transform.scale = {
                particle.startScale.x * scale,
                particle.startScale.y * scale,
                1.0f
            };

            particle.transform.rotate.z +=
                0.015f;

            particle.color.w =
                0.45f * (1.0f - time);
        } else if (particle.effectType == 3) {
            const float pi =
                std::numbers::pi_v<float>;

            const float pulse =
                std::sin(time * pi);

            particle.transform.scale.x =
                particle.startScale.x *
                (0.75f + pulse * 0.45f);

            particle.transform.scale.y =
                particle.startScale.y *
                (0.8f + time * 0.65f);

            particle.transform.rotate.z +=
                0.008f;

            float fadeIn =
                time / 0.1f;

            fadeIn =
                (std::min)(fadeIn, 1.0f);

            const float fadeOut =
                (1.0f - time) *
                (1.0f - time);

            particle.color.w =
                fadeIn * fadeOut;
        } else if (particle.effectType == 4) {
            
            particle.transform.rotate.z +=
                particle.angularVelocity;

            const float flutter =
                0.72f +
                std::sin(
                    time * 18.0f +
                    particle.startScale.x * 31.0f
                ) * 0.28f;

            particle.transform.scale.x =
                particle.startScale.x *
                (std::max)(flutter, 0.15f);

            particle.transform.scale.y =
                particle.startScale.y *
                (0.9f + 0.15f * std::sin(time * 12.0f));

            
            particle.transform.translate.x +=
                std::sin(
                    time * 13.0f +
                    particle.transform.rotate.z
                ) * 0.004f;

            const float fadeIn =
                (std::min)(time / 0.08f, 1.0f);

            const float fadeOut =
                (1.0f - time) *
                (1.0f - time);

            particle.color.w =
                fadeIn * fadeOut;
        } else if (particle.effectType == 5) {
            
            const float pulse =
                std::sin(
                    time *
                    std::numbers::pi_v<float>
                );

            const float scale =
                0.25f + pulse * 1.35f;

            particle.transform.scale = {
                particle.startScale.x * scale,
                particle.startScale.y * scale,
                1.0f
            };

            particle.transform.rotate.z +=
                particle.angularVelocity;

            particle.color.w =
                (1.0f - time) *
                (1.0f - time);
        }

        ++iterator;
    }

    uint32_t index = 0;

    Matrix4x4 billboardMatrix =
        Math::MakeBillboardMatrix(viewMatrix);

    for (const Particle& particle : particles_) {
        if (index >= kMaxParticles) {
            break;
        }

        const Matrix4x4 scaleMatrix =
            Math::Matrix4x4MakeScaleMatrix(
                particle.transform.scale
            );

        const Matrix4x4 rotationMatrix =
            Math::MakeRotateZMatrix(
                particle.transform.rotate.z
            );

        const Matrix4x4 translationMatrix =
            Math::MakeTranslateMatrix(
                particle.transform.translate
            );

        const Matrix4x4 worldMatrix =
            Math::Multiply(
                Math::Multiply(
                    scaleMatrix,
                    rotationMatrix
                ),
                Math::Multiply(
                    billboardMatrix,
                    translationMatrix
                )
            );

        const Matrix4x4 viewProjection =
            Math::Multiply(
                viewMatrix,
                projectionMatrix
            );

        instancingDataMapped_[index].WVP =
            Math::Multiply(
                worldMatrix,
                viewProjection
            );

        instancingDataMapped_[index].color =
            particle.color;

        instancingDataMapped_[index].effectType =
            static_cast<float>(
                particle.effectType
                );

        ++index;
    }
}


