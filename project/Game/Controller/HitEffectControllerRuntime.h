// ============================================================================
// ファイルの役割: ヒットエフェクトのプリセット、保存・読込、実行時再生を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void HitEffectController::Initialize(
    Primitive* primitive,
    Ring* ring,
    Cylinder* cylinder,
    ParticleManager* particleManager,
    GPUParticleManager* gpuParticleManager
) {
    primitive_ = primitive;
    ring_ = ring;
    cylinder_ = cylinder;
    particleManager_ = particleManager;
    gpuParticleManager_ = gpuParticleManager;
}

std::string HitEffectController::MakeSafePresetName(
    const std::string& name
) const {
    std::string safeName;

    for (unsigned char character : name) {
        if (
            std::isalnum(character) ||
            character == '_' ||
            character == '-'
            ) {
            safeName += static_cast<char>(character);
        }
    }

    if (safeName.empty()) {
        safeName = "NewPreset";
    }

    return safeName;
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void HitEffectController::UpdateActiveFlags() {
    if (primitive_) {
        primitive_->SetIsActive(enablePrimitive_);
    }

    if (ring_) {
        ring_->SetIsActive(enableRing_);
    }

    if (cylinder_) {
        cylinder_->SetIsActive(enableCylinder_);
    }
}

// 処理概要: HitEffectControllerが担当する「Emit」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void HitEffectController::Emit(const Vector3& position) {
    const float effectSize =
        (std::max)(size_, 0.01f);

    const auto emitComponents = [&]() {
        if (enablePrimitive_ && primitive_) {
            Primitive::Settings& settings =
                primitive_->GetSettings();

            const float originalWidth = settings.width;
            const float originalMinLength = settings.minLength;
            const float originalMaxLength = settings.maxLength;
            const float originalMoveSpeed = settings.moveSpeed;

            settings.width = originalWidth * effectSize;
            settings.minLength = originalMinLength * effectSize;
            settings.maxLength = originalMaxLength * effectSize;
            settings.moveSpeed = originalMoveSpeed * effectSize;

            primitive_->Emit(position);

            settings.width = originalWidth;
            settings.minLength = originalMinLength;
            settings.maxLength = originalMaxLength;
            settings.moveSpeed = originalMoveSpeed;
        }

        if (enableRing_ && ring_) {
            Ring::Settings& settings =
                ring_->GetSettings();

            const float originalStartScale = settings.startScale;
            const float originalEndScale = settings.endScale;

            settings.startScale = originalStartScale * effectSize;
            settings.endScale = originalEndScale * effectSize;

            ring_->Emit(position);

            settings.startScale = originalStartScale;
            settings.endScale = originalEndScale;
        }

        if (enableCylinder_ && cylinder_) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();

            const float originalRadius = settings.radius;
            const float originalStartHeight = settings.startHeight;
            const float originalEndHeight = settings.endHeight;
            const float originalRiseDistance = settings.riseDistance;
            const Vector3 originalPositionOffset = settings.positionOffset;

            settings.radius = originalRadius * effectSize;
            settings.startHeight = originalStartHeight * effectSize;
            settings.endHeight = originalEndHeight * effectSize;
            settings.riseDistance = originalRiseDistance * effectSize;
            settings.positionOffset = {
                originalPositionOffset.x * effectSize,
                originalPositionOffset.y * effectSize,
                originalPositionOffset.z * effectSize
            };

            cylinder_->Emit(position);

            settings.radius = originalRadius;
            settings.startHeight = originalStartHeight;
            settings.endHeight = originalEndHeight;
            settings.riseDistance = originalRiseDistance;
            settings.positionOffset = originalPositionOffset;
        }
        };

    
    if (type_ == Type::Sakura)
    {
        const bool useGPUParticle =
            gpuParticleManager_ &&
            gpuParticleManager_->GetSettings().enabled;

        if (useGPUParticle)
        {
            gpuParticleManager_->EmitSakura(
                position,
                static_cast<uint32_t>(
                    gpuParticleManager_
                    ->GetSettings()
                    .sakuraCount
                    ),
                effectSize
            );
        } else if (particleManager_)
        {
            
            particleManager_->EmitSakura(
                position,
                effectSize
            );
        }

        emitComponents();
        return;
    }

    const bool useGPUParticle =
        gpuParticleManager_ &&
        gpuParticleManager_->GetSettings().enabled;

    if (useGPUParticle)
    {
        gpuParticleManager_->Emit(
            position,
            static_cast<uint32_t>(
                gpuParticleManager_
                ->GetSettings()
                .fireCount
                ),
            effectSize
        );
    } else if (particleManager_)
    {
        
        particleManager_->Emit(
            position,
            28
        );
    }

    emitComponents();
}

// 処理概要: 指定された設定や計算結果を現在の実行状態へ反映する。
// 注意事項: 関連するキャッシュやGPU定数も必要に応じて更新する。
void HitEffectController::ApplyFirePreset() {
    if (primitive_) {
        Primitive::Settings& settings =
            primitive_->GetSettings();

        settings = Primitive::Settings{};

        settings.color = {
            1.0f,
            0.32f,
            0.02f,
            1.0f
        };

        settings.count = 18;
        settings.intensity = 1.8f;
        settings.width = 0.035f;
        settings.minLength = 0.3f;
        settings.maxLength = 1.15f;
        settings.minLifeTime = 0.18f;
        settings.maxLifeTime = 0.42f;
        settings.moveSpeed = 1.2f;
        settings.rotationSpeed = 0.0f;
        settings.endWidthScale = 0.1f;
        settings.endLengthScale = 0.45f;
        settings.fadePower = 2.2f;
    }

    if (ring_) {
        Ring::Settings& settings =
            ring_->GetSettings();

        settings = Ring::Settings{};

        settings.color = {
            1.0f,
            0.22f,
            0.01f,
            0.8f
        };

        settings.intensity = 1.8f;
        settings.startScale = 0.25f;
        settings.endScale = 1.35f;
        settings.lifeTime = 0.32f;
        settings.fadeInRatio = 0.05f;
        settings.easePower = 4.0f;
        settings.rotationSpeed = 0.0f;

        ring_->SetThickness(0.12f);
    }

    if (cylinder_) {
        Cylinder::Settings& settings =
            cylinder_->GetSettings();

        settings = Cylinder::Settings{};

        settings.color = {
            1.0f,
            0.18f,
            0.01f,
            0.38f
        };

        settings.intensity = 1.5f;
        settings.radius = 0.48f;
        settings.startHeight = 0.05f;
        settings.endHeight = 1.8f;
        settings.lifeTime = 0.55f;
        settings.riseDistance = 0.45f;
        settings.easePower = 3.0f;
        settings.fadePower = 2.0f;
        settings.positionOffset = {
            0.0f,
            -0.15f,
            0.0f
        };
    }
}

