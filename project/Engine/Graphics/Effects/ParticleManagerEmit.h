// ============================================================================
// ファイルの役割: パーティクルの生成、更新、描画、GPUリソースを管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void ParticleManager::Emit(
    const Vector3& pos,
    uint32_t count
) {
    std::uniform_real_distribution<float> distRotate(
        -std::numbers::pi_v<float>,
        std::numbers::pi_v<float>
    );

    std::uniform_real_distribution<float> distScaleY(
        0.55f,
        1.55f
    );

    std::uniform_real_distribution<float> distSpeed(
        0.035f,
        0.11f
    );

    std::uniform_real_distribution<float> distColor(
        0.75f,
        1.0f
    );

    std::uniform_real_distribution<float> distTime(
        0.22f,
        0.48f
    );

    // 謾ｾ蟆・憾縺ｮ轣ｫ闃ｱ
    for (uint32_t i = 0; i < count; ++i) {
        if (particles_.size() >= kMaxParticles) {
            return;
        }

        Particle particle{};

        particle.transform.scale = {
            0.035f,
            distScaleY(engine),
            1.0f
        };

        particle.startScale = particle.transform.scale;

        particle.transform.rotate = {
            0.0f,
            0.0f,
            distRotate(engine)
        };

        particle.transform.translate = pos;

        const float speed = distSpeed(engine);
        const float rotateZ = particle.transform.rotate.z;

        particle.velocity = {
    -std::sin(rotateZ) *
        speed * 0.55f,

    std::abs(
        std::cos(rotateZ)
    ) * speed + 0.015f,

    0.0f
        };


        const float randomColor =
            distColor(engine);

        particle.color = {
            1.0f,
            0.18f + randomColor * 0.45f,
            0.02f,
            1.0f
        };

        particle.lifeTime = 0.0f;
        particle.maxTime = distTime(engine);
        particle.effectType = 0;

        particles_.push_back(particle);
    }

    
    for (
        uint32_t i = 0;
        i < 3 && particles_.size() < kMaxParticles;
        ++i
        ) {
        Particle particle{};

        const float baseScale =
            0.75f + static_cast<float>(i) * 0.32f;

        particle.transform.scale = {
            baseScale,
            baseScale,
            1.0f
        };

        particle.startScale = particle.transform.scale;

        particle.transform.rotate = {
            0.0f,
            0.0f,
            distRotate(engine)
        };

        particle.transform.translate = pos;
        particle.velocity = { 0.0f, 0.0f, 0.0f };

        if (i == 0) {
            particle.color = {
                1.0f, 1.0f, 0.75f, 1.0f
            };
        } else {
            particle.color = {
                1.0f, 0.35f,0.04f, 0.9f
            };
        }

        particle.lifeTime = 0.0f;
        particle.maxTime =
            0.12f + static_cast<float>(i) * 0.055f;

        particle.effectType = 1;

        particles_.push_back(particle);
    }

    
    if (particles_.size() < kMaxParticles) {
        Particle particle{};

        particle.transform.scale = {
            1.35f,
            1.35f,
            1.0f
        };

        particle.startScale = particle.transform.scale;

        particle.transform.rotate = {
            0.0f,
            0.0f,
            distRotate(engine)
        };

        particle.transform.translate = pos;
        particle.velocity = { 0.0f, 0.012f, 0.0f };

        particle.color = {
            1.0f,
            0.12f,
            0.01f,
            0.5f
        };

        particle.lifeTime = 0.0f;
        particle.maxTime = 0.55f;
        particle.effectType = 2;

        particles_.push_back(particle);
    }

   
    std::uniform_real_distribution<float>
        flamePositionX(
            -0.35f,
            0.35f
        );

    std::uniform_real_distribution<float>
        flamePositionY(
            -0.15f,
            0.20f
        );

    std::uniform_real_distribution<float>
        flameWidth(
            0.20f,
            0.48f
        );

    std::uniform_real_distribution<float>
        flameHeight(
            0.45f,
            1.10f
        );

    std::uniform_real_distribution<float>
        flameVelocityX(
            -0.008f,
            0.008f
        );

    std::uniform_real_distribution<float>
        flameVelocityY(
            0.015f,
            0.042f
        );

    std::uniform_real_distribution<float>
        flameRotation(
            -0.28f,
            0.28f
        );

    std::uniform_real_distribution<float>
        flameLifeTime(
            0.32f,
            0.68f
        );

    const uint32_t flameCount = 24;

    for (
        uint32_t index = 0;
        index < flameCount;
        ++index
        ) {
        if (particles_.size() >=
            kMaxParticles) {
            break;
        }

        Particle particle{};

        particle.transform.scale = {
            flameWidth(engine),
            flameHeight(engine),
            1.0f
        };

        particle.startScale =
            particle.transform.scale;

        particle.transform.rotate = {
            0.0f,
            0.0f,
            flameRotation(engine)
        };

        particle.transform.translate = {
            pos.x + flamePositionX(engine),
            pos.y + flamePositionY(engine),
            pos.z
        };

        particle.velocity = {
            flameVelocityX(engine),
            flameVelocityY(engine),
            0.0f
        };

        switch (index % 3) {
        case 0:
            
            particle.color = {
                1.0f,
                0.92f,
                0.35f,
                1.0f
            };
            break;

        case 1:
            
            particle.color = {
                1.0f,
                0.35f,
                0.025f,
                1.0f
            };
            break;

        default:
           
            particle.color = {
                1.0f,
                0.08f,
                0.005f,
                0.75f
            };
            break;
        }

        particle.lifeTime = 0.0f;

        particle.maxTime =
            flameLifeTime(engine);

        
        particle.effectType = 3;

        particles_.push_back(particle);
    }
    
} 

