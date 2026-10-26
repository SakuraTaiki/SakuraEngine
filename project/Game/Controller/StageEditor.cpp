// ============================================================================
// ファイルの役割: 16×16タイル基準のステージ編集、保存・読込、ゲームプレイ実行を統括する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "StageEditor.h"

#include "ModelManager.h"
#include "Object3dCommon.h"
#include "Camera.h"
#include "Input.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cfloat>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include "Json.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

namespace {
constexpr float kPi = 3.14159265358979323846f;
constexpr float kTitleBounceDuration = 0.22f;
constexpr float kTitleStorageStart = 0.18f;
constexpr float kTitleStorageDelay = 0.055f;
constexpr float kTitleStorageDuration = 0.20f;

// 左端、右端、左から2文字目、右から2文字目……の順番を返す。
// 偶数番を左、奇数番を右へ割り当てることで、最後は中央の文字になる。
float GetTitleStorageOrder(size_t index, size_t letterCount) {
    const size_t half = letterCount / 2;
    return static_cast<float>(index < half
        ? index * 2
        : (letterCount - 1 - index) * 2 + 1);
}

const StageEditor::ItemDefinition kItems[] = {
    {1, "Ground", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.32f,0.72f,0.25f,1}, {2.0f,0.5f,1.0f}},
    {2, "Wall", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.72f,0.34f,0.22f,1}, {1.0f,2.0f,1.0f}},
    {3, "Brick", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.72f,0.28f,0.18f,1}, {1.0f,1.0f,1.0f}},
    {4, "PBlock", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.25f,0.55f,0.95f,1}, {1,1,1}},
    {5, "PBlock (On)", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.3f,0.9f,1,1}, {1,1,1}},
    {6, "Crumbling Floor", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.72f,0.55f,0.3f,1}, {1,0.4f,1}},
    {7, "Ice Block", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.65f,0.9f,1,0.85f}, {1,1,1}},
    {8, "Moving Floor", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.62f,0.32f,0.95f,1}, {2,0.3f,1}},
    {9, "Key Block", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.95f,0.72f,0.2f,1}, {1,1,1}},
    {10, "Timed Block", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.9f,0.35f,0.65f,1}, {1,1,1}},
    {11, "On Block", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.95f,0.2f,0.2f,1}, {1,1,1}},
    {12, "Off Block", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.2f,0.45f,0.95f,1}, {1,1,1}},
    {13, "Transparent Block", StageEditor::Category::Basic, "Resources/Editor", "joint_box.obj", {0.7f,0.8f,1,0.4f}, {1,1,1}},
    {14, "Ladder", StageEditor::Category::Gimmick, "Resources/Editor", "joint_box.obj", {0.65f,0.4f,0.18f,1}, {0.5f,1.5f,0.2f}},
    {15, "Star", StageEditor::Category::Gimmick, "Resources/Editor", "ico_sphere.obj", {1,0.9f,0.15f,1}, {0.45f,0.45f,0.45f}},
    {16, "Bubble Pickup", StageEditor::Category::Gimmick, "Resources/Editor", "ico_sphere.obj", {0.45f,0.85f,1,0.65f}, {0.55f,0.55f,0.55f}},
    {17, "Goal", StageEditor::Category::Gimmick, "Resources/Editor", "joint_box.obj", {1,0.85f,0.2f,1}, {0.3f,2,0.3f}},
    {18, "Door", StageEditor::Category::Gimmick, "Resources/Editor", "joint_box.obj", {0.45f,0.2f,0.08f,1}, {0.8f,1.5f,0.2f}},
    {19, "P Switch", StageEditor::Category::Gimmick, "Resources/Editor", "joint_box.obj", {0.15f,0.45f,0.95f,1}, {0.8f,0.25f,0.8f}},
    {20, "Key", StageEditor::Category::Gimmick, "Resources/Editor", "ico_sphere.obj", {1,0.72f,0.1f,1}, {0.35f,0.35f,0.35f}},
    {21, "On/Off Switch", StageEditor::Category::Gimmick, "Resources/Editor", "joint_box.obj", {0.9f,0.25f,0.25f,1}, {0.8f,0.35f,0.8f}},
    {22, "Enemy Walker", StageEditor::Category::Enemy, "Resources/Editor", "ico_sphere.obj", {0.95f,0.48f,0.2f,1}, {0.6f,0.6f,0.6f}},
    {23, "Enemy Flyer", StageEditor::Category::Enemy, "Resources/Editor", "ico_sphere.obj", {0.85f,0.25f,0.85f,1}, {0.55f,0.55f,0.55f}},
    {24, "Enemy Chaser", StageEditor::Category::Enemy, "Resources/Editor", "ico_sphere.obj", {0.95f,0.15f,0.15f,1}, {0.65f,0.65f,0.65f}},
    {25, "Player Start", StageEditor::Category::System, "Resources/Editor", "ico_sphere.obj", {0.2f,0.72f,1,1}, {0.5f,0.5f,0.5f}},
    {26, "Checkpoint", StageEditor::Category::System, "Resources/Editor", "joint_box.obj", {0.25f,1,0.45f,1}, {0.25f,1.4f,0.25f}},
    {27, "Spike", StageEditor::Category::Gimmick, "Resources/Editor", "ico_sphere.obj", {0.95f,0.18f,0.18f,1}, {0.45f,0.45f,0.45f}},
    {28, "Falling Floor", StageEditor::Category::Gimmick, "Resources/Editor", "joint_box.obj", {0.82f,0.48f,0.18f,1}, {1.0f,0.3f,1.0f}},
    {29, "Jump Pad", StageEditor::Category::Gimmick, "Resources/Editor", "joint_box.obj", {0.15f,0.9f,0.95f,1}, {0.9f,0.2f,1.0f}},
    {30, "Conveyor Floor", StageEditor::Category::Gimmick, "Resources/Editor", "joint_box.obj", {0.95f,0.72f,0.12f,1}, {1.0f,0.22f,1.0f}},
    {31, "Save Point", StageEditor::Category::System, "Resources/Editor", "joint_box.obj", {1.0f,0.72f,0.12f,1}, {0.35f,1.2f,0.35f}},
};

float DistanceSquared(const Vector3& a, const Vector3& b) {
    const float x=a.x-b.x, y=a.y-b.y, z=a.z-b.z;
    return x*x+y*y+z*z;
}

Vector4 TransformPoint4(const Vector4& value, const Matrix4x4& matrix) {
    return {
        value.x * matrix.m[0][0] + value.y * matrix.m[1][0] + value.z * matrix.m[2][0] + value.w * matrix.m[3][0],
        value.x * matrix.m[0][1] + value.y * matrix.m[1][1] + value.z * matrix.m[2][1] + value.w * matrix.m[3][1],
        value.x * matrix.m[0][2] + value.y * matrix.m[1][2] + value.z * matrix.m[2][2] + value.w * matrix.m[3][2],
        value.x * matrix.m[0][3] + value.y * matrix.m[1][3] + value.z * matrix.m[2][3] + value.w * matrix.m[3][3]
    };
}
}

// 処理概要: StageEditorが担当する「GridToWorld」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
Vector3 StageEditor::GridToWorld(int gridX, int gridY, float z) {
    return {
        (static_cast<float>(gridX) + 0.5f) * kTileWorldSize,
        (static_cast<float>(gridY) + 0.5f) * kTileWorldSize,
        z
    };
}

// 処理概要: StageEditorが担当する「TileIndex」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
int StageEditor::TileIndex(int gridX, int gridY) const {
    if (gridX < 0 || gridY < 0 || gridX >= stageWidth_ || gridY >= stageHeight_) {
        return -1;
    }
    return gridY * stageWidth_ + gridX;
}

// 処理概要: 外部から必要な状態またはリソース参照を取得する。
// 注意事項: 返す参照やポインターの寿命は所有オブジェクトに従う。
int StageEditor::GetTileAt(int gridX, int gridY) const {
    const int index = TileIndex(gridX, gridY);
    return index >= 0 && index < static_cast<int>(tiles_.size()) ? tiles_[index] : 0;
}

// 処理概要: 現在の状態が指定された条件を満たすか判定する。
// 注意事項: 状態を変更せず、判定結果だけを返す。
bool StageEditor::IsSolidTile(int gridX, int gridY) const {
    return GetTileCollisionType(gridX, gridY) == TileCollisionType::Solid;
}

// 処理概要: 外部から必要な状態またはリソース参照を取得する。
// 注意事項: 返す参照やポインターの寿命は所有オブジェクトに従う。
StageEditor::TileCollisionType StageEditor::GetTileCollisionType(int gridX, int gridY) const {
    // Keep collision semantics separate from the visual item ID. Future tiles
    // such as ladders, one-way platforms and slopes can be added here without
    // rewriting player movement.
    const int index = TileIndex(gridX, gridY);
    if (index >= 0 && index < static_cast<int>(runtimeTileActive_.size()) &&
        runtimeTileActive_[index] == 0) {
        return TileCollisionType::Empty;
    }
    switch (GetTileAt(gridX, gridY)) {
    case 0:
        return TileCollisionType::Empty;
    case 11:
        return onOffActive_ ? TileCollisionType::Solid : TileCollisionType::Empty;
    case 12:
        return onOffActive_ ? TileCollisionType::Empty : TileCollisionType::Solid;
    default:
        return TileCollisionType::Solid;
    }
}

