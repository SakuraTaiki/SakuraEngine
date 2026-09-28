// ============================================================================
// ファイルの役割: Compute Shaderを利用したGPUパーティクルの生成・更新・描画を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void GPUParticleManager::Emit(
    const Vector3& position,
    uint32_t count,
    float sizeMultiplier
) {
    if (!settings_.enabled)
    {
        return;
    }

    sizeMultiplier =
        (std::max)(sizeMultiplier, 0.01f);

    const int emitCount =
        std::clamp(
            settings_.fireCount,
            1,
            static_cast<int>(kMaxParticles)
        );

    emitterData_->translate = position;

    emitterData_->radius =
        settings_.spawnRadius *
        sizeMultiplier;

    emitterData_->count =
        static_cast<uint32_t>(emitCount);

    emitterData_->frequency = 0.5f;
    emitterData_->frequencyTime = 0.0f;
    emitterData_->emit = 1;
    emitterData_->effectType = 0.0f;

    emitterData_->sizeMultiplier =
        sizeMultiplier *
        (std::max)(settings_.particleScale, 0.01f);

    emitterData_->mainColor =
        settings_.fireMainColor;

    emitterData_->subColor =
        settings_.fireSubColor;

    emitRequested_ = true;

    // 既存APIとの互換性のため引数は残す
    (void)count;
}

void GPUParticleManager::EmitSakura(
    const Vector3& position,
    uint32_t count,
    float sizeMultiplier
) {
    if (!settings_.enabled)
    {
        return;
    }

    sizeMultiplier =
        (std::max)(sizeMultiplier, 0.01f);

    const int emitCount =
        std::clamp(
            settings_.sakuraCount,
            1,
            static_cast<int>(kMaxParticles)
        );

    emitterData_->translate = position;

    emitterData_->radius =
        settings_.spawnRadius *
        sizeMultiplier;

    emitterData_->count =
        static_cast<uint32_t>(emitCount);

    emitterData_->frequency = 0.5f;
    emitterData_->frequencyTime = 0.0f;
    emitterData_->emit = 1;
    emitterData_->effectType = 1.0f;

    emitterData_->sizeMultiplier =
        sizeMultiplier *
        (std::max)(settings_.particleScale, 0.01f);

    emitterData_->mainColor =
        settings_.sakuraMainColor;

    emitterData_->subColor =
        settings_.sakuraSubColor;

    emitRequested_ = true;

    (void)count;
}

