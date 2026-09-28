// ============================================================================
// ファイルの役割: ゲームビューと各種デバッグ編集ウィンドウをImGui上へ構築する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
#pragma once

#include "HitEffectController.h"
#include "ParticleManager.h"
#include "GPUParticleManager.h"
#include "Ring.h"
#include "Cylinder.h"
#include "Primitive.h"

#include <algorithm>
#include <cstdint>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

namespace SceneDebugPanelDetail {
#ifdef USE_IMGUI
    inline void DrawHdrColorEdit4(const char* label, float* color)
    {
        ImGui::ColorEdit4(
            label,
            color,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );
    }

    // Most effect setting structs reset by assigning Settings{}.
    // Keeping that pattern here removes repeated reset button code.
    template <class Settings>
    inline void DrawResetSettingsButton(const char* label, Settings& settings)
    {
        if (ImGui::Button(label)) {
            settings = Settings{};
        }
    }

    // GPU particle editing includes both UI values and preview emission.
    // Keeping it separate makes DrawEffectTab read as a high-level flow.
    inline void DrawGpuParticleSettings(
        GPUParticleManager& gpuParticleManager,
        HitEffectController& hitEffect
    )
    {
        GPUParticleManager::Settings& settings =
            gpuParticleManager.GetSettings();

        ImGui::SeparatorText("GPU Particle");
        ImGui::Checkbox("Enable GPU Particle", &settings.enabled);

        ImGui::DragInt(
            "GPU Fire Count",
            &settings.fireCount,
            1.0f,
            1,
            static_cast<int>(GPUParticleManager::kMaxParticles)
        );

        ImGui::DragInt(
            "GPU Sakura Count",
            &settings.sakuraCount,
            1.0f,
            1,
            static_cast<int>(GPUParticleManager::kMaxParticles)
        );

        ImGui::DragFloat("GPU Particle Scale", &settings.particleScale, 0.01f, 0.01f, 10.0f, "%.2f");
        ImGui::DragFloat("GPU Spawn Radius", &settings.spawnRadius, 0.01f, 0.0f, 20.0f, "%.2f");

        DrawHdrColorEdit4("GPU Fire Main Color", &settings.fireMainColor.x);
        DrawHdrColorEdit4("GPU Fire Sub Color", &settings.fireSubColor.x);
        DrawHdrColorEdit4("GPU Sakura Main Color", &settings.sakuraMainColor.x);
        DrawHdrColorEdit4("GPU Sakura Sub Color", &settings.sakuraSubColor.x);

        DrawResetSettingsButton("Reset GPU Particle", settings);

        ImGui::SameLine();
        ImGui::Text("GPU Particle : %s", settings.enabled ? "ON" : "OFF");

        ImGui::Text(
            "Emit Position : %.2f, %.2f, %.2f",
            hitEffect.GetPosition().x,
            hitEffect.GetPosition().y,
            hitEffect.GetPosition().z
        );
        ImGui::Text("Fire Count : %d", settings.fireCount);

        if (ImGui::Button("Preview GPU Particle")) {
            // Preview should be visible even when the runtime toggle is currently off.
            settings.enabled = true;

            const Vector3 emitPosition =
                hitEffect.GetPosition();

            const float emitSize =
                (std::max)(hitEffect.GetSize(), 0.01f);

            if (hitEffect.GetType() == HitEffectController::Type::Sakura) {
                settings.sakuraCount =
                    std::clamp(
                        settings.sakuraCount,
                        1,
                        static_cast<int>(GPUParticleManager::kMaxParticles)
                    );

                gpuParticleManager.EmitSakura(
                    emitPosition,
                    static_cast<uint32_t>(settings.sakuraCount),
                    emitSize
                );
            } else {
                settings.fireCount =
                    std::clamp(
                        settings.fireCount,
                        1,
                        static_cast<int>(GPUParticleManager::kMaxParticles)
                    );

                gpuParticleManager.Emit(
                    emitPosition,
                    static_cast<uint32_t>(settings.fireCount),
                    emitSize
                );
            }
        }
    }