// 処理概要: 現在の状態が指定された条件を満たすか判定する。
// 注意事項: 状態を変更せず、判定結果だけを返す。
bool StageEditor::IsTileItem(int itemId) const {
    switch (itemId) {
    case 1:  // Ground
    case 2:  // Wall
    case 3:  // Brick
    case 6:  // Crumbling Floor
    case 7:  // Ice Block
    case 11: // On Block
    case 12: // Off Block
    case 13: // Transparent Block
        return true;
    default:
        return false;
    }
}

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void StageEditor::Initialize(Object3dCommon* common, uint32_t environmentTexture, float environmentCoefficient) {
    object3dCommon_=common;
    environmentTexture_=environmentTexture;
    environmentCoefficient_=environmentCoefficient;
#if !defined(USE_IMGUI) || defined(PLAYER_RELEASE)
    // Release builds do not provide the editor UI, so starting in Editor
    // mode would leave the placement cursor visible with no way to enter
    // gameplay. Start directly in the runtime mode instead.
    mode_=Mode::GamePlay;
#endif
    ModelManager::Load("Resources/Editor", "joint_box.obj");
    ModelManager::Load("Resources/Editor", "ico_sphere.obj");
    // タイトルは文字ごとのOBJとして読み込み、各文字を独立して動かす。
    constexpr size_t titleLetterCount = 12;
    titleLetterObjects_.reserve(titleLetterCount);
    for (size_t i = 0; i < titleLetterCount; ++i) {
        char fileName[64]{};
        std::snprintf(fileName, sizeof(fileName), "title_letter_%02zu.obj", i);
        Model* letterModel = ModelManager::Load("Resources/Title", fileName);
        auto letterObject = std::make_unique<Object3d>();
        letterObject->Initialize(object3dCommon_);
        letterObject->SetModel(letterModel);
        // テクスチャの控えめな白をそのまま表示するため、照明の影響を受けないようにする。
        letterObject->SetEnableLighting(false);
        letterObject->SetEnvironmentTexture(environmentTexture_);
        letterObject->SetEnvironmentCoefficient(0.0f);
        titleLetterObjects_.push_back(std::move(letterObject));
    }

    Model* titlePromptModel = ModelManager::Load(
        "Resources/Title",
        "press_enter.obj"
    );
    titlePromptObject_ = std::make_unique<Object3d>();
    titlePromptObject_->Initialize(object3dCommon_);
    titlePromptObject_->SetModel(titlePromptModel);
    titlePromptObject_->SetEnableLighting(false);
    titlePromptObject_->SetEnvironmentTexture(environmentTexture_);
    titlePromptObject_->SetEnvironmentCoefficient(0.0f);
    constexpr const char* countdownModelFiles[] = {
        "countdown_1.obj", "countdown_2.obj", "countdown_3.obj"
    };
    countdownDigitObjects_.reserve(std::size(countdownModelFiles));
    for (const char* modelFile : countdownModelFiles) {
        auto digitObject = std::make_unique<Object3d>();
        digitObject->Initialize(object3dCommon_);
        digitObject->SetModel(ModelManager::Load("Resources/Title", modelFile));
        digitObject->SetEnableLighting(false);
        digitObject->SetEnvironmentTexture(environmentTexture_);
        digitObject->SetEnvironmentCoefficient(0.0f);
        countdownDigitObjects_.push_back(std::move(digitObject));
    }
    // 決定パーティクルはDevelopmentでも確認できるよう、専用ブロックを常に読み込む。
    Model* titleAmbientBlockModel = ModelManager::Load(
        "Resources/Title",
        "title_block_particle.obj"
    );
    constexpr Vector3 respawnDirections[] = {
        {-0.95f, 0.75f, 0.10f}, {-0.55f, 1.00f, -0.08f},
        {-0.15f, 0.82f, 0.12f}, {0.25f, 1.05f, -0.10f},
        {0.65f, 0.88f, 0.08f}, {0.98f, 0.62f, -0.12f},
        {-0.82f, 0.25f, -0.06f}, {-0.35f, 0.38f, 0.10f},
        {0.42f, 0.30f, -0.08f}, {0.84f, 0.18f, 0.06f}
    };
    constexpr Vector3 assemblyOffsets[] = {
        {-0.24f, 0.24f, 0.0f}, {0.0f, 0.24f, 0.0f}, {0.24f, 0.24f, 0.0f},
        {-0.24f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}, {0.24f, 0.0f, 0.0f},
        {-0.24f, -0.24f, 0.0f}, {0.0f, -0.24f, 0.0f}, {0.24f, -0.24f, 0.0f},
        {0.0f, 0.48f, 0.0f}
    };
    constexpr Vector4 respawnColors[] = {
        {0.62f, 0.30f, 1.0f, 1.0f},
        {0.12f, 0.88f, 1.0f, 1.0f},
        {0.95f, 0.98f, 1.0f, 1.0f}
    };
    respawnEffectBlocks_.reserve(std::size(respawnDirections));
    for (size_t i = 0; i < std::size(respawnDirections); ++i) {
        RespawnEffectBlock block{};
        block.object = std::make_unique<Object3d>();
        block.object->Initialize(object3dCommon_);
        block.object->SetModel(titleAmbientBlockModel);
        block.object->SetEnableLighting(false);
        block.object->SetEnvironmentTexture(environmentTexture_);
        block.object->SetEnvironmentCoefficient(0.0f);
        block.direction = respawnDirections[i];
        block.assemblyOffset = assemblyOffsets[i];
        block.color = respawnColors[i % std::size(respawnColors)];
        block.rotationSpeed = 2.8f + static_cast<float>(i % 5) * 0.55f;
        respawnEffectBlocks_.push_back(std::move(block));
    }
#ifndef PLAYER_RELEASE
    (void)titleAmbientBlockModel;
#endif
#ifdef PLAYER_RELEASE
    // タイトル専用の小型ブロックOBJを複数生成し、背景に奥行きを作る。
    constexpr Vector3 ambientBlockOffsets[] = {
        {-8.4f,  3.8f, 24.0f}, {-6.9f, -1.9f, 25.5f}, {-5.4f,  4.5f, 27.0f},
        {-3.8f, -3.9f, 26.0f}, {-2.4f,  3.6f, 28.0f}, {-0.8f, -4.5f, 24.8f},
        { 1.0f,  4.4f, 26.8f}, { 2.6f, -3.7f, 28.2f}, { 4.1f,  3.7f, 25.2f},
        { 5.7f, -2.6f, 27.5f}, { 7.2f,  4.2f, 24.5f}, { 8.6f, -1.4f, 26.5f},
        {-9.2f,  0.7f, 28.5f}, {-4.7f,  0.2f, 29.0f}, { 4.9f,  0.5f, 29.2f},
        { 9.3f,  1.3f, 28.0f}
    };
    constexpr Vector4 ambientBlockColors[] = {
        {0.64f, 0.30f, 1.00f, 0.66f}, // vivid violet
        {0.15f, 0.85f, 1.00f, 0.62f}, // cyan
        {1.00f, 0.68f, 0.18f, 0.60f}  // warm gold
    };
    titleAmbientBlocks_.reserve(std::size(ambientBlockOffsets));
    for (size_t i = 0; i < std::size(ambientBlockOffsets); ++i) {
        TitleAmbientBlock block{};
        block.object = std::make_unique<Object3d>();
        block.object->Initialize(object3dCommon_);
        block.object->SetModel(titleAmbientBlockModel);
        block.object->SetEnableLighting(false);
        block.object->SetEnvironmentTexture(environmentTexture_);
        block.object->SetEnvironmentCoefficient(0.0f);
        block.baseOffset = ambientBlockOffsets[i];
        block.color = ambientBlockColors[i % std::size(ambientBlockColors)];
        block.phase = static_cast<float>(i) * 0.73f;
        block.speed = 0.55f + static_cast<float>(i % 5) * 0.09f;
        block.scale = 0.24f + static_cast<float>(i % 4) * 0.055f;
        titleAmbientBlocks_.push_back(std::move(block));
    }
#endif
    Model* cursorFrameModel=ModelManager::Load("Resources/Editor","placement_cursor_frame.obj");
    cursorFrameObject_=std::make_unique<Object3d>();
    cursorFrameObject_->Initialize(object3dCommon_);
    cursorFrameObject_->SetModel(cursorFrameModel);
    cursorFrameObject_->SetColor({0.0f,0.0f,0.0f,1.0f});
    cursorFrameObject_->SetEnableLighting(false);
    cursorFrameObject_->SetEnvironmentTexture(environmentTexture_);
    cursorFrameObject_->SetEnvironmentCoefficient(0.0f);
    playerObject_=std::make_unique<Object3d>();
    playerObject_->Initialize(object3dCommon_);
    playerObject_->SetModel(ModelManager::Load("Resources/Editor", "joint_box.obj"));
    playerObject_->SetColor({0.12f,0.55f,1.0f,1.0f});
    playerObject_->SetEnableLighting(false);
    playerObject_->SetEnvironmentTexture(environmentTexture_);
    playerObject_->SetEnvironmentCoefficient(0.0f);
    NewStage();
    RefreshStageFiles();
    LoadPlaylist();
    RefreshSaveDataAvailability();
#if !defined(USE_IMGUI) || defined(PLAYER_RELEASE)
    // Release版はエディターやプレイリストの選択状態に依存させず、
    // 製品版ステージとして指定したSaveStage.jsonだけを読み込む。
    // ビルド後はResourcesフォルダーごとexeの隣へコピーされるため、
    // 開発環境と配布用exeのどちらでも同じ相対パスを使用できる。
    constexpr const char* releaseStagePath = "Resources/Stages/SaveStage.json";
    if (LoadStage(releaseStagePath)) {
        mode_ = Mode::GamePlay;
        gameFlowState_ = GameFlowState::Title;
        titleLetterAnimationTimer_ = 0.0f;
        status_ = "Release stage loaded: SaveStage.json";
    } else {
        // 読み込みに失敗しても不正な別ステージへ切り替えず、原因をデバッガへ通知する。
        OutputDebugStringA("Release startup failed: Resources/Stages/SaveStage.json could not be loaded.\n");
    }
#endif
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void StageEditor::Finalize() {
    titleAmbientBlocks_.clear();
    respawnEffectBlocks_.clear();
    titleStartParticles_.clear();
    titlePromptObject_.reset();
    countdownDigitObjects_.clear();
    titleLetterObjects_.clear();
    playerObject_.reset();
    cursorFrameObject_.reset();
    cursorObject_.reset();
    tileObjects_.clear();
    objects_.clear();
    placements_.clear();
    object3dCommon_=nullptr;
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void StageEditor::Update(Input* input) {
    if (!active_) {
        return;
    }

    constexpr float deltaTime = 1.0f / 60.0f;
    lifeChangeTimer_ = (std::max)(0.0f, lifeChangeTimer_ - deltaTime);
    if (mode_ == Mode::GamePlay) UpdateGameFlow(input);
    UpdateTitleStartParticles(deltaTime);
    UpdateGoalPreviewEffect();
    if (mode_ == Mode::GamePlay && gameFlowState_ == GameFlowState::Playing &&
        !respawnEffectActive_) UpdateRuntimeObjects(deltaTime);
    for (auto& object:tileObjects_) if(object) object->Update();
    for (auto& object:objects_) if(object) object->Update();
    if (mode_ == Mode::GamePlay && gameFlowState_ == GameFlowState::Playing) {
        UpdatePlayer(input);
    }
    if(cursorObject_ && mode_==Mode::Editor) {
        Placement preview{};
        preview.itemId = selectedItemId_;
        preview.position = cursorPosition_;
        preview.rotation = cursorRotation_;
        preview.scale = cursorScale_;
        ApplyPlacementVisualTransform(cursorObject_.get(), preview, cursorPosition_);
        cursorObject_->Update();
    }
    if(cursorFrameObject_ && mode_==Mode::Editor){
        cursorFrameObject_->SetPosition(cursorPosition_);
        cursorFrameObject_->SetRotation({0,0,0});
        cursorFrameObject_->SetScale({1.04f,1.04f,1.04f});
        cursorFrameObject_->Update();
    }

#ifdef PLAYER_RELEASE
    if (!titleAmbientBlocks_.empty() && mode_ == Mode::GamePlay &&
        gameFlowState_ == GameFlowState::Title) {
        titleAmbientBlockTimer_ += deltaTime;
        Camera* camera = object3dCommon_ ? object3dCommon_->GetDefaultCamera() : nullptr;
        if (camera) {
            const Vector3& cameraPosition = camera->GetTranslate();
            for (size_t i = 0; i < titleAmbientBlocks_.size(); ++i) {
                auto& block = titleAmbientBlocks_[i];
                if (!block.object) continue;

                const float time = titleAmbientBlockTimer_ * block.speed + block.phase;
                // 縦方向を主軸にゆっくり漂わせ、横方向は小さく揺らして文字を遮らない。
                const float transitionProgress = startTransitionActive_
                    ? std::clamp(
                        startTransitionTimer_ / kStartTransitionBuildDuration,
                        0.0f,
                        1.0f)
                    : 0.0f;
                const float escapeDirection = block.baseOffset.x < 0.0f ? -1.0f : 1.0f;
                block.object->SetPosition({
                    cameraPosition.x + block.baseOffset.x + std::sin(time * 0.7f) * 0.22f +
                        escapeDirection * transitionProgress * 7.5f,
                    cameraPosition.y + block.baseOffset.y + std::sin(time) * 0.42f,
                    cameraPosition.z + block.baseOffset.z + transitionProgress * 4.0f
                });
                block.object->SetRotation({
                    time * 0.45f,
                    time * 0.62f,
                    time * 0.30f
                });
                const float pulse = 0.88f + 0.12f * std::sin(time * 1.4f);
                const float scale = block.scale * pulse;
                block.object->SetScale({scale, scale, scale});
                Vector4 transitionColor = block.color;
                transitionColor.w *= 1.0f - transitionProgress * 0.75f;
                block.object->SetColor(transitionColor);
                block.object->Update();
            }
        }
    } else {
        titleAmbientBlockTimer_ = 0.0f;
    }
#endif

    if (!titleLetterObjects_.empty() && mode_ == Mode::GamePlay &&
        gameFlowState_ == GameFlowState::Title) {
        titleLetterAnimationTimer_ += deltaTime;
        Camera* camera = object3dCommon_ ? object3dCommon_->GetDefaultCamera() : nullptr;
        if (camera) {
            const Vector3& cameraPosition = camera->GetTranslate();
            constexpr float targetOffsets[] = {
                -6.564614f, -5.441343f, -4.619050f, -3.851377f,
                -2.676099f, -0.524542f,  0.656207f,  1.864047f,
                 2.959787f,  4.167627f,  5.438631f,  6.567530f
            };
            constexpr float letterDelay = 0.08f;
            constexpr float letterDuration = 0.90f;

#ifdef PLAYER_RELEASE
            // タイトル画面を展示用に繰り返すRelease版では、最後の文字が整列した後も
            // すぐに巻き戻さず、完成したロゴを確認できる時間を設けてから再演する。
            constexpr float completedTitleHoldDuration = 2.5f;
            const float titleAnimationDuration = letterDuration +
                static_cast<float>(titleLetterObjects_.size() - 1) * letterDelay;
            if (titleLetterAnimationTimer_ >=
                titleAnimationDuration + completedTitleHoldDuration) {
                titleLetterAnimationTimer_ = 0.0f;
            }
#endif

            for (size_t i = 0; i < titleLetterObjects_.size(); ++i) {
                auto& letter = titleLetterObjects_[i];
                if (!letter) continue;

                // 文字ごとに開始時刻をずらし、左から順番に飛び込んでくる流れを作る。
                const float localTime = titleLetterAnimationTimer_ - static_cast<float>(i) * letterDelay;
                const float progress = std::clamp(localTime / letterDuration, 0.0f, 1.0f);
                // 終点付近でゆっくり止まるイーズアウトにし、文字が整列する瞬間を見やすくする。
                const float inverse = 1.0f - progress;
                const float eased = 1.0f - inverse * inverse * inverse;
                const float remaining = 1.0f - eased;
                const float direction = (i % 2 == 0) ? 1.0f : -1.0f;
                float bounce = 0.0f;
                float storageProgress = 0.0f;
                if (startTransitionActive_) {
                    const float bounceProgress = std::clamp(
                        startTransitionTimer_ / kTitleBounceDuration, 0.0f, 1.0f);
                    // 最初にタイトル全体を一度だけ手前へ弾ませ、決定の手応えを作る。
                    bounce = std::sin(bounceProgress * kPi);
                    const float storageBegin = kTitleStorageStart +
                        GetTitleStorageOrder(i, titleLetterObjects_.size()) * kTitleStorageDelay;
                    storageProgress = std::clamp(
                        (startTransitionTimer_ - storageBegin) / kTitleStorageDuration,
                        0.0f, 1.0f);
                }
                const float storageEase = storageProgress * storageProgress *
                    (3.0f - 2.0f * storageProgress);

                letter->SetPosition({
                    cameraPosition.x + targetOffsets[i],
                    cameraPosition.y + 2.2f + direction * remaining * 1.1f,
                    // カメラ奥から最終位置へ近づけ、遠近感でも文字を小さく見せる。
                    // 決定後は左右端から順に奥へ収納し、壁の材料になる流れを作る。
                    cameraPosition.z + 22.0f + remaining * 24.0f - bounce * 1.35f +
                        storageEase * 18.0f
                });
                letter->SetRotation({
                    kPi * 0.5f + direction * remaining * kPi * 2.0f,
                    remaining * kPi * 1.5f + storageEase * kPi,
                    direction * remaining * kPi + direction * storageEase * kPi * 0.35f
                });
                const float storageScale = (1.0f + bounce * 0.12f) *
                    (1.0f - storageEase * 0.94f);
                letter->SetScale({-storageScale, storageScale, storageScale});
                // 青みを含んだ白に統一し、紫の背景から輪郭を浮かせる。
                // 飛来中だけ少しシアンを強め、整列すると落ち着いた白へ戻す。
                letter->SetColor({
                    0.90f+eased*0.08f,
                    0.96f+eased*0.03f,
                    1.00f,
                    1.00f - storageEase});
                letter->Update();
            }
        }
    } else if (gameFlowState_ != GameFlowState::Title) {
        titleLetterAnimationTimer_ = 0.0f;
    }

    if (titlePromptObject_ && mode_ == Mode::GamePlay &&
        gameFlowState_ == GameFlowState::Title) {
        titlePromptBlinkTimer_ += deltaTime;
        Camera* camera = object3dCommon_ ? object3dCommon_->GetDefaultCamera() : nullptr;
        if (camera) {
            const Vector3& cameraPosition = camera->GetTranslate();
            titlePromptObject_->SetPosition({
                cameraPosition.x,
                // 現在のカメラ距離と720px基準の表示範囲では、約2.8ワールド単位が
                // 画面上のおよそ200pxに相当するため、従来位置からその分だけ下げる。
                cameraPosition.y - 2.95f +
                    std::sin(titlePromptBlinkTimer_ * (2.0f * kPi / 2.2f)) * 0.15f,
                cameraPosition.z + 21.8f
            });
            // ゲーム名と同じ座標変換を適用し、正面から正しく読める向きにする。
            titlePromptObject_->SetRotation({kPi * 0.5f, 0.0f, 0.0f});
            titlePromptObject_->SetScale({-1.0f, 1.0f, 1.0f});

            // 約1.6秒周期で明暗を滑らかに往復させる。
            // 完全に消さず最低25%を残し、入力案内を見失わないようにする。
            constexpr float blinkCycleSeconds = 1.6f;
            const float wave = 0.5f + 0.5f * std::sin(
                titlePromptBlinkTimer_ * (2.0f * kPi / blinkCycleSeconds)
            );
            const float transitionFade = startTransitionActive_
                ? 1.0f - std::clamp(
                    startTransitionTimer_ / (kStartTransitionBuildDuration * 0.56f),
                    0.0f,
                    1.0f)
                : 1.0f;
            const float alpha = (0.25f + wave * 0.75f) * transitionFade;
            // 入力案内はタイトルより一段鮮やかなシアンにし、視線の優先順位を作る。
            titlePromptObject_->SetColor({0.30f, 0.90f, 1.0f, alpha});
            titlePromptObject_->Update();
        }
    } else {
        titlePromptBlinkTimer_ = 0.0f;
    }

    if (mode_ == Mode::GamePlay && gameFlowState_ == GameFlowState::Countdown &&
        countdownDigitObjects_.size() >= 3) {
        const float beatDuration = kStartCountdownDuration / 3.0f;
        const int beatIndex = (std::min)(2, static_cast<int>(startCountdownTimer_ / beatDuration));
        const int count = 3 - beatIndex;
        const float beatProgress = std::fmod(startCountdownTimer_, beatDuration) / beatDuration;
        if (countdownImpactBeat_ != beatIndex) {
            countdownImpactBeat_ = beatIndex;
            countdownImpactEmitted_ = false;
        }
        Camera* camera = object3dCommon_ ? object3dCommon_->GetDefaultCamera() : nullptr;
        if (camera) {
            const Vector3& cameraPosition = camera->GetTranslate();
            auto& digit = countdownDigitObjects_[static_cast<size_t>(count - 1)];

            // 1拍を「奥から接近 → 着地 → 静止 → 奥へ収納」に分ける。
            constexpr float arrivalEnd = 0.28f;
            constexpr float impactEnd = 0.50f;
            constexpr float exitStart = 0.78f;
            const auto smoothStep = [](float value) {
                const float t = std::clamp(value, 0.0f, 1.0f);
                return t * t * (3.0f - 2.0f * t);
            };

            float scaleX = 0.52f;
            float scaleY = 0.52f;
            float scaleZ = 0.52f;
            float depthOffset = 0.0f;
            float rotationY = 0.0f;
            float rotationZ = 0.0f;
            Vector4 digitColor{0.86f, 1.0f, 0.92f, 1.0f};

            if (beatProgress < arrivalEnd) {
                const float progress = smoothStep(beatProgress / arrivalEnd);
                const float approachScale = 0.16f + 0.42f * progress;
                scaleX = approachScale;
                scaleY = approachScale;
                scaleZ = approachScale;
                depthOffset = 5.5f * (1.0f - progress);
                rotationY = (1.0f - progress) * kPi * 2.0f;
                rotationZ = (1.0f - progress) * kPi * 0.22f;
                digitColor = {0.76f, 0.88f, 1.0f, 0.72f + progress * 0.28f};
            } else if (beatProgress < impactEnd) {
                const float progress = (beatProgress - arrivalEnd) / (impactEnd - arrivalEnd);
                const float bounce = std::sin(progress * kPi) * (1.0f - progress);
                // 着地直後だけ横へ広がり、縦へ潰れてから元の形へ戻す。
                scaleX = 0.52f + bounce * 0.13f;
                scaleY = 0.52f - bounce * 0.11f;
                scaleZ = 0.52f + bounce * 0.05f;
                digitColor = count == 1
                    ? Vector4{1.0f, 0.73f + progress * 0.16f, 0.20f, 1.0f}
                    : Vector4{0.18f + progress * 0.68f, 0.92f, 1.0f, 1.0f};
                if (!countdownImpactEmitted_) {
                    EmitCountdownImpactParticles(count);
                    countdownImpactEmitted_ = true;
                }
            } else if (beatProgress >= exitStart) {
                const float progress = smoothStep(
                    (beatProgress - exitStart) / (1.0f - exitStart));
                const float shrink = 1.0f - progress;
                scaleX = 0.52f * shrink;
                scaleY = 0.52f * shrink;
                scaleZ = 0.52f * shrink;
                depthOffset = 4.5f * progress;
                rotationY = progress * kPi * 1.2f;
                rotationZ = -progress * kPi * 0.16f;
                digitColor.w = 1.0f - progress;
            }

            digit->SetPosition({
                cameraPosition.x,
                cameraPosition.y + 0.45f,
                cameraPosition.z + 21.2f + depthOffset
            });
            // Blender生成の数字OBJはX-Y平面が正面で、Z方向が厚み。
            // タイトル文字用のX軸90度回転を流用すると側面しか見えないため、
            // 数字は無回転でカメラへ正面を向け、Blenderとエンジンで逆になる
            // 横方向だけX軸の負スケールで補正する。
            digit->SetRotation({0.0f, rotationY, rotationZ});
            digit->SetScale({-scaleX, scaleY, scaleZ});
            digit->SetColor(digitColor);
            digit->Update();
        }
    }
}

// 処理概要: 決定入力時にPRESS ENTERの周囲へ初動パーティクルを生成する。
// 注意事項: 文字の消滅とは分離し、白・シアン・金色の短い決定演出として扱う。
void StageEditor::EmitTitleStartParticles() {
    Camera* camera = object3dCommon_ ? object3dCommon_->GetDefaultCamera() : nullptr;
    // 専用の小型ブロックを使い、決定入力を鮮やかに見せる。
    Model* particleModel = ModelManager::Find(
        "Resources/Title/title_block_particle.obj"
    );
    if (!camera || !particleModel) {
        return;
    }

    titleStartParticles_.clear();
    constexpr int promptParticleCount = 24;
    const Vector3& cameraPosition = camera->GetTranslate();
    const Vector3 origin{
        cameraPosition.x,
        cameraPosition.y - 2.95f,
        cameraPosition.z + 21.5f
    };
    const Vector4 colors[] = {
        {0.95f, 0.98f, 1.00f, 1.0f},
        {0.15f, 0.88f, 1.00f, 1.0f},
        {1.00f, 0.72f, 0.20f, 1.0f}
    };

    titleStartParticles_.reserve(promptParticleCount);
    for (int i = 0; i < promptParticleCount; ++i) {
        const float angle = (2.0f * kPi * static_cast<float>(i)) /
            static_cast<float>(promptParticleCount);
        const float speed = 2.2f + static_cast<float>((i * 7) % 6) * 0.18f;

        TitleStartParticle particle{};
        particle.object = std::make_unique<Object3d>();
        particle.object->Initialize(object3dCommon_);
        particle.object->SetModel(particleModel);
        particle.object->SetEnableLighting(false);
        particle.object->SetEnvironmentTexture(environmentTexture_);
        particle.object->SetEnvironmentCoefficient(0.0f);
        particle.position = origin;
        particle.velocity = {
            std::cos(angle) * speed,
            std::sin(angle) * speed * 0.72f + 0.35f,
            -0.15f - static_cast<float>(i % 3) * 0.05f
        };
        particle.color = colors[i % 3];
        particle.lifetime = 0.65f + static_cast<float>(i % 5) * 0.045f;
        particle.baseScale = 0.08f + static_cast<float>(i % 4) * 0.025f;
        particle.rotationSpeed = 3.0f + static_cast<float>(i % 7) * 0.55f;
        particle.object->SetPosition(particle.position);
        particle.object->SetScale({
            particle.baseScale,
            particle.baseScale,
            particle.baseScale
        });
        particle.object->SetColor(particle.color);
        particle.object->Update();
        titleStartParticles_.push_back(std::move(particle));
    }
}

// 処理概要: カウント数字が着地した瞬間、周囲へ小型ブロックを飛散させる。
// 注意事項: 1拍につき一度だけ呼び出し、1のときだけ金色を強めてGOへつなげる。
void StageEditor::EmitCountdownImpactParticles(int count) {
    Camera* camera = object3dCommon_ ? object3dCommon_->GetDefaultCamera() : nullptr;
    Model* particleModel = ModelManager::Find(
        "Resources/Title/title_block_particle.obj"
    );
    if (!camera || !particleModel) return;

    constexpr int particleCount = 6;
    const Vector3& cameraPosition = camera->GetTranslate();
    const Vector3 origin{
        cameraPosition.x,
        cameraPosition.y + 0.45f,
        cameraPosition.z + 21.0f
    };

    for (int i = 0; i < particleCount; ++i) {
        const float angle = (2.0f * kPi * static_cast<float>(i)) /
            static_cast<float>(particleCount) + kPi * 0.08f;
        TitleStartParticle particle{};
        particle.object = std::make_unique<Object3d>();
        particle.object->Initialize(object3dCommon_);
        particle.object->SetModel(particleModel);
        particle.object->SetEnableLighting(false);
        particle.object->SetEnvironmentTexture(environmentTexture_);
        particle.object->SetEnvironmentCoefficient(0.0f);
        particle.position = origin;
        particle.velocity = {
            std::cos(angle) * (1.35f + static_cast<float>(i % 2) * 0.25f),
            std::sin(angle) * 1.05f + 0.18f,
            -0.08f - static_cast<float>(i % 3) * 0.035f
        };
        if (count == 1) {
            particle.color = i % 2 == 0
                ? Vector4{1.0f, 0.70f, 0.16f, 1.0f}
                : Vector4{1.0f, 0.94f, 0.66f, 1.0f};
        } else {
            particle.color = i % 2 == 0
                ? Vector4{0.12f, 0.88f, 1.0f, 1.0f}
                : Vector4{0.90f, 0.98f, 1.0f, 1.0f};
        }
        particle.lifetime = 0.42f + static_cast<float>(i % 3) * 0.04f;
        particle.baseScale = 0.065f + static_cast<float>(i % 2) * 0.018f;
        particle.rotationSpeed = 4.2f + static_cast<float>(i) * 0.45f;
        particle.object->SetPosition(particle.position);
        particle.object->SetScale({
            particle.baseScale, particle.baseScale, particle.baseScale});
        particle.object->SetColor(particle.color);
        particle.object->Update();
        titleStartParticles_.push_back(std::move(particle));
    }
}

// 処理概要: 放射したタイトル粒子を広げながら縮小・消滅させる。
// 注意事項: ゲーム状態がタイトルから切り替わっても、寿命までは更新して自然に消す。
void StageEditor::UpdateTitleStartParticles(float deltaTime) {
    for (auto& particle : titleStartParticles_) {
        particle.age += deltaTime;
        particle.velocity.y -= 0.9f * deltaTime;
        particle.position.x += particle.velocity.x * deltaTime;
        particle.position.y += particle.velocity.y * deltaTime;
        particle.position.z += particle.velocity.z * deltaTime;

        const float progress = std::clamp(
            particle.age / particle.lifetime,
            0.0f,
            1.0f
        );
        const float scale = particle.baseScale * (1.0f - progress * 0.65f);
        Vector4 color = particle.color;
        color.w = 1.0f - progress;

        particle.object->SetPosition(particle.position);
        particle.object->SetRotation({
            particle.age * particle.rotationSpeed,
            particle.age * particle.rotationSpeed * 0.7f,
            particle.age * particle.rotationSpeed * 0.4f
        });
        particle.object->SetScale({scale, scale, scale});
        particle.object->SetColor(color);
        particle.object->Update();
    }

    std::erase_if(
        titleStartParticles_,
        [](const TitleStartParticle& particle) {
            return particle.age >= particle.lifetime;
        }
    );
}

// 処理概要: ステージ紹介でゴールを映している間、ゴール本体を発光するように強調する。
// 注意事項: 強調区間外では色・大きさ・ライティングを必ず通常値へ戻す。
void StageEditor::UpdateGoalPreviewEffect() {
    const float progress=GetIntroCameraProgress();
    const bool highlight=
        mode_==Mode::GamePlay && gameFlowState_==GameFlowState::IntroCamera &&
        progress>=0.58f && progress<0.70f;
    const float phase=highlight ? (progress-0.58f)/(0.70f-0.58f) : 0.0f;
    const float pulse=highlight ? 0.5f+0.5f*std::sin(phase*kPi*4.0f) : 0.0f;

    for(size_t i=0;i<placements_.size() && i<objects_.size();++i){
        if(placements_[i].itemId!=17 || !objects_[i]) continue;
        Object3d* goal=objects_[i].get();
        const Vector3 visualScale = GetPlacementModelScale(placements_[i]);
        if(highlight){
            // ライトの影響を一時的に外して鮮やかな黄色を保ち、わずかな拡縮で発光感を出す。
            const float scale=1.08f+pulse*0.10f;
            goal->SetColor({1.0f,1.0f,0.38f,1.0f});
            goal->SetScale({
                visualScale.x*scale,
                visualScale.y*scale,
                visualScale.z*scale});
            goal->SetEnableLighting(false);
            goal->SetEnvironmentCoefficient(0.0f);
        }else{
            const ItemDefinition* definition=FindItem(17);
            if(definition) goal->SetColor(definition->color);
            goal->SetScale(visualScale);
            goal->SetEnableLighting(true);
            goal->SetEnvironmentCoefficient(environmentCoefficient_);
        }
    }
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void StageEditor::Draw3D() {
    if (!active_) {
        return;
    }

    // タイトル画面では、読み込み済みのステージを描画しない。
    // ステージデータ自体はメモリ上に保持しておき、フェード中央で
    // IntroCameraへ切り替わったフレームから描画を開始する。
    const bool drawStageContents =
        mode_ != Mode::GamePlay || gameFlowState_ != GameFlowState::Title;

    if (drawStageContents) {
        for (size_t i = 0; i < tileObjects_.size(); ++i) {
            if (!tileObjects_[i]) continue;
            if (mode_ == Mode::GamePlay) {
                if (i < runtimeTileActive_.size() && runtimeTileActive_[i] == 0) continue;
                const int itemId = i < tiles_.size() ? tiles_[i] : 0;
                if ((itemId == 11 && !onOffActive_) || (itemId == 12 && onOffActive_)) continue;
            }
            tileObjects_[i]->Draw();
        }

        for (size_t i = 0; i < objects_.size(); ++i) {
            // Player Start is an editor marker, not a runtime object.
            if (mode_ == Mode::GamePlay &&
                (i >= placements_.size() || placements_[i].itemId == 25 || !IsRuntimePlacementActive(i))) continue;
            if (objects_[i]) objects_[i]->Draw();
        }

        if (playerObject_ && mode_ == Mode::GamePlay && !respawnEffectActive_ &&
            gameFlowState_ != GameFlowState::GameOver) {
            playerObject_->Draw();
        }
        if (mode_ == Mode::GamePlay && respawnEffectActive_) {
            for (auto& block : respawnEffectBlocks_) {
                if (block.object) block.object->Draw();
            }
        }
    }
    // 配置済みの実体と半透明プレビューを同じ座標へ描画すると、
    // 深度値が競合して一瞬別画像のようにちらつく（Z-fighting）。
    // 既に配置されている場所ではプレビュー本体だけを隠し、
    // この後に描画する外枠カーソルは残して現在位置を確認できるようにする。
    bool cursorOverPlacedItem = false;
    if (mode_ == Mode::Editor) {
        if (IsTileItem(selectedItemId_)) {
            const int gridX = static_cast<int>(
                std::floor(cursorPosition_.x / kTileWorldSize));
            const int gridY = static_cast<int>(
                std::floor(cursorPosition_.y / kTileWorldSize));
            cursorOverPlacedItem = GetTileAt(gridX, gridY) != 0;
        } else {
            cursorOverPlacedItem = FindNearestPlacement(0.25f) >= 0;
        }
    }

    if (cursorObject_ && mode_ == Mode::Editor && !cursorOverPlacedItem) {
        cursorObject_->Draw();
    }
    if(cursorFrameObject_ && mode_==Mode::Editor) cursorFrameObject_->Draw();

    // 背景ブロックを先に描画し、タイトル文字の読みやすさを最前面で保つ。
#ifdef PLAYER_RELEASE
    if (mode_ == Mode::GamePlay && gameFlowState_ == GameFlowState::Title) {
        for (auto& block : titleAmbientBlocks_) {
            if (block.object) block.object->Draw();
        }
    }
#endif

    // ゲーム名はタイトル画面だけに表示し、開始後のプレイ画面には残さない。
    if (mode_ == Mode::GamePlay && gameFlowState_ == GameFlowState::Title) {
        for (size_t i = 0; i < titleLetterObjects_.size(); ++i) {
            auto& letter = titleLetterObjects_[i];
            const float storageEnd = kTitleStorageStart +
                GetTitleStorageOrder(i, titleLetterObjects_.size()) * kTitleStorageDelay +
                kTitleStorageDuration;
            // 収納完了後の極小モデルを描くと黒い点が残るため、完全に描画を止める。
            const bool stored = startTransitionActive_ && startTransitionTimer_ >= storageEnd;
            if (letter && !stored) letter->Draw();
        }
    }
    if (titlePromptObject_ && mode_ == Mode::GamePlay &&
        gameFlowState_ == GameFlowState::Title) {
        titlePromptObject_->Draw();
    }
    if (mode_ == Mode::GamePlay && gameFlowState_ == GameFlowState::Countdown &&
        countdownDigitObjects_.size() >= 3) {
        const float beatDuration = kStartCountdownDuration / 3.0f;
        const int beatIndex = (std::min)(2, static_cast<int>(startCountdownTimer_ / beatDuration));
        const int count = 3 - beatIndex;
        auto& digit = countdownDigitObjects_[static_cast<size_t>(count - 1)];
        if (digit) digit->Draw();
    }
    for (auto& particle : titleStartParticles_) {
        if (particle.object) {
            particle.object->Draw();
        }
    }
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void StageEditor::Draw() {
#ifdef USE_IMGUI
    if (!active_) {
        return;
    }

    ImGui::SetNextWindowSize({330,720},ImGuiCond_FirstUseEver);
    if(!ImGui::Begin("3D Stage Editor")){ImGui::End();return;}
    DrawModeSwitcher();
    ImGui::Separator();
    if(mode_==Mode::Editor){
        DrawPalette();
        DrawSelectedItemSettings();
        DrawTransformPanel();
        ImGui::Separator();
        if(ImGui::Button("PLACE 3D OBJECT",{-1,36}))PlaceItem();
        if(ImGui::Button("REMOVE NEAREST",{-1,30}))RemoveNearest();
        if(ImGui::Button("ROTATE Y 90 deg",{-1,28}))RotateCursor();
        if(ImGui::Button("Undo"))Undo(); ImGui::SameLine(); if(ImGui::Button("Redo"))Redo();
        DrawFilePanel();
        DrawPlaylistManager();
    }else{
        ImGui::TextWrapped("GamePlayMode: placement cursor and editor input are disabled. Placed 3D objects remain in the stage.");
        ImGui::TextWrapped("Player: A/D or Left/Right to move, Enter/Space or gamepad A to jump. W/S climbs ladders and Up enters doors.");
        ImGui::Text("Player tile: %d, %d%s",
            static_cast<int>(std::floor(playerPosition_.x / kTileWorldSize)),
            static_cast<int>(std::floor(playerPosition_.y / kTileWorldSize)),
            playerGrounded_ ? " (grounded)" : "");
        ImGui::Text("Stars: %d  Bubbles: %d  Keys: %d",collectedStars_,collectedBubbles_,keyCount_);
        if(gameFlowState_==GameFlowState::Title) ImGui::TextColored({0.25f,0.75f,1.0f,1.0f},"TITLE: press Enter / gamepad A to start");
        if(goalReached_) ImGui::TextColored({0.2f,1.0f,0.35f,1.0f},"GOAL CLEAR!");
        ImGui::Text("Placed objects: %d",static_cast<int>(placements_.size()));
    }
    ImGui::Separator(); ImGui::TextWrapped("%s",status_.c_str());
    ImGui::End();
#endif
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void StageEditor::DrawModeSwitcher() {
#ifdef USE_IMGUI
    ImGui::TextUnformatted("MODE");
    int modeIndex=mode_==Mode::Editor?0:1;
    const char* modeNames[]={"StageEditor","GamePlay"};
    if(ImGui::Combo("App Mode",&modeIndex,modeNames,2))SetMode(modeIndex==0?Mode::Editor:Mode::GamePlay);
    ImGui::PushStyleColor(ImGuiCol_Button,mode_==Mode::Editor?ImVec4(.15f,.55f,.9f,1):ImVec4(.18f,.2f,.24f,1));
    if(ImGui::Button("EditorMode",{150,38}))SetMode(Mode::Editor);
    ImGui::PopStyleColor(); ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button,mode_==Mode::GamePlay?ImVec4(.18f,.7f,.35f,1):ImVec4(.18f,.2f,.24f,1));
    if(ImGui::Button("GamePlayMode",{150,38}))SetMode(Mode::GamePlay);
    ImGui::PopStyleColor();
#endif
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void StageEditor::DrawPalette() {
#ifdef USE_IMGUI
    const char* categories[]={"Basic Blocks","Gimmicks & Interactables","Enemies","System"};
    if(ImGui::BeginTabBar("BlockCategoryTabs")){
        for(int categoryIndex=0;categoryIndex<4;++categoryIndex){
            if(ImGui::BeginTabItem(categories[categoryIndex])){
                category_=static_cast<Category>(categoryIndex);
                int column=0;
                for(const auto& item:kItems){
                    if(item.category!=category_)continue;
                    if(column++%2)ImGui::SameLine();
                    ImGui::PushID(item.id);
                    const bool selected=selectedItemId_==item.id;
                    if(selected)ImGui::PushStyleColor(ImGuiCol_Button,{0.2f,0.6f,0.2f,1});
                    if(ImGui::Button(item.name,{145,30})){selectedItemId_=item.id;cursorScale_=IsTileItem(item.id)?Vector3{1,1,1}:item.defaultScale;UpdateCursorObject();}
                    if(selected)ImGui::PopStyleColor();
                    ImGui::PopID();
                }
                ImGui::EndTabItem();
            }
        }
        ImGui::EndTabBar();
    }
#endif
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void StageEditor::DrawSelectedItemSettings(){
#ifdef USE_IMGUI
    if(selectedItemId_==8&&ImGui::CollapsingHeader("Moving Floor Settings",ImGuiTreeNodeFlags_DefaultOpen))
        ImGui::DragFloat3("Move Offset",&movingFloorOffset_.x,1.0f,-20.0f,20.0f,"%.0f");
    if(selectedItemId_==18&&ImGui::CollapsingHeader("Door Settings",ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::SliderInt("Door ID Number",&selectedDoorId_,1,9,"ID: %d");ImGui::TextWrapped("Doors with the same ID connect to each other.");
    }
    if((selectedItemId_==4||selectedItemId_==5||selectedItemId_==19)&&ImGui::CollapsingHeader("P Switch Settings",ImGuiTreeNodeFlags_DefaultOpen))
        ImGui::SliderInt("P Switch ID Number",&selectedSwitchId_,1,9,"ID: %d");
    if(selectedItemId_==10&&ImGui::CollapsingHeader("Timed Block Settings",ImGuiTreeNodeFlags_DefaultOpen)){
        ImGui::SliderInt("Group ID",&selectedTimedGroupId_,1,9);ImGui::SliderInt("Order ID",&selectedTimedOrderId_,0,9);
    }
    if(selectedItemId_==30&&ImGui::CollapsingHeader("Conveyor Settings",ImGuiTreeNodeFlags_DefaultOpen)){
        if(ImGui::RadioButton("Move Left",selectedConveyorDirection_<0))selectedConveyorDirection_=-1;
        ImGui::SameLine();
        if(ImGui::RadioButton("Move Right",selectedConveyorDirection_>0))selectedConveyorDirection_=1;
        ImGui::TextDisabled("Carries the player while standing on top.");
    }
#endif
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void StageEditor::DrawTransformPanel() {
#ifdef USE_IMGUI
    if(!ImGui::CollapsingHeader("16 x 16 Tile Placement",ImGuiTreeNodeFlags_DefaultOpen))return;
    const int gridX = static_cast<int>(std::floor(cursorPosition_.x / kTileWorldSize));
    const int gridY = static_cast<int>(std::floor(cursorPosition_.y / kTileWorldSize));
    ImGui::Text("Grid: %d, %d", gridX, gridY);
    ImGui::Text("Design pixels: %d, %d", gridX * kTileSizePixels, gridY * kTileSizePixels);
    ImGui::Checkbox("Show 16x16 grid", &showGrid_);
    if (!IsTileItem(selectedItemId_)) {
        ImGui::DragFloat3("Object Rotation",&cursorRotation_.x,0.01f);
        ImGui::DragFloat3("Logical Size / Collider",&cursorScale_.x,0.02f,0.02f,100.0f);
        ImGui::TextDisabled("Model correction is defined per item, not per stage object.");
    }
    ImGui::TextDisabled("Left click/drag: place   Right click/drag: erase");
    ImGui::TextDisabled("WASD: move cursor   Enter: place   Ctrl+Z/Y: history");
#endif
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void StageEditor::DrawGameView(float x,float y,float width,float height) {
#ifdef USE_IMGUI
    if (!active_) {
        gameViewHovered_ = false;
        return;
    }

#ifdef PLAYER_RELEASE
    // Release版はゲーム画像を直接ウィンドウへ表示するため、編集用のItemは存在しない。
    // 前景レイヤーへゲーム演出だけを描き、エディター入力は完全に無効化する。
    gameViewHovered_ = false;
    ImDrawList* draw = ImGui::GetForegroundDrawList();
#else
    gameViewHovered_=ImGui::IsItemHovered();

    // ゲームビュー画像の上で回したホイールは、この画像の操作として扱う。
    // ImGuiに所有権を通知しておくことで、ステージをズームしたい時に
    // 背後のStageEditorメニューまで同時に上下スクロールすることを防ぐ。
    if (gameViewHovered_) {
        ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
    }

    HandleEditorInput(x, y, width, height);
    ImDrawList* draw=ImGui::GetWindowDrawList();
#endif
    const bool editing=mode_==Mode::Editor;
#ifndef PLAYER_RELEASE
    const ImU32 color=editing?IM_COL32(38,145,235,235):IM_COL32(35,185,85,235);
    draw->AddRectFilled({x+10,y+10},{x+174,y+40},color,5);
    draw->AddText({x+20,y+18},IM_COL32_WHITE,editing?"EDITOR MODE":"GAMEPLAY MODE");
#endif
    if(editing){
        char buffer[160]{};
        const int gridX=static_cast<int>(std::floor(cursorPosition_.x/kTileWorldSize));
        const int gridY=static_cast<int>(std::floor(cursorPosition_.y/kTileWorldSize));
        snprintf(buffer,sizeof(buffer),"Tile %d, %d  (%d px, %d px)",gridX,gridY,gridX*kTileSizePixels,gridY*kTileSizePixels);
        draw->AddRectFilled({x+10,y+46},{x+250,y+72},IM_COL32(10,12,18,205),4);
        draw->AddText({x+18,y+52},IM_COL32(255,230,70,255),buffer);
        DrawGridOverlay(x, y, width, height);
    } else if (gameFlowState_ == GameFlowState::Title) {
        // 濃紺から赤紫へ薄く色を重ね、天球の階調を残しながら彩度を補強する。
        // 旧仮タイトル文字は、OBJ版「PRISM PASSAGE」と重なるため描画しない。
        draw->AddRectFilledMultiColor(
            {x,y},{x+width,y+height},
            IM_COL32(10,20,72,48),IM_COL32(10,20,72,48),
            IM_COL32(118,35,126,42),IM_COL32(118,35,126,42));
        // Release版は新規ゲームとしてSaveStage.jsonを開始するため、
        // 開発確認用のコンティニュー案内をタイトル画面へ表示しない。
#ifndef PLAYER_RELEASE
        if(hasSaveData_){
            const char* resume="C / GAMEPAD Y : CONTINUE FROM SAVE POINT";
            const ImVec2 resumeSize=ImGui::CalcTextSize(resume);
            draw->AddText({x+(width-resumeSize.x)*0.5f,y+height*0.55f},IM_COL32(255,220,80,255),resume);
        }
#endif
    } else if (gameFlowState_ == GameFlowState::Clear) {
        draw->AddRectFilled({x,y},{x+width,y+height},IM_COL32(4,18,10,220));
        const char* clear="STAGE CLEAR!";
        const ImVec2 clearSize=ImGui::CalcTextSize(clear);
        draw->AddText({x+(width-clearSize.x)*0.5f,y+height*0.32f},IM_COL32(90,255,120,255),clear);
        const char* returning="RETURNING TO TITLE...";
        const ImVec2 returningSize=ImGui::CalcTextSize(returning);
        draw->AddText(
            {x+(width-returningSize.x)*0.5f,y+height*0.50f},
            IM_COL32_WHITE,
            returning);
    } else if (gameFlowState_ == GameFlowState::GameOver) {
        const auto smoothStep=[](float value){
            const float t=std::clamp(value,0.0f,1.0f);
            return t*t*(3.0f-2.0f*t);
        };
        const float exitProgress=gameOverExitActive_
            ? smoothStep(gameOverExitTimer_/kGameOverExitDuration):0.0f;
        const float centerX=x+width*0.5f;
        const float centerY=y+height*0.5f;

        // 大きなブロックが四辺から飛来して画面全体を覆う。
        // 従来の半透明パネルよりプレイ画面との差を強くし、ゲーム終了を一目で伝える。
        constexpr int columns=12;
        constexpr int rows=7;
        const float blockWidth=width/static_cast<float>(columns);
        const float blockHeight=height/static_cast<float>(rows);
        for(int row=0;row<rows;++row){
            for(int column=0;column<columns;++column){
                const int index=row*columns+column;
                const float targetX=x+(static_cast<float>(column)+0.5f)*blockWidth;
                const float targetY=y+(static_cast<float>(row)+0.5f)*blockHeight;
                const float distanceX=std::abs(static_cast<float>(column)-(columns-1)*0.5f);
                const float distanceY=std::abs(static_cast<float>(row)-(rows-1)*0.5f);
                const float delay=(distanceX+distanceY)*0.018f;
                const float localProgress=smoothStep((gameOverTimer_-delay)/0.62f);
                const bool horizontalEdge=distanceX/(columns*0.5f)>distanceY/(rows*0.5f);
                float startX=targetX;
                float startY=targetY;
                if(horizontalEdge) startX=column<columns/2?x-width*0.18f:x+width*1.18f;
                else startY=row<rows/2?y-height*0.22f:y+height*1.22f;
                float blockX=startX+(targetX-startX)*localProgress;
                float blockY=startY+(targetY-startY)*localProgress;
                const float burst=exitProgress*(0.45f+static_cast<float>((index*17)%11)/18.0f);
                blockX+=(targetX-centerX)*burst*2.2f;
                blockY+=(targetY-centerY)*burst*2.2f;
                const int alpha=static_cast<int>(255.0f*localProgress*(1.0f-exitProgress));
                const int shade=18+((row+column)%3)*7;
                const ImU32 fill=(index%9==0)
                    ? IM_COL32(43,42,84,alpha)
                    : IM_COL32(shade,18,45+((index*7)%17),alpha);
                const ImVec2 blockMin{blockX-blockWidth*0.49f,blockY-blockHeight*0.49f};
                const ImVec2 blockMax{blockX+blockWidth*0.49f,blockY+blockHeight*0.49f};
                draw->AddRectFilled(blockMin,blockMax,fill,2.0f);
                draw->AddLine(blockMin,{blockMax.x,blockMin.y},
                    IM_COL32(74,93,132,alpha/2),1.0f);
                draw->AddLine(blockMin,{blockMin.x,blockMax.y},
                    IM_COL32(70,45,100,alpha/2),1.0f);
            }
        }

        // 壁完成時に強く揺らし、文字を一文字ずつ落下させる。
        const float impactAge=gameOverTimer_-0.78f;
        const float impactStrength=impactAge>=0.0f&&impactAge<0.32f
            ? (1.0f-impactAge/0.32f)*9.0f:0.0f;
        const float shakeX=std::sin(gameOverTimer_*92.0f)*impactStrength;
        const float shakeY=std::cos(gameOverTimer_*77.0f)*impactStrength*0.55f;
        const char* gameOverText="GAME OVER";
        constexpr float gameOverFontSize=86.0f;
        const ImVec2 titleSize=ImGui::GetFont()->CalcTextSizeA(
            gameOverFontSize,FLT_MAX,0.0f,gameOverText);
        float letterX=centerX-titleSize.x*0.5f+shakeX;
        for(int i=0;gameOverText[i]!='\0';++i){
            char letter[2]{gameOverText[i],'\0'};
            const ImVec2 letterSize=ImGui::GetFont()->CalcTextSizeA(
                gameOverFontSize,FLT_MAX,0.0f,letter);
            if(gameOverText[i]==' '){letterX+=letterSize.x;continue;}
            const float letterProgress=smoothStep((gameOverTimer_-0.72f-i*0.055f)/0.34f);
            if(letterProgress<=0.0f)continue;
            const float bounce=std::sin(letterProgress*kPi)*28.0f*(1.0f-letterProgress);
            const float letterY=centerY-94.0f-(1.0f-letterProgress)*height*0.62f+bounce+shakeY;
            const int alpha=static_cast<int>(255.0f*(1.0f-exitProgress));
            draw->AddText(ImGui::GetFont(),gameOverFontSize,
                {letterX+6.0f,letterY+8.0f},IM_COL32(6,7,18,alpha),letter);
            draw->AddText(ImGui::GetFont(),gameOverFontSize,
                {letterX,letterY},IM_COL32(232,241,246,alpha),letter);
            letterX+=letterSize.x;
        }

        const float detailProgress=smoothStep((gameOverTimer_-1.18f)/0.36f)*(1.0f-exitProgress);
        const char* lostText="ALL BLOCKS LOST";
        const ImVec2 lostSize=ImGui::CalcTextSize(lostText);
        draw->AddText({centerX-lostSize.x*0.5f,centerY+34.0f},
            IM_COL32(255,151,62,static_cast<int>(255.0f*detailProgress)),lostText);

        // 残機ブロックを順番に落下・消灯させ、0になったことを視覚化する。
        for(int i=0;i<kMaxPlayerLives;++i){
            const float blockProgress=smoothStep((gameOverTimer_-1.02f-i*0.14f)/0.34f);
            const float blockX=centerX-54.0f+static_cast<float>(i)*54.0f;
            const float targetY=centerY+81.0f;
            const float blockY=targetY-(1.0f-blockProgress)*115.0f;
            const int alpha=static_cast<int>(245.0f*blockProgress*(1.0f-exitProgress));
            draw->AddRectFilled({blockX-17.0f,blockY-17.0f},{blockX+17.0f,blockY+17.0f},
                IM_COL32(28,30,48,alpha),3.0f);
            draw->AddRect({blockX-17.0f,blockY-17.0f},{blockX+17.0f,blockY+17.0f},
                IM_COL32(222,70,92,alpha),3.0f,0,2.0f);
            draw->AddLine({blockX-9.0f,blockY-9.0f},{blockX+9.0f,blockY+9.0f},
                IM_COL32(255,91,91,alpha),3.0f);
            draw->AddLine({blockX+9.0f,blockY-9.0f},{blockX-9.0f,blockY+9.0f},
                IM_COL32(255,91,91,alpha),3.0f);
        }

        if(gameOverTimer_>=kGameOverInputDelay&&!gameOverExitActive_){
            const float blink=0.58f+0.42f*std::sin(
                (gameOverTimer_-kGameOverInputDelay)*kPi*2.0f);
            const char* retryText="ENTER / GAMEPAD A : REBUILD";
            const ImVec2 retrySize=ImGui::CalcTextSize(retryText);
            const float promptY=y+height*0.77f+std::sin(gameOverTimer_*3.4f)*4.0f;
            draw->AddRectFilled(
                {centerX-retrySize.x*0.5f-22.0f,promptY-11.0f},
                {centerX+retrySize.x*0.5f+22.0f,promptY+retrySize.y+11.0f},
                IM_COL32(5,15,29,205),6.0f);
            draw->AddText({centerX-retrySize.x*0.5f,promptY},
                IM_COL32(82,232,255,static_cast<int>(blink*255.0f)),retryText);
            const char* titleText="ESC : RETURN TO TITLE";
            const ImVec2 titleSize=ImGui::CalcTextSize(titleText);
            draw->AddText({centerX-titleSize.x*0.5f,promptY+42.0f},
                IM_COL32(180,190,210,220),titleText);
        }
    } else if (gameFlowState_ == GameFlowState::IntroCamera) {
        // 操作開始前であることが分かるよう、控えめなシネマティック表示を重ねる。
        const char* previewText="STAGE PREVIEW";
        const ImVec2 previewSize=ImGui::CalcTextSize(previewText);
        const ImVec2 previewPosition{x+(width-previewSize.x)*0.5f,y+height*0.10f};
        draw->AddRectFilled(
            {previewPosition.x-22.0f,previewPosition.y-9.0f},
            {previewPosition.x+previewSize.x+22.0f,previewPosition.y+previewSize.y+9.0f},
            IM_COL32(4,10,20,190),8.0f);
        draw->AddText(previewPosition,IM_COL32(100,225,255,255),previewText);

        if(introCameraSeen_){
            const char* skipText="ENTER / GAMEPAD A : SKIP";
            const ImVec2 skipSize=ImGui::CalcTextSize(skipText);
            draw->AddText(
                {x+(width-skipSize.x)*0.5f,y+height*0.90f},
                IM_COL32(205,220,235,205),skipText);
        }

        const float introProgress=GetIntroCameraProgress();
        if(introProgress>=0.58f && introProgress<0.70f){
            // ゴール本体の真上へ矢印を投影し、どのオブジェクトが目的地かを明確にする。
            const float goalPhase=(introProgress-0.58f)/(0.70f-0.58f);
            for(const Placement& placement:placements_){
                if(placement.itemId!=17) continue;
                Camera* camera=object3dCommon_ ? object3dCommon_->GetDefaultCamera() : nullptr;
                if(!camera) break;
                const Vector3 goalHalfExtents = GetPlacementHalfExtents(placement);
                const Vector3 markerPosition{
                    placement.position.x,
                    placement.position.y + goalHalfExtents.y + 0.30f,
                    placement.position.z};
                const Vector4 clip=TransformPoint4(
                    {markerPosition.x,markerPosition.y,markerPosition.z,1.0f},
                    camera->GetViewProjectionMatrix());
                if(clip.w<=0.001f) break;
                const ImVec2 goalTop{
                    x+(clip.x/clip.w*0.5f+0.5f)*width,
                    y+(-clip.y/clip.w*0.5f+0.5f)*height};
                const float arrowBob=std::sin(goalPhase*kPi*4.0f)*5.0f;
                const ImVec2 arrowTip{goalTop.x,goalTop.y-12.0f+arrowBob};
                const ImVec2 arrowLeft{goalTop.x-17.0f,arrowTip.y-25.0f};
                const ImVec2 arrowRight{goalTop.x+17.0f,arrowTip.y-25.0f};
                draw->AddTriangleFilled(
                    arrowTip,arrowLeft,arrowRight,IM_COL32(255,245,145,250));
                draw->AddLine(
                    {goalTop.x,arrowTip.y-48.0f},
                    {goalTop.x,arrowTip.y-20.0f},
                    IM_COL32(255,245,145,250),8.0f);
                const char* goalText="GOAL";
                const ImVec2 goalTextSize=ImGui::CalcTextSize(goalText);
                draw->AddText(
                    {goalTop.x-goalTextSize.x*0.5f,arrowTip.y-72.0f},
                    IM_COL32(255,245,170,255),goalText);
                break;
            }
        }
    } else if (gameFlowState_ == GameFlowState::Countdown) {
        // カメラがスタート地点へ戻ってから、落ち着いたテンポで開始を数える。
        const float beatDuration=kStartCountdownDuration/3.0f;
        const float beatProgress=std::fmod(startCountdownTimer_,beatDuration)/beatDuration;
        const ImVec2 circleCenter{
            x+width*0.5f,
            y+height*0.43f};
        const float ringRadius=62.0f+beatProgress*16.0f;
        // 数字は3Dパスで先に描画されるため、ここで塗りつぶし円を重ねると
        // OBJ全体が隠れてしまう。輪郭リングだけを重ねて数字を前面に残す。
        draw->AddCircle(
            circleCenter,ringRadius,
            IM_COL32(105,255,190,static_cast<int>((1.0f-beatProgress)*235.0f)),
            48,3.0f);
        const char* readyText="GET READY";
        const ImVec2 readySize=ImGui::CalcTextSize(readyText);
        draw->AddText(
            {x+(width-readySize.x)*0.5f,circleCenter.y+78.0f},
            IM_COL32(170,245,205,255),readyText);
    } else if (startGoTimer_ > 0.0f) {
        // 操作可能になった瞬間を、大きな文字と外へ抜けるリングで伝える。
        const float progress=1.0f-std::clamp(startGoTimer_/kStartGoDuration,0.0f,1.0f);
        const float eased=progress*progress*(3.0f-2.0f*progress);
        const float alpha=1.0f-std::clamp((progress-0.58f)/0.42f,0.0f,1.0f);
        const ImVec2 center{x+width*0.5f,y+height*0.43f};
        const float ringRadius=62.0f+eased*150.0f;
        draw->AddCircle(
            center,ringRadius,
            IM_COL32(105,255,190,static_cast<int>(alpha*230.0f)),
            64,4.0f);
        const float goFontSize=92.0f*(1.18f-0.18f*eased);
        const char* goText="GO!";
        const ImVec2 goSize=ImGui::GetFont()->CalcTextSizeA(goFontSize,FLT_MAX,0.0f,goText);
        draw->AddText(
            ImGui::GetFont(),goFontSize,
            {center.x-goSize.x*0.5f,center.y-goSize.y*0.5f},
            IM_COL32(220,255,235,static_cast<int>(alpha*255.0f)),goText);
    } else if (runtimeMessageTimer_ > 0.0f && !runtimeMessage_.empty()) {
        const ImVec2 textSize = ImGui::CalcTextSize(runtimeMessage_.c_str());
        const ImVec2 textPosition{x + (width - textSize.x) * 0.5f, y + height * 0.18f};
        draw->AddRectFilled(
            {textPosition.x - 18.0f, textPosition.y - 10.0f},
            {textPosition.x + textSize.x + 18.0f, textPosition.y + textSize.y + 10.0f},
            IM_COL32(8,12,20,220), 7.0f);
        draw->AddText(textPosition, goalReached_ ? IM_COL32(90,255,120,255) : IM_COL32(255,235,90,255), runtimeMessage_.c_str());
    }

    // 操作開始前のカウントダウンからプレイ中まで、残機をブロック3個で表示する。
    // 増減直後のスロットだけ色・揺れ・小片を加え、どこが変わったかを明確にする。
    if (mode_ == Mode::GamePlay &&
        (gameFlowState_ == GameFlowState::Countdown ||
         gameFlowState_ == GameFlowState::Playing)) {
        const ImVec2 panelMin{x + width - 204.0f, y + 18.0f};
        const ImVec2 panelMax{x + width - 18.0f, y + 82.0f};
        draw->AddRectFilled(panelMin, panelMax, IM_COL32(5, 12, 27, 210), 9.0f);
        draw->AddRect(panelMin, panelMax, IM_COL32(70, 205, 235, 155), 9.0f, 0, 1.5f);
        draw->AddText({panelMin.x + 13.0f, panelMin.y + 9.0f},
            IM_COL32(190, 238, 250, 255), "LIFE");

        char lifeCount[16]{};
        std::snprintf(lifeCount, sizeof(lifeCount), "x %d", playerLives_);
        draw->AddText({panelMax.x - 43.0f, panelMin.y + 9.0f},
            IM_COL32(235, 248, 255, 255), lifeCount);

        const float changeProgress = lifeChangeTimer_ > 0.0f
            ? 1.0f - lifeChangeTimer_ / kLifeChangeDuration
            : 1.0f;
        const float changeAlpha = 1.0f - std::clamp(changeProgress, 0.0f, 1.0f);
        for (int i = 0; i < kMaxPlayerLives; ++i) {
            const bool activeLife = i < playerLives_;
            const bool changing = lifeChangeTimer_ > 0.0f && i == lifeChangedSlot_;
            float shakeX = 0.0f;
            float popScale = 1.0f;
            if (changing) {
                shakeX = lifeChangeAmount_ < 0
                    ? std::sin(changeProgress * kPi * 12.0f) * 4.0f * changeAlpha
                    : 0.0f;
                popScale += std::sin(std::clamp(changeProgress * 1.8f, 0.0f, 1.0f) * kPi) * 0.28f;
            }

            const ImVec2 center{
                panelMin.x + 71.0f + static_cast<float>(i) * 37.0f + shakeX,
                panelMin.y + 43.0f};
            const float half = 11.0f * popScale;
            ImU32 fill = activeLife
                ? IM_COL32(35, 220, 245, 255)
                : IM_COL32(35, 48, 64, 220);
            ImU32 border = activeLife
                ? IM_COL32(190, 250, 255, 255)
                : IM_COL32(75, 88, 105, 210);
            if (changing) {
                fill = lifeChangeAmount_ > 0
                    ? IM_COL32(255, 194, 55, 255)
                    : IM_COL32(205, 55, 70, static_cast<int>(180.0f * changeAlpha + 50.0f));
                border = lifeChangeAmount_ > 0
                    ? IM_COL32(255, 239, 160, 255)
                    : IM_COL32(255, 145, 150, 255);
            }
            draw->AddRectFilled(
                {center.x - half, center.y - half},
                {center.x + half, center.y + half}, fill, 2.0f);
            draw->AddRect(
                {center.x - half, center.y - half},
                {center.x + half, center.y + half}, border, 2.0f, 0, 2.0f);
            draw->AddLine(
                {center.x - half + 3.0f, center.y - half + 3.0f},
                {center.x + half - 3.0f, center.y - half + 3.0f},
                activeLife ? IM_COL32(220, 255, 255, 210) : IM_COL32(95, 105, 120, 130),
                2.0f);

            if (changing && lifeChangeAmount_ < 0) {
                for (int fragment = 0; fragment < 4; ++fragment) {
                    const float angle = kPi * 0.25f + kPi * 0.5f * static_cast<float>(fragment);
                    const float distance = 13.0f + changeProgress * 15.0f;
                    const ImVec2 fragmentCenter{
                        center.x + std::cos(angle) * distance,
                        center.y + std::sin(angle) * distance};
                    const float fragmentHalf = 2.5f * changeAlpha;
                    draw->AddRectFilled(
                        {fragmentCenter.x - fragmentHalf, fragmentCenter.y - fragmentHalf},
                        {fragmentCenter.x + fragmentHalf, fragmentCenter.y + fragmentHalf},
                        IM_COL32(255, 105, 115, static_cast<int>(changeAlpha * 255.0f)));
                }
            }
        }

        if (lifeChangeTimer_ > 0.0f && lifeChangeAmount_ != 0) {
            const char* changeText = lifeChangeAmount_ > 0 ? "+1" : "-1";
            const ImU32 changeColor = lifeChangeAmount_ > 0
                ? IM_COL32(255, 220, 80, static_cast<int>(changeAlpha * 255.0f))
                : IM_COL32(255, 100, 110, static_cast<int>(changeAlpha * 255.0f));
            draw->AddText(
                {panelMax.x - 33.0f, panelMax.y + 5.0f - changeProgress * 13.0f},
                changeColor, changeText);
        }
    }

    // 小型ブロックが画面奥から飛来して一枚の壁を組み上げ、完全に隠れた瞬間に
    // ステージへ切り替える。後半は壁を奥へ崩し、隙間からプレイ画面を公開する。
    if (startTransitionActive_) {
        constexpr float buildDuration = kStartTransitionBuildDuration;
        constexpr float loadingDuration = kStartTransitionLoadingDuration;
        constexpr float openStart = buildDuration + loadingDuration;
        const bool loadingStage = startTransitionTimer_ >= buildDuration &&
            startTransitionTimer_ < openStart;
        const bool openingStage = startTransitionTimer_ >= openStart;
        // タイトルが手前へ弾み始めた直後から壁の材料を出し、収納する文字と同期させる。
        constexpr float wallBuildStart = kTitleStorageStart + 0.04f;
        const float phaseProgress = openingStage
            ? (startTransitionTimer_ - openStart) / kStartTransitionOpenDuration
            : (loadingStage
                ? 1.0f
                : (startTransitionTimer_ - wallBuildStart) /
                    (buildDuration - wallBuildStart));
        const float clampedPhase = std::clamp(phaseProgress, 0.0f, 1.0f);
        const float easedPhase = clampedPhase * clampedPhase * (3.0f - 2.0f * clampedPhase);
        const float wallProgress = openingStage ? 1.0f - easedPhase : easedPhase;

        draw->PushClipRect({x, y}, {x + width, y + height}, true);
        constexpr float blockWidth = 80.0f;
        constexpr float blockHeight = 58.0f;
        constexpr float buildEdgeWidth = 0.24f;
        const int columnCount = static_cast<int>(std::ceil(width / blockWidth)) + 1;
        const int rowCount = static_cast<int>(std::ceil(height / blockHeight)) + 1;
        const float centerColumn = static_cast<float>(columnCount - 1) * 0.5f;
        const float centerRow = static_cast<float>(rowCount - 1) * 0.5f;
        const float maximumHorizontalDistance = (std::max)(centerColumn, 1.0f);
        const float maximumVerticalDistance = (std::max)(centerRow, 1.0f);

        // 左右端から中央へ向かって組み上げる。中央行にも少し遅れを加え、
        // 最後の中央ブロックが鍵のようにはまって壁が完成する順番にする。
        const auto makeVariation = [](int column, int row) {
            uint32_t value = static_cast<uint32_t>(column) * 73856093u;
            value ^= static_cast<uint32_t>(row) * 19349663u;
            value ^= value >> 13;
            value *= 1274126177u;
            value ^= value >> 16;
            return static_cast<float>(value & 0xffffu) / 65535.0f;
        };

        for (int row = 0; row < rowCount; ++row) {
            for (int column = 0; column < columnCount; ++column) {
                const float horizontalDistance = std::abs(
                    static_cast<float>(column) - centerColumn);
                const float verticalDistance = std::abs(
                    static_cast<float>(row) - centerRow);
                const float inwardOrder = 1.0f - std::clamp(
                    horizontalDistance / maximumHorizontalDistance, 0.0f, 1.0f);
                const float centerRowOrder = 1.0f - std::clamp(
                    verticalDistance / maximumVerticalDistance, 0.0f, 1.0f);
                const float variation = makeVariation(column, row);
                const float threshold = std::clamp(
                    inwardOrder * 0.68f + centerRowOrder * 0.12f + variation * 0.05f,
                    0.0f,
                    1.0f - buildEdgeWidth);
                const float blockAmount = std::clamp(
                    (wallProgress - threshold) / buildEdgeWidth,
                    0.0f,
                    1.0f);
                if (blockAmount <= 0.0f) {
                    continue;
                }

                // 前半は奥から手前へ飛来し、後半は再び奥へ吸い込まれるように縮小する。
                const float blockScale = 0.25f + blockAmount * 0.83f;
                const float cellCenterX = x + (static_cast<float>(column) + 0.5f) * blockWidth;
                const float cellCenterY = y + (static_cast<float>(row) + 0.5f) * blockHeight;
                const float halfWidth = blockWidth * 0.5f * blockScale + 1.0f;
                const float halfHeight = blockHeight * 0.5f * blockScale + 1.0f;
                const ImVec2 minimum{cellCenterX - halfWidth, cellCenterY - halfHeight};
                const ImVec2 maximum{cellCenterX + halfWidth, cellCenterY + halfHeight};
                const int alpha = static_cast<int>(blockAmount * 255.0f);
                const int colorVariant = (column + row * 2) % 3;
                const ImU32 faceColor = colorVariant == 0
                    ? IM_COL32(18, 27, 48, alpha)
                    : colorVariant == 1
                        ? IM_COL32(26, 38, 62, alpha)
                        : IM_COL32(31, 29, 57, alpha);
                draw->AddRectFilled(minimum, maximum, faceColor, 4.0f);

                // 上辺と右辺に異なる明度を付け、平面ではなく積み上がったブロックに見せる。
                draw->AddLine(
                    {minimum.x + 3.0f, minimum.y + 3.0f},
                    {maximum.x - 3.0f, minimum.y + 3.0f},
                    IM_COL32(105, 185, 215, static_cast<int>(blockAmount * 150.0f)),
                    2.0f);
                draw->AddLine(
                    {maximum.x - 3.0f, minimum.y + 3.0f},
                    {maximum.x - 3.0f, maximum.y - 3.0f},
                    IM_COL32(82, 58, 130, static_cast<int>(blockAmount * 175.0f)),
                    3.0f);
            }
        }

        if (loadingStage) {
            const float loadingProgress = std::clamp(
                (startTransitionTimer_ - buildDuration) / loadingDuration,
                0.0f, 1.0f);
            constexpr int loadingBlockCount = 8;
            constexpr float loadingBlockSize = 26.0f;
            constexpr float loadingBlockGap = 9.0f;
            const float loadingWidth = loadingBlockCount * loadingBlockSize +
                (loadingBlockCount - 1) * loadingBlockGap;
            const float loadingLeft = x + (width - loadingWidth) * 0.5f;
            const float loadingTop = y + height * 0.5f - loadingBlockSize * 0.5f;
            const char* loadingText = "LOADING";
            const ImVec2 loadingTextSize = ImGui::CalcTextSize(loadingText);
            draw->AddText(
                {x + (width - loadingTextSize.x) * 0.5f, loadingTop - 38.0f},
                IM_COL32(205, 230, 244, 235), loadingText);
            const int illuminatedCount = (std::min)(
                loadingBlockCount,
                static_cast<int>(loadingProgress * static_cast<float>(loadingBlockCount + 1)));
            const float completionFlash = std::clamp(
                (loadingProgress - 0.90f) / 0.10f, 0.0f, 1.0f);
            const float flashPulse = completionFlash > 0.0f
                ? std::sin(completionFlash * kPi)
                : 0.0f;

            for (int index = 0; index < loadingBlockCount; ++index) {
                const float left = loadingLeft +
                    static_cast<float>(index) * (loadingBlockSize + loadingBlockGap);
                const ImVec2 minimum{left, loadingTop};
                const ImVec2 maximum{left + loadingBlockSize, loadingTop + loadingBlockSize};
                const bool illuminated = index < illuminatedCount;
                ImU32 faceColor = IM_COL32(30, 38, 58, 255);
                ImU32 edgeColor = IM_COL32(65, 78, 104, 255);
                if (illuminated) {
                    const bool finalBlock = index == loadingBlockCount - 1;
                    faceColor = finalBlock
                        ? IM_COL32(255, 181, 48, 255)
                        : IM_COL32(38, 211, 245, 255);
                    edgeColor = finalBlock
                        ? IM_COL32(255, 235, 160, 255)
                        : IM_COL32(190, 250, 255, 255);
                }
                if (flashPulse > 0.0f) {
                    const int white = static_cast<int>(flashPulse * 185.0f);
                    faceColor = IM_COL32(
                        (std::min)(255, static_cast<int>((faceColor >> IM_COL32_R_SHIFT) & 0xff) + white),
                        (std::min)(255, static_cast<int>((faceColor >> IM_COL32_G_SHIFT) & 0xff) + white),
                        (std::min)(255, static_cast<int>((faceColor >> IM_COL32_B_SHIFT) & 0xff) + white),
                        255);
                }
                draw->AddRectFilled(minimum, maximum, faceColor, 4.0f);
                draw->AddRect(minimum, maximum, edgeColor, 4.0f, 0, 2.0f);
            }
        }

        // ステージ切替フレームだけは継ぎ目も完全に隠し、切替前後の画像混入を防ぐ。
        if (std::abs(startTransitionTimer_ - openStart) < 0.035f) {
            draw->AddRectFilled({x, y}, {x + width, y + height}, IM_COL32(8, 12, 24, 255));
        }
        draw->PopClipRect();
    }
#endif
}

// 処理概要: StageEditorが担当する「HandleEditorInput」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void StageEditor::HandleEditorInput(float rectX, float rectY, float rectWidth, float rectHeight) {
#ifdef USE_IMGUI
    ImGuiIO& io=ImGui::GetIO();
    if(mode_!=Mode::Editor||!gameViewHovered_||io.WantTextInput)return;
    if(ImGui::IsKeyPressed(ImGuiKey_A,true))cursorPosition_.x-=kTileWorldSize;
    if(ImGui::IsKeyPressed(ImGuiKey_D,true))cursorPosition_.x+=kTileWorldSize;
    if(ImGui::IsKeyPressed(ImGuiKey_W,true))cursorPosition_.y+=kTileWorldSize;
    if(ImGui::IsKeyPressed(ImGuiKey_S,true))cursorPosition_.y-=kTileWorldSize;
    cursorPosition_.x=std::clamp(cursorPosition_.x,kTileWorldSize*0.5f,(static_cast<float>(stageWidth_)-0.5f)*kTileWorldSize);
    cursorPosition_.y=std::clamp(cursorPosition_.y,kTileWorldSize*0.5f,(static_cast<float>(stageHeight_)-0.5f)*kTileWorldSize);
    cursorPosition_.z=0.0f;

    int mouseGridX = -1;
    int mouseGridY = -1;
    const bool mouseOnGrid = MouseToGrid(
        io.MousePos.x, io.MousePos.y,
        rectX, rectY, rectWidth, rectHeight,
        mouseGridX, mouseGridY
    );
    if (mouseOnGrid) {
        cursorPosition_ = GridToWorld(mouseGridX, mouseGridY);
        const bool leftPainting = ImGui::IsMouseDown(ImGuiMouseButton_Left);
        const bool rightPainting = ImGui::IsMouseDown(ImGuiMouseButton_Right);
        if ((leftPainting || rightPainting) &&
            (mouseGridX != lastPaintGridX_ || mouseGridY != lastPaintGridY_)) {
            if (leftPainting) {
                if (IsTileItem(selectedItemId_)) PaintTile(mouseGridX, mouseGridY);
                else PlaceItem();
            } else {
                EraseTile(mouseGridX, mouseGridY);
            }
            lastPaintGridX_ = mouseGridX;
            lastPaintGridY_ = mouseGridY;
        }
    }
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsMouseDown(ImGuiMouseButton_Right)) {
        lastPaintGridX_ = lastPaintGridY_ = -1;
    }
    if(ImGui::IsKeyPressed(ImGuiKey_Enter))PlaceItem();
    if(ImGui::IsKeyPressed(ImGuiKey_Space)||ImGui::IsKeyPressed(ImGuiKey_Delete)||ImGui::IsKeyPressed(ImGuiKey_Backspace))RemoveNearest();
    if(ImGui::IsKeyPressed(ImGuiKey_R))RotateCursor();
    if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_Z))Undo();
    if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_Y))Redo();
#endif
}

bool StageEditor::MouseToGrid(
    float mouseX,
    float mouseY,
    float rectX,
    float rectY,
    float rectWidth,
    float rectHeight,
    int& gridX,
    int& gridY
) const {
#ifdef USE_IMGUI
    if (!object3dCommon_ || !object3dCommon_->GetDefaultCamera() ||
        rectWidth <= 0.0f || rectHeight <= 0.0f) {
        return false;
    }
    const float ndcX = ((mouseX - rectX) / rectWidth) * 2.0f - 1.0f;
    const float ndcY = 1.0f - ((mouseY - rectY) / rectHeight) * 2.0f;
    const Matrix4x4 inverseViewProjection = Math::Inverse(
        object3dCommon_->GetDefaultCamera()->GetViewProjectionMatrix()
    );
    Vector4 nearPoint = TransformPoint4({ndcX, ndcY, 0.0f, 1.0f}, inverseViewProjection);
    Vector4 farPoint = TransformPoint4({ndcX, ndcY, 1.0f, 1.0f}, inverseViewProjection);
    if (std::abs(nearPoint.w) < 0.00001f || std::abs(farPoint.w) < 0.00001f) {
        return false;
    }
    nearPoint.x /= nearPoint.w; nearPoint.y /= nearPoint.w; nearPoint.z /= nearPoint.w;
    farPoint.x /= farPoint.w; farPoint.y /= farPoint.w; farPoint.z /= farPoint.w;
    const float directionZ = farPoint.z - nearPoint.z;
    if (std::abs(directionZ) < 0.00001f) {
        return false;
    }
    const float t = -nearPoint.z / directionZ;
    if (t < 0.0f) {
        return false;
    }
    const float worldX = nearPoint.x + (farPoint.x - nearPoint.x) * t;
    const float worldY = nearPoint.y + (farPoint.y - nearPoint.y) * t;
    gridX = static_cast<int>(std::floor(worldX / kTileWorldSize));
    gridY = static_cast<int>(std::floor(worldY / kTileWorldSize));
    return TileIndex(gridX, gridY) >= 0;
#else
    (void)mouseX; (void)mouseY; (void)rectX; (void)rectY;
    (void)rectWidth; (void)rectHeight; (void)gridX; (void)gridY;
    return false;
#endif
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void StageEditor::DrawGridOverlay(float rectX, float rectY, float rectWidth, float rectHeight) {
#ifdef USE_IMGUI
    if (!showGrid_ || !object3dCommon_ || !object3dCommon_->GetDefaultCamera()) return;
    const Matrix4x4& viewProjection = object3dCommon_->GetDefaultCamera()->GetViewProjectionMatrix();
    auto project = [&](const Vector3& point, ImVec2& screen) {
        const Vector4 clip = TransformPoint4({point.x, point.y, point.z, 1.0f}, viewProjection);
        if (clip.w <= 0.001f) return false;
        const float ndcX = clip.x / clip.w;
        const float ndcY = clip.y / clip.w;
        screen = {
            rectX + (ndcX * 0.5f + 0.5f) * rectWidth,
            rectY + (-ndcY * 0.5f + 0.5f) * rectHeight
        };
        return true;
    };
    ImDrawList* draw = ImGui::GetWindowDrawList();
    draw->PushClipRect({rectX, rectY}, {rectX + rectWidth, rectY + rectHeight}, true);
    const ImU32 gridColor = IM_COL32(90, 175, 255, 70);
    for (int x = 0; x <= stageWidth_; ++x) {
        ImVec2 a{}, b{};
        if (project({x * kTileWorldSize, 0.0f, 0.01f}, a) &&
            project({x * kTileWorldSize, stageHeight_ * kTileWorldSize, 0.01f}, b)) {
            draw->AddLine(a, b, gridColor, 1.0f);
        }
    }
    for (int y = 0; y <= stageHeight_; ++y) {
        ImVec2 a{}, b{};
        if (project({0.0f, y * kTileWorldSize, 0.01f}, a) &&
            project({stageWidth_ * kTileWorldSize, y * kTileWorldSize, 0.01f}, b)) {
            draw->AddLine(a, b, gridColor, 1.0f);
        }
    }
    draw->PopClipRect();
#else
    (void)rectX; (void)rectY; (void)rectWidth; (void)rectHeight;
#endif
}

// 処理概要: StageEditorが担当する「PaintTile」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void StageEditor::PaintTile(int gridX, int gridY) {
    const int index = TileIndex(gridX, gridY);
    if (index < 0 || !IsTileItem(selectedItemId_) || tiles_[index] == selectedItemId_) return;
    PushUndo();
    tiles_[index] = selectedItemId_;
    if (tileObjects_.size() != tiles_.size()) {
        RebuildObjects();
    } else {
        Placement tile;
        tile.itemId = selectedItemId_;
        tile.position = GridToWorld(gridX, gridY);
        tile.scale = {1.0f, 1.0f, 1.0f};
        tileObjects_[index] = CreateObject(tile);
    }
    status_ = "Painted 16x16 tile";
}

// 処理概要: StageEditorが担当する「EraseTile」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void StageEditor::EraseTile(int gridX, int gridY) {
    const int index = TileIndex(gridX, gridY);
    if (index >= 0 && tiles_[index] != 0) {
        PushUndo();
        tiles_[index] = 0;
        if (index < static_cast<int>(tileObjects_.size())) tileObjects_[index].reset();
        status_ = "Erased tile";
        return;
    }
    cursorPosition_ = GridToWorld(gridX, gridY);
    RemoveNearest();
}

// 処理概要: 外部から渡された値を、担当オブジェクトの状態へ反映する。
// 注意事項: 必要に応じて範囲制限や依存データの再計算も行う。
void StageEditor::SetMode(Mode mode){
    if(mode_==mode)return;
    mode_=mode;
    if (mode_ == Mode::GamePlay) { ResetPlayer(); gameFlowState_=GameFlowState::Title; RefreshSaveDataAvailability(); }
    status_=mode==Mode::Editor?"EditorMode enabled":"GamePlayMode enabled";
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void StageEditor::UpdateGameFlow(Input* input) {
    if (!input) return;
    constexpr float deltaTime = 1.0f / 60.0f;
    constexpr float startTransitionDuration = kStartTransitionDuration;
    // 壁が完成してロードブロックが全点灯した瞬間にステージへ切り替える。
    constexpr float startTransitionSwitchTime =
        kStartTransitionBuildDuration + kStartTransitionLoadingDuration;
    startGoTimer_=(std::max)(0.0f,startGoTimer_-deltaTime);

    // プレイ中・導入演出中・画面遷移中のどこからでもEscでタイトルへ戻す。
    // タイトル画面上ではEscを消費せず、既存のウィンドウ操作へ影響させない。
    if (gameFlowState_ != GameFlowState::Title && input->TriggerKey(DIK_ESCAPE)) {
        ResetRuntimeState();
        gameFlowState_ = GameFlowState::Title;
        titleLetterAnimationTimer_ = 0.0f;
        titlePromptBlinkTimer_ = 0.0f;
        titleAmbientBlockTimer_ = 0.0f;
        titleStartParticles_.clear();
        RefreshSaveDataAvailability();
        status_ = "Returned to title";
        return;
    }

    // フェードの中央（画面が完全に黒い瞬間）でタイトルからゲームへ切り替える。
    // 演出中は追加の決定入力を受け付けず、二重開始を防止する。
    if (startTransitionActive_) {
        const float previousTimer = startTransitionTimer_;
        startTransitionTimer_ += deltaTime;

        if (previousTimer < startTransitionSwitchTime &&
            startTransitionTimer_ >= startTransitionSwitchTime) {
            // 暗転の中央でステージへ切り替え、操作可能状態へ直接入らず
            // スタートからゴールを見せる導入カメラへ接続する。
            gameFlowState_ = GameFlowState::IntroCamera;
            introCameraTimer_ = 0.0f;
        }

        if (startTransitionTimer_ >= startTransitionDuration) {
            startTransitionActive_ = false;
            startTransitionContinue_ = false;
            startTransitionTimer_ = 0.0f;
        }
        return;
    }

    if (gameFlowState_ == GameFlowState::IntroCamera) {
        // ステージ紹介中はカメラだけを移動させる。
        // 自機はスタート地点へ固定し、カメラ演出や直前の入力状態によって
        // ワールド座標が変化しないよう毎フレーム明示的に戻す。
        playerPosition_ = playerRespawnPosition_;
        playerPosition_.z = 0.0f;
        playerVelocity_ = {};
        playerGrounded_ = false;
        if (playerObject_) {
            playerObject_->SetPosition(playerPosition_);
            playerObject_->SetRotation({});
            playerObject_->SetScale({1.0f, 1.0f, 1.0f});
            playerObject_->Update();
        }

        const bool skipIntro=introCameraSeen_ && (
            input->TriggerKey(DIK_RETURN) || input->TriggerKey(DIK_SPACE) ||
            input->TriggerGamepadButton(XINPUT_GAMEPAD_A));
        if(skipIntro){
            // 初回閲覧後は全景紹介だけを省略し、開始タイミングを示す短いカウントへ移る。
            gameFlowState_=GameFlowState::Countdown;
            startCountdownTimer_=0.0f;
            return;
        }

        // 演出中はUpdatePlayer/UpdateRuntimeObjectsが呼ばれないため、
        // 自機・敵・ギミックを停止したままカメラだけを進められる。
        introCameraTimer_ = (std::min)(kIntroCameraDuration,introCameraTimer_+deltaTime);
        if (introCameraTimer_ >= kIntroCameraDuration) {
            // スタート地点へ完全に戻った後、独立した3カウントへ進む。
            introCameraSeen_=true;
            gameFlowState_=GameFlowState::Countdown;
            startCountdownTimer_=0.0f;
        }
        return;
    }

    if (gameFlowState_ == GameFlowState::Countdown) {
        startCountdownTimer_=(std::min)(kStartCountdownDuration,startCountdownTimer_+deltaTime);
        if (startCountdownTimer_>=kStartCountdownDuration) {
            gameFlowState_=GameFlowState::Playing;
            startGoTimer_=kStartGoDuration;
        }
        return;
    }

    if (gameFlowState_ == GameFlowState::GameOver) {
        gameOverTimer_ += deltaTime;
        if (gameOverExitActive_) {
            gameOverExitTimer_ += deltaTime;
            if (gameOverExitTimer_ >= kGameOverExitDuration) {
                // 壁が外側へ崩れ切ってからランタイムを戻し、場面転換を明確にする。
                ResetRuntimeState();
                gameFlowState_ = GameFlowState::Countdown;
                startCountdownTimer_ = 0.0f;
                countdownImpactBeat_ = -1;
                countdownImpactEmitted_ = false;
                suppressStartJumpUntilReleased_ = true;
                gameOverExitActive_ = false;
                gameOverExitTimer_ = 0.0f;
            }
            return;
        }
        const bool retry = input->TriggerKey(DIK_RETURN) ||
            input->TriggerKey(DIK_SPACE) ||
            input->TriggerGamepadButton(XINPUT_GAMEPAD_A);
        if (gameOverTimer_ >= kGameOverInputDelay && retry) {
            gameOverExitActive_ = true;
            gameOverExitTimer_ = 0.0f;
        }
        return;
    }

    const bool accept = input->TriggerKey(DIK_RETURN) || input->TriggerKey(DIK_SPACE) ||
        input->TriggerGamepadButton(XINPUT_GAMEPAD_A);
    const bool continueGame = input->TriggerKey(DIK_C) ||
        input->TriggerGamepadButton(XINPUT_GAMEPAD_Y);
    if (gameFlowState_ == GameFlowState::Title) {
        if (continueGame && hasSaveData_) {
            ResetRuntimeState();
            // ステージ読込はフェード開始前に完了させる。
            // LoadStage()内のリセットで演出状態が解除されることを防ぎ、
            // 読込後の画面も黒い切り替え境界で自然に隠す。
            if (LoadSavedProgress()) {
                startTransitionActive_ = true;
                startTransitionContinue_ = true;
                suppressStartJumpUntilReleased_ = true;
            }
        } else if (accept) {
            ResetRuntimeState();
            EmitTitleStartParticles();
            // 決定パーティクルと同時に、タイトルを中央から左右へ開く遷移を開始する。
            startTransitionActive_ = true;
            startTransitionContinue_ = false;
            suppressStartJumpUntilReleased_ = true;
        }
    } else if (gameFlowState_ == GameFlowState::Clear) {
        // クリア結果を確認できる時間を確保した後、自動でタイトルへ戻す。
        // リトライとタイトル復帰をキーで選ばせず、ゲームループを
        // 「タイトル → プレイ → クリア → タイトル」に統一する。
        clearReturnTimer_ = (std::max)(0.0f, clearReturnTimer_ - deltaTime);
        if (clearReturnTimer_ <= 0.0f) {
            ResetRuntimeState();
            gameFlowState_ = GameFlowState::Title;
            RefreshSaveDataAvailability();
        }
    }
}
// 処理概要: 担当する状態を再利用可能な初期値へ戻す。
// 注意事項: リセット後も参照先とコンテナの整合性を保つ。
void StageEditor::ResetPlayer() {
    ResetRuntimeState();
}

// 処理概要: 担当する状態を再利用可能な初期値へ戻す。
// 注意事項: リセット後も参照先とコンテナの整合性を保つ。
void StageEditor::ResetRuntimeState() {
    runtimeTime_ = 0.0f;
    runtimeMessageTimer_ = 0.0f;
    doorCooldown_ = 0.0f;
    clearReturnTimer_ = 0.0f;
    gameOverTimer_ = 0.0f;
    gameOverExitActive_ = false;
    gameOverExitTimer_ = 0.0f;
    startTransitionTimer_ = 0.0f;
    startTransitionActive_ = false;
    startTransitionContinue_ = false;
    introCameraTimer_ = 0.0f;
    startCountdownTimer_ = 0.0f;
    countdownImpactBeat_ = -1;
    countdownImpactEmitted_ = false;
    startGoTimer_ = 0.0f;
    suppressStartJumpUntilReleased_ = false;
    playerLives_ = kMaxPlayerLives;
    lifeChangeAmount_ = 0;
    lifeChangedSlot_ = -1;
    lifeChangeTimer_ = 0.0f;
    respawnEffectActive_ = false;
    respawnEffectFromFall_ = false;
    respawnEffectTimer_ = 0.0f;
    respawnEffectOrigin_ = {};
    playerFlashTimer_ = 0.0f;
    playerDamageCooldown_ = 0.0f;
    runtimeMessage_.clear();
    collectedStars_ = 0;
    collectedBubbles_ = 0;
    keyCount_ = 0;
    pSwitchActive_ = false;
    onOffActive_ = true;
    goalReached_ = false;
    runtimeTileActive_.assign(tiles_.size(), 1);
    runtimePlacementActive_.assign(placements_.size(), 1);
    runtimePlacementTouching_.assign(placements_.size(), 0);
    runtimePlacementPositions_.resize(placements_.size());
    runtimeEnemyDirections_.assign(placements_.size(), 1.0f);
    runtimeEnemyAlerted_.assign(placements_.size(), 0);
    runtimeFallingFloorStates_.assign(placements_.size(), 0);
    runtimeFallingFloorTimers_.assign(placements_.size(), 0.0f);
    runtimeFallingFloorVelocities_.assign(placements_.size(), 0.0f);
    runtimeSavePointUsed_.assign(placements_.size(), 0);

    playerRespawnPosition_ = GridToWorld(1, 2);
    for (size_t i = 0; i < placements_.size(); ++i) {
        runtimePlacementPositions_[i] = placements_[i].position;
        runtimeEnemyDirections_[i] = (i % 2 == 0) ? 1.0f : -1.0f;
        if (placements_[i].itemId == 25) playerRespawnPosition_ = placements_[i].position;
        if (placements_[i].itemId == 5) runtimePlacementActive_[i] = 0;
        // 新規ゲームでは使用済み表示も初期色へ戻す。
        if ((placements_[i].itemId == 26 || placements_[i].itemId == 31) &&
            i < objects_.size() && objects_[i]) {
            if (const ItemDefinition* definition=FindItem(placements_[i].itemId)) {
                objects_[i]->SetColor(definition->color);
            }
        }
    }
    playerRespawnPosition_.z = 0.0f;
    RespawnPlayer(false);
}

// 処理概要: StageEditorが担当する「RespawnPlayer」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void StageEditor::RespawnPlayer(bool damaged) {
    playerPosition_ = playerRespawnPosition_;
    playerPosition_.z = 0.0f;
    playerVelocity_ = {};
    playerGrounded_ = false;
    playerFlashTimer_ = damaged ? 0.8f : 0.0f;
    playerDamageCooldown_ = damaged ? 1.0f : 0.0f;
    if (damaged) ShowRuntimeMessage("DAMAGE!  RESPAWN", 1.2f);
    if (playerObject_) {
        playerObject_->SetPosition(playerPosition_);
        playerObject_->SetRotation({});
        playerObject_->SetScale({1.0f,1.0f,1.0f});
        playerObject_->Update();
    }
}

// 処理概要: ミスした位置でプレイヤーを小型ブロックへ分解する演出を開始する。
// 注意事項: 演出中は通常のプレイヤー更新と敵・ギミック更新を止め、重複発動を防ぐ。
void StageEditor::BeginRespawnEffect(bool fellOutOfStage) {
    if (respawnEffectActive_) return;
    ChangePlayerLives(-1);
    respawnEffectActive_ = true;
    respawnEffectFromFall_ = fellOutOfStage;
    respawnEffectTimer_ = 0.0f;
    respawnEffectOrigin_ = playerPosition_;
    playerVelocity_ = {};
    playerGrounded_ = false;
    playerDamageCooldown_ = kRespawnEffectDuration + 0.5f;

    for (auto& block : respawnEffectBlocks_) {
        if (!block.object) continue;
        block.object->SetPosition(respawnEffectOrigin_);
        block.object->SetRotation({});
        block.object->SetScale({0.14f, 0.14f, 0.14f});
        block.object->SetColor(block.color);
        block.object->Update();
    }
}

// 処理概要: 残機を0～最大値へ制限し、HUDへ増減アニメーションを通知する。
// 注意事項: 同じ値になる変更では演出を再発火せず、死亡処理の重複表示を防ぐ。
void StageEditor::ChangePlayerLives(int amount) {
    if (amount == 0) return;
    const int previousLives = playerLives_;
    playerLives_ = std::clamp(playerLives_ + amount, 0, kMaxPlayerLives);
    const int appliedAmount = playerLives_ - previousLives;
    if (appliedAmount == 0) return;

    lifeChangeAmount_ = appliedAmount;
    lifeChangedSlot_ = appliedAmount < 0
        ? playerLives_
        : playerLives_ - 1;
    lifeChangeTimer_ = kLifeChangeDuration;
}

// 処理概要: 分解、収束、リスポーン地点での再構築を約3秒で連続再生する。
// 注意事項: 最後に通常のRespawnPlayerへ戻し、位置・速度・当たり判定を確実に初期化する。
void StageEditor::UpdateRespawnEffect(float deltaTime) {
    if (!respawnEffectActive_) return;
    respawnEffectTimer_ = (std::min)(
        kRespawnEffectDuration, respawnEffectTimer_ + deltaTime);

    // 死亡時の分解を約1秒、復活時の再構築を約1秒延長する。
    constexpr float scatterEnd = 0.88f;
    constexpr float collapseEnd = 1.48f;
    constexpr float assemblyEnd = 2.70f;
    const auto smoothStep = [](float value) {
        const float t = std::clamp(value, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    };
    const auto lerpVector = [](const Vector3& from, const Vector3& to, float t) {
        return Vector3{
            from.x + (to.x - from.x) * t,
            from.y + (to.y - from.y) * t,
            from.z + (to.z - from.z) * t};
    };

    if (respawnEffectTimer_ >= collapseEnd) {
        // ブロックが消えた瞬間に復帰地点へカメラの追従先だけ移す。
        // プレイヤー本体は演出完了まで描画しない。
        playerPosition_ = playerRespawnPosition_;
        playerPosition_.z = 0.0f;
    }

    for (size_t i = 0; i < respawnEffectBlocks_.size(); ++i) {
        auto& block = respawnEffectBlocks_[i];
        if (!block.object) continue;
        Vector3 position = respawnEffectOrigin_;
        float scale = 0.14f;
        Vector4 color = block.color;

        if (respawnEffectTimer_ < scatterEnd) {
            const float progress = smoothStep(respawnEffectTimer_ / scatterEnd);
            position.x += block.direction.x * progress * 0.90f;
            position.y += block.direction.y * progress * 0.90f;
            position.z += block.direction.z * progress;
            scale *= 0.72f + progress * 0.28f;
        } else if (respawnEffectTimer_ < collapseEnd) {
            const float progress = smoothStep(
                (respawnEffectTimer_ - scatterEnd) / (collapseEnd - scatterEnd));
            Vector3 scattered = respawnEffectOrigin_;
            scattered.x += block.direction.x * 0.90f;
            scattered.y += block.direction.y * 0.90f;
            scattered.z += block.direction.z;
            position = lerpVector(scattered, respawnEffectOrigin_, progress);
            scale *= 1.0f - progress * 0.92f;
            color.w = 1.0f - progress;
        } else if (respawnEffectTimer_ < assemblyEnd) {
            const float progress = smoothStep(
                (respawnEffectTimer_ - collapseEnd) / (assemblyEnd - collapseEnd));
            Vector3 start = playerRespawnPosition_;
            start.x += block.assemblyOffset.x * 2.4f;
            start.y += -1.35f - static_cast<float>(i % 3) * 0.12f;
            Vector3 target = playerRespawnPosition_;
            target.x += block.assemblyOffset.x;
            target.y += block.assemblyOffset.y;
            position = lerpVector(start, target, progress);
            scale = 0.05f + progress * 0.10f;
            color = (i == respawnEffectBlocks_.size() - 1)
                ? Vector4{1.0f, 0.72f, 0.20f, 1.0f}
                : Vector4{0.35f + progress * 0.55f, 0.90f + progress * 0.08f, 1.0f, 1.0f};
        } else {
            const float progress = smoothStep(
                (respawnEffectTimer_ - assemblyEnd) /
                (kRespawnEffectDuration - assemblyEnd));
            position = playerRespawnPosition_;
            position.x += block.assemblyOffset.x * (1.0f - progress);
            position.y += block.assemblyOffset.y * (1.0f - progress);
            scale = 0.15f * (1.0f - progress);
            color = {1.0f, 0.88f + progress * 0.12f, 0.55f + progress * 0.45f,
                1.0f - progress};
        }

        block.object->SetPosition(position);
        block.object->SetRotation({
            respawnEffectTimer_ * block.rotationSpeed,
            respawnEffectTimer_ * block.rotationSpeed * 0.7f,
            respawnEffectTimer_ * block.rotationSpeed * 0.45f});
        block.object->SetScale({scale, scale, scale});
        block.object->SetColor(color);
        block.object->Update();
    }

    if (respawnEffectTimer_ >= kRespawnEffectDuration) {
        const bool fellOutOfStage = respawnEffectFromFall_;
        respawnEffectActive_ = false;
        if (playerLives_ <= 0) {
            // 最後の分解演出を最後まで見せた後、プレイヤーを復元せず
            // 専用のゲームオーバー画面へ切り替える。
            playerVelocity_ = {};
            playerGrounded_ = false;
            gameOverTimer_ = 0.0f;
            gameOverExitActive_ = false;
            gameOverExitTimer_ = 0.0f;
            gameFlowState_ = GameFlowState::GameOver;
            runtimeMessage_.clear();
            runtimeMessageTimer_ = 0.0f;
            return;
        }
        RespawnPlayer(false);
        playerFlashTimer_ = 0.65f;
        playerDamageCooldown_ = 1.0f;
        ShowRuntimeMessage(
            fellOutOfStage ? "BACK TO CHECKPOINT" : "DAMAGE!  REBUILT",
            1.0f);
    }
}

// 処理概要: StageEditorが担当する「ShowRuntimeMessage」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void StageEditor::ShowRuntimeMessage(const std::string& message, float seconds) {
    runtimeMessage_ = message;
    runtimeMessageTimer_ = seconds;
}

// 処理概要: 現在の状態を再読込可能な形式で永続化する。
// 注意事項: 保存先と書き込み結果を検証し、失敗を呼び出し側へ伝える。
bool StageEditor::SaveProgressAt(size_t placementIndex) {
    if (placementIndex >= placements_.size() || currentFile_.empty()) return false;
    namespace fs = std::filesystem;
    std::error_code error;
    fs::create_directories(fs::path(kProgressSavePath).parent_path(), error);
    if (error) return false;

    nlohmann::json root;
    root["version"] = 1;
    root["stage_file"] = fs::path(currentFile_).generic_string();
    root["stage_name"] = stageName_;
    root["save_point_id"] = placements_[placementIndex].id;
    root["position"] = {playerRespawnPosition_.x, playerRespawnPosition_.y, playerRespawnPosition_.z};
    root["stars"] = collectedStars_;
    root["bubbles"] = collectedBubbles_;
    root["keys"] = keyCount_;
    root["lives"] = playerLives_;
    root["used_save_point_ids"] = nlohmann::json::array();
    for (size_t i=0;i<placements_.size()&&i<runtimeSavePointUsed_.size();++i) {
        if (runtimeSavePointUsed_[i]!=0) {
            root["used_save_point_ids"].push_back(placements_[i].id);
        }
    }
    root["inactive_placement_ids"] = nlohmann::json::array();
    for (size_t i = 0; i < placements_.size() && i < runtimePlacementActive_.size(); ++i) {
        if (runtimePlacementActive_[i] == 0) root["inactive_placement_ids"].push_back(placements_[i].id);
    }

    std::ofstream output(kProgressSavePath);
    if (!output) return false;
    output << std::setw(2) << root;
    const bool saved = output.good();
    output.close();
    hasSaveData_ = saved;
    return saved;
}

// 処理概要: 外部データを読み込み、実行時に扱える形式へ変換する。
// 注意事項: 読込失敗時に既存の有効な状態を不必要に破壊しない。
bool StageEditor::LoadSavedProgress() {
    try {
        std::ifstream input(kProgressSavePath);
        if (!input) return false;
        nlohmann::json root;
        input >> root;
        const std::string savedStage = root.value("stage_file", std::string{});
        if (savedStage.empty() || !std::filesystem::exists(savedStage)) return false;
        if (std::filesystem::path(currentFile_).lexically_normal() !=
            // 処理概要: filesystemが担当する「path」処理を実行する。
            // 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
            std::filesystem::path(savedStage).lexically_normal()) {
            if (!LoadStage(savedStage)) return false;
        }

        auto position = root.value("position", std::vector<float>{});
        if (position.size() != 3) return false;
        playerRespawnPosition_ = {position[0], position[1], 0.0f};
        playerPosition_ = playerRespawnPosition_;
        playerVelocity_ = {};
        collectedStars_ = root.value("stars", 0);
        collectedBubbles_ = root.value("bubbles", 0);
        keyCount_ = root.value("keys", 0);
        playerLives_ = std::clamp(root.value("lives", kMaxPlayerLives), 0, kMaxPlayerLives);

        // コンティニュー後も使用済みセーブポイントを復元し、
        // 過去の地点へ戻って触れてもリスポーン位置が巻き戻らないようにする。
        if (runtimeSavePointUsed_.size()!=placements_.size()) {
            runtimeSavePointUsed_.assign(placements_.size(),0);
        }
        if (root.contains("used_save_point_ids")&&root["used_save_point_ids"].is_array()) {
            for (const auto& value:root["used_save_point_ids"]) {
                const uint32_t id=value.get<uint32_t>();
                for (size_t i=0;i<placements_.size();++i) {
                    if (placements_[i].id!=id||i>=runtimeSavePointUsed_.size()) continue;
                    runtimeSavePointUsed_[i]=1;
                    if (i<objects_.size()&&objects_[i]) objects_[i]->SetColor({1.0f,0.9f,0.2f,1.0f});
                    break;
                }
            }
        } else {
            // 旧形式のセーブデータでは、最後に記録した地点だけでも使用済みにする。
            const uint32_t savedPointId=root.value("save_point_id",0u);
            for (size_t i=0;i<placements_.size();++i) {
                if (placements_[i].id==savedPointId&&i<runtimeSavePointUsed_.size()) {
                    runtimeSavePointUsed_[i]=1;
                    if (i<objects_.size()&&objects_[i]) objects_[i]->SetColor({1.0f,0.9f,0.2f,1.0f});
                    break;
                }
            }
        }

        if (root.contains("inactive_placement_ids") && root["inactive_placement_ids"].is_array()) {
            for (const auto& value : root["inactive_placement_ids"]) {
                const uint32_t id = value.get<uint32_t>();
                for (size_t i = 0; i < placements_.size(); ++i) {
                    if (placements_[i].id == id && i < runtimePlacementActive_.size()) {
                        runtimePlacementActive_[i] = 0;
                        break;
                    }
                }
            }
        }
        ShowRuntimeMessage("CONTINUE FROM SAVE POINT", 1.6f);
        return true;
    } catch (const std::exception&) {
        hasSaveData_ = false;
        return false;
    }
}

// 処理概要: StageEditorが担当する「RefreshSaveDataAvailability」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void StageEditor::RefreshSaveDataAvailability() {
    std::error_code error;
    hasSaveData_ = std::filesystem::is_regular_file(kProgressSavePath, error) && !error;
}
// 処理概要: 現在の状態が指定された条件を満たすか判定する。
// 注意事項: 状態を変更せず、判定結果だけを返す。
bool StageEditor::IsRuntimePlacementActive(size_t index) const {
    return index >= runtimePlacementActive_.size() || runtimePlacementActive_[index] != 0;
}

// 処理概要: 外部から必要な状態またはリソース参照を取得する。
// 注意事項: 返す参照やポインターの寿命は所有オブジェクトに従う。
Vector3 StageEditor::GetRuntimePlacementPosition(size_t index) const {
    return index < runtimePlacementPositions_.size() ? runtimePlacementPositions_[index] : placements_[index].position;
}

// 処理概要: 現在の状態が指定された条件を満たすか判定する。
// 注意事項: 状態を変更せず、判定結果だけを返す。
bool StageEditor::IsPlayerOverlappingPlacement(size_t index, float padding) const {
    if (index >= placements_.size() || !IsRuntimePlacementActive(index)) return false;
    const Placement& placement = placements_[index];
    const Vector3 position = GetRuntimePlacementPosition(index);
    const Vector3 halfExtents = GetPlacementHalfExtents(placement);
    const float halfX = halfExtents.x + padding;
    const float halfY = halfExtents.y + padding;
    return std::abs(playerPosition_.x - position.x) < kPlayerHalfSize + halfX &&
        std::abs(playerPosition_.y - position.y) < kPlayerHalfSize + halfY;
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void StageEditor::UpdateRuntimeObjects(float deltaTime) {
    runtimeTime_ += deltaTime;
    runtimeMessageTimer_ = (std::max)(0.0f, runtimeMessageTimer_ - deltaTime);
    doorCooldown_ = (std::max)(0.0f, doorCooldown_ - deltaTime);
    playerFlashTimer_ = (std::max)(0.0f, playerFlashTimer_ - deltaTime);
    playerDamageCooldown_ = (std::max)(0.0f, playerDamageCooldown_ - deltaTime);

    if (runtimePlacementPositions_.size() != placements_.size()) ResetRuntimeState();
    UpdateEnemies(deltaTime);
    for (size_t i = 0; i < placements_.size(); ++i) {
        const Vector3 previousPosition = runtimePlacementPositions_[i];
        const int itemId = placements_[i].itemId;
        Vector3 position = ((itemId >= 22 && itemId <= 24) || itemId == 28)
            ? runtimePlacementPositions_[i]
            : placements_[i].position;
        if (placements_[i].itemId == 8) {
            const float amount = 0.5f - 0.5f * std::cos(runtimeTime_ * 1.5f);
            position.x += placements_[i].moveOffset.x * amount;
            position.y += placements_[i].moveOffset.y * amount;
            position.z += placements_[i].moveOffset.z * amount;
            const Vector3 halfExtents = GetPlacementHalfExtents(placements_[i]);
            const float halfX = halfExtents.x;
            const float halfY = halfExtents.y;
            const float oldTop = previousPosition.y + halfY;
            const bool standingOnPlatform =
                std::abs(playerPosition_.x - previousPosition.x) <= kPlayerHalfSize + halfX &&
                std::abs((playerPosition_.y - kPlayerHalfSize) - oldTop) <= 0.08f;
            if (standingOnPlatform) {
                playerPosition_.x += position.x - previousPosition.x;
                playerPosition_.y += position.y - previousPosition.y;
            }
        } else if (itemId == 28) {
            const Vector3 halfExtents = GetPlacementHalfExtents(placements_[i]);
            const float halfX = halfExtents.x;
            const float halfY = halfExtents.y;
            const float oldTop = previousPosition.y + halfY;
            const bool standingOnFloor = IsRuntimePlacementActive(i) &&
                std::abs(playerPosition_.x - previousPosition.x) <= kPlayerHalfSize + halfX &&
                std::abs((playerPosition_.y - kPlayerHalfSize) - oldTop) <= 0.09f;
            uint8_t& state = runtimeFallingFloorStates_[i];
            float& timer = runtimeFallingFloorTimers_[i];
            float& velocity = runtimeFallingFloorVelocities_[i];

            if (state == 0) { // Waiting for the player.
                position = placements_[i].position;
                if (standingOnFloor) {
                    state = 1;
                    timer = 0.7f;
                    ShowRuntimeMessage("FLOOR IS FALLING!", 0.8f);
                }
            } else if (state == 1) { // Warning shake before falling.
                timer -= deltaTime;
                position = placements_[i].position;
                position.x += std::sin(runtimeTime_ * 55.0f) * 0.035f;
                if (timer <= 0.0f) {
                    state = 2;
                    velocity = 0.0f;
                }
            } else if (state == 2) { // Falling under gravity.
                velocity -= 18.0f * deltaTime;
                position.y += velocity * deltaTime;
                if (position.y < -3.0f) {
                    state = 3;
                    timer = 1.6f;
                    runtimePlacementActive_[i] = 0;
                    position = placements_[i].position;
                }
            } else { // Hidden briefly, then restored at its saved position.
                timer -= deltaTime;
                position = placements_[i].position;
                if (timer <= 0.0f) {
                    state = 0;
                    velocity = 0.0f;
                    runtimePlacementActive_[i] = 1;
                }
            }

            // 警告用の横揺れは床の見た目だけに適用し、自機のX座標は動かさない。
            // 1マス幅の穴で自機まで横へ揺らすと、左右の通常床へわずかに食い込み、
            // タイル衝突によって横へ弾き出される原因になる。
            //
            // 実際に床が落下している間はY方向の移動量だけを自機へ加える。
            // 自機が床上に乗り続けている場合は床と一緒に真下へ落ち、
            // 落下ラインを越えた時に通常のリスポーン処理へ移行する。
            if (standingOnFloor && state == 2) {
                playerPosition_.y += position.y - previousPosition.y;
            }
            if (i < objects_.size() && objects_[i]) {
                objects_[i]->SetColor(state == 1
                    ? Vector4{1.0f,0.2f,0.08f,1.0f}
                    : Vector4{0.82f,0.48f,0.18f,1.0f});
            }
        }
        runtimePlacementPositions_[i] = position;
        if (i < objects_.size() && objects_[i]) {
            ApplyPlacementVisualTransform(objects_[i].get(), placements_[i], position);
        }
    }

    if (playerObject_) {
        const Vector4 color = goalReached_
            ? Vector4{0.2f,1.0f,0.35f,1.0f}
            : (playerFlashTimer_ > 0.0f ? Vector4{1.0f,0.15f,0.12f,1.0f} : Vector4{0.12f,0.55f,1.0f,1.0f});
        playerObject_->SetColor(color);
    }
}

// 処理概要: 現在の状態が指定された条件を満たすか判定する。
// 注意事項: 状態を変更せず、判定結果だけを返す。
bool StageEditor::IsPlacementBlocked(const Vector3& position, float halfX, float halfY) const {
    constexpr float inset = 0.01f;
    const int minX = static_cast<int>(std::floor((position.x - halfX + inset) / kTileWorldSize));
    const int maxX = static_cast<int>(std::floor((position.x + halfX - inset) / kTileWorldSize));
    const int minY = static_cast<int>(std::floor((position.y - halfY + inset) / kTileWorldSize));
    const int maxY = static_cast<int>(std::floor((position.y + halfY - inset) / kTileWorldSize));
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            if (IsCollisionSolid(x, y)) return true;
        }
    }
    return false;
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void StageEditor::UpdateEnemies(float deltaTime) {
    if (goalReached_) return;
    if (runtimeEnemyDirections_.size() != placements_.size()) {
        runtimeEnemyDirections_.assign(placements_.size(), 1.0f);
    }
    if (runtimeEnemyAlerted_.size() != placements_.size()) {
        runtimeEnemyAlerted_.assign(placements_.size(), 0);
    }

    for (size_t i = 0; i < placements_.size(); ++i) {
        const Placement& placement = placements_[i];
        if (placement.itemId < 22 || placement.itemId > 24 || !IsRuntimePlacementActive(i)) continue;

        Vector3 position = runtimePlacementPositions_[i];
        const Vector3 halfExtents = GetPlacementHalfExtents(placement);
        const float halfX = halfExtents.x;
        const float halfY = halfExtents.y;

        if (placement.itemId == 22) { // Walker: walls and ledges reverse its patrol direction.
            constexpr float speed = 1.6f;
            float direction = runtimeEnemyDirections_[i];
            Vector3 candidate = position;
            candidate.x += direction * speed * deltaTime;
            const int aheadX = static_cast<int>(std::floor(
                (candidate.x + direction * (halfX + 0.04f)) / kTileWorldSize));
            // Enemies are visually smaller than one tile, but their placement is
            // tile-centered. Probe from the bottom of that placement cell so a
            // walker placed directly above a floor recognizes the floor below.
            const int groundY = static_cast<int>(std::floor(
                (candidate.y - (std::max)(halfY, kTileWorldSize * 0.5f) - 0.06f) / kTileWorldSize));
            const bool wallAhead = IsPlacementBlocked(candidate, halfX, halfY);
            const bool ledgeAhead = !IsCollisionSolid(aheadX, groundY);
            if (wallAhead || ledgeAhead) {
                runtimeEnemyDirections_[i] = -direction;
            } else {
                position = candidate;
            }
        } else if (placement.itemId == 23) { // Flyer: loops around its saved spawn point.
            const float phase = static_cast<float>(i) * 0.73f;
            Vector3 candidate = placement.position;
            candidate.x += std::sin(runtimeTime_ * 1.25f + phase) * 2.0f;
            candidate.y += std::sin(runtimeTime_ * 2.1f + phase) * 0.65f;
            if (!IsPlacementBlocked(candidate, halfX, halfY)) position = candidate;
        } else { // Chaser: detects the player, pursues, then returns to its spawn point.
            constexpr float detectDistance = 7.0f;
            constexpr float loseDistance = 9.0f;
            const float playerDx = playerPosition_.x - position.x;
            const float playerDy = playerPosition_.y - position.y;
            const float playerDistance = std::sqrt(playerDx * playerDx + playerDy * playerDy);
            const bool wasAlerted = runtimeEnemyAlerted_[i] != 0;
            if (!wasAlerted && playerDistance <= detectDistance) {
                runtimeEnemyAlerted_[i] = 1;
                ShowRuntimeMessage("CHASER ALERT!", 0.8f);
            } else if (wasAlerted && playerDistance >= loseDistance) {
                runtimeEnemyAlerted_[i] = 0;
            }

            const bool alerted = runtimeEnemyAlerted_[i] != 0;
            const Vector3 target = alerted ? playerPosition_ : placement.position;
            const float dx = target.x - position.x;
            const float dy = target.y - position.y;
            const float distance = std::sqrt(dx * dx + dy * dy);
            if (distance > 0.02f) {
                const float moveDistance = (alerted ? 2.6f : 1.4f) * deltaTime;
                const float step = (std::min)(moveDistance, distance) / distance;
                Vector3 candidate = position;
                candidate.x += dx * step;
                if (!IsPlacementBlocked(candidate, halfX, halfY)) position.x = candidate.x;
                candidate = position;
                candidate.y += dy * step;
                if (!IsPlacementBlocked(candidate, halfX, halfY)) position.y = candidate.y;
            }
            if (i < objects_.size() && objects_[i]) {
                objects_[i]->SetColor(alerted
                    ? Vector4{1.0f,0.9f,0.1f,1.0f}
                    : Vector4{0.95f,0.12f,0.12f,1.0f});
            }
        }

        runtimePlacementPositions_[i] = position;
    }
}

// 処理概要: 現在の状態が指定された条件を満たすか判定する。
// 注意事項: 状態を変更せず、判定結果だけを返す。
bool StageEditor::IsCollisionSolid(int gridX, int gridY) const {
    // 左右の範囲外だけは見えない壁として扱い、ステージ外へ横方向に
    // 出られないようにする。
    if (gridX < 0 || gridX >= stageWidth_) return true;

    // 上下の範囲外には衝突を置かない。
    // 特にgridY < 0をSolidにすると、穴へ落ちた自機がY=0付近の
    // 「見えない床」に着地してしまい、落下復帰ラインへ到達できない。
    if (gridY < 0 || gridY >= stageHeight_) return false;

    // 崩れる床は配置オブジェクトとして独自の当たり判定を持つ。
    // 古いステージデータには、崩れる床と同じセルに通常床タイルが残っているものがあり、
    // 崩れる床だけが消えた後も通常床の判定が「透明な床」として残ってしまう。
    // 崩れる床が置かれたセルでは下地タイルを常に無視し、床の状態は配置物側だけで管理する。
    for (const Placement& placement : placements_) {
        if (placement.itemId != 28) continue;
        const int floorGridX = static_cast<int>(std::floor(placement.position.x / kTileWorldSize));
        const int floorGridY = static_cast<int>(std::floor(placement.position.y / kTileWorldSize));
        if (floorGridX == gridX && floorGridY == gridY) return false;
    }

    return GetTileCollisionType(gridX, gridY) == TileCollisionType::Solid;
}

// 処理概要: StageEditorが担当する「MovePlayerHorizontal」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void StageEditor::MovePlayerHorizontal(float amount) {
    if (amount == 0.0f) return;
    constexpr float epsilon = 0.001f;
    float targetX = playerPosition_.x + amount;
    const float bottom = playerPosition_.y - kPlayerHalfSize + epsilon;
    const float top = playerPosition_.y + kPlayerHalfSize - epsilon;
    const int minY = static_cast<int>(std::floor(bottom / kTileWorldSize));
    const int maxY = static_cast<int>(std::floor(top / kTileWorldSize));
    const int tileX = static_cast<int>(std::floor(
        (targetX + (amount > 0.0f ? kPlayerHalfSize : -kPlayerHalfSize)) / kTileWorldSize));
    for (int y = minY; y <= maxY; ++y) {
        if (!IsCollisionSolid(tileX, y)) continue;
        targetX = amount > 0.0f
            ? tileX * kTileWorldSize - kPlayerHalfSize - epsilon
            : (tileX + 1) * kTileWorldSize + kPlayerHalfSize + epsilon;
        playerVelocity_.x = 0.0f;
        break;
    }
    playerPosition_.x = targetX;
}

