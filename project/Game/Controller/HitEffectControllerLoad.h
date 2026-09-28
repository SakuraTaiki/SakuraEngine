// ============================================================================
// ファイルの役割: ヒットエフェクトのプリセット、保存・読込、実行時再生を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
bool HitEffectController::LoadPreset(
    const std::string& presetName
) {
    namespace fs = std::filesystem;

    const std::string safeName =
        MakeSafePresetName(presetName);

    const fs::path filePath =
        fs::path("Resources/Settings/HitEffects") /
        (safeName + ".txt");

    std::ifstream file(filePath);

    if (!file.is_open()) {
        message_ = "Preset not found : " + safeName;
        return false;
    }

    type_ = Type::Fire;

    if (primitive_) {
        primitive_->GetSettings() = Primitive::Settings{};
    }

    if (ring_) {
        ring_->GetSettings() = Ring::Settings{};
    }

    if (cylinder_) {
        cylinder_->GetSettings() = Cylinder::Settings{};
    }

    if (particleManager_) {
        particleManager_->GetSakuraSettings() =
            ParticleManager::SakuraSettings{};
    }

    const auto readVector3 =
        [&](Vector3& value) {
        file >> value.x >> value.y >> value.z;
        };

    const auto readVector4 =
        [&](Vector4& value) {
        file >> value.x >> value.y >> value.z >> value.w;
        };

    int presetVersion = 1;
    std::string key;

    while (file >> key) {
        if (key == "PresetVersion") {
            file >> presetVersion;
        } else if (key == "EffectPosition") {
            readVector3(position_);
        } else if (key == "EffectSize") {
            file >> size_;
        } else if (key == "HitEffectType") {
            int effectType = 0;
            file >> effectType;

            type_ =
                effectType == static_cast<int>(Type::Sakura)
                ? Type::Sakura
                : Type::Fire;
        } else if (key == "EnablePrimitive") {
            file >> enablePrimitive_;
        } else if (key == "EnableRing") {
            file >> enableRing_;
        } else if (key == "EnableCylinder") {
            file >> enableCylinder_;
        } else if (key == "PrimitiveColor" && primitive_) {
            readVector4(primitive_->GetSettings().color);
        } else if (key == "PrimitiveEndColor" && primitive_) {
            readVector4(primitive_->GetSettings().endColor);
        } else if (key == "PrimitiveCount" && primitive_) {
            file >> primitive_->GetSettings().count;
        } else if (key == "PrimitiveIntensity" && primitive_) {
            file >> primitive_->GetSettings().intensity;
        } else if (key == "PrimitiveWidth" && primitive_) {
            file >> primitive_->GetSettings().width;
        } else if (key == "PrimitiveWidthRandomness" && primitive_) {
            file >> primitive_->GetSettings().widthRandomness;
        } else if (key == "PrimitiveLength" && primitive_) {
            Primitive::Settings& settings =
                primitive_->GetSettings();
            file >> settings.minLength >> settings.maxLength;
        } else if (key == "PrimitiveLifeTime" && primitive_) {
            Primitive::Settings& settings =
                primitive_->GetSettings();
            file >> settings.minLifeTime >> settings.maxLifeTime;
        } else if (key == "PrimitiveMoveSpeed" && primitive_) {
            file >> primitive_->GetSettings().moveSpeed;
        } else if (key == "PrimitiveMoveSpeedRandomness" && primitive_) {
            file >> primitive_->GetSettings().moveSpeedRandomness;
        } else if (key == "PrimitiveRotationSpeed" && primitive_) {
            file >> primitive_->GetSettings().rotationSpeed;
        } else if (key == "PrimitiveRotationSpeedRandomness" && primitive_) {
            file >> primitive_->GetSettings().rotationSpeedRandomness;
        } else if (key == "PrimitiveDirection" && primitive_) {
            Primitive::Settings& settings =
                primitive_->GetSettings();
            file >> settings.directionAngle >> settings.directionSpread;
        } else if (key == "PrimitiveSpawnRadius" && primitive_) {
            file >> primitive_->GetSettings().spawnRadius;
        } else if (key == "PrimitiveAcceleration" && primitive_) {
            readVector3(primitive_->GetSettings().acceleration);
        } else if (key == "PrimitiveEndScale" && primitive_) {
            Primitive::Settings& settings =
                primitive_->GetSettings();
            file >> settings.endWidthScale >> settings.endLengthScale;
        } else if (key == "PrimitiveScaleEasePower" && primitive_) {
            file >> primitive_->GetSettings().scaleEasePower;
        } else if (key == "PrimitiveFadeInRatio" && primitive_) {
            file >> primitive_->GetSettings().fadeInRatio;
        } else if (key == "PrimitiveFadePower" && primitive_) {
            file >> primitive_->GetSettings().fadePower;
        } else if (key == "RingColor" && ring_) {
            readVector4(ring_->GetSettings().color);
        } else if (key == "RingEndColor" && ring_) {
            readVector4(ring_->GetSettings().endColor);
        } else if (key == "RingIntensity" && ring_) {
            file >> ring_->GetSettings().intensity;
        } else if (key == "RingScale" && ring_) {
            Ring::Settings& settings =
                ring_->GetSettings();
            file >> settings.startScale >> settings.endScale;
        } else if (key == "RingThickness" && ring_) {
            file >> ring_->GetSettings().thickness;
        } else if (key == "RingEndThickness" && ring_) {
            file >> ring_->GetSettings().endThickness;
        } else if (key == "RingLifeTime" && ring_) {
            file >> ring_->GetSettings().lifeTime;
        } else if (key == "RingFadeInRatio" && ring_) {
            file >> ring_->GetSettings().fadeInRatio;
        } else if (key == "RingEasePower" && ring_) {
            file >> ring_->GetSettings().easePower;
        } else if (key == "RingRotationSpeed" && ring_) {
            file >> ring_->GetSettings().rotationSpeed;
        } else if (key == "RingEdgeSoftness" && ring_) {
            file >> ring_->GetSettings().edgeSoftness;
        } else if (key == "RingGlowStrength" && ring_) {
            file >> ring_->GetSettings().glowStrength;
        } else if (key == "RingDistortion" && ring_) {
            Ring::Settings& settings =
                ring_->GetSettings();
            file
                >> settings.distortionStrength
                >> settings.distortionFrequency
                >> settings.distortionSpeed;
        } else if (key == "CylinderColor" && cylinder_) {
            readVector4(cylinder_->GetSettings().color);
        } else if (key == "CylinderEndColor" && cylinder_) {
            readVector4(cylinder_->GetSettings().endColor);
        } else if (key == "CylinderIntensity" && cylinder_) {
            file >> cylinder_->GetSettings().intensity;
        } else if (key == "CylinderRadius" && cylinder_) {
            file >> cylinder_->GetSettings().radius;
        } else if (key == "CylinderEndRadius" && cylinder_) {
            file >> cylinder_->GetSettings().endRadius;
        } else if (key == "CylinderHeight" && cylinder_) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();
            file >> settings.startHeight >> settings.endHeight;
        } else if (key == "CylinderRadiusScale" && cylinder_) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();
            file >> settings.bottomRadiusScale >> settings.topRadiusScale;
        } else if (key == "CylinderLifeTime" && cylinder_) {
            file >> cylinder_->GetSettings().lifeTime;
        } else if (key == "CylinderRiseDistance" && cylinder_) {
            file >> cylinder_->GetSettings().riseDistance;
        } else if (key == "CylinderEasePower" && cylinder_) {
            file >> cylinder_->GetSettings().easePower;
        } else if (key == "CylinderFadeInRatio" && cylinder_) {
            file >> cylinder_->GetSettings().fadeInRatio;
        } else if (key == "CylinderFadePower" && cylinder_) {
            file >> cylinder_->GetSettings().fadePower;
        } else if (key == "CylinderTwist" && cylinder_) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();
            file >> settings.twistAmount >> settings.twistSpeed;
        } else if (key == "CylinderNoise" && cylinder_) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();
            file
                >> settings.noiseStrength
                >> settings.noiseFrequency
                >> settings.noiseSpeed;
        } else if (key == "CylinderVerticalFade" && cylinder_) {
            Cylinder::Settings& settings =
                cylinder_->GetSettings();
            file >> settings.topFade >> settings.bottomFade;
        } else if (key == "CylinderPositionOffset" && cylinder_) {
            readVector3(cylinder_->GetSettings().positionOffset);
        } else if (key == "SakuraColor" && particleManager_) {
            readVector4(particleManager_->GetSakuraSettings().color);
        } else if (key == "SakuraSubColor" && particleManager_) {
            readVector4(particleManager_->GetSakuraSettings().subColor);
        } else if (key == "SakuraPetalCount" && particleManager_) {
            file >> particleManager_->GetSakuraSettings().petalCount;
        } else if (key == "SakuraSize" && particleManager_) {
            auto& settings =
                particleManager_->GetSakuraSettings();
            file >> settings.minSize >> settings.maxSize;
        } else if (key == "SakuraSpawnRadius" && particleManager_) {
            file >> particleManager_->GetSakuraSettings().spawnRadius;
        } else if (key == "SakuraSpeed" && particleManager_) {
            auto& settings =
                particleManager_->GetSakuraSettings();
            file >> settings.spreadSpeed >> settings.upwardSpeed;
        } else if (key == "SakuraLifeTime" && particleManager_) {
            auto& settings =
                particleManager_->GetSakuraSettings();
            file >> settings.minLifeTime >> settings.maxLifeTime;
        } else if (key == "SakuraGravity" && particleManager_) {
            file >> particleManager_->GetSakuraSettings().gravity;
        } else if (key == "SakuraRotationSpeed" && particleManager_) {
            file >> particleManager_->GetSakuraSettings().rotationSpeed;
        } else if (key == "SakuraFlash" && particleManager_) {
            auto& settings =
                particleManager_->GetSakuraSettings();
            file >> settings.flashSize >> settings.flashLifeTime;
        } else {
            std::string unusedLine;
            std::getline(file, unusedLine);
        }
    }

    size_ =
        std::clamp(
            size_,
            0.1f,
            5.0f
        );

    if (primitive_) {
        Primitive::Settings& settings =
            primitive_->GetSettings();

        settings.count =
            std::clamp(
                settings.count,
                1,
                128
            );
    }

    if (ring_) {
        ring_->SetThickness(
            ring_->GetSettings().thickness
        );
    }

    UpdateActiveFlags();

    message_ =
        "Loaded V" +
        std::to_string(presetVersion) +
        " : " +
        safeName;

    return true;
}

