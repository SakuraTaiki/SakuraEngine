// ============================================================================
// ファイルの役割: ゲームビューと各種デバッグ編集ウィンドウをImGui上へ構築する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void SceneDebugPanel::DrawSceneControl(GameSceneDrawMode& drawMode, HitEffectController& hitEffect, AnimationDebugController& animationDebug)
{

#ifdef USE_IMGUI
    ImGui::Text("Scene Control");
    ImGui::Separator();

    if (drawMode == GameSceneDrawMode::Effect) {
        if (ImGui::Button("Emit Effect SPACE", ImVec2(180.0f, 32.0f))) {
            hitEffect.Emit({ 0.0f, 3.0f, 0.0f });
            Detail::AddEditorLog(Detail::EditorLogType::Info, "Effect emitted from Debug Tools.");
        }
        ImGui::SameLine();
        ImGui::TextDisabled("SPACE key also emits");
    } else {
        ImGui::TextDisabled("Effect emission is available only in Effect / Debug OBJ mode.");
    }

    ImGui::Spacing();

    ImGui::Text("Draw Mode");
    ImGui::Separator();

    int currentMode = static_cast<int>(drawMode);

    if (ImGui::RadioButton("Stage / GamePlay", currentMode == 0)) {
        drawMode = GameSceneDrawMode::NormalObj;
    }

    if (ImGui::RadioButton("Animation glTF", currentMode == 1)) {
        drawMode = GameSceneDrawMode::Animation;
    }

    if (ImGui::RadioButton("Effect / Debug OBJ", currentMode == 2)) {
        drawMode = GameSceneDrawMode::Effect;
    }

    ImGui::Spacing();

    ImGui::Text("Skeleton");
    ImGui::BulletText("Joint Count : %d", animationDebug.GetJointCount());
    ImGui::BulletText("Animation Time : %.2f", animationDebug.GetAnimationTime());

    ImGui::Spacing();

    ImGui::Checkbox(
        "Show Skeleton Debug",
        &animationDebug.ShowSkeletonDebug()
    );
#endif

}

