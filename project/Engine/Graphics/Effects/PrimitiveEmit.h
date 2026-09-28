// ============================================================================
// ファイルの役割: デバッグ・演出用プリミティブの生成、更新、描画を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void Primitive::Emit(
    const Vector3& position,
    uint32_t count
) {
    if (!isActive_) {
        return;
    }

    const float minimumLength =
        (std::max)(
            0.001f,
            (std::min)(
                settings_.minLength,
                settings_.maxLength
                )
            );

    const float maximumLength =
        (std::max)(
            minimumLength,
            (std::max)(
                settings_.minLength,
                settings_.maxLength
                )
            );

    const float minimumLifeTime =
        (std::max)(
            0.001f,
            (std::min)(
                settings_.minLifeTime,
                settings_.maxLifeTime
                )
            );

    const float maximumLifeTime =
        (std::max)(
            minimumLifeTime,
            (std::max)(
                settings_.minLifeTime,
                settings_.maxLifeTime
                )
            );

    const float spread =
        std::clamp(
            settings_.directionSpread,
            0.0f,
            std::numbers::pi_v<float>
        );

    std::uniform_real_distribution<float>
        angleDistribution(
            settings_.directionAngle - spread,
            settings_.directionAngle + spread
        );

    std::uniform_real_distribution<float>
        lengthDistribution(
            minimumLength,
            maximumLength
        );

    std::uniform_real_distribution<float>
        lifeTimeDistribution(
            minimumLifeTime,
            maximumLifeTime
        );

    std::uniform_real_distribution<float>
        unitDistribution(
            0.0f,
            1.0f
        );

    std::uniform_real_distribution<float>
        signedDistribution(
            -1.0f,
            1.0f
        );

    for (
        uint32_t index = 0;
        index < count;
        ++index
        ) {
        if (
            particles_.size() >=
            kMaxParticles
            ) {
            return;
        }

        Particle particle{};

        particle.settings =
            settings_;

        const float angle =
            angleDistribution(
                primitiveRandomEngine
            );

        const float widthScale =
            (std::max)(
                0.01f,
                1.0f +
                signedDistribution(
                    primitiveRandomEngine
                ) *
                settings_.widthRandomness
                );

        particle.transform.scale = {
            settings_.width * widthScale,
            lengthDistribution(
                primitiveRandomEngine
            ),
            1.0f
        };

        particle.initialScale =
            particle.transform.scale;

        particle.transform.rotate = {
            0.0f,
            0.0f,
            angle
        };

        const float spawnAngle =
            unitDistribution(
                primitiveRandomEngine
            ) *
            std::numbers::pi_v<float> *
            2.0f;

        const float spawnDistance =
            std::sqrt(
                unitDistribution(
                    primitiveRandomEngine
                )
            ) *
            (std::max)(
                settings_.spawnRadius,
                0.0f
                );

        particle.transform.translate = {
            position.x +
                std::cos(spawnAngle) *
                spawnDistance,

            position.y +
                std::sin(spawnAngle) *
                spawnDistance,

            position.z
        };

        const float speedScale =
            (std::max)(
                0.0f,
                1.0f +
                signedDistribution(
                    primitiveRandomEngine
                ) *
                settings_.moveSpeedRandomness
                );

        const float speed =
            settings_.moveSpeed *
            speedScale;

        particle.velocity = {
            -std::sin(angle) * speed,
            std::cos(angle) * speed,
            0.0f
        };

        particle.angularVelocity =
            settings_.rotationSpeed +
            signedDistribution(
                primitiveRandomEngine
            ) *
            settings_.rotationSpeedRandomness;

        particle.color =
            settings_.color;

        particle.lifeTime = 0.0f;

        particle.maxTime =
            lifeTimeDistribution(
                primitiveRandomEngine
            );

        particles_.push_back(particle);
    }
}