// 処理概要: StageEditorが担当する「MovePlayerVertical」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void StageEditor::MovePlayerVertical(float amount) {
    if (amount == 0.0f) return;
    constexpr float epsilon = 0.001f;
    float targetY = playerPosition_.y + amount;
    const float left = playerPosition_.x - kPlayerHalfSize + epsilon;
    const float right = playerPosition_.x + kPlayerHalfSize - epsilon;
    const int minX = static_cast<int>(std::floor(left / kTileWorldSize));
    const int maxX = static_cast<int>(std::floor(right / kTileWorldSize));
    const int tileY = static_cast<int>(std::floor(
        (targetY + (amount > 0.0f ? kPlayerHalfSize : -kPlayerHalfSize)) / kTileWorldSize));
    for (int x = minX; x <= maxX; ++x) {
        if (!IsCollisionSolid(x, tileY)) continue;
        targetY = amount > 0.0f
            ? tileY * kTileWorldSize - kPlayerHalfSize - epsilon
            : (tileY + 1) * kTileWorldSize + kPlayerHalfSize + epsilon;
        if (amount < 0.0f) playerGrounded_ = true;
        playerVelocity_.y = 0.0f;
        break;
    }
    playerPosition_.y = targetY;
}

// 処理概要: StageEditorが担当する「ResolvePlacedSolidCollisions」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void StageEditor::ResolvePlacedSolidCollisions() {
    constexpr float epsilon = 0.001f;
    for (size_t i = 0; i < placements_.size(); ++i) {
        if (!IsRuntimePlacementActive(i)) continue;
        const int itemId = placements_[i].itemId;
        if (itemId != 4 && itemId != 5 && itemId != 8 && itemId != 9 && itemId != 10 && itemId != 28 && itemId != 30) continue;

        if (itemId == 28) {
            // 崩れる床には通常の箱型押し出しを使わず、上面だけの一方向床判定を使う。
            // これにより、床の横端や揺れに触れても自機が左右へ弾かれず、
            // 下側から接触した場合にも床へ挟まらない。
            if (i >= runtimeFallingFloorStates_.size() || runtimeFallingFloorStates_[i] >= 3) {
                // 画面下へ落ちて消滅した後は完全に当たり判定をなくす。
                continue;
            }

            const Vector3 floorPosition = GetRuntimePlacementPosition(i);
            const Vector3 floorHalfExtents = GetPlacementHalfExtents(placements_[i]);
            const float floorHalfX = floorHalfExtents.x;
            const float floorHalfY = floorHalfExtents.y;
            const float floorTop = floorPosition.y + floorHalfY;
            const float playerBottom = playerPosition_.y - kPlayerHalfSize;
            const bool overlapsHorizontally =
                std::abs(playerPosition_.x - floorPosition.x) < kPlayerHalfSize + floorHalfX;
            const bool crossedTopSurface =
                playerBottom <= floorTop + 0.03f && playerBottom >= floorTop - 0.18f;

            if (overlapsHorizontally && crossedTopSurface &&
                playerPosition_.y >= floorPosition.y && playerVelocity_.y <= 0.0f) {
                // 上から降りてきた時だけ床上面へ着地させる。
                playerPosition_.y = floorTop + kPlayerHalfSize + epsilon;
                playerVelocity_.y = 0.0f;
                playerGrounded_ = true;
            }
            continue;
        }

        if (!IsPlayerOverlappingPlacement(i)) continue;

        const Vector3 position = GetRuntimePlacementPosition(i);
        const Vector3 halfExtents = GetPlacementHalfExtents(placements_[i]);
        const float halfX = halfExtents.x;
        const float halfY = halfExtents.y;
        const float dx = playerPosition_.x - position.x;
        const float dy = playerPosition_.y - position.y;
        const float penetrationX = kPlayerHalfSize + halfX - std::abs(dx);
        const float penetrationY = kPlayerHalfSize + halfY - std::abs(dy);

        if (penetrationY <= penetrationX) {
            if (dy >= 0.0f) {
                playerPosition_.y += penetrationY + epsilon;
                playerGrounded_ = true;
            } else {
                playerPosition_.y -= penetrationY + epsilon;
            }
            playerVelocity_.y = 0.0f;
        } else {
            playerPosition_.x += dx >= 0.0f ? penetrationX + epsilon : -penetrationX - epsilon;
            playerVelocity_.x = 0.0f;
        }
    }
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void StageEditor::UpdateGimmickCollisions(Input* input, float deltaTime) {
    const bool enterDoor = input &&
        (input->PushKey(DIK_W) || input->PushKey(DIK_UP) || input->GetLeftStickY() > 0.5f);

    for (size_t i = 0; i < placements_.size(); ++i) {
        if (!IsRuntimePlacementActive(i)) continue;
        const int itemId = placements_[i].itemId;
        const bool touching = IsPlayerOverlappingPlacement(i, 0.04f);
        const bool wasTouching = i < runtimePlacementTouching_.size() && runtimePlacementTouching_[i] != 0;
        if (i < runtimePlacementTouching_.size()) runtimePlacementTouching_[i] = touching ? 1 : 0;
        if (!touching) continue;

        switch (itemId) {
        case 9: // Key Block
            if (keyCount_ > 0) {
                --keyCount_;
                runtimePlacementActive_[i] = 0;
                ShowRuntimeMessage("KEY BLOCK OPEN", 1.2f);
            } else if (!wasTouching) {
                ShowRuntimeMessage("A KEY IS REQUIRED", 1.2f);
            }
            break;
        case 15: // Star
            runtimePlacementActive_[i] = 0;
            ++collectedStars_;
            ShowRuntimeMessage("STAR GET!", 1.2f);
            break;
        case 16: // Bubble
            runtimePlacementActive_[i] = 0;
            ++collectedBubbles_;
            ShowRuntimeMessage("BUBBLE GET!", 1.2f);
            break;
        case 17: // Goal
            if (!goalReached_) {
                goalReached_ = true;
                playerVelocity_ = {};
                gameFlowState_ = GameFlowState::Clear;
                // クリア表示を短時間見せた後、UpdateGameFlow()でタイトルへ自動復帰する。
                clearReturnTimer_ = 2.5f;
                ShowRuntimeMessage("GOAL CLEAR!", clearReturnTimer_);
            }
            break;
        case 18: // Door
            if (enterDoor && doorCooldown_ <= 0.0f) {
                for (size_t destination = 0; destination < placements_.size(); ++destination) {
                    if (destination == i || placements_[destination].itemId != 18 ||
                        placements_[destination].variant != placements_[i].variant) continue;
                    playerPosition_ = GetRuntimePlacementPosition(destination);
                    playerPosition_.z = 0.0f;
                    playerVelocity_ = {};
                    doorCooldown_ = 0.75f;
                    ShowRuntimeMessage("DOOR WARP", 1.0f);
                    break;
                }
            } else if (!wasTouching && doorCooldown_ <= 0.0f) {
                ShowRuntimeMessage("UP: ENTER DOOR", 1.0f);
            }
            break;
        case 19: // P Switch
            if (!wasTouching) {
                pSwitchActive_ = !pSwitchActive_;
                if (i < objects_.size() && objects_[i]) {
                    objects_[i]->SetColor(pSwitchActive_
                        ? Vector4{0.2f,1.0f,0.35f,1.0f}
                        : Vector4{0.15f,0.45f,0.95f,1.0f});
                }
                for (size_t block = 0; block < placements_.size(); ++block) {
                    if (placements_[block].variant != placements_[i].variant) continue;
                    if (placements_[block].itemId == 4) runtimePlacementActive_[block] = pSwitchActive_ ? 0 : 1;
                    if (placements_[block].itemId == 5) runtimePlacementActive_[block] = pSwitchActive_ ? 1 : 0;
                }
                ShowRuntimeMessage(pSwitchActive_ ? "P SWITCH: ON" : "P SWITCH: OFF", 1.2f);
            }
            break;
        case 20: // Key
            runtimePlacementActive_[i] = 0;
            ++keyCount_;
            ShowRuntimeMessage("KEY GET!", 1.2f);
            break;
        case 21: // On/Off Switch
            if (!wasTouching) {
                onOffActive_ = !onOffActive_;
                if (i < objects_.size() && objects_[i]) {
                    objects_[i]->SetColor(onOffActive_
                        ? Vector4{0.95f,0.2f,0.2f,1.0f}
                        : Vector4{0.2f,0.45f,0.95f,1.0f});
                }
                ShowRuntimeMessage(onOffActive_ ? "ON BLOCKS ACTIVE" : "OFF BLOCKS ACTIVE", 1.2f);
            }
            break;
        case 22: // Enemy Walker
        case 23: // Enemy Flyer
        case 24: // Enemy Chaser
        case 27: // Spike
            // Ignore hazards briefly after respawning. Without this guard an
            // enemy overlapping the checkpoint could immediately damage the
            // player again on every frame.
            if (playerDamageCooldown_ <= 0.0f) BeginRespawnEffect(false);
            return;
        case 29: { // Jump Pad
            const Vector3 padPosition = GetRuntimePlacementPosition(i);
            const float padHalfY = GetPlacementHalfExtents(placements_[i]).y;
            // Activate only while landing from above; side and underside
            // contacts must not unexpectedly launch the player.
            if (playerPosition_.y >= padPosition.y && playerVelocity_.y <= 0.0f) {
                playerPosition_.y = padPosition.y + padHalfY + kPlayerHalfSize + 0.001f;
                playerVelocity_.y = kPlayerJumpSpeed * 1.55f;
                playerGrounded_ = false;
                ShowRuntimeMessage("JUMP PAD!", 0.7f);
            }
            break;
        }
        case 30: { // Conveyor Floor
            const Vector3 conveyorPosition = GetRuntimePlacementPosition(i);
            const float conveyorHalfY = GetPlacementHalfExtents(placements_[i]).y;
            const float playerBottom = playerPosition_.y - kPlayerHalfSize;
            const float conveyorTop = conveyorPosition.y + conveyorHalfY;
            // Only carry a player standing on the upper face. Side contact must
            // not pull the player through a wall or across the underside.
            if (playerVelocity_.y <= 0.0f && std::abs(playerBottom - conveyorTop) <= 0.12f) {
                const float direction = placements_[i].variant < 0 ? -1.0f : 1.0f;
                MovePlayerHorizontal(direction * 3.0f * deltaTime);
                if (!wasTouching) ShowRuntimeMessage(direction < 0.0f ? "CONVEYOR: LEFT" : "CONVEYOR: RIGHT", 0.8f);
            }
            break;
        }
        case 26: // Checkpoint
            if (!wasTouching && i < runtimeSavePointUsed_.size() && runtimeSavePointUsed_[i] == 0) {
                // 一度有効化したCheckpointは、以後接触しても復帰地点を書き換えない。
                runtimeSavePointUsed_[i] = 1;
                playerRespawnPosition_ = GetRuntimePlacementPosition(i);
                playerRespawnPosition_.y += 0.6f;
                playerRespawnPosition_.z = 0.0f;
                if (i < objects_.size() && objects_[i]) {
                    objects_[i]->SetColor({1.0f,0.85f,0.15f,1.0f});
                }
                ShowRuntimeMessage("CHECKPOINT!", 1.4f);
            }
            break;
        case 31: // Save Point
            if (!wasTouching && i < runtimeSavePointUsed_.size() && runtimeSavePointUsed_[i] == 0) {
                playerRespawnPosition_ = GetRuntimePlacementPosition(i);
                playerRespawnPosition_.y += 0.6f;
                playerRespawnPosition_.z = 0.0f;
                // SaveProgressAt()が現在地点も使用済みとしてJSONへ書けるよう、
                // 保存処理の前にフラグを立てる。保存失敗時は再試行できるよう戻す。
                runtimeSavePointUsed_[i] = 1;
                if (SaveProgressAt(i)) {
                    if (i < objects_.size() && objects_[i]) objects_[i]->SetColor({1.0f,0.9f,0.2f,1.0f});
                    ShowRuntimeMessage("GAME SAVED!", 1.5f);
                } else {
                    runtimeSavePointUsed_[i] = 0;
                    ShowRuntimeMessage("SAVE FAILED", 1.5f);
                }
            }
            break;        default:
            break;
        }
    }
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void StageEditor::UpdatePlayer(Input* input) {
    constexpr float deltaTime = 1.0f / 60.0f;
    if (respawnEffectActive_) {
        UpdateRespawnEffect(deltaTime);
        return;
    }

    float moveInput = 0.0f;
    float climbInput = 0.0f;
    bool jumpTriggered = false;
    if (input) {
        if (input->PushKey(DIK_A) || input->PushKey(DIK_LEFT)) moveInput -= 1.0f;
        if (input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT)) moveInput += 1.0f;
        const float stickX = input->GetLeftStickX();
        if (std::abs(stickX) > std::abs(moveInput)) moveInput = stickX;
        if (input->PushKey(DIK_W) || input->PushKey(DIK_UP)) climbInput += 1.0f;
        if (input->PushKey(DIK_S) || input->PushKey(DIK_DOWN)) climbInput -= 1.0f;
        const float stickY = input->GetLeftStickY();
        if (std::abs(stickY) > std::abs(climbInput)) climbInput = stickY;
        if (suppressStartJumpUntilReleased_) {
            const bool startInputHeld = input->PushKey(DIK_RETURN) ||
                input->PushKey(DIK_SPACE) ||
                input->PushGamepadButton(XINPUT_GAMEPAD_A);
            // プレイ開始後に全決定ボタンが一度離されるまでは、タイトル画面での
            // 連打をジャンプとして扱わない。離した次の新規入力から通常操作へ戻す。
            if (!startInputHeld) suppressStartJumpUntilReleased_ = false;
            jumpTriggered = false;
        } else {
            jumpTriggered = input->TriggerKey(DIK_RETURN) || input->TriggerKey(DIK_SPACE) ||
                input->TriggerGamepadButton(XINPUT_GAMEPAD_A);
        }
    }

    if (goalReached_) {
        playerVelocity_ = {};
        if (playerObject_) playerObject_->Update();
        return;
    }

    bool onLadder = false;
    for (size_t i = 0; i < placements_.size(); ++i) {
        if (placements_[i].itemId == 14 && IsPlayerOverlappingPlacement(i, 0.08f)) {
            onLadder = true;
            break;
        }
    }
    playerVelocity_.x = moveInput * kPlayerMoveSpeed;
    if (onLadder && std::abs(climbInput) > 0.0f) {
        playerVelocity_.y = climbInput * kPlayerMoveSpeed * 0.75f;
        playerGrounded_ = false;
    } else if (jumpTriggered && playerGrounded_) {
        playerVelocity_.y = kPlayerJumpSpeed;
        playerGrounded_ = false;
    } else {
        playerVelocity_.y -= kPlayerGravity * deltaTime;
    }
    MovePlayerHorizontal(playerVelocity_.x * deltaTime);
    playerGrounded_ = false;
    MovePlayerVertical(playerVelocity_.y * deltaTime);
    UpdateGimmickCollisions(input, deltaTime);
    if (respawnEffectActive_) return;
    ResolvePlacedSolidCollisions();

    if (playerGrounded_) {
        const int tileX = static_cast<int>(std::floor(playerPosition_.x / kTileWorldSize));
        const int tileY = static_cast<int>(std::floor((playerPosition_.y - kPlayerHalfSize - 0.01f) / kTileWorldSize));
        const int tileIndex = TileIndex(tileX, tileY);
        if (GetTileAt(tileX, tileY) == 6 && tileIndex >= 0 &&
            tileIndex < static_cast<int>(runtimeTileActive_.size()) && runtimeTileActive_[tileIndex] != 0) {
            runtimeTileActive_[tileIndex] = 0;
            ShowRuntimeMessage("CRUMBLING!", 0.8f);
        }
    }

    // 落下判定の主条件には、ステージ下端より十分下に置いた固定の復帰ラインを使う。
    // 以前はカメラへの投影結果だけで判定していたため、clip.wが無効になる位置まで
    // 落ちると判定が更新されず、床のない場所で自機が残り続ける場合があった。
    bool fellOutOfStage = playerPosition_.y < kPlayerFallLimitY;

    // 見た目上すでに画面下へ消えた場合は、固定ラインへ到達する前でも復帰させる。
    // こちらは演出を自然にするための補助判定であり、失敗しても上の固定ラインが
    // 最終的に必ず落下を検出する。
    Camera* camera = object3dCommon_ ? object3dCommon_->GetDefaultCamera() : nullptr;
    if (camera) {
        const Vector4 clipPosition = TransformPoint4(
            {playerPosition_.x, playerPosition_.y, playerPosition_.z, 1.0f},
            camera->GetViewProjectionMatrix());
        if (clipPosition.w > 0.001f) {
            const float normalizedDeviceY = clipPosition.y / clipPosition.w;
            fellOutOfStage = fellOutOfStage || normalizedDeviceY < -1.0f;
        }
    }

    // playerRespawnPosition_はPlayer Startを初期値とし、Checkpointまたは
    // Save Pointに触れるたびに直近の位置へ更新される。
    if (fellOutOfStage) {
        BeginRespawnEffect(true);
        return;
    }
    if (playerObject_) {
        playerObject_->SetPosition(playerPosition_);
        playerObject_->SetScale({1.0f,1.0f,1.0f});
        playerObject_->Update();
    }
}

