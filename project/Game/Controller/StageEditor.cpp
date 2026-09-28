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
#ifndef USE_IMGUI
    // Release builds do not provide the editor UI, so starting in Editor
    // mode would leave the placement cursor visible with no way to enter
    // gameplay. Start directly in the runtime mode instead.
    mode_=Mode::GamePlay;
#endif
    ModelManager::Load("Resources/Editor", "joint_box.obj");
    ModelManager::Load("Resources/Editor", "ico_sphere.obj");
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
#ifndef USE_IMGUI
    if (!campaignFiles_.empty()) {
        std::filesystem::path stagePath = campaignFiles_.front();
        if (!stagePath.has_parent_path()) {
            stagePath = std::filesystem::path("Resources/Stages") / stagePath;
        }
        LoadStage(stagePath.string());
    } else if (!stageFiles_.empty()) {
        LoadStage(stageFiles_.front());
    }
#endif
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void StageEditor::Finalize() {
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
    if (mode_ == Mode::GamePlay) UpdateGameFlow(input);
    if (mode_ == Mode::GamePlay && gameFlowState_ == GameFlowState::Playing) UpdateRuntimeObjects(deltaTime);
    for (auto& object:tileObjects_) if(object) object->Update();
    for (auto& object:objects_) if(object) object->Update();
    if (mode_ == Mode::GamePlay && gameFlowState_ == GameFlowState::Playing) {
        UpdatePlayer(input);
    }
    if(cursorObject_ && mode_==Mode::Editor) {
        cursorObject_->SetPosition(cursorPosition_);
        cursorObject_->SetRotation(cursorRotation_);
        cursorObject_->SetScale(cursorScale_);
        cursorObject_->Update();
    }
    if(cursorFrameObject_ && mode_==Mode::Editor){
        cursorFrameObject_->SetPosition(cursorPosition_);
        cursorFrameObject_->SetRotation({0,0,0});
        cursorFrameObject_->SetScale({1.04f,1.04f,1.04f});
        cursorFrameObject_->Update();
    }
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void StageEditor::Draw3D() {
    if (!active_) {
        return;
    }

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
    if(playerObject_ && mode_==Mode::GamePlay && gameFlowState_!=GameFlowState::Title) playerObject_->Draw();
    if(cursorObject_ && mode_==Mode::Editor) cursorObject_->Draw();
    if(cursorFrameObject_ && mode_==Mode::Editor) cursorFrameObject_->Draw();
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
        ImGui::DragFloat3("Object Scale",&cursorScale_.x,0.02f,0.02f,100.0f);
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

    gameViewHovered_=ImGui::IsItemHovered();

    // ゲームビュー画像の上で回したホイールは、この画像の操作として扱う。
    // ImGuiに所有権を通知しておくことで、ステージをズームしたい時に
    // 背後のStageEditorメニューまで同時に上下スクロールすることを防ぐ。
    if (gameViewHovered_) {
        ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
    }

    HandleEditorInput(x, y, width, height);
    ImDrawList* draw=ImGui::GetWindowDrawList();
    const bool editing=mode_==Mode::Editor;
    const ImU32 color=editing?IM_COL32(38,145,235,235):IM_COL32(35,185,85,235);
    draw->AddRectFilled({x+10,y+10},{x+174,y+40},color,5);
    draw->AddText({x+20,y+18},IM_COL32_WHITE,editing?"EDITOR MODE":"GAMEPLAY MODE");
    if(editing){
        char buffer[160]{};
        const int gridX=static_cast<int>(std::floor(cursorPosition_.x/kTileWorldSize));
        const int gridY=static_cast<int>(std::floor(cursorPosition_.y/kTileWorldSize));
        snprintf(buffer,sizeof(buffer),"Tile %d, %d  (%d px, %d px)",gridX,gridY,gridX*kTileSizePixels,gridY*kTileSizePixels);
        draw->AddRectFilled({x+10,y+46},{x+250,y+72},IM_COL32(10,12,18,205),4);
        draw->AddText({x+18,y+52},IM_COL32(255,230,70,255),buffer);
        DrawGridOverlay(x, y, width, height);
    } else if (gameFlowState_ == GameFlowState::Title) {
        draw->AddRectFilled({x,y},{x+width,y+height},IM_COL32(5,10,22,225));
        const char* title="STAGE MAKER ADVENTURE";
        const ImVec2 titleSize=ImGui::CalcTextSize(title);
        draw->AddText({x+(width-titleSize.x)*0.5f,y+height*0.28f},IM_COL32(80,220,255,255),title);
        const char* start="ENTER / GAMEPAD A : NEW GAME";
        const ImVec2 startSize=ImGui::CalcTextSize(start);
        draw->AddText({x+(width-startSize.x)*0.5f,y+height*0.48f},IM_COL32_WHITE,start);
        if(hasSaveData_){
            const char* resume="C / GAMEPAD Y : CONTINUE FROM SAVE POINT";
            const ImVec2 resumeSize=ImGui::CalcTextSize(resume);
            draw->AddText({x+(width-resumeSize.x)*0.5f,y+height*0.55f},IM_COL32(255,220,80,255),resume);
        }
    } else if (gameFlowState_ == GameFlowState::Clear) {
        draw->AddRectFilled({x,y},{x+width,y+height},IM_COL32(4,18,10,220));
        const char* clear="STAGE CLEAR!";
        const ImVec2 clearSize=ImGui::CalcTextSize(clear);
        draw->AddText({x+(width-clearSize.x)*0.5f,y+height*0.32f},IM_COL32(90,255,120,255),clear);
        const char* retry="ENTER / GAMEPAD A : RETRY";
        const ImVec2 retrySize=ImGui::CalcTextSize(retry);
        draw->AddText({x+(width-retrySize.x)*0.5f,y+height*0.50f},IM_COL32_WHITE,retry);
        const char* back="T / GAMEPAD B : TITLE";
        const ImVec2 backSize=ImGui::CalcTextSize(back);
        draw->AddText({x+(width-backSize.x)*0.5f,y+height*0.57f},IM_COL32(190,205,225,255),back);
    } else if (runtimeMessageTimer_ > 0.0f && !runtimeMessage_.empty()) {
        const ImVec2 textSize = ImGui::CalcTextSize(runtimeMessage_.c_str());
        const ImVec2 textPosition{x + (width - textSize.x) * 0.5f, y + height * 0.18f};
        draw->AddRectFilled(
            {textPosition.x - 18.0f, textPosition.y - 10.0f},
            {textPosition.x + textSize.x + 18.0f, textPosition.y + textSize.y + 10.0f},
            IM_COL32(8,12,20,220), 7.0f);
        draw->AddText(textPosition, goalReached_ ? IM_COL32(90,255,120,255) : IM_COL32(255,235,90,255), runtimeMessage_.c_str());
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
    const bool accept = input->TriggerKey(DIK_RETURN) || input->TriggerKey(DIK_SPACE) ||
        input->TriggerGamepadButton(XINPUT_GAMEPAD_A);
    const bool continueGame = input->TriggerKey(DIK_C) ||
        input->TriggerGamepadButton(XINPUT_GAMEPAD_Y);
    const bool returnToTitle = input->TriggerKey(DIK_T) || input->TriggerKey(DIK_ESCAPE) ||
        input->TriggerGamepadButton(XINPUT_GAMEPAD_B);

    if (gameFlowState_ == GameFlowState::Title) {
        if (continueGame && hasSaveData_) {
            ResetRuntimeState();
            if (LoadSavedProgress()) gameFlowState_ = GameFlowState::Playing;
        } else if (accept) {
            ResetRuntimeState();
            gameFlowState_ = GameFlowState::Playing;
            ShowRuntimeMessage("START!", 1.0f);
        }
    } else if (gameFlowState_ == GameFlowState::Clear) {
        if (returnToTitle) {
            ResetRuntimeState();
            gameFlowState_ = GameFlowState::Title;
            RefreshSaveDataAvailability();
        } else if (accept) {
            ResetRuntimeState();
            gameFlowState_ = GameFlowState::Playing;
            ShowRuntimeMessage("RETRY!", 1.0f);
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

    playerRespawnPosition_ = GridToWorld(1, 2);
    for (size_t i = 0; i < placements_.size(); ++i) {
        runtimePlacementPositions_[i] = placements_[i].position;
        runtimeEnemyDirections_[i] = (i % 2 == 0) ? 1.0f : -1.0f;
        if (placements_[i].itemId == 25) playerRespawnPosition_ = placements_[i].position;
        if (placements_[i].itemId == 5) runtimePlacementActive_[i] = 0;
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
    const float halfX = (std::max)(std::abs(placement.scale.x) * 0.5f, 0.15f) + padding;
    const float halfY = (std::max)(std::abs(placement.scale.y) * 0.5f, 0.15f) + padding;
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
            const float halfX = (std::max)(std::abs(placements_[i].scale.x) * 0.5f, 0.15f);
            const float halfY = (std::max)(std::abs(placements_[i].scale.y) * 0.5f, 0.15f);
            const float oldTop = previousPosition.y + halfY;
            const bool standingOnPlatform =
                std::abs(playerPosition_.x - previousPosition.x) <= kPlayerHalfSize + halfX &&
                std::abs((playerPosition_.y - kPlayerHalfSize) - oldTop) <= 0.08f;
            if (standingOnPlatform) {
                playerPosition_.x += position.x - previousPosition.x;
                playerPosition_.y += position.y - previousPosition.y;
            }
        } else if (itemId == 28) {
            const float halfX = (std::max)(std::abs(placements_[i].scale.x) * 0.5f, 0.15f);
            const float halfY = (std::max)(std::abs(placements_[i].scale.y) * 0.5f, 0.15f);
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

            if (standingOnFloor && state != 3) {
                playerPosition_.x += position.x - previousPosition.x;
                playerPosition_.y += position.y - previousPosition.y;
            }
            if (i < objects_.size() && objects_[i]) {
                objects_[i]->SetColor(state == 1
                    ? Vector4{1.0f,0.2f,0.08f,1.0f}
                    : Vector4{0.82f,0.48f,0.18f,1.0f});
            }
        }
        runtimePlacementPositions_[i] = position;
        if (i < objects_.size() && objects_[i]) objects_[i]->SetPosition(position);
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
        const float halfX = (std::max)(std::abs(placement.scale.x) * 0.5f, 0.15f);
        const float halfY = (std::max)(std::abs(placement.scale.y) * 0.5f, 0.15f);

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
    // The left/right/bottom borders close the playable area. The top stays
    // open so tall jumps and later camera layouts are not artificially capped.
    if (gridX < 0 || gridX >= stageWidth_ || gridY < 0) return true;
    if (gridY >= stageHeight_) return false;
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
        if (!IsPlayerOverlappingPlacement(i)) continue;

        const Vector3 position = GetRuntimePlacementPosition(i);
        const float halfX = (std::max)(std::abs(placements_[i].scale.x) * 0.5f, 0.15f);
        const float halfY = (std::max)(std::abs(placements_[i].scale.y) * 0.5f, 0.15f);
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
                ShowRuntimeMessage("GOAL CLEAR!", 4.0f);
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
            if (playerDamageCooldown_ <= 0.0f) RespawnPlayer(true);
            return;
        case 29: { // Jump Pad
            const Vector3 padPosition = GetRuntimePlacementPosition(i);
            const float padHalfY = (std::max)(std::abs(placements_[i].scale.y) * 0.5f, 0.1f);
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
            const float conveyorHalfY = (std::max)(std::abs(placements_[i].scale.y) * 0.5f, 0.1f);
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
            if (!wasTouching) {
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
            if (!wasTouching) {
                playerRespawnPosition_ = GetRuntimePlacementPosition(i);
                playerRespawnPosition_.y += 0.6f;
                playerRespawnPosition_.z = 0.0f;
                if (SaveProgressAt(i)) {
                    if (i < objects_.size() && objects_[i]) objects_[i]->SetColor({1.0f,0.9f,0.2f,1.0f});
                    ShowRuntimeMessage("GAME SAVED!", 1.5f);
                } else {
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
        jumpTriggered = input->TriggerKey(DIK_RETURN) || input->TriggerKey(DIK_SPACE) ||
            input->TriggerGamepadButton(XINPUT_GAMEPAD_A);
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

    if (playerPosition_.y < -5.0f) RespawnPlayer(true);
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

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
std::unique_ptr<Object3d> StageEditor::CreateObject(const Placement& p) const {
    const ItemDefinition* item=FindItem(p.itemId); if(!item||!object3dCommon_)return nullptr;
    Model* model=ModelManager::Load(item->modelDirectory,item->modelFile);
    auto object=std::make_unique<Object3d>(); object->Initialize(object3dCommon_); object->SetModel(model);
    object->SetPosition(p.position); object->SetRotation(p.rotation); object->SetScale(p.scale); object->SetColor(item->color);
    object->SetEnvironmentTexture(environmentTexture_); object->SetEnvironmentCoefficient(environmentCoefficient_); return object;
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
#endif
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
    nlohmann::json root;root["version"]=5;root["name"]=stageName_;root["coordinate_system"]="tilemap_xy";root["tile_size"]=kTileSizePixels;root["tile_world_size"]=kTileWorldSize;root["size"]={stageWidth_,stageHeight_};

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

void StageEditor::RefreshStageFiles(){stageFiles_.clear();selectedStageFile_=-1;std::error_code ec;const std::filesystem::path dir="Resources/Stages";if(!std::filesystem::exists(dir,ec))return;for(const auto& e:std::filesystem::directory_iterator(dir,ec))if(e.is_regular_file()&&e.path().extension()==".json")stageFiles_.push_back(e.path().generic_string());std::sort(stageFiles_.begin(),stageFiles_.end());}

// 処理概要: 外部データを読み込み、実行時に扱える形式へ変換する。
// 注意事項: 読込失敗時に既存の有効な状態を不必要に破壊しない。
void StageEditor::LoadPlaylist(){
    campaignFiles_.clear();availableFiles_.clear();std::ifstream in("Resources/Stages/sequence.txt");std::string line;while(std::getline(in,line))if(!line.empty())campaignFiles_.push_back(line);
    for(const auto& path:stageFiles_){const std::string name=std::filesystem::path(path).filename().string();if(std::find(campaignFiles_.begin(),campaignFiles_.end(),name)==campaignFiles_.end())availableFiles_.push_back(name);}
    selectedCampaignIndex_=selectedAvailableIndex_=-1;
}
void StageEditor::SavePlaylist(){std::filesystem::create_directories("Resources/Stages");std::ofstream out("Resources/Stages/sequence.txt");for(const auto& file:campaignFiles_)out<<file<<'\n';status_="Playlist saved";}
