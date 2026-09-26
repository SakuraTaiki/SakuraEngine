#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "MyMath.h"
#include "Object3d.h"

class Object3dCommon;
class Model;
class Input;

// Free-position 3D stage editor for a 2.5D game.
class StageEditor {
public:
    static constexpr int kTileSizePixels = 16;
    static constexpr float kTileWorldSize = 1.0f;

    enum class Mode { Editor, GamePlay };
    enum class GameFlowState { Title, Playing, Clear };
    enum class Category { Basic, Gimmick, Enemy, System };
    enum class TileCollisionType { Empty, Solid, OneWay, Ladder, SlopeUpRight, SlopeUpLeft };

    struct ItemDefinition {
        int id = 0;
        const char* name = "";
        Category category = Category::Basic;
        const char* modelDirectory = "";
        const char* modelFile = "";
        Vector4 color{1,1,1,1};
        Vector3 defaultScale{1,1,1};
    };

    struct Placement {
        uint32_t id = 0;
        int itemId = 0;
        Vector3 position{};
        Vector3 rotation{};
        Vector3 scale{1,1,1};
        int variant = 0;
        Vector3 moveOffset{0,3,0};
    };

    void Initialize(Object3dCommon* common, uint32_t environmentTexture, float environmentCoefficient);
    void Finalize();
    void Update(Input* input);
    void Draw3D();
    void Draw();
    void DrawGameView(float rectX, float rectY, float rectWidth, float rectHeight);

    bool IsEditingGameView() const { return mode_ == Mode::Editor; }
    bool IsGamePlayMode() const { return mode_ == Mode::GamePlay; }
    bool IsActive() const { return active_; }
    void SetActive(bool active) {
        active_ = active;
        if (!active_) {
            gameViewHovered_ = false;
        }
    }
    void ToggleMode();

    int GetStageWidthInTiles() const { return stageWidth_; }
    int GetStageHeightInTiles() const { return stageHeight_; }
    const Vector3& GetPlayerPosition() const { return playerPosition_; }
    int GetTileAt(int gridX, int gridY) const;
    bool IsSolidTile(int gridX, int gridY) const;
    TileCollisionType GetTileCollisionType(int gridX, int gridY) const;
    static Vector3 GridToWorld(int gridX, int gridY, float z = 0.0f);

private:
    struct Snapshot {
        std::vector<int> tiles;
        std::vector<Placement> placements;
        uint32_t nextId = 1;
    };

    void DrawModeSwitcher();
    void DrawPalette();
    void DrawTransformPanel();
    void DrawFilePanel();
    void DrawPlaylistManager();
    void DrawSelectedItemSettings();
    void HandleEditorInput(
        float rectX,
        float rectY,
        float rectWidth,
        float rectHeight
    );
    void DrawGridOverlay(float rectX, float rectY, float rectWidth, float rectHeight);
    bool MouseToGrid(
        float mouseX,
        float mouseY,
        float rectX,
        float rectY,
        float rectWidth,
        float rectHeight,
        int& gridX,
        int& gridY
    ) const;

    void SetMode(Mode mode);
    void ResetPlayer();
    void ResetRuntimeState();
    void RespawnPlayer(bool damaged = false);
    void UpdatePlayer(Input* input);
    void UpdateGameFlow(Input* input);
    void UpdateRuntimeObjects(float deltaTime);
    void UpdateEnemies(float deltaTime);
    void UpdateGimmickCollisions(Input* input, float deltaTime);
    void ResolvePlacedSolidCollisions();
    bool IsPlayerOverlappingPlacement(size_t index, float padding = 0.0f) const;
    bool IsRuntimePlacementActive(size_t index) const;
    Vector3 GetRuntimePlacementPosition(size_t index) const;
    void ShowRuntimeMessage(const std::string& message, float seconds = 1.5f);
    bool SaveProgressAt(size_t placementIndex);
    bool LoadSavedProgress();
    void RefreshSaveDataAvailability();
    void MovePlayerHorizontal(float amount);
    void MovePlayerVertical(float amount);
    bool IsCollisionSolid(int gridX, int gridY) const;
    bool IsPlacementBlocked(const Vector3& position, float halfX, float halfY) const;
    void PlaceItem();
    void RemoveNearest();
    void RotateCursor();
    void PaintTile(int gridX, int gridY);
    void EraseTile(int gridX, int gridY);
    bool IsTileItem(int itemId) const;
    int TileIndex(int gridX, int gridY) const;
    int FindNearestPlacement(float maxDistance) const;
    const ItemDefinition* FindItem(int id) const;