void StageEditor::ToggleMode(){SetMode(mode_==Mode::Editor?Mode::GamePlay:Mode::Editor);}

// 処理概要: StageEditorが担当する「PlaceItem」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void StageEditor::PlaceItem(){
    const ItemDefinition* item=FindItem(selectedItemId_); if(!item)return;
    const int gridX = static_cast<int>(std::floor(cursorPosition_.x / kTileWorldSize));
    const int gridY = static_cast<int>(std::floor(cursorPosition_.y / kTileWorldSize));
    if (IsTileItem(selectedItemId_)) {
        PaintTile(gridX, gridY);
        return;
    }
    PushUndo();
    const int existing=FindNearestPlacement(0.25f);
    if(existing>=0){placements_.erase(placements_.begin()+existing);objects_.erase(objects_.begin()+existing);}
    Placement p; p.id=nextPlacementId_++; p.itemId=item->id;
    p.position=cursorPosition_;
    p.rotation=cursorRotation_; p.scale=cursorScale_;
    if(item->id==18)p.variant=selectedDoorId_;
    else if(item->id==4||item->id==5||item->id==19)p.variant=selectedSwitchId_;
    else if(item->id==10)p.variant=selectedTimedGroupId_*10+selectedTimedOrderId_;
    else if(item->id==30)p.variant=selectedConveyorDirection_;
    if(item->id==8)p.moveOffset=movingFloorOffset_;
    placements_.push_back(p); RebuildObjects(); status_="Placed stage object: "+std::string(item->name);
}