    // Sakura has particle-only controls that are not used by the fire effect.
    // Separating the block prevents DrawEffectTab from becoming a giant settings list.
    inline void DrawSakuraSettings(ParticleManager::SakuraSettings& settings)
    {
        ImGui::SeparatorText("Sakura Settings");

        DrawHdrColorEdit4("Sakura Main Color", &settings.color.x);
        DrawHdrColorEdit4("Sakura Sub Color", &settings.subColor.x);

        ImGui::DragInt("Petal Count", &settings.petalCount, 1.0f, 1, 256);
        ImGui::DragFloatRange2("Petal Size", &settings.minSize, &settings.maxSize, 0.005f, 0.01f, 2.0f);
        ImGui::DragFloat("Spawn Radius", &settings.spawnRadius, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Spread Speed", &settings.spreadSpeed, 0.001f, 0.0f, 1.0f);
        ImGui::DragFloat("Upward Speed", &settings.upwardSpeed, 0.001f, -1.0f, 1.0f);
        ImGui::DragFloatRange2("Petal Life Time", &settings.minLifeTime, &settings.maxLifeTime, 0.01f, 0.05f, 10.0f);
        ImGui::DragFloat("Petal Gravity", &settings.gravity, 0.0001f, -0.1f, 0.1f, "%.4f");
        ImGui::DragFloat("Petal Rotation", &settings.rotationSpeed, 0.005f, 0.0f, 1.0f);
        ImGui::DragFloat("Flash Size", &settings.flashSize, 0.01f, 0.01f, 10.0f);
        ImGui::DragFloat("Flash Life Time", &settings.flashLifeTime, 0.01f, 0.05f, 5.0f);

        DrawResetSettingsButton("Reset Sakura Settings", settings);
    }

    // Ring settings are grouped by shape, fade, glow, and distortion controls.
    // This keeps the individual effect editor easy to compare with the others.
    inline void DrawRingSettings(Ring& ring)
    {
        Ring::Settings& settings =
            ring.GetSettings();

        ImGui::SeparatorText("Ring");

        DrawHdrColorEdit4("Ring Start Color", &settings.color.x);
        DrawHdrColorEdit4("Ring End Color", &settings.endColor.x);

        ImGui::DragFloat("Ring Glow Intensity", &settings.intensity, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Ring Start Scale", &settings.startScale, 0.01f, 0.01f, 10.0f);
        ImGui::DragFloat("Ring End Scale", &settings.endScale, 0.01f, 0.01f, 20.0f);
        ImGui::SliderFloat("Ring Start Thickness", &settings.thickness, 0.001f, 0.5f);
        ImGui::SliderFloat("Ring End Thickness", &settings.endThickness, 0.001f, 0.5f);
        ImGui::DragFloat("Ring Life Time", &settings.lifeTime, 0.01f, 0.05f, 5.0f);
        ImGui::SliderFloat("Ring Fade In Ratio", &settings.fadeInRatio, 0.0f, 1.0f);
        ImGui::DragFloat("Ring Ease Power", &settings.easePower, 0.01f, 0.01f, 10.0f);
        ImGui::DragFloat("Ring Rotation Speed", &settings.rotationSpeed, 0.01f, -20.0f, 20.0f);
        ImGui::DragFloat("Ring Edge Softness", &settings.edgeSoftness, 0.001f, 0.001f, 0.25f);
        ImGui::DragFloat("Ring Glow Strength", &settings.glowStrength, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Ring Distortion", &settings.distortionStrength, 0.001f, 0.0f, 0.5f);
        ImGui::DragFloat("Ring Distortion Frequency", &settings.distortionFrequency, 0.1f, 1.0f, 32.0f);
        ImGui::DragFloat("Ring Distortion Speed", &settings.distortionSpeed, 0.01f, -10.0f, 10.0f);

        DrawResetSettingsButton("Reset Ring", settings);
    }

    // Primitive controls are numerous, so they live in their own editor block.
    // DrawEffectTab only decides when this block should be shown.
    inline void DrawPrimitiveSettings(Primitive& primitive)
    {
        Primitive::Settings& settings =
            primitive.GetSettings();

        ImGui::SeparatorText("Primitive");

        DrawHdrColorEdit4("Primitive Start Color", &settings.color.x);
        DrawHdrColorEdit4("Primitive End Color", &settings.endColor.x);

        ImGui::DragInt("Primitive Count", &settings.count, 1.0f, 1, 128);
        ImGui::DragFloat("Primitive Glow", &settings.intensity, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Primitive Width", &settings.width, 0.001f, 0.001f, 2.0f);
        ImGui::SliderFloat("Primitive Width Random", &settings.widthRandomness, 0.0f, 1.0f);
        ImGui::DragFloatRange2("Primitive Length", &settings.minLength, &settings.maxLength, 0.01f, 0.01f, 10.0f);
        ImGui::DragFloatRange2("Primitive Life Time", &settings.minLifeTime, &settings.maxLifeTime, 0.01f, 0.01f, 5.0f);
        ImGui::DragFloat("Primitive Move Speed", &settings.moveSpeed, 0.01f, 0.0f, 20.0f);
        ImGui::SliderFloat("Primitive Speed Random", &settings.moveSpeedRandomness, 0.0f, 1.0f);
        ImGui::DragFloat("Primitive Rotation Speed", &settings.rotationSpeed, 0.01f, -20.0f, 20.0f);
        ImGui::DragFloat("Primitive Rotation Random", &settings.rotationSpeedRandomness, 0.01f, 0.0f, 20.0f);
        ImGui::SliderAngle("Primitive Direction", &settings.directionAngle, -180.0f, 180.0f);
        ImGui::SliderAngle("Primitive Direction Spread", &settings.directionSpread, 0.0f, 180.0f);
        ImGui::DragFloat("Primitive Spawn Radius", &settings.spawnRadius, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat3("Primitive Acceleration", &settings.acceleration.x, 0.01f, -20.0f, 20.0f);
        ImGui::DragFloat("Primitive End Width Scale", &settings.endWidthScale, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Primitive End Length Scale", &settings.endLengthScale, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Primitive Scale Ease", &settings.scaleEasePower, 0.01f, 0.01f, 10.0f);
        ImGui::SliderFloat("Primitive Fade In", &settings.fadeInRatio, 0.0f, 1.0f);
        ImGui::DragFloat("Primitive Fade Power", &settings.fadePower, 0.01f, 0.01f, 10.0f);

        DrawResetSettingsButton("Reset Primitive", settings);
    }

    // Cylinder settings are grouped here because they tune one visual effect.
    // New cylinder parameters can be added without stretching DrawEffectTab.
    inline void DrawCylinderSettings(Cylinder& cylinder)
    {
        Cylinder::Settings& settings =
            cylinder.GetSettings();

        ImGui::SeparatorText("Cylinder");

        DrawHdrColorEdit4("Cylinder Start Color", &settings.color.x);
        DrawHdrColorEdit4("Cylinder End Color", &settings.endColor.x);

        ImGui::DragFloat("Cylinder Glow", &settings.intensity, 0.01f, 0.0f, 10.0f);
        ImGui::DragFloat("Cylinder Start Radius", &settings.radius, 0.01f, 0.01f, 10.0f);
        ImGui::DragFloat("Cylinder End Radius", &settings.endRadius, 0.01f, 0.01f, 10.0f);
        ImGui::DragFloat("Cylinder Start Height", &settings.startHeight, 0.01f, 0.01f, 20.0f);
        ImGui::DragFloat("Cylinder End Height", &settings.endHeight, 0.01f, 0.01f, 30.0f);
        ImGui::DragFloat("Cylinder Bottom Radius Scale", &settings.bottomRadiusScale, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Cylinder Top Radius Scale", &settings.topRadiusScale, 0.01f, 0.0f, 5.0f);
        ImGui::DragFloat("Cylinder Life Time", &settings.lifeTime, 0.01f, 0.05f, 10.0f);
        ImGui::DragFloat("Cylinder Rise Distance", &settings.riseDistance, 0.01f, -10.0f, 20.0f);
        ImGui::DragFloat("Cylinder Ease Power", &settings.easePower, 0.01f, 0.01f, 10.0f);
        ImGui::SliderFloat("Cylinder Fade In", &settings.fadeInRatio, 0.0f, 1.0f);
        ImGui::DragFloat("Cylinder Fade Power", &settings.fadePower, 0.01f, 0.01f, 10.0f);
        ImGui::DragFloat("Cylinder Twist Amount", &settings.twistAmount, 0.01f, -20.0f, 20.0f);
        ImGui::DragFloat("Cylinder Twist Speed", &settings.twistSpeed, 0.01f, -20.0f, 20.0f);
        ImGui::SliderFloat("Cylinder Noise Strength", &settings.noiseStrength, 0.0f, 1.0f);
        ImGui::DragFloat("Cylinder Noise Frequency", &settings.noiseFrequency, 0.1f, 1.0f, 32.0f);
        ImGui::DragFloat("Cylinder Noise Speed", &settings.noiseSpeed, 0.01f, -10.0f, 10.0f);
        ImGui::SliderFloat("Cylinder Top Fade", &settings.topFade, 0.001f, 1.0f);
        ImGui::SliderFloat("Cylinder Bottom Fade", &settings.bottomFade, 0.001f, 1.0f);
        ImGui::DragFloat3("Cylinder Position Offset", &settings.positionOffset.x, 0.01f, -20.0f, 20.0f);

        DrawResetSettingsButton("Reset Cylinder", settings);
    }
#endif
} // namespace SceneDebugPanelDetail


