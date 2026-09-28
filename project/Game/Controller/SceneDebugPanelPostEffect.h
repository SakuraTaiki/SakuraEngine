// ============================================================================
// ファイルの役割: ゲームビューと各種デバッグ編集ウィンドウをImGui上へ構築する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void SceneDebugPanel::DrawPostEffectTab(
    EngineContext* context
) {
#ifdef USE_IMGUI
    if (!context) {
        return;
    }

    DirectXCommon* dxCommon =
        context->GetDxCommon();

    if (!dxCommon) {
        return;
    }

    auto& vignette =
        dxCommon->GetVignetteSettings();

    auto& smoothing =
        dxCommon->GetSmoothingSettings();

    auto& gaussian =
        dxCommon->GetGaussianSettings();

    auto& radialBlur =
        dxCommon->GetRadialBlurSettings();

    auto& outline =
        dxCommon->GetOutlineSettings();

    auto& dissolve =
        dxCommon->GetDissolveSettings();

    auto& random =
        dxCommon->GetRandomSettings();

    Camera* camera =
        context->GetCamera();

    if (camera) {
        outline.nearClip =
            camera->GetNearClip();

        outline.farClip =
            camera->GetFarClip();
    }

    ImGui::Text("Post Effect Settings");
    ImGui::Separator();

    //====================
    // GrayScale
    //====================

    bool enableGrayScale =
        dxCommon->GetGrayScale();

    if (ImGui::Checkbox(
        "GrayScale",
        &enableGrayScale
    )) {
        dxCommon->SetGrayScale(
            enableGrayScale
        );

        if (enableGrayScale) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            radialBlur.enabled = false;
            outline.enabled = false;
        }
    }

    //====================
    // Vignetting
    //====================

    ImGui::SeparatorText("Vignetting");

    if (ImGui::Checkbox(
        "Enable Vignetting",
        &vignette.enabled
    )) {
        if (vignette.enabled) {
            smoothing.enabled = false;
            gaussian.enabled = false;
            radialBlur.enabled = false;
            outline.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !vignette.enabled
    );

    ImGui::SliderFloat(
        "Vignette Intensity",
        &vignette.intensity,
        0.0f,
        1.0f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Vignette Radius",
        &vignette.radius,
        0.0f,
        1.5f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Vignette Softness",
        &vignette.softness,
        0.01f,
        1.0f,
        "%.2f"
    );

    if (ImGui::Button(
        "Reset Vignetting"
    )) {
        const bool wasEnabled =
            vignette.enabled;

        vignette =
            DirectXCommon::VignetteSettings{};

        vignette.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();

    //====================
    // Smoothing
    //====================

    ImGui::SeparatorText("Smoothing");

    if (ImGui::Checkbox(
        "Enable Smoothing",
        &smoothing.enabled
    )) {
        if (smoothing.enabled) {
            vignette.enabled = false;
            gaussian.enabled = false;
            radialBlur.enabled = false;
            outline.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !smoothing.enabled
    );

    ImGui::SliderInt(
        "Blur Radius",
        &smoothing.radius,
        1,
        4
    );

    ImGui::SliderFloat(
        "Blur Strength",
        &smoothing.strength,
        0.0f,
        1.0f,
        "%.2f"
    );

    const int smoothingKernelSize =
        smoothing.radius * 2 + 1;

    ImGui::Text(
        "Kernel : %d x %d",
        smoothingKernelSize,
        smoothingKernelSize
    );

    if (ImGui::Button(
        "Reset Smoothing"
    )) {
        const bool wasEnabled =
            smoothing.enabled;

        smoothing =
            DirectXCommon::SmoothingSettings{};

        smoothing.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();

    //====================
    // Gaussian Filter
    //====================

    ImGui::SeparatorText(
        "Gaussian Filter"
    );

    if (ImGui::Checkbox(
        "Enable Gaussian Filter",
        &gaussian.enabled
    )) {
        if (gaussian.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            radialBlur.enabled = false;
            outline.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !gaussian.enabled
    );

    ImGui::SliderInt(
        "Gaussian Radius",
        &gaussian.radius,
        1,
        4
    );

    ImGui::SliderFloat(
        "Gaussian Sigma",
        &gaussian.sigma,
        0.1f,
        10.0f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Gaussian Strength",
        &gaussian.strength,
        0.0f,
        1.0f,
        "%.2f"
    );

    const int gaussianKernelSize =
        gaussian.radius * 2 + 1;

    ImGui::Text(
        "Gaussian Kernel : %d x %d",
        gaussianKernelSize,
        gaussianKernelSize
    );

    if (ImGui::Button(
        "Reset Gaussian"
    )) {
        const bool wasEnabled =
            gaussian.enabled;

        gaussian =
            DirectXCommon::GaussianSettings{};

        gaussian.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();

    //====================
    // Radial Blur
    //====================

    ImGui::SeparatorText("Radial Blur");

    if (ImGui::Checkbox(
        "Enable Radial Blur",
        &radialBlur.enabled
    )) {
        if (radialBlur.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            outline.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !radialBlur.enabled
    );

    ImGui::SliderFloat2(
        "Blur Center (UV)",
        radialBlur.center,
        0.0f,
        1.0f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Radial Blur Width",
        &radialBlur.blurWidth,
        0.0f,
        0.1f,
        "%.4f"
    );

    ImGui::SliderFloat(
        "Radial Blur Strength",
        &radialBlur.strength,
        0.0f,
        1.0f,
        "%.2f"
    );

    ImGui::SliderInt(
        "Radial Samples",
        &radialBlur.sampleCount,
        2,
        32
    );

    ImGui::TextDisabled(
        "Center: (0,0) top-left / "
        "(1,1) bottom-right"
    );

    if (ImGui::Button(
        "Reset Radial Blur"
    )) {
        const bool wasEnabled =
            radialBlur.enabled;

        radialBlur =
            DirectXCommon::RadialBlurSettings{};

        radialBlur.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();

    //====================
    // Depth Based Outline
    //====================

    ImGui::SeparatorText(
        "Depth Based Outline"
    );

    if (ImGui::Checkbox(
        "Enable Depth Outline",
        &outline.enabled
    )) {
        if (outline.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            radialBlur.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !outline.enabled
    );

    ImGui::ColorEdit4(
        "Outline Color",
        outline.color
    );

    ImGui::SliderFloat(
        "Outline Threshold",
        &outline.threshold,
        0.001f,
        1.0f,
        "%.3f"
    );

    ImGui::SliderFloat(
        "Outline Strength",
        &outline.strength,
        0.0f,
        5.0f,
        "%.2f"
    );

    ImGui::SliderInt(
        "Outline Thickness",
        &outline.thickness,
        1,
        4
    );

    ImGui::Text(
        "Near Clip : %.3f",
        outline.nearClip
    );

    ImGui::Text(
        "Far Clip : %.3f",
        outline.farClip
    );

    if (ImGui::Button(
        "Reset Outline"
    )) {
        const bool wasEnabled =
            outline.enabled;

        outline =
            DirectXCommon::OutlineSettings{};

        outline.enabled =
            wasEnabled;

        if (camera) {
            outline.nearClip =
                camera->GetNearClip();

            outline.farClip =
                camera->GetFarClip();
        }
    }

    ImGui::EndDisabled();

    //====================
    // Dissolve
    //====================

    ImGui::SeparatorText("Dissolve");

    if (ImGui::Checkbox(
        "Enable Dissolve",
        &dissolve.enabled
    )) {
        if (dissolve.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            outline.enabled = false;

            // RadialBlur追加済みの場吁E
            radialBlur.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !dissolve.enabled
    );

    ImGui::SliderFloat(
        "Dissolve Threshold",
        &dissolve.threshold,
        0.0f,
        1.0f,
        "%.3f"
    );

    ImGui::SliderFloat(
        "Dissolve Edge Width",
        &dissolve.edgeWidth,
        0.001f,
        0.25f,
        "%.3f"
    );

    ImGui::SliderFloat(
        "Dissolve Edge Intensity",
        &dissolve.edgeIntensity,
        0.0f,
        1.0f,
        "%.2f"
    );

    ImGui::ColorEdit4(
        "Dissolve Edge Color",
        dissolve.edgeColor
    );

    if (ImGui::Button(
        "Reset Dissolve"
    )) {
        const bool wasEnabled =
            dissolve.enabled;

        dissolve =
            DirectXCommon::DissolveSettings{};

        dissolve.enabled =
            wasEnabled;
    }

    ImGui::EndDisabled();

    //====================
    //RandomNoise
    //====================

    ImGui::SeparatorText("Random Noise");

    if (ImGui::Checkbox(
        "Enable Random Noise",
        &random.enabled
    )) {
        if (random.enabled) {
            vignette.enabled = false;
            smoothing.enabled = false;
            gaussian.enabled = false;
            outline.enabled = false;

            // 追加済みの場吁E
            radialBlur.enabled = false;
            dissolve.enabled = false;

            dxCommon->SetGrayScale(false);
        }
    }

    ImGui::BeginDisabled(
        !random.enabled
    );

    ImGui::Checkbox(
        "Animate Random Noise",
        &random.animate
    );

    ImGui::Checkbox(
        "Show Noise Only",
        &random.showNoiseOnly
    );

    ImGui::SliderFloat(
        "Random Speed",
        &random.speed,
        0.0f,
        10.0f,
        "%.2f"
    );

    ImGui::SliderFloat(
        "Random Scale",
        &random.scale,
        1.0f,
        2000.0f,
        "%.0f"
    );

    ImGui::SliderFloat(
        "Random Strength",
        &random.strength,
        0.0f,
        1.0f,
        "%.2f"
    );

    if (ImGui::Button(
        "Reset Random Settings"
    )) {
        const bool wasEnabled =
            random.enabled;

        random =
            DirectXCommon::RandomSettings{};

        random.enabled =
            wasEnabled;

        dxCommon->ResetRandomTime();
    }

    ImGui::SameLine();

    if (ImGui::Button(
        "Reset Random Time"
    )) {
        dxCommon->ResetRandomTime();
    }

    ImGui::EndDisabled();

    //====================
    // Current Effect
    //====================

    ImGui::SeparatorText(
        "Current Post Effect"
    );

    const char* currentEffect = "None";

    if (outline.enabled) {
        currentEffect =
            "Depth Based Outline";
    } else if (random.enabled) {
        currentEffect =
            "Random Noise";
    } else if (dissolve.enabled) {
        currentEffect =
            "Dissolve";
    } else if (radialBlur.enabled) {
        currentEffect =
            "Radial Blur";
    } else if (gaussian.enabled) {
        currentEffect =
            "Gaussian Filter";
    } else if (smoothing.enabled) {
        currentEffect =
            "Smoothing";
    } else if (vignette.enabled) {
        currentEffect =
            "Vignetting";
    } else if (dxCommon->GetGrayScale()) {
        currentEffect =
            "GrayScale";
    }

    ImGui::Text(
        "Active : %s",
        currentEffect
    );

    if (ImGui::Button(
        "Disable All Post Effects"
    )) {
        vignette.enabled = false;
        smoothing.enabled = false;
        gaussian.enabled = false;
        radialBlur.enabled = false;
        outline.enabled = false;
        dissolve.enabled = false;
        random.enabled = false;

        dxCommon->SetGrayScale(false);
    }
#endif
}