// 処理概要: 指定された要素を管理対象から取り除く。
// 注意事項: 削除後に残る参照やインデックスを無効なまま残さない。
void StageEditor::RemoveNearest(){
    const int gridX = static_cast<int>(std::floor(cursorPosition_.x / kTileWorldSize));
    const int gridY = static_cast<int>(std::floor(cursorPosition_.y / kTileWorldSize));
    const int tileIndex = TileIndex(gridX, gridY);
    if (tileIndex >= 0 && tiles_[tileIndex] != 0) {
        PushUndo(); tiles_[tileIndex] = 0; if(tileIndex<static_cast<int>(tileObjects_.size()))tileObjects_[tileIndex].reset(); status_="Removed tile"; return;
    }
    const int index=FindNearestPlacement(0.25f); if(index<0){status_="No object at cursor";return;}
    PushUndo(); placements_.erase(placements_.begin()+index); objects_.erase(objects_.begin()+index); status_="Removed nearest object";
}

// 処理概要: StageEditorが担当する「RotateCursor」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void StageEditor::RotateCursor(){
    if (IsTileItem(selectedItemId_)) { status_="Tiles do not require rotation"; return; }
    const int index=FindNearestPlacement(0.25f);
    if(index>=0){PushUndo();placements_[index].rotation.y+=kPi*0.5f;if(placements_[index].rotation.y>=kPi*2)placements_[index].rotation.y-=kPi*2;objects_[index]=CreateObject(placements_[index]);status_="Rotated placed block";return;}
    cursorRotation_.y+=kPi*0.5f;if(cursorRotation_.y>=kPi*2)cursorRotation_.y-=kPi*2;UpdateCursorObject();
}

