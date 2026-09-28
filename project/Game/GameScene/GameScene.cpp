// ============================================================================
// ファイルの役割: ゲームで利用する各コントローラーを組み立て、更新・描画順序を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "GameScene.h"

#include "ModelManager.h"

#include "Input.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "TextureManager.h"
#include "SpriteCommon.h"
#include "Object3dCommon.h"
#include "ParticleManager.h"
#include "GPUParticleManager.h"
#include "ImGuiManager.h"
#include "camera.h"
#include "WinApp.h"

#include <algorithm>
#include <cmath>
#include <filesystem>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif
// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void GameScene::Initialize(EngineContext* context) {
    context_ = context;

#ifndef USE_IMGUI
    // The submitted Release build is evaluated on the animation feature.
    // Start directly in the glTF animation view because no ImGui scene
    // selector is available in that configuration.
    drawMode_ = GameSceneDrawMode::Animation;
#endif

    ModelManager::Initialize(context_->GetObject3dCommon());

    InitializeModels();
    InitializeSprite();

    environment_.Initialize(
        context_->GetDxCommon(),
        context_->GetTextureManager()
    );

    Object3dCommon* object3dCommon =
        context_->GetObject3dCommon();

    sceneObjects_.Initialize(
        object3dCommon,
        environment_.GetEnvironmentTextureHandle(),
        environment_.GetEnvironmentCoefficient()
    );

    // Blender export is authoritative whenever it exists. A missing model
    // referenced by Task.json must not make us replace the whole scene with
    // LevelScene.json.
    const bool hasBlenderTask =
        std::filesystem::exists("Resources/Task.json");
    bool levelLoaded = false;
    if (hasBlenderTask) {
        levelLoaded = sceneObjects_.LoadLevelSceneFromJson(
            "Resources/Task.json"
        );
    } else {
        levelLoaded = sceneObjects_.LoadLevelSceneFromJson(
            "Resources/LevelScene.json"
        );
    }

    sceneObjects_.ShowTerrain() = false;
    sceneObjects_.ShowAxis() = false;

    if (!levelLoaded) {
        OutputDebugStringA(
            hasBlenderTask
                ? "Task.json was loaded with missing or invalid assets\n"
                : "Failed to load Resources/LevelScene.json\n"
        );
    }

    InitializeTaskJsonHotReload();

    animationDebug_.Initialize(
        object3dCommon,
        environment_.GetEnvironmentTextureHandle(),
        environment_.GetEnvironmentCoefficient()
    );

    InitializeRing();
    InitializeCylinder();
    InitializePrimitive();



    hitEffect_.Initialize(
        primitive_.get(),
        ring_.get(),
        cylinder_.get(),
        context_->GetParticleManager(),
        context_->GetGPUParticleManager()
    );

    hitEffect_.ApplyFirePreset();
    hitEffect_.RefreshPresetList();

    soundController_.Initialize();
    stageEditor_.Initialize(
        object3dCommon,
        environment_.GetEnvironmentTextureHandle(),
        environment_.GetEnvironmentCoefficient()
    );
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void GameScene::Finalize() {
    stageEditor_.Finalize();
    sceneObjects_.Finalize();

    animationDebug_.Finalize();
    environment_.Finalize();
    sprite_.reset();

    soundController_.Finalize();
    ModelManager::Finalize();

    context_ = nullptr;
}

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void GameScene::InitializeModels() {
    ModelManager::Load("Resources/terrain", "terrain.obj");
    ModelManager::Load("axis.obj");

    ModelManager::Load("Resources/human", "walk.gltf");
}




// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void GameScene::InitializeSprite() {
    uint32_t texHandle =
        context_->GetTextureManager()->LoadTexture("Resources/white.png");

    sprite_ = std::make_unique<Sprite>();
    sprite_->Initialize(context_->GetSpriteCommon(), texHandle);
}


// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void GameScene::InitializeRing() {
    ring_ = std::make_unique<Ring>();
    ring_->Initialize(context_->GetDxCommon(), context_->GetTextureManager());
}

void GameScene::InitializeCylinder()
{
    cylinder_ = std::make_unique<Cylinder>();
    cylinder_->Initialize(
        context_->GetDxCommon(),
        context_->GetTextureManager()
    );
}

void GameScene::InitializePrimitive()
{
    primitive_ = std::make_unique<Primitive>();

    primitive_->Initialize(
        context_->GetDxCommon(),
        context_->GetTextureManager()
    );

}


// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void GameScene::Update() {
    Input* input = context_->GetInput();

    if (drawMode_ == GameSceneDrawMode::Effect) {
        UpdatePostEffectShortcuts();
    }

    if (drawMode_ == GameSceneDrawMode::Effect && input->TriggerKey(DIK_SPACE)) {
        hitEffect_.Emit({ 0.0f, 3.0f, 0.0f });
    }

    soundController_.Update(context_->GetInput());
    UpdateTaskJsonHotReload();
    UpdateObjects();
    // Draw modes are exclusive tool contexts.  Animation mode must not
    // update the stage placement cursor or consume the same WASD input.
    stageEditor_.SetActive(drawMode_ == GameSceneDrawMode::NormalObj);
    stageEditor_.Update(input);

    if (drawMode_ == GameSceneDrawMode::NormalObj && stageEditor_.IsGamePlayMode()) {
        UpdateSideScrollCamera();
    } else {
        sideScrollCameraActive_ = false;

        // StageEditor編集中は、ゲームビュー上にマウスがある時だけ
        // デバッグカメラへマウス入力を渡す。メニュー操作中の誤ズームを防ぐ。
        bool allowDebugCameraMouseOperation = true;
#ifdef USE_IMGUI
        if (drawMode_ == GameSceneDrawMode::NormalObj &&
            stageEditor_.IsEditingGameView()) {
            allowDebugCameraMouseOperation = stageEditor_.IsGameViewHovered();
        }
#endif

        cameraDebug_.Update(
            context_->GetCamera(),
            input,
            allowDebugCameraMouseOperation
        );
    }

    WinApp* winApp =
        context_->GetWinApp();

    if (
        winApp &&
        winApp->GetHeight() > 0
        ) {
        context_->GetCamera()->SetAspectRatio(
            static_cast<float>(
                winApp->GetWidth()
                ) /
            static_cast<float>(
                winApp->GetHeight()
                )
        );
    }

    context_->GetCamera()->Update();

    const Matrix4x4& view = context_->GetCamera()->GetViewMatrix();
    const Matrix4x4& projection = context_->GetCamera()->GetProjectionMatrix();

    environment_.Update(
        view,
        projection
    );

    if (sprite_) {
        sprite_->Update();
    }

    const bool effectMode = drawMode_ == GameSceneDrawMode::Effect;
    if (ring_) {
        ring_->SetIsActive(effectMode && hitEffect_.EnableRing());
        ring_->Update(view, projection);
    }

    if (cylinder_) {
        cylinder_->SetIsActive(effectMode && hitEffect_.EnableCylinder());
        cylinder_->Update(view, projection);
    }

    if (primitive_) {
        primitive_->SetIsActive(
            effectMode && hitEffect_.EnablePrimitive()
        );

        primitive_->Update(
            view,
            projection
        );
    }

    if (effectMode) {
        context_->GetParticleManager()->Update(view, projection);
        context_->GetGPUParticleManager()->Update(view, projection);
    }

    sceneDebugPanel_.Draw(
        context_,
        drawMode_,
        hitEffect_,
        animationDebug_,
        soundController_,
        cameraDebug_,
        environment_,
        sceneObjects_,
        stageEditor_,
        ring_.get(),
        cylinder_.get(),
        primitive_.get()
    );
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void GameScene::UpdateSideScrollCamera() {
    Camera* camera = context_ ? context_->GetCamera() : nullptr;
    if (!camera) return;

    constexpr float deltaTime = 1.0f / 60.0f;
    constexpr float cameraZ = -30.0f;
    constexpr float cameraY = 7.0f;
    constexpr float followSmoothness = 10.0f;

    const float distance = std::abs(cameraZ);
    const float halfViewHeight = std::tan(camera->GetFovY() * 0.5f) * distance;
    const float halfViewWidth = halfViewHeight * camera->GetAspectRatio();
    const float stageWidth =
        static_cast<float>(stageEditor_.GetStageWidthInTiles()) * StageEditor::kTileWorldSize;

    const float minimumCameraX = halfViewWidth;
    const float maximumCameraX = (std::max)(minimumCameraX, stageWidth - halfViewWidth);
    const float playerX = stageEditor_.GetPlayerPosition().x;

    // Start at the left edge. Once the player crosses the screen center,
    // follow only to the right; clamping prevents showing outside the stage.
    float targetCameraX = std::clamp(
        (std::max)(minimumCameraX, playerX),
        minimumCameraX,
        maximumCameraX
    );

    if (!sideScrollCameraActive_) {
        sideScrollCameraX_ = targetCameraX;
        sideScrollCameraActive_ = true;
    } else {
        const float blend = 1.0f - std::exp(-followSmoothness * deltaTime);
        sideScrollCameraX_ += (targetCameraX - sideScrollCameraX_) * blend;
        sideScrollCameraX_ = std::clamp(
            sideScrollCameraX_,
            minimumCameraX,
            maximumCameraX
        );
    }

    camera->SetRotate({0.0f, 0.0f, 0.0f});
    camera->SetTranslate({sideScrollCameraX_, cameraY, cameraZ});
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void GameScene::UpdatePostEffectShortcuts() {
    Input* input = context_->GetInput();
    DirectXCommon* dxCommon = context_->GetDxCommon();
    if (!input || !dxCommon) {
        return;
    }

    auto& vignette = dxCommon->GetVignetteSettings();
    auto& smoothing = dxCommon->GetSmoothingSettings();
    auto& gaussian = dxCommon->GetGaussianSettings();
    auto& outline = dxCommon->GetOutlineSettings();
    auto& radialBlur = dxCommon->GetRadialBlurSettings();
    auto& dissolve = dxCommon->GetDissolveSettings();
    auto& random = dxCommon->GetRandomSettings();

    auto triggered = [input](BYTE mainKey, BYTE numpadKey) {
        return input->TriggerKey(mainKey) || input->TriggerKey(numpadKey);
    };

    enum class Effect {
        None,
        GrayScale,
        Vignette,
        Smoothing,
        Gaussian,
        Outline,
        RadialBlur,
        Dissolve,
        Random
    };

    Effect effect = Effect::None;
    bool enable = false;

    if (triggered(DIK_1, DIK_NUMPAD1)) {
        effect = Effect::GrayScale;
        enable = !dxCommon->GetGrayScale();
    } else if (triggered(DIK_2, DIK_NUMPAD2)) {
        effect = Effect::Vignette;
        enable = !vignette.enabled;
    } else if (triggered(DIK_3, DIK_NUMPAD3)) {
        effect = Effect::Smoothing;
        enable = !smoothing.enabled;
    } else if (triggered(DIK_4, DIK_NUMPAD4)) {
        effect = Effect::Gaussian;
        enable = !gaussian.enabled;
    } else if (triggered(DIK_5, DIK_NUMPAD5)) {
        effect = Effect::Outline;
        enable = !outline.enabled;
    } else if (triggered(DIK_6, DIK_NUMPAD6)) {
        effect = Effect::RadialBlur;
        enable = !radialBlur.enabled;
    } else if (triggered(DIK_7, DIK_NUMPAD7)) {
        effect = Effect::Dissolve;
        enable = !dissolve.enabled;
    } else if (triggered(DIK_8, DIK_NUMPAD8)) {
        effect = Effect::Random;
        enable = !random.enabled;
    }

    if (effect == Effect::None) {
        return;
    }

    dxCommon->SetGrayScale(false);
    vignette.enabled = false;
    smoothing.enabled = false;
    gaussian.enabled = false;
    outline.enabled = false;
    radialBlur.enabled = false;
    dissolve.enabled = false;
    random.enabled = false;

    if (!enable) {
        return;
    }

    switch (effect) {
    case Effect::GrayScale:
        dxCommon->SetGrayScale(true);
        break;
    case Effect::Vignette:
        vignette.enabled = true;
        break;
    case Effect::Smoothing:
        // Uniform 9x9 box blur: intentionally broad for the evaluation demo.
        smoothing.radius = 4;
        smoothing.strength = 1.0f;
        smoothing.enabled = true;
        break;
    case Effect::Gaussian:
        // Center-weighted 9x9 blur, visually distinct from the box filter.
        gaussian.radius = 4;
        gaussian.sigma = 1.0f;
        gaussian.strength = 1.0f;
        gaussian.enabled = true;
        break;
    case Effect::Outline:
        outline.enabled = true;
        break;
    case Effect::RadialBlur:
        radialBlur.enabled = true;
        break;
    case Effect::Dissolve:
        // The default threshold is zero and produces almost no visible change.
        dissolve.threshold = 0.5f;
        dissolve.edgeWidth = 0.08f;
        dissolve.edgeIntensity = 1.0f;
        dissolve.enabled = true;
        break;
    case Effect::Random:
        random.enabled = true;
        dxCommon->ResetRandomTime();
        break;
    default:
        break;
    }
}

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void GameScene::InitializeTaskJsonHotReload() {
    constexpr const char* kTaskJsonPath = "Resources/Task.json";
    std::error_code error;

    if (!std::filesystem::exists(kTaskJsonPath, error) || error) {
        taskJsonWatchInitialized_ = false;
        taskJsonReloadPending_ = false;
        return;
    }

    taskJsonObservedWriteTime_ =
        std::filesystem::last_write_time(kTaskJsonPath, error);
    taskJsonWatchInitialized_ = !error;
    taskJsonReloadPending_ = false;
    taskJsonReloadAttempts_ = 0;
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void GameScene::UpdateTaskJsonHotReload() {
    constexpr const char* kTaskJsonPath = "Resources/Task.json";
    constexpr auto kReloadDebounce = std::chrono::milliseconds(250);
    constexpr int kMaxReloadAttempts = 10;

    std::error_code error;
    if (!std::filesystem::exists(kTaskJsonPath, error) || error) {
        taskJsonWatchInitialized_ = false;
        taskJsonReloadPending_ = false;
        return;
    }

    const auto currentWriteTime =
        std::filesystem::last_write_time(kTaskJsonPath, error);
    if (error) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (!taskJsonWatchInitialized_) {
        taskJsonObservedWriteTime_ = currentWriteTime;
        taskJsonWatchInitialized_ = true;
        taskJsonReloadPending_ = true;
        taskJsonReloadAttempts_ = 0;
        taskJsonChangeDetectedAt_ = now;
        return;
    }

    if (currentWriteTime != taskJsonObservedWriteTime_) {
        taskJsonObservedWriteTime_ = currentWriteTime;
        taskJsonReloadPending_ = true;
        taskJsonReloadAttempts_ = 0;
        taskJsonChangeDetectedAt_ = now;
        return;
    }

    if (!taskJsonReloadPending_ ||
        now - taskJsonChangeDetectedAt_ < kReloadDebounce) {
        return;
    }

    const bool loaded =
        sceneObjects_.LoadLevelSceneFromJson(kTaskJsonPath);
    if (loaded) {
        taskJsonReloadPending_ = false;
        taskJsonReloadAttempts_ = 0;
        OutputDebugStringA("Hot reloaded Resources/Task.json\n");
        return;
    }

    ++taskJsonReloadAttempts_;
    if (taskJsonReloadAttempts_ >= kMaxReloadAttempts) {
        taskJsonReloadPending_ = false;
        OutputDebugStringA("Task.json hot reload failed after retries\n");
    } else {
        taskJsonChangeDetectedAt_ = now;
    }
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void GameScene::UpdateObjects() {
    if (drawMode_ == GameSceneDrawMode::Effect) {
        sceneObjects_.Update();
    }

    if (drawMode_ == GameSceneDrawMode::Animation) {
        animationDebug_.Update(context_->GetInput());
    }
}



// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void GameScene::Draw() {
    Draw3D();
    Draw2D();
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void GameScene::Draw3D() {
    DirectXCommon* dxCommon = context_->GetDxCommon();

    dxCommon->PreDrawForRenderTexture();

    context_->GetSrvManager()->PreDraw();

    context_->GetObject3dCommon()->PreDraw();

    // The stage editor owns the editing viewport.  Do not mix the legacy
    // SceneObjectController scene (terrain/axis/Blender objects) into it:
    // placed stage items must be previewed against the same clean view that
    // will be used for the stage itself.  Runtime scene objects return when
    // entering GamePlay mode.
    if (drawMode_ == GameSceneDrawMode::Effect) {
        sceneObjects_.Draw();
    }

    if (drawMode_ == GameSceneDrawMode::NormalObj) {
        stageEditor_.Draw3D();
    }

    if (drawMode_ == GameSceneDrawMode::Animation) {
        animationDebug_.Draw();
    }

    environment_.Draw();

    if (drawMode_ == GameSceneDrawMode::Effect && ring_) {
        ring_->Draw();
    }

    if (drawMode_ == GameSceneDrawMode::Effect && cylinder_) {
        cylinder_->Draw();
    }

    if (drawMode_ == GameSceneDrawMode::Effect && primitive_) {
        primitive_->Draw();
    }

    if (drawMode_ == GameSceneDrawMode::Effect) {
        context_->GetParticleManager()->Draw();
        context_->GetGPUParticleManager()->Draw();
    }
}



void GameScene::Draw2D()
{
    DirectXCommon* dxCommon =
        context_->GetDxCommon();

    dxCommon->PreDraw();

   
    Camera* camera =
        context_->GetCamera();

    if (camera) {
        auto& outline =
            dxCommon->GetOutlineSettings();

        outline.nearClip =
            camera->GetNearClip();

        outline.farClip =
            camera->GetFarClip();
    }


	dxCommon->DrawRenderTextureToSwapChain();
    
    dxCommon->PrepareRenderTextureForImgui();

    if (context_->GetImGuiManager()) {
        context_->GetImGuiManager()->UpdateGameViewTexture();
    }

    context_->GetSrvManager()->PreDraw();
    context_->GetSpriteCommon()->PreDraw();

    if (sprite_ && showDebugSprite_) {
        sprite_->Draw();
    }

    context_->GetImGuiManager()->Draw();

    dxCommon->PostDraw();
}
