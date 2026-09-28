// ============================================================================
// ファイルの役割: 環境マップやライティング係数など、シーン環境設定を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "EnvironmentController.h"
#include "DirectXCommon.h"
#include "TextureManager.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

void EnvironmentController::Initialize(
    DirectXCommon* dxCommon,
    TextureManager* textureManager
) {
    skybox_ = std::make_unique<Skybox>();

    skybox_->Initialize(
        dxCommon,
        textureManager,
        "Resources/skybox/rostock_laage_airport_4k.dds"
    );

    skybox_->SetScale({ 100.0f, 100.0f, 100.0f });

    environmentTextureHandle_ =
        textureManager->LoadTexture(
            "Resources/skybox/rostock_laage_airport_4k.dds"
        );
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void EnvironmentController::Finalize() {
    skybox_.reset();
}

void EnvironmentController::Update(
    const Matrix4x4& view,
    const Matrix4x4& projection
) {
    if (!enableSkybox_ || !skybox_) {
        return;
    }

    skybox_->SetCamera(
        view,
        projection
    );

    skybox_->Update();
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void EnvironmentController::Draw() {
    if (!enableSkybox_ || !skybox_) {
        return;
    }

    skybox_->Draw();
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
bool EnvironmentController::DrawImGui() {
#ifdef USE_IMGUI
    bool coefficientChanged = false;

    ImGui::Checkbox(
        "Enable Skybox",
        &enableSkybox_
    );

    ImGui::Text("Environment Lighting");
    ImGui::Separator();

    coefficientChanged =
        ImGui::SliderFloat(
            "Environment Coefficient",
            &environmentCoefficient_,
            0.0f,
            1.0f
        );

    return coefficientChanged;
#else
    return false;
#endif
}