// 処理概要: 条件に一致するデータまたはリソースを検索する。
// 注意事項: 見つからない場合を正常な結果として扱えるようにする。
int StageEditor::FindNearestPlacement(float maxDistance) const {
    int result=-1; float best=maxDistance*maxDistance;
    for(int i=0;i<static_cast<int>(placements_.size());++i){const float d=DistanceSquared(cursorPosition_,placements_[i].position);if(d<best){best=d;result=i;}}
    return result;
}

const StageEditor::ItemDefinition* StageEditor::FindItem(int id) const {for(const auto& item:kItems)if(item.id==id)return &item;return nullptr;}

// 処理概要: 論理配置位置へモデル固有のローカルオフセットを合成する。
// 注意事項: 配置のY回転に追従させ、モデル差し替え時の原点補正をJSONへ混在させない。
Vector3 StageEditor::GetPlacementModelPosition(
    const Placement& placement,
    const Vector3& logicalPosition) const {
    const ItemDefinition* item = FindItem(placement.itemId);
    if (!item) return logicalPosition;
    const float cosine = std::cos(placement.rotation.y);
    const float sine = std::sin(placement.rotation.y);
    return {
        logicalPosition.x + item->modelOffset.x * cosine + item->modelOffset.z * sine,
        logicalPosition.y + item->modelOffset.y,
        logicalPosition.z - item->modelOffset.x * sine + item->modelOffset.z * cosine
    };
}

