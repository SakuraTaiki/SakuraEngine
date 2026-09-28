// ============================================================================
// ファイルの役割: ゲームで利用する各コントローラーを組み立て、更新・描画順序を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <memory>
#include <vector>
#include <chrono>
#include <filesystem>

#include "IScene.h"
#include "EngineContext.h"

#include "Object3d.h"
#include "Sprite.h"
#include "Ring.h"
#include "Cylinder.h"
#include "Primitive.h"
#include "HitEffectController.h"
#include "AnimationDebugController.h"
#include "SoundController.h"
#include "CameraDebugController.h"
#include "EnvironmentController.h"
#include "SceneObjectController.h"
#include "GameSceneDrawMode.h"
#include "SceneDebugPanel.h"
#include "StageEditor.h"


// 実際のゲーム・デモ内容を持つ Scene。
// EngineContext 経由で Input / Graphics / Camera などを使う。
class GameScene : public IScene {
public:
    void Initialize(EngineContext* context) override;
    void Finalize() override;

    void Update() override;
    void Draw() override;


private:
    // 初期化処理を役割ごとに分ける。
    void InitializeModels();
    void InitializeSprite();
    void InitializeRing();
    void InitializeCylinder();
    void InitializePrimitive();
    void InitializeTaskJsonHotReload();
    void UpdateTaskJsonHotReload();
    void UpdatePostEffectShortcuts();
    void UpdateSideScrollCamera();
   

    // 更新処理。
    void UpdateObjects();

    // 描画処理。
    void Draw3D();
    void Draw2D();

private:
    
    GameSceneDrawMode drawMode_ = GameSceneDrawMode::NormalObj;
    // 非所有。実体は Engine が持つ。
    EngineContext* context_ = nullptr;

    std::unique_ptr<Sprite> sprite_;
    std::unique_ptr<Ring> ring_;
    std::unique_ptr<Cylinder> cylinder_;

    Vector3 ringPosition_ = { 0.0f, 2.0f, 0.0f };
    Vector3 ringScale_ = { 2.0f, 2.0f, 1.0f };
    Vector4 ringColor_ = { 1.0f, 1.0f, 1.0f, 0.7f };

    std::unique_ptr<Primitive> primitive_;

   
    bool showDebugSprite_ = false;

    HitEffectController hitEffect_;
    AnimationDebugController animationDebug_;
    SoundController soundController_;
    CameraDebugController cameraDebug_;
    EnvironmentController environment_;
    SceneObjectController sceneObjects_;
    SceneDebugPanel sceneDebugPanel_;
    StageEditor stageEditor_;

    std::filesystem::file_time_type taskJsonObservedWriteTime_{};
    std::chrono::steady_clock::time_point taskJsonChangeDetectedAt_{};
    bool taskJsonWatchInitialized_ = false;
    bool taskJsonReloadPending_ = false;
    int taskJsonReloadAttempts_ = 0;

    bool sideScrollCameraActive_ = false;
    float sideScrollCameraX_ = 0.0f;
};