    std::unique_ptr<Object3d> CreateObject(const Placement& placement) const;
    void RebuildObjects();
    void UpdateCursorObject();

    Snapshot MakeSnapshot() const;
    void PushUndo();
    void Undo();
    void Redo();

    void NewStage();
    bool SaveAsNewStage();
    bool LoadStage(const std::string& path);
    void RefreshStageFiles();
    void LoadPlaylist();
    void SavePlaylist();
    std::string MakeUniqueStagePath() const;
    static std::string SanitizeFileName(const std::string& value);

    Object3dCommon* object3dCommon_ = nullptr;
    uint32_t environmentTexture_ = 0;
    float environmentCoefficient_ = 0.0f;

    Mode mode_ = Mode::Editor;
    Category category_ = Category::Basic;
    int selectedItemId_ = 1;
    uint32_t nextPlacementId_ = 1;

    Vector3 cursorPosition_{0.0f, 2.0f, 0.0f};
    Vector3 cursorRotation_{};
    Vector3 cursorScale_{1.0f, 1.0f, 1.0f};
    float cursorMoveSpeed_ = 4.0f;
    bool gameViewHovered_ = false;
    bool active_ = true;

    std::vector<Placement> placements_;
    std::vector<int> tiles_;
    std::vector<std::unique_ptr<Object3d>> tileObjects_;
    std::vector<std::unique_ptr<Object3d>> objects_;
    std::unique_ptr<Object3d> cursorObject_;
    std::unique_ptr<Object3d> cursorFrameObject_;
    std::unique_ptr<Object3d> playerObject_;

    Vector3 playerPosition_{1.5f, 2.5f, 0.0f};
    Vector3 playerVelocity_{};
    static constexpr float kPlayerHalfSize = 0.5f;
    static constexpr float kPlayerMoveSpeed = 5.0f;
    static constexpr float kPlayerJumpSpeed = 9.0f;
    static constexpr float kPlayerGravity = 24.0f;
    bool playerGrounded_ = false;
    Vector3 playerRespawnPosition_{1.5f,2.5f,0.0f};
    std::vector<uint8_t> runtimePlacementActive_;
    std::vector<uint8_t> runtimePlacementTouching_;
    std::vector<Vector3> runtimePlacementPositions_;
    std::vector<float> runtimeEnemyDirections_;
    std::vector<uint8_t> runtimeEnemyAlerted_;
    std::vector<uint8_t> runtimeFallingFloorStates_;
    std::vector<float> runtimeFallingFloorTimers_;
    std::vector<float> runtimeFallingFloorVelocities_;
    std::vector<uint8_t> runtimeTileActive_;
    float runtimeTime_ = 0.0f;
    float playerFlashTimer_ = 0.0f;
    float playerDamageCooldown_ = 0.0f;
    float runtimeMessageTimer_ = 0.0f;
    float doorCooldown_ = 0.0f;
    std::string runtimeMessage_;
    int collectedStars_ = 0;
    int collectedBubbles_ = 0;
    int keyCount_ = 0;
    bool pSwitchActive_ = false;
    bool onOffActive_ = true;
    bool goalReached_ = false;
    GameFlowState gameFlowState_ = GameFlowState::Title;
    bool hasSaveData_ = false;
    static constexpr const char* kProgressSavePath = "SaveData/stage_progress.json";

    std::vector<Snapshot> undoStack_;
    std::vector<Snapshot> redoStack_;
    std::vector<std::string> stageFiles_;
    int selectedStageFile_ = -1;
    std::vector<std::string> campaignFiles_;
    std::vector<std::string> availableFiles_;
    int selectedCampaignIndex_ = -1;
    int selectedAvailableIndex_ = -1;

    int stageWidth_ = 200;
    int stageHeight_ = 15;
    int stageDepth_ = 1;
    int lastPaintGridX_ = -1;
    int lastPaintGridY_ = -1;
    bool showGrid_ = true;
    int selectedDoorId_ = 1;
    int selectedSwitchId_ = 1;
    int selectedTimedGroupId_ = 1;
    int selectedTimedOrderId_ = 0;
    int selectedConveyorDirection_ = 1;
    Vector3 movingFloorOffset_{0,3,0};

    std::string stageName_ = "stage_01";
    std::string currentFile_;
    std::string status_ = "Ready";
};