// 処理概要: 論理回転とモデルの軸補正を合成する。
Vector3 StageEditor::GetPlacementModelRotation(const Placement& placement) const {
    const ItemDefinition* item = FindItem(placement.itemId);
    if (!item) return placement.rotation;
    return {
        placement.rotation.x + item->modelRotation.x,
        placement.rotation.y + item->modelRotation.y,
        placement.rotation.z + item->modelRotation.z
    };
}

// 処理概要: 論理サイズとモデル固有の表示倍率を成分ごとに合成する。
Vector3 StageEditor::GetPlacementModelScale(const Placement& placement) const {
    const ItemDefinition* item = FindItem(placement.itemId);
    if (!item) return placement.scale;
    return {
        placement.scale.x * item->modelScale.x,
        placement.scale.y * item->modelScale.y,
        placement.scale.z * item->modelScale.z
    };
}

// 処理概要: 描画モデルに依存しない論理当たり判定の半径を返す。
Vector3 StageEditor::GetPlacementHalfExtents(const Placement& placement) const {
    const ItemDefinition* item = FindItem(placement.itemId);
    const Vector3 colliderScale = item ? item->colliderScale : Vector3{1.0f, 1.0f, 1.0f};
    return {
        (std::max)(std::abs(placement.scale.x * colliderScale.x) * 0.5f, 0.15f),
        (std::max)(std::abs(placement.scale.y * colliderScale.y) * 0.5f, 0.15f),
        (std::max)(std::abs(placement.scale.z * colliderScale.z) * 0.5f, 0.15f)
    };
}