void ParticleManager::EmitSakura(const Vector3& position, float sizeMultiplier)
{

    sizeMultiplier =
        (std::max)(sizeMultiplier, 0.01f);

    const SakuraSettings& settings =
        sakuraSettings_;

    std::uniform_real_distribution<float>
        random01(0.0f, 1.0f);

    std::uniform_real_distribution<float>
        randomAngle(
            -std::numbers::pi_v<float>,
            std::numbers::pi_v<float>
        );

    std::uniform_real_distribution<float>
        randomSize(
            settings.minSize,
            settings.maxSize
        );

    std::uniform_real_distribution<float>
        randomLifeTime(
            settings.minLifeTime,
            settings.maxLifeTime
        );

    std::uniform_real_distribution<float>
        randomRotationSpeed(
            -settings.rotationSpeed,
            settings.rotationSpeed
        );

    std::uniform_real_distribution<float>
        randomHeight(-0.35f, 0.35f);

    std::uniform_real_distribution<float>
        randomDepth(-0.25f, 0.25f);

    
    if (particles_.size() < kMaxParticles) {
        Particle flash{};

        flash.transform.scale = {
            settings.flashSize * sizeMultiplier,
            settings.flashSize * sizeMultiplier,
            1.0f
        };

        flash.startScale =
            flash.transform.scale;

        flash.transform.rotate = {
            0.0f,
            0.0f,
            randomAngle(engine)
        };

        flash.transform.translate =
            position;

        flash.velocity = {
            0.0f,
            0.0f,
            0.0f
        };

        flash.acceleration = {
            0.0f,
            0.0f,
            0.0f
        };

        flash.color = {
            1.0f,
            0.82f,
            0.92f,
            1.0f
        };

        flash.angularVelocity = 0.025f;
        flash.lifeTime = 0.0f;
        flash.maxTime = settings.flashLifeTime;
        flash.effectType = 5;

        particles_.push_back(flash);
    }

    
    const int petalCount =
        std::clamp(
            settings.petalCount,
            1,
            256
        );

    for (int index = 0; index < petalCount; ++index) {
        if (particles_.size() >= kMaxParticles) {
            break;
        }

        Particle petal{};

        const float angle =
            randomAngle(engine);

        const float radius =
            std::sqrt(random01(engine)) *
            settings.spawnRadius *
            sizeMultiplier;

        const float size =
            randomSize(engine) *
            sizeMultiplier;

        const float speed =
            settings.spreadSpeed *
            (0.55f + random01(engine) * 0.9f) *
            sizeMultiplier;

        petal.transform.scale = {
            size,
            size * (1.25f + random01(engine) * 0.45f),
            1.0f
        };

        petal.startScale =
            petal.transform.scale;

        petal.transform.rotate = {
            0.0f,
            0.0f,
            randomAngle(engine)
        };

        petal.transform.translate = {
            position.x + std::cos(angle) * radius,
            position.y + randomHeight(engine) * sizeMultiplier,
            position.z + randomDepth(engine) * sizeMultiplier
        };

        petal.velocity = {
            std::cos(angle) * speed,
            settings.upwardSpeed *
                (0.35f + random01(engine)),
            std::sin(angle) * speed * 0.35f
        };

        petal.acceleration = {
            0.0f,
            -settings.gravity,
            0.0f
        };

        const float colorBlend =
            random01(engine);

        petal.color = {
            settings.color.x +
                (settings.subColor.x - settings.color.x) *
                colorBlend,

            settings.color.y +
                (settings.subColor.y - settings.color.y) *
                colorBlend,

            settings.color.z +
                (settings.subColor.z - settings.color.z) *
                colorBlend,

            1.0f
        };

        petal.angularVelocity =
            randomRotationSpeed(engine);

        petal.lifeTime = 0.0f;
        petal.maxTime = randomLifeTime(engine);
        petal.effectType = 4;

        particles_.push_back(petal);
    }

}



