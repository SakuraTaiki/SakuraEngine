// ============================================================================
// ファイルの役割: ヒットエフェクトのプリセット、保存・読込、実行時再生を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
bool HitEffectController::SavePreset(
    const std::string& presetName
) {
    namespace fs = std::filesystem;

    const std::string safeName =
        MakeSafePresetName(presetName);

    const fs::path directory =
        "Resources/Settings/HitEffects";

    fs::create_directories(directory);

    const fs::path filePath =
        directory / (safeName + ".txt");

    std::ofstream file(filePath);

    if (!file.is_open()) {
        message_ = "Preset save failed";
        return false;
    }

    const auto writeVector3 =
        [&](const char* name, const Vector3& value) {
        file << name << " "
            << value.x << " "
            << value.y << " "
            << value.z << "\n";
        };

    const auto writeVector4 =
        [&](const char* name, const Vector4& value) {
        file << name << " "
            << value.x << " "
            << value.y << " "
            << value.z << " "
            << value.w << "\n";
        };

    file << "PresetVersion 2\n";
    writeVector3("EffectPosition", position_);
    file << "EffectSize " << size_ << "\n";
    file << "HitEffectType " << static_cast<int>(type_) << "\n";
    file << "EnablePrimitive " << enablePrimitive_ << "\n";
    file << "EnableRing " << enableRing_ << "\n";
    file << "EnableCylinder " << enableCylinder_ << "\n";

    if (primitive_) {
        const Primitive::Settings& settings =
            primitive_->GetSettings();

        writeVector4("PrimitiveColor", settings.color);
        writeVector4("PrimitiveEndColor", settings.endColor);
        file << "PrimitiveCount " << settings.count << "\n";
        file << "PrimitiveIntensity " << settings.intensity << "\n";
        file << "PrimitiveWidth " << settings.width << "\n";
        file << "PrimitiveWidthRandomness " << settings.widthRandomness << "\n";
        file << "PrimitiveLength " << settings.minLength << " " << settings.maxLength << "\n";
        file << "PrimitiveLifeTime " << settings.minLifeTime << " " << settings.maxLifeTime << "\n";
        file << "PrimitiveMoveSpeed " << settings.moveSpeed << "\n";
        file << "PrimitiveMoveSpeedRandomness " << settings.moveSpeedRandomness << "\n";
        file << "PrimitiveRotationSpeed " << settings.rotationSpeed << "\n";
        file << "PrimitiveRotationSpeedRandomness " << settings.rotationSpeedRandomness << "\n";
        file << "PrimitiveDirection " << settings.directionAngle << " " << settings.directionSpread << "\n";
        file << "PrimitiveSpawnRadius " << settings.spawnRadius << "\n";
        writeVector3("PrimitiveAcceleration", settings.acceleration);
        file << "PrimitiveEndScale " << settings.endWidthScale << " " << settings.endLengthScale << "\n";
        file << "PrimitiveScaleEasePower " << settings.scaleEasePower << "\n";
        file << "PrimitiveFadeInRatio " << settings.fadeInRatio << "\n";
        file << "PrimitiveFadePower " << settings.fadePower << "\n";
    }

    if (ring_) {
        const Ring::Settings& settings =
            ring_->GetSettings();

        writeVector4("RingColor", settings.color);
        writeVector4("RingEndColor", settings.endColor);
        file << "RingIntensity " << settings.intensity << "\n";
        file << "RingScale " << settings.startScale << " " << settings.endScale << "\n";
        file << "RingThickness " << settings.thickness << "\n";
        file << "RingEndThickness " << settings.endThickness << "\n";
        file << "RingLifeTime " << settings.lifeTime << "\n";
        file << "RingFadeInRatio " << settings.fadeInRatio << "\n";
        file << "RingEasePower " << settings.easePower << "\n";
        file << "RingRotationSpeed " << settings.rotationSpeed << "\n";
        file << "RingEdgeSoftness " << settings.edgeSoftness << "\n";
        file << "RingGlowStrength " << settings.glowStrength << "\n";
        file << "RingDistortion "
            << settings.distortionStrength << " "
            << settings.distortionFrequency << " "
            << settings.distortionSpeed << "\n";
    }

    if (cylinder_) {
        const Cylinder::Settings& settings =
            cylinder_->GetSettings();

        writeVector4("CylinderColor", settings.color);
        writeVector4("CylinderEndColor", settings.endColor);
        file << "CylinderIntensity " << settings.intensity << "\n";
        file << "CylinderRadius " << settings.radius << "\n";
        file << "CylinderEndRadius " << settings.endRadius << "\n";
        file << "CylinderHeight " << settings.startHeight << " " << settings.endHeight << "\n";
        file << "CylinderRadiusScale " << settings.bottomRadiusScale << " " << settings.topRadiusScale << "\n";
        file << "CylinderLifeTime " << settings.lifeTime << "\n";
        file << "CylinderRiseDistance " << settings.riseDistance << "\n";
        file << "CylinderEasePower " << settings.easePower << "\n";
        file << "CylinderFadeInRatio " << settings.fadeInRatio << "\n";
        file << "CylinderFadePower " << settings.fadePower << "\n";
        file << "CylinderTwist " << settings.twistAmount << " " << settings.twistSpeed << "\n";
        file << "CylinderNoise "
            << settings.noiseStrength << " "
            << settings.noiseFrequency << " "
            << settings.noiseSpeed << "\n";
        file << "CylinderVerticalFade " << settings.topFade << " " << settings.bottomFade << "\n";
        writeVector3("CylinderPositionOffset", settings.positionOffset);
    }

    if (particleManager_) {
        const ParticleManager::SakuraSettings& settings =
            particleManager_->GetSakuraSettings();

        writeVector4("SakuraColor", settings.color);
        writeVector4("SakuraSubColor", settings.subColor);
        file << "SakuraPetalCount " << settings.petalCount << "\n";
        file << "SakuraSize " << settings.minSize << " " << settings.maxSize << "\n";
        file << "SakuraSpawnRadius " << settings.spawnRadius << "\n";
        file << "SakuraSpeed " << settings.spreadSpeed << " " << settings.upwardSpeed << "\n";
        file << "SakuraLifeTime " << settings.minLifeTime << " " << settings.maxLifeTime << "\n";
        file << "SakuraGravity " << settings.gravity << "\n";
        file << "SakuraRotationSpeed " << settings.rotationSpeed << "\n";
        file << "SakuraFlash " << settings.flashSize << " " << settings.flashLifeTime << "\n";
    }

    message_ = "Saved : " + safeName;
    RefreshPresetList();

    return true;
}