// 処理概要: 論理Transformを保持したまま、描画側へモデル補正を適用する。
void StageEditor::ApplyPlacementVisualTransform(
    Object3d* object,
    const Placement& placement,
    const Vector3& logicalPosition) const {
    if (!object) return;
    object->SetPosition(GetPlacementModelPosition(placement, logicalPosition));
    object->SetRotation(GetPlacementModelRotation(placement));
    object->SetScale(GetPlacementModelScale(placement));
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
std::unique_ptr<Object3d> StageEditor::CreateObject(const Placement& p) const {
    const ItemDefinition* item=FindItem(p.itemId); if(!item||!object3dCommon_)return nullptr;
    Model* model=ModelManager::Load(item->modelDirectory,item->modelFile);
    auto object=std::make_unique<Object3d>(); object->Initialize(object3dCommon_); object->SetModel(model);
    ApplyPlacementVisualTransform(object.get(), p, p.position); object->SetColor(item->color);
    object->SetEnvironmentTexture(environmentTexture_); object->SetEnvironmentCoefficient(environmentCoefficient_);

    // StageEditorの入力処理は3D描画より先に実行されるため、配置したオブジェクトは
    // 生成された同じフレームからDraw3D()の描画対象になる。
    // 初回Update()を行わないとGPUへ渡すWVP行列が単位行列のままになり、
    // 1フレームだけモデルが画面中央へ巨大な2D画像のように表示されてしまう。
    // 生成時点でTransformをGPU用定数バッファへ反映し、最初の描画から正しい位置にする。
    object->Update();

    return object;
}

// 処理概要: StageEditorが担当する「RebuildObjects」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void StageEditor::RebuildObjects(){
    objects_.clear();
    objects_.reserve(placements_.size());
    for(const auto& p:placements_)objects_.push_back(CreateObject(p));
    tileObjects_.clear();
    tileObjects_.resize(tiles_.size());
    for (int y = 0; y < stageHeight_; ++y) {
        for (int x = 0; x < stageWidth_; ++x) {
            const int itemId = GetTileAt(x, y);
            if (itemId == 0) continue;
            Placement tile;
            tile.itemId = itemId;
            tile.position = GridToWorld(x, y);
            tile.scale = {1.0f, 1.0f, 1.0f};
            tileObjects_[y * stageWidth_ + x] = CreateObject(tile);
        }
    }
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void StageEditor::UpdateCursorObject(){
    const ItemDefinition* item=FindItem(selectedItemId_); if(!item||!object3dCommon_)return;
    Placement p; p.itemId=selectedItemId_;p.position=cursorPosition_;p.rotation=cursorRotation_;p.scale=cursorScale_;
    cursorObject_=CreateObject(p); if(cursorObject_){Vector4 c=item->color;c.w=.42f;cursorObject_->SetColor(c);cursorObject_->SetEnableLighting(false);}
}

StageEditor::Snapshot StageEditor::MakeSnapshot()const{return{tiles_,placements_,nextPlacementId_};}
void StageEditor::PushUndo(){undoStack_.push_back(MakeSnapshot());if(undoStack_.size()>100)undoStack_.erase(undoStack_.begin());redoStack_.clear();}
void StageEditor::Undo(){if(undoStack_.empty())return;redoStack_.push_back(MakeSnapshot());auto s=undoStack_.back();undoStack_.pop_back();tiles_=std::move(s.tiles);placements_=std::move(s.placements);nextPlacementId_=s.nextId;RebuildObjects();status_="Undo";}
void StageEditor::Redo(){if(redoStack_.empty())return;undoStack_.push_back(MakeSnapshot());auto s=redoStack_.back();redoStack_.pop_back();tiles_=std::move(s.tiles);placements_=std::move(s.placements);nextPlacementId_=s.nextId;RebuildObjects();status_="Redo";}

void StageEditor::NewStage(){placements_.clear();tiles_.assign(stageWidth_*stageHeight_,0);tileObjects_.clear();tileObjects_.resize(tiles_.size());objects_.clear();nextPlacementId_=1;undoStack_.clear();redoStack_.clear();currentFile_.clear();stageName_="new_stage";cursorPosition_=GridToWorld(0,2);cursorRotation_={};const auto* item=FindItem(selectedItemId_);cursorScale_=(item&&!IsTileItem(item->id))?item->defaultScale:Vector3{1,1,1};UpdateCursorObject();ResetPlayer();status_="New empty 16x16 tile stage - enter a name before saving";}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void StageEditor::DrawFilePanel(){
#ifdef USE_IMGUI
    if(!ImGui::CollapsingHeader("Stage Files",ImGuiTreeNodeFlags_DefaultOpen))return;
    int dimensions[2]={stageWidth_,stageHeight_};
    if(ImGui::InputInt2("Stage Size (tiles)",dimensions)){
        const int newWidth=std::clamp(dimensions[0],1,2000);
        const int newHeight=std::clamp(dimensions[1],1,200);
        if(newWidth!=stageWidth_||newHeight!=stageHeight_){
            std::vector<int> resized(newWidth*newHeight,0);
            for(int y=0;y<(std::min)(stageHeight_,newHeight);++y)
                for(int x=0;x<(std::min)(stageWidth_,newWidth);++x)
                    resized[y*newWidth+x]=tiles_[y*stageWidth_+x];
            tiles_=std::move(resized);stageWidth_=newWidth;stageHeight_=newHeight;stageDepth_=1;RebuildObjects();
        }
    }
    ImGui::TextDisabled("1 tile = 16 x 16 design pixels / %.1f world unit",kTileWorldSize);
    if(ImGui::Button("NEW EMPTY STAGE",{-1,28}))NewStage();
    char name[96]{};std::copy_n(stageName_.c_str(),(std::min)(stageName_.size(),sizeof(name)-1),name);
    if(ImGui::InputText("Stage Display Name",name,sizeof(name)))stageName_=name;
    ImGui::TextDisabled("File: %s.json",SanitizeFileName(stageName_).c_str());
    if(ImGui::Button("SAVE AS NEW FILE",{-1,34}))SaveAsNewStage();

    // currentFile_には、最後にロードまたは保存したステージのファイルパスが入る。
    // 新規ステージ作成直後など、対象ファイルがまだ決まっていない状態では
    // 間違ったファイルを上書きしないようにボタンを操作不能にする。
    if(currentFile_.empty()){
        ImGui::BeginDisabled();
        ImGui::Button("OVERWRITE LOADED FILE",{-1,34});
        ImGui::EndDisabled();
        ImGui::TextDisabled("Load or save a stage before overwriting.");
    }else{
        // 実際に上書きされるファイル名を表示し、ユーザーが保存先を確認できるようにする。
        const std::string loadedFileName=std::filesystem::path(currentFile_).filename().string();
        ImGui::TextDisabled("Loaded file: %s",loadedFileName.c_str());

        // ボタンを押しただけでは上書きせず、確認用のモーダルを開く。
        // これにより、操作ミスによるステージデータの消失を防ぐ。
        if(ImGui::Button("OVERWRITE LOADED FILE",{-1,34})){
            ImGui::OpenPopup("Confirm Stage Overwrite");
        }
    }

    // 上書きは元の内容を置き換える操作なので、実行前に必ず確認を取る。
    if(ImGui::BeginPopupModal("Confirm Stage Overwrite",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
        ImGui::TextUnformatted("Overwrite this stage file with the current edits?");

        // ポップアップを表示している間に保存対象が失われた場合も考慮し、
        // currentFile_が空なら安全な代替文字列を表示する。
        const std::string overwriteFileName=currentFile_.empty()
            ? "(no file)"
            : std::filesystem::path(currentFile_).filename().string();
        ImGui::Text("%s",overwriteFileName.c_str());
        ImGui::Spacing();

        if(ImGui::Button("OVERWRITE",{150,32})){
            // 確認後にだけ、現在ロードしているファイルへ保存する。
            SaveCurrentStage();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel",{100,32})){
            // キャンセル時はファイルへ一切書き込まず、確認画面だけを閉じる。
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }

    if(ImGui::Button("Refresh list"))RefreshStageFiles();
    std::string preview=selectedStageFile_>=0&&selectedStageFile_<static_cast<int>(stageFiles_.size())?std::filesystem::path(stageFiles_[selectedStageFile_]).filename().string():"Select stage...";
    if(ImGui::BeginCombo("Load",preview.c_str())){for(int i=0;i<static_cast<int>(stageFiles_.size());++i){std::string f=std::filesystem::path(stageFiles_[i]).filename().string();if(ImGui::Selectable(f.c_str(),selectedStageFile_==i))selectedStageFile_=i;}ImGui::EndCombo();}
    if(selectedStageFile_>=0&&selectedStageFile_<static_cast<int>(stageFiles_.size())&&ImGui::Button("LOAD SELECTED",{-1,28}))LoadStage(stageFiles_[selectedStageFile_]);

    const bool hasSelectedStage =
        selectedStageFile_ >= 0 && selectedStageFile_ < static_cast<int>(stageFiles_.size());
    if(!hasSelectedStage) ImGui::BeginDisabled();
    if(ImGui::Button("DELETE SELECTED STAGE",{-1,28})){
        // 一覧の選択が後から変わっても別ファイルを消さないよう、
        // 確認画面を開く時点で削除対象のパスを固定する。
        pendingDeleteFile_=stageFiles_[selectedStageFile_];
        ImGui::OpenPopup("Confirm Stage Delete");
    }
    if(!hasSelectedStage) ImGui::EndDisabled();

    // ステージ削除は元に戻せないため、ファイル名を表示して必ず確認を取る。
    if(ImGui::BeginPopupModal("Confirm Stage Delete",nullptr,ImGuiWindowFlags_AlwaysAutoResize)){
        ImGui::TextUnformatted("Permanently delete this stage JSON?");
        const std::string deleteFileName=pendingDeleteFile_.empty()
            ? "(no file)"
            : std::filesystem::path(pendingDeleteFile_).filename().string();
        ImGui::TextColored({1.0f,0.35f,0.25f,1.0f},"%s",deleteFileName.c_str());
        ImGui::Spacing();

        if(ImGui::Button("DELETE",{150,32})){
            DeleteStageFile(pendingDeleteFile_);
            pendingDeleteFile_.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if(ImGui::Button("Cancel",{100,32})){
            pendingDeleteFile_.clear();
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
#endif
}

// 処理概要: 導入カメラ演出の進行度を0～1で返す。
float StageEditor::GetIntroCameraProgress() const {
    return std::clamp(introCameraTimer_ / kIntroCameraDuration,0.0f,1.0f);
}

// 処理概要: 導入カメラが最初に映すスタート位置を返す。
float StageEditor::GetIntroCameraStartX() const {
    return playerRespawnPosition_.x;
}

// 処理概要: 導入カメラの目的地となるゴール位置を返す。
float StageEditor::GetIntroCameraGoalX() const {
    for (const Placement& placement : placements_) {
        if (placement.itemId == 17) return placement.position.x;
    }
    // ゴール未配置のテストステージでは右端を目的地として演出を成立させる。
    return static_cast<float>(stageWidth_)*kTileWorldSize;
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void StageEditor::DrawPlaylistManager(){
#ifdef USE_IMGUI
    if(!ImGui::CollapsingHeader("Playlist Manager",ImGuiTreeNodeFlags_DefaultOpen))return;
    ImGui::TextDisabled("Resources/Stages/sequence.txt");
    ImGui::TextUnformatted("Available Stages");
    ImGui::BeginChild("AvailableStages",{0,110},true);
    for(int i=0;i<static_cast<int>(availableFiles_.size());++i)if(ImGui::Selectable(availableFiles_[i].c_str(),selectedAvailableIndex_==i))selectedAvailableIndex_=i;
    ImGui::EndChild();
    if(ImGui::Button("Add to Playlist",{-1,28})&&selectedAvailableIndex_>=0&&selectedAvailableIndex_<static_cast<int>(availableFiles_.size())){campaignFiles_.push_back(availableFiles_[selectedAvailableIndex_]);availableFiles_.erase(availableFiles_.begin()+selectedAvailableIndex_);selectedAvailableIndex_=-1;}
    ImGui::Spacing();
    ImGui::TextUnformatted("Campaign Stages (Playlist)");
    ImGui::BeginChild("CampaignStages",{0,140},true);
    for(int i=0;i<static_cast<int>(campaignFiles_.size());++i){std::string label="Stage "+std::to_string(i+1)+": "+campaignFiles_[i];if(ImGui::Selectable(label.c_str(),selectedCampaignIndex_==i))selectedCampaignIndex_=i;}
    ImGui::EndChild();
    if(ImGui::Button("Remove")&&selectedCampaignIndex_>=0&&selectedCampaignIndex_<static_cast<int>(campaignFiles_.size())){availableFiles_.push_back(campaignFiles_[selectedCampaignIndex_]);campaignFiles_.erase(campaignFiles_.begin()+selectedCampaignIndex_);selectedCampaignIndex_=-1;}
    ImGui::SameLine();if(ImGui::Button("Up")&&selectedCampaignIndex_>0){std::swap(campaignFiles_[selectedCampaignIndex_],campaignFiles_[selectedCampaignIndex_-1]);--selectedCampaignIndex_;}
    ImGui::SameLine();if(ImGui::Button("Down")&&selectedCampaignIndex_>=0&&selectedCampaignIndex_+1<static_cast<int>(campaignFiles_.size())){std::swap(campaignFiles_[selectedCampaignIndex_],campaignFiles_[selectedCampaignIndex_+1]);++selectedCampaignIndex_;}
    ImGui::Separator();if(ImGui::Button("Save Playlist"))SavePlaylist();ImGui::SameLine();if(ImGui::Button("Reload"))LoadPlaylist();
#endif
}

std::string StageEditor::SanitizeFileName(const std::string& value){std::string r;for(unsigned char c:value)if(c>=0x80||std::isalnum(c)||c=='_'||c=='-'||c==' ')r.push_back(static_cast<char>(c));return r.empty()?"stage":r;}
std::string StageEditor::MakeUniqueStagePath()const{namespace fs=std::filesystem;const fs::path dir="Resources/Stages";const std::string base=SanitizeFileName(stageName_);fs::path p=dir/(base+".json");for(int n=2;fs::exists(p);++n)p=dir/(base+"_"+std::to_string(n)+".json");return p.generic_string();}

// 処理概要: 現在の状態を再読込可能な形式で永続化する。
// 注意事項: 保存先と書き込み結果を検証し、失敗を呼び出し側へ伝える。
bool StageEditor::SaveAsNewStage(){
#ifdef USE_IMGUI
    // 新規保存では、既存ファイルと名前が重ならないパスを作成する。
    // JSONの生成と書き込み自体は、上書き保存と同じ共通関数へ任せる。
    return SaveStageToPath(MakeUniqueStagePath(),"Saved new stage: ");
#else
    return false;
#endif
}

// 処理概要: 現在の状態を再読込可能な形式で永続化する。
// 注意事項: 保存先と書き込み結果を検証し、失敗を呼び出し側へ伝える。
bool StageEditor::SaveCurrentStage(){
#ifdef USE_IMGUI
    // ロード済みのファイルがない場合、上書き先を判断できないため保存しない。
    // UI側でもボタンを無効化しているが、関数側でも検査することで安全性を高める。
    if(currentFile_.empty()){
        status_="Overwrite failed: no stage file is loaded";
        return false;
    }

    const std::filesystem::path currentPath=currentFile_;

    // ステージデータ以外のファイルを誤って上書きしないよう、
    // 拡張子が.jsonであり、実際に存在するファイルであることを確認する。
    if(currentPath.extension()!=".json"||!std::filesystem::is_regular_file(currentPath)){
        status_="Overwrite failed: loaded stage file was not found";
        return false;
    }

    // LoadStage()で記録したcurrentFile_をそのまま保存先として使用する。
    // これにより「ロード → 編集 → 同じファイルへ保存」という流れになる。
    return SaveStageToPath(currentPath.generic_string(),"Overwrote stage: ");
#else
    return false;
#endif
}

// 処理概要: 現在の状態を再読込可能な形式で永続化する。
// 注意事項: 保存先と書き込み結果を検証し、失敗を呼び出し側へ伝える。
bool StageEditor::SaveStageToPath(const std::string& path,const std::string& successMessage){
#ifdef USE_IMGUI
    namespace fs=std::filesystem;

    // Resources/Stagesフォルダーが存在しない場合でも保存できるよう、
    // 書き込み前に保存先の親フォルダーを作成する。
    std::error_code directoryError;
    fs::create_directories(fs::path(path).parent_path(),directoryError);
    if(directoryError){
        status_="Save failed: could not create stage directory";
        return false;
    }

    // ここから先は、新規保存と上書き保存で共通となるJSON生成処理。
    // ステージの基本設定、タイル、配置オブジェクトを1つのJSONへまとめる。
    nlohmann::json root;root["version"]=6;root["name"]=stageName_;root["coordinate_system"]="tilemap_xy";root["tile_size"]=kTileSizePixels;root["tile_world_size"]=kTileWorldSize;root["size"]={stageWidth_,stageHeight_};
    root["placement_transform"]="logical";
    root["model_corrections"]="item_definition";

    // タイルは二次元配列として保存する。
    // y行ごとにx方向のタイルIDを並べることで、ロード時に同じ配置を復元できる。
    root["tiles"]=nlohmann::json::array();
    for(int y=0;y<stageHeight_;++y){nlohmann::json row=nlohmann::json::array();for(int x=0;x<stageWidth_;++x)row.push_back(GetTileAt(x,y));root["tiles"].push_back(std::move(row));}

    // 敵やギミックなどの配置物は、固有IDとTransform、種類別の追加情報を保存する。
    // variantにはコンベアの向きなど、move_offsetには移動床の移動量などが入る。
    root["objects"]=nlohmann::json::array();
    for(const auto& p:placements_)root["objects"].push_back({{"id",p.id},{"item_id",p.itemId},{"position",{p.position.x,p.position.y,p.position.z}},{"rotation",{p.rotation.x,p.rotation.y,p.rotation.z}},{"scale",{p.scale.x,p.scale.y,p.scale.z}},{"variant",p.variant},{"move_offset",{p.moveOffset.x,p.moveOffset.y,p.moveOffset.z}}});

    // std::ios::truncを指定すると、既存ファイルの場合は古い内容を消してから書き直す。
    // 新規ファイルの場合は、通常どおり新しいファイルが作成される。
    std::ofstream out(path,std::ios::trunc);
    if(!out){
        status_="Save failed: could not open "+path;
        return false;
    }

    // setw(2)でインデント付きJSONとして保存し、手作業でも内容を確認しやすくする。
    out<<std::setw(2)<<root;
    out.flush();

    // ディスク容量不足などで書き込み途中に失敗した場合を検知する。
    if(!out){
        status_="Save failed while writing: "+path;
        return false;
    }

    // 保存成功後は、このファイルを次回の上書き対象として記録する。
    // 新規保存した直後でも、続けて同じファイルへ上書きできるようになる。
    currentFile_=path;
    status_=successMessage+path;

    // ファイル一覧とプレイリスト候補を更新し、保存した内容をUIへ即時反映する。
    RefreshStageFiles();
    LoadPlaylist();
    return true;
#else
    return false;
#endif
}

// 処理概要: 外部データを読み込み、実行時に扱える形式へ変換する。
// 注意事項: 読込失敗時に既存の有効な状態を不必要に破壊しない。
bool StageEditor::LoadStage(const std::string& path){
    try{std::ifstream in(path);if(!in)throw std::runtime_error("file not found");nlohmann::json root;in>>root;std::vector<Placement> loaded;uint32_t maxId=0;
        auto size=root.value("size",std::vector<int>{200,15});if(size.size()>=2){stageWidth_=std::clamp(size[0],1,2000);stageHeight_=std::clamp(size[1],1,200);stageDepth_=1;}
        tiles_.assign(stageWidth_*stageHeight_,0);
        if(root.contains("tiles")&&root["tiles"].is_array()){
            const auto& rows=root["tiles"];
            for(int y=0;y<(std::min)(stageHeight_,static_cast<int>(rows.size()));++y){if(!rows[y].is_array())continue;for(int x=0;x<(std::min)(stageWidth_,static_cast<int>(rows[y].size()));++x){const int itemId=rows[y][x].get<int>();if(IsTileItem(itemId))tiles_[y*stageWidth_+x]=itemId;}}
        }
        const auto objectJson=root.contains("objects")?root["objects"]:root.value("placements",nlohmann::json::array());
        for(const auto& j:objectJson){Placement p;p.id=j.value("id",0u);p.itemId=j.value("item_id",0);p.variant=j.value("variant",0);auto pos=j.value("position",std::vector<float>{0,0,0});auto rot=j.value("rotation",std::vector<float>{0,0,0});auto scale=j.value("scale",std::vector<float>{1,1,1});auto move=j.value("move_offset",std::vector<float>{0,3,0});if(pos.size()!=3||rot.size()!=3||scale.size()!=3||move.size()!=3)continue;p.position={pos[0],pos[1],pos[2]};p.rotation={rot[0],rot[1],rot[2]};p.scale={scale[0],scale[1],scale[2]};p.moveOffset={move[0],move[1],move[2]};if(!FindItem(p.itemId))continue;
            if(!root.contains("tiles")&&IsTileItem(p.itemId)){const int x=std::clamp(static_cast<int>(std::floor(p.position.x/kTileWorldSize)),0,stageWidth_-1);const int y=std::clamp(static_cast<int>(std::floor(p.position.y/kTileWorldSize)),0,stageHeight_-1);tiles_[y*stageWidth_+x]=p.itemId;}
            else{loaded.push_back(p);maxId=(std::max)(maxId,p.id);}}
        placements_=std::move(loaded);nextPlacementId_=maxId+1;currentFile_=path;stageName_=root.value("name",std::filesystem::path(path).stem().string());undoStack_.clear();redoStack_.clear();RebuildObjects();ResetPlayer();status_="Loaded 3D stage: "+path;return true;
    }catch(const std::exception& e){status_=std::string("Load failed: ")+e.what();return false;}
}

// 処理概要: Resources/Stages内にある指定ステージJSONを削除する。
// 注意事項: ディレクトリ外やJSON以外のファイルは削除対象にしない。
bool StageEditor::DeleteStageFile(const std::string& path){
#ifdef USE_IMGUI
    namespace fs=std::filesystem;
    std::error_code error;

    // 相対パス表記や「..」によるディレクトリ移動を正規化し、
    // Resources/Stages直下のJSONだけが削除できることを確認する。
    const fs::path stageDirectory=fs::weakly_canonical("Resources/Stages",error);
    if(error){
        status_="Delete failed: stage directory was not found";
        return false;
    }
    const fs::path target=fs::weakly_canonical(path,error);
    if(error||target.parent_path()!=stageDirectory||target.extension()!=".json"){
        status_="Delete failed: invalid stage file";
        return false;
    }
    if(!fs::is_regular_file(target,error)||error){
        status_="Delete failed: stage file was not found";
        return false;
    }

    const std::string deletedName=target.filename().string();
    if(!fs::remove(target,error)||error){
        status_="Delete failed: could not remove "+deletedName;
        return false;
    }

    // 削除したステージがプレイリストに残るとRelease実行時のロードに失敗するため、
    // 同じファイル名をすべて取り除き、sequence.txtも同時に更新する。
    campaignFiles_.erase(
        std::remove(campaignFiles_.begin(),campaignFiles_.end(),deletedName),
        campaignFiles_.end());
    SavePlaylist();

    // 現在ロード中のファイルを削除した場合、メモリ上の編集内容は残すが、
    // 存在しないファイルへの上書き保存はできないよう対象パスを解除する。
    std::error_code currentError;
    if(!currentFile_.empty()&&fs::weakly_canonical(currentFile_,currentError)==target&&!currentError){
        currentFile_.clear();
    }

    RefreshStageFiles();
    LoadPlaylist();
    status_="Deleted stage: "+deletedName;
    return true;
#else
    (void)path;
    return false;
#endif
}

void StageEditor::RefreshStageFiles(){stageFiles_.clear();selectedStageFile_=-1;std::error_code ec;const std::filesystem::path dir="Resources/Stages";if(!std::filesystem::exists(dir,ec))return;for(const auto& e:std::filesystem::directory_iterator(dir,ec))if(e.is_regular_file()&&e.path().extension()==".json")stageFiles_.push_back(e.path().generic_string());std::sort(stageFiles_.begin(),stageFiles_.end());}

// 処理概要: 外部データを読み込み、実行時に扱える形式へ変換する。
// 注意事項: 読込失敗時に既存の有効な状態を不必要に破壊しない。
void StageEditor::LoadPlaylist(){
    campaignFiles_.clear();availableFiles_.clear();std::ifstream in("Resources/Stages/sequence.txt");std::string line;while(std::getline(in,line))if(!line.empty())campaignFiles_.push_back(line);
    for(const auto& path:stageFiles_){const std::string name=std::filesystem::path(path).filename().string();if(std::find(campaignFiles_.begin(),campaignFiles_.end(),name)==campaignFiles_.end())availableFiles_.push_back(name);}
    selectedCampaignIndex_=selectedAvailableIndex_=-1;
}
void StageEditor::SavePlaylist(){std::filesystem::create_directories("Resources/Stages");std::ofstream out("Resources/Stages/sequence.txt");for(const auto& file:campaignFiles_)out<<file<<'\n';status_="Playlist saved";}
