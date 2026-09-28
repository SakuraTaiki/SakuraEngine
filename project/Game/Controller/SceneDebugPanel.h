// ============================================================================
// ファイルの役割: ゲームビューと各種デバッグ編集ウィンドウをImGui上へ構築する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <cstddef>

#include "GameSceneDrawMode.h"
#include "AnimationDebugPanel.h"

class EngineContext;
class HitEffectController;
class AnimationDebugController;
class SoundController;
class CameraDebugController;
class EnvironmentController;
class SceneObjectController;
class Ring;
class Cylinder;
class Primitive;
class Object3dCommon;
class StageEditor;

class SceneDebugPanel {
public:
    void Draw(
        EngineContext* context,
        GameSceneDrawMode& drawMode,
        HitEffectController& hitEffect,
        AnimationDebugController& animationDebug,
        SoundController& soundController,
        CameraDebugController& cameraDebug,
        EnvironmentController& environment,
        SceneObjectController& sceneObjects,
        StageEditor& stageEditor,
        Ring* ring,
        Cylinder* cylinder,
        Primitive* primitive
    );

private:
    enum class Workspace {
        StageEditing,
        GameTesting,
        SceneEditing,
        Effects,
        Animation,
        Custom
    };

    enum class EditorSelection {
        None,
        Terrain,
        AxisPositive,
        AxisNegative,
        Camera,
        Environment,
        Effects,
        PostEffect,
        Sound,
        Asset,
        DynamicObject
    };

    void DrawMainMenuBar(
        EngineContext* context,
        GameSceneDrawMode& drawMode,
        HitEffectController& hitEffect,
        AnimationDebugController& animationDebug,
        SceneObjectController& sceneObjects,
        StageEditor& stageEditor
    );

    void DrawHierarchyWindow(SceneObjectController& sceneObjects);

    void DrawInspectorWindow(
        EngineContext* context,
        EnvironmentController& environment,
        SceneObjectController& sceneObjects,
        AnimationDebugController& animationDebug
    );

    void DrawProjectWindow();
    void DrawConsoleWindow();

    void DrawDebugToolsWindow(
        EngineContext* context,
        GameSceneDrawMode& drawMode,
        HitEffectController& hitEffect,
        AnimationDebugController& animationDebug,
        SoundController& soundController,
        EnvironmentController& environment,
        SceneObjectController& sceneObjects,
        Ring* ring,
        Cylinder* cylinder,
        Primitive* primitive
    );

    void DrawSceneControl(
        GameSceneDrawMode& drawMode,
        HitEffectController& hitEffect,
        AnimationDebugController& animationDebug
    );

    void DrawEffectTab(
        EngineContext* context,
        HitEffectController& hitEffect,
        Ring* ring,
        Cylinder* cylinder,
        Primitive* primitive
    );

    void DrawPostEffectTab(EngineContext* context);

    void DrawEnvironmentTab(
        Object3dCommon* object3dCommon,
        EnvironmentController& environment,
        SceneObjectController& sceneObjects,
        AnimationDebugController& animationDebug
    );

    void DrawGameViewWindow(
        EngineContext* context,
        SceneObjectController& sceneObjects,
        StageEditor& stageEditor
    );

    void DrawTransformGizmo(
        EngineContext* context,
        SceneObjectController& sceneObjects,
        float rectX,
        float rectY,
        float rectWidth,
        float rectHeight
    );

    void DrawColliderOverlay(
        EngineContext* context,
        SceneObjectController& sceneObjects,
        float rectX,
        float rectY,
        float rectWidth,
        float rectHeight
    );

    void ApplyWorkspace(Workspace workspace);
    const char* GetWorkspaceName() const;

    EditorSelection selected_ = EditorSelection::Terrain;
    size_t selectedObjectIndex_ = 0;

    AnimationDebugPanel animationDebugPanel_;

    bool showTransformGizmo_ = true;
    bool showColliders_ = false;
    int gizmoOperation_ = 0;
    int gizmoMode_ = 0;

    Workspace workspace_ = Workspace::StageEditing;
    bool showHierarchyWindow_ = true;
    bool showInspectorWindow_ = false;
    bool showProjectWindow_ = true;
    bool showConsoleWindow_ = true;
    bool showDebugToolsWindow_ = false;
    bool showGameViewWindow_ = true;
    bool showStageEditorWindow_ = true;
    bool gameViewMaximized_ = false;
    bool workspaceModeSyncInitialized_ = false;
    bool previousGamePlayMode_ = false;
};