void SceneDebugPanel::DrawEffectTab(
    EngineContext* context,
    HitEffectController& hitEffect,
    Ring* ring,
    Cylinder* cylinder,
    Primitive* primitive
) {
#ifdef USE_IMGUI
    ImGui::Checkbox(
        "Enable Primitive",
        &hitEffect.EnablePrimitive()
    );

    ImGui::Checkbox(
        "Enable Ring",
        &hitEffect.EnableRing()
    );

    ImGui::Checkbox(
        "Enable Cylinder",
        &hitEffect.EnableCylinder()
    );

    // GPU particle controls are large enough to deserve their own editor function.
    // DrawEffectTab keeps only the ordering of the visible sections.
    if (GPUParticleManager* gpuParticleManager = context->GetGPUParticleManager()) {
        Detail::DrawGpuParticleSettings(
            *gpuParticleManager,
            hitEffect
        );
    }

    ImGui::SeparatorText("Hit Effect Type");

    if (ImGui::RadioButton(
        "Fire",
        hitEffect.GetType() == HitEffectController::Type::Fire
    )) {
        hitEffect.GetType() =
            HitEffectController::Type::Fire;
    }

    ImGui::SameLine();

    if (ImGui::RadioButton(
        "Sakura",
        hitEffect.GetType() == HitEffectController::Type::Sakura
    )) {
        hitEffect.GetType() =
            HitEffectController::Type::Sakura;
    }

    ImGui::SliderFloat(
        "Hit Effect Size",
        &hitEffect.GetSize(),
        0.1f,
        5.0f,
        "%.2f"
    );

    if (ImGui::Button("Reset Effect Size")) {
        hitEffect.GetSize() = 1.0f;
    }

    ImGui::SameLine();

    ImGui::TextDisabled(
        "Current : %.2f x",
        hitEffect.GetSize()
    );

    if (
        hitEffect.GetType() ==
        HitEffectController::Type::Sakura
        ) {
        Detail::DrawSakuraSettings(
            context
            ->GetParticleManager()
            ->GetSakuraSettings()
        );
    }

    ImGui::SeparatorText("Emit");

    ImGui::DragFloat3(
        "Effect Position",
        &hitEffect.GetPosition().x,
        0.05f,
        -20.0f,
        20.0f
    );

    ImGui::SeparatorText("Effect Presets");

    ImGui::InputText(
        "Preset Name",
        hitEffect.GetPresetNameBuffer().data(),
        hitEffect.GetPresetNameBuffer().size()
    );

    if (ImGui::Button(
        "Save Current Preset",
        ImVec2(180.0f, 30.0f)
    )) {
        hitEffect.SavePreset(
            hitEffect.GetPresetNameBuffer().data()
        );
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Refresh Presets",
        ImVec2(160.0f, 30.0f)
    )) {
        hitEffect.RefreshPresetList();
    }

    if (!hitEffect.GetPresetNames().empty()) {
        hitEffect.GetSelectedPreset() =
            std::clamp(
                hitEffect.GetSelectedPreset(),
                0,
                static_cast<int>(
                    hitEffect.GetPresetNames().size()
                    ) - 1
            );

        const char* previewName =
            hitEffect.GetPresetNames()[
                hitEffect.GetSelectedPreset()
            ].c_str();

        if (ImGui::BeginCombo(
            "Saved Presets",
            previewName
        )) {
            for (
                int index = 0;
                index <
                static_cast<int>(
                    hitEffect.GetPresetNames().size()
                    );
                    ++index
                ) {
                const bool selected =
                    hitEffect.GetSelectedPreset() ==
                    index;

                if (ImGui::Selectable(
                    hitEffect.GetPresetNames()[index].c_str(),
                    selected
                )) {
                    hitEffect.GetSelectedPreset() =
                        index;
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::Button(
            "Load Selected",
            ImVec2(180.0f, 30.0f)
        )) {
            hitEffect.LoadPreset(
                hitEffect.GetPresetNames()[
                    hitEffect.GetSelectedPreset()
                ]
            );
        }

        ImGui::SameLine();

        if (ImGui::Button(
            "Load And Preview",
            ImVec2(180.0f, 30.0f)
        )) {
            if (hitEffect.LoadPreset(
                hitEffect.GetPresetNames()[
                    hitEffect.GetSelectedPreset()
                ]
            )) {
                hitEffect.Emit(
                    hitEffect.GetPosition()
                );
            }
        }
    } else {
        ImGui::TextDisabled(
            "No saved presets"
        );
    }

    if (!hitEffect.GetMessage().empty()) {
        ImGui::TextDisabled(
            "%s",
            hitEffect.GetMessage().c_str()
        );
    }

    ImGui::SeparatorText("Preview");

    if (ImGui::Button(
        "Preview Current",
        ImVec2(180.0f, 32.0f)
    )) {
        hitEffect.Emit(
            hitEffect.GetPosition()
        );
    }

    if (ImGui::Button(
        "Apply Fire Preset",
        ImVec2(180.0f, 30.0f)
    )) {
        hitEffect.GetType() =
            HitEffectController::Type::Fire;

        hitEffect.ApplyFirePreset();
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Apply Fire And Preview",
        ImVec2(180.0f, 30.0f)
    )) {
        hitEffect.GetType() =
            HitEffectController::Type::Fire;

        hitEffect.ApplyFirePreset();

        hitEffect.Emit(
            hitEffect.GetPosition()
        );
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Preview Fire",
        ImVec2(180.0f, 30.0f)
    )) {
        hitEffect.Emit(hitEffect.GetPosition());
    }

    // Detailed effect setting panels are split into small editor functions.
    // That leaves this tab focused on the user-facing order of controls.
    if (ring) {
        Detail::DrawRingSettings(*ring);
    }

    if (primitive) {
        Detail::DrawPrimitiveSettings(*primitive);
    }

    if (cylinder) {
        Detail::DrawCylinderSettings(*cylinder);
    }
#endif
}


