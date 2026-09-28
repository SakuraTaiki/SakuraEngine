// ============================================================================
// ファイルの役割: 環境マップやライティング係数など、シーン環境設定を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void SceneDebugPanel::DrawEnvironmentTab(
    Object3dCommon* object3dCommon,
    EnvironmentController& environment,
    SceneObjectController& sceneObjects,
    AnimationDebugController& animationDebug
) {
#ifdef USE_IMGUI
    ImGui::Checkbox(
        "Show Terrain",
        &sceneObjects.ShowTerrain()
    );

    ImGui::Checkbox(
        "Show Axis",
        &sceneObjects.ShowAxis()
    );

    if (environment.DrawImGui()) {
        sceneObjects.SetEnvironmentCoefficient(
            environment.GetEnvironmentCoefficient()
        );

        animationDebug.SetEnvironmentCoefficient(
            environment.GetEnvironmentCoefficient()
        );
    }

    if (!object3dCommon) {
        return;
    }

    ImGui::SeparatorText("Light Settings");

    // ==================================================
    // DirectionalLight
    // ==================================================

    if (ImGui::CollapsingHeader(
        "Directional Light",
        ImGuiTreeNodeFlags_DefaultOpen
    )) {
        DirectionalLight& light =
            object3dCommon->GetDirectionalLight();

        ImGui::ColorEdit4(
            "Directional Color",
            &light.color.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        if (ImGui::DragFloat3(
            "Directional Direction",
            &light.direction.x,
            0.01f,
            -1.0f,
            1.0f
        )) {
            float length = std::sqrt(
                light.direction.x * light.direction.x +
                light.direction.y * light.direction.y +
                light.direction.z * light.direction.z
            );

            if (length > 0.0001f) {
                light.direction.x /= length;
                light.direction.y /= length;
                light.direction.z /= length;
            }
        }

        ImGui::DragFloat(
            "Directional Intensity",
            &light.intensity,
            0.01f,
            0.0f,
            20.0f
        );

        if (ImGui::Button("Reset Directional Light")) {
            light.color =
            { 1.0f, 1.0f, 1.0f, 1.0f };

            light.direction =
            { 0.0f, -1.0f, 0.0f };

            light.intensity = 0.2f;
        }
    }

    // ==================================================
    // SpotLight
    // ==================================================

    if (ImGui::CollapsingHeader(
        "Spot Light",
        ImGuiTreeNodeFlags_DefaultOpen
    )) {
        SpotLight& light =
            object3dCommon->GetSpotLight();

        ImGui::ColorEdit4(
            "Spot Color",
            &light.color.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::DragFloat3(
            "Spot Position",
            &light.position.x,
            0.05f,
            -100.0f,
            100.0f
        );

        if (ImGui::DragFloat3(
            "Spot Direction",
            &light.direction.x,
            0.01f,
            -1.0f,
            1.0f
        )) {
            float length = std::sqrt(
                light.direction.x * light.direction.x +
                light.direction.y * light.direction.y +
                light.direction.z * light.direction.z
            );

            if (length > 0.0001f) {
                light.direction.x /= length;
                light.direction.y /= length;
                light.direction.z /= length;
            }
        }

        ImGui::DragFloat(
            "Spot Intensity",
            &light.intensity,
            0.05f,
            0.0f,
            100.0f
        );

        ImGui::DragFloat(
            "Spot Distance",
            &light.distance,
            0.1f,
            0.1f,
            200.0f
        );

        ImGui::DragFloat(
            "Spot Decay",
            &light.decay,
            0.01f,
            0.01f,
            10.0f
        );

        constexpr float kPi =
            3.14159265358979323846f;

        // shader縺ｫ縺ｯcos蛟､繧呈ｸ｡縺励※縺・・ｽ・ｽ縺溘ａ縲・
        // ImGui縺ｧ縺ｯ謇ｱ縺・・ｽ・ｽ縺吶＞隗貞ｺｦ縺ｸ荳蠎ｦ謌ｻ縺・
        float outerAngle =
            std::acos(std::clamp(
                light.cosAngle,
                -1.0f,
                1.0f
            )) * 180.0f / kPi;

        float innerAngle =
            std::acos(std::clamp(
                light.cosFalloffStart,
                -1.0f,
                1.0f
            )) * 180.0f / kPi;

        bool angleChanged = false;

        angleChanged |= ImGui::SliderFloat(
            "Spot Inner Angle",
            &innerAngle,
            0.0f,
            89.0f,
            "%.1f deg"
        );

        angleChanged |= ImGui::SliderFloat(
            "Spot Outer Angle",
            &outerAngle,
            1.0f,
            89.0f,
            "%.1f deg"
        );

        if (angleChanged) {
            // 蜀・・ｽE隗貞ｺｦ縺悟､厄ｿｽE繧定ｶ・・ｽ・ｽ縺ｪ縺・・ｽ・ｽ縺・・ｽ・ｽ縺吶ｋ
            innerAngle = std::clamp(
                innerAngle,
                0.0f,
                outerAngle
            );

            outerAngle = std::clamp(
                outerAngle,
                innerAngle,
                89.0f
            );

            light.cosAngle =
                std::cos(
                    outerAngle * kPi / 180.0f
                );

            light.cosFalloffStart =
                std::cos(
                    innerAngle * kPi / 180.0f
                );
        }

        ImGui::Spacing();

        ImGui::TextDisabled(
            "Inner: %.1f deg / Outer: %.1f deg",
            innerAngle,
            outerAngle
        );

        ImGui::TextDisabled(
            "Cos Inner: %.3f / Cos Outer: %.3f",
            light.cosFalloffStart,
            light.cosAngle
        );

        if (ImGui::Button("Reset Spot Light")) {
            light.color =
            { 1.0f, 1.0f, 1.0f, 1.0f };

            light.position =
            { 0.0f, 5.0f, 0.0f };

            light.direction =
            { 0.0f, -1.0f, 0.0f };

            light.intensity = 4.0f;
            light.distance = 15.0f;
            light.decay = 2.0f;

            light.cosAngle =
                std::cos(
                    30.0f * kPi / 180.0f
                );

            light.cosFalloffStart =
                std::cos(
                    20.0f * kPi / 180.0f
                );

            light.padding = 0.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Turn Off Spot Light")) {
            light.intensity = 0.0f;
        }
    }

    //===================================
    //PointLight
    //===================================
    if (ImGui::CollapsingHeader(
        "Point Light",
        ImGuiTreeNodeFlags_DefaultOpen
    )) {
        PointLight& light =
            object3dCommon->GetPointLight();

        ImGui::ColorEdit4(
            "Point Color",
            &light.color.x,
            ImGuiColorEditFlags_Float |
            ImGuiColorEditFlags_HDR
        );

        ImGui::DragFloat3(
            "Point Position",
            &light.position.x,
            0.05f,
            -100.0f,
            100.0f
        );

        ImGui::DragFloat(
            "Point Intensity",
            &light.intensity,
            0.05f,
            0.0f,
            100.0f
        );

        ImGui::DragFloat(
            "Point Radius",
            &light.radius,
            0.1f,
            0.1f,
            200.0f
        );

        ImGui::DragFloat(
            "Point Decay",
            &light.decay,
            0.01f,
            0.01f,
            10.0f
        );

        if (ImGui::Button("Reset Point Light")) {
            light.color =
            { 1.0f, 1.0f, 1.0f, 1.0f };

            light.position =
            { 0.0f, 3.0f, 0.0f };

            light.intensity = 2.0f;
            light.radius = 10.0f;
            light.decay = 2.0f;

            light.padding[0] = 0.0f;
            light.padding[1] = 0.0f;
        }

        ImGui::SameLine();

        if (ImGui::Button("Turn Off Point Light")) {
            light.intensity = 0.0f;
        }
    } // Point Light縺ｮCollapsingHeader

#endif
} // DrawEnvironmentTab

