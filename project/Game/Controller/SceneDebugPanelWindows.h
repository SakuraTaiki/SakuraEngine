// ============================================================================
// ファイルの役割: ゲームビューと各種デバッグ編集ウィンドウをImGui上へ構築する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void SceneDebugPanel::Draw(
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
) {
#ifdef USE_IMGUI
    if (!context) {
        return;
    }

    context->GetImGuiManager()->Begin();

    const bool gamePlayMode = stageEditor.IsGamePlayMode();
    if (!workspaceModeSyncInitialized_) {
        previousGamePlayMode_ = gamePlayMode;
        workspaceModeSyncInitialized_ = true;
    } else if (gamePlayMode != previousGamePlayMode_) {
        ApplyWorkspace(gamePlayMode ? Workspace::GameTesting : Workspace::StageEditing);
        drawMode = GameSceneDrawMode::NormalObj;
        previousGamePlayMode_ = gamePlayMode;
    }

    if (ImGui::IsKeyPressed(ImGuiKey_F11)) {
        gameViewMaximized_ = !gameViewMaximized_;
        showGameViewWindow_ = true;
    }
    if (ImGui::IsKeyPressed(ImGuiKey_F5)) {
        stageEditor.ToggleMode();
        drawMode = GameSceneDrawMode::NormalObj;
    }
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_P)) {
        showProjectWindow_ = !showProjectWindow_;
        workspace_ = Workspace::Custom;
    }
    if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_GraveAccent)) {
        showConsoleWindow_ = !showConsoleWindow_;
        workspace_ = Workspace::Custom;
    }

    DrawMainMenuBar(
        context,
        drawMode,
        hitEffect,
        animationDebug,
        sceneObjects,
        stageEditor
    );

    if (showHierarchyWindow_) {
        DrawHierarchyWindow(sceneObjects);
    }

    if (showInspectorWindow_) {
        DrawInspectorWindow(context, environment, sceneObjects, animationDebug);
    }

    if (showProjectWindow_) DrawProjectWindow();

    if (showGameViewWindow_) DrawGameViewWindow(context, sceneObjects, stageEditor);

    if (showConsoleWindow_) DrawConsoleWindow();

    if (showDebugToolsWindow_) {
        DrawDebugToolsWindow(
            context, drawMode, hitEffect, animationDebug, soundController,
            environment, sceneObjects, ring, cylinder, primitive
        );
    }

    if (showStageEditorWindow_) stageEditor.Draw();

    context->GetImGuiManager()->End();
#endif
}

// 処理概要: 指定された設定や計算結果を現在の実行状態へ反映する。
// 注意事項: 関連するキャッシュやGPU定数も必要に応じて更新する。
void SceneDebugPanel::ApplyWorkspace(Workspace workspace) {
    workspace_ = workspace;
    gameViewMaximized_ = false;
    switch (workspace) {
    case Workspace::StageEditing:
        showHierarchyWindow_ = true;
        showInspectorWindow_ = false;
        showProjectWindow_ = true;
        showConsoleWindow_ = true;
        showDebugToolsWindow_ = false;
        showGameViewWindow_ = true;
        showStageEditorWindow_ = true;
        break;
    case Workspace::GameTesting:
        showHierarchyWindow_ = false;
        showInspectorWindow_ = false;
        showProjectWindow_ = false;
        showConsoleWindow_ = false;
        showDebugToolsWindow_ = false;
        showGameViewWindow_ = true;
        showStageEditorWindow_ = false;
        gameViewMaximized_ = true;
        break;
    case Workspace::SceneEditing:
        showHierarchyWindow_ = true;
        showInspectorWindow_ = true;
        showProjectWindow_ = true;
        showConsoleWindow_ = false;
        showDebugToolsWindow_ = false;
        showGameViewWindow_ = true;
        showStageEditorWindow_ = false;
        break;
    case Workspace::Effects:
        showHierarchyWindow_ = false;
        showInspectorWindow_ = false;
        showProjectWindow_ = false;
        showConsoleWindow_ = true;
        showDebugToolsWindow_ = true;
        showGameViewWindow_ = true;
        showStageEditorWindow_ = false;
        break;
    case Workspace::Animation:
        showHierarchyWindow_ = true;
        showInspectorWindow_ = false;
        showProjectWindow_ = false;
        showConsoleWindow_ = false;
        showDebugToolsWindow_ = true;
        showGameViewWindow_ = true;
        showStageEditorWindow_ = false;
        break;
    case Workspace::Custom:
        break;
    }
}

// 処理概要: 外部から必要な状態またはリソース参照を取得する。
// 注意事項: 返す参照やポインターの寿命は所有オブジェクトに従う。
const char* SceneDebugPanel::GetWorkspaceName() const {
    switch (workspace_) {
    case Workspace::StageEditing: return "Stage Editing";
    case Workspace::GameTesting: return "Game Testing";
    case Workspace::SceneEditing: return "Scene Editing";
    case Workspace::Effects: return "Effects";
    case Workspace::Animation: return "Animation";
    default: return "Custom";
    }
}

void SceneDebugPanel::DrawMainMenuBar(
    EngineContext* context,
    GameSceneDrawMode& drawMode,
    HitEffectController& hitEffect,
    AnimationDebugController& animationDebug,
    SceneObjectController& sceneObjects,
    StageEditor& stageEditor
) {
#ifdef USE_IMGUI
    //==================================
    //Docking対応版Imgui
    //=================================

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            ImGui::MenuItem("New Scene", nullptr, false, false);
            ImGui::Separator();

            if (ImGui::MenuItem("Save Runtime Task Json", "Ctrl+S")) {
                if (sceneObjects.SaveLevelSceneToJson("Resources/Task.json")) {
                    Detail::AddEditorLog(
                        Detail::EditorLogType::Info,
                        "Saved runtime scene: Resources/Task.json"
                    );
                } else {
                    Detail::AddEditorLog(
                        Detail::EditorLogType::Error,
                        "Save Resources/Task.json failed."
                    );
                }
            }

            if (ImGui::MenuItem("Reload Runtime Task Json", "Ctrl+O")) {
                const bool hasBlenderTask =
                    std::filesystem::exists("Resources/Task.json");
                const char* loadedPath = hasBlenderTask
                    ? "Resources/Task.json"
                    : "Resources/LevelScene.json";
                bool loaded = false;
                if (hasBlenderTask) {
                    loaded = sceneObjects.LoadLevelSceneFromJson("Resources/Task.json");
                } else {
                    loaded = sceneObjects.LoadLevelSceneFromJson("Resources/LevelScene.json");
                }
                if (loaded) {
                    selected_ = EditorSelection::None;
                    selectedObjectIndex_ = 0;
                    Detail::GetModelApplyTargetIndex() = 0;

                    Detail::AddEditorLog(
                        Detail::EditorLogType::Info,
                        std::string("Reloaded level scene: ") + loadedPath
                    );
                } else {
                    Detail::AddEditorLog(
                        hasBlenderTask
                            ? Detail::EditorLogType::Warning
                            : Detail::EditorLogType::Error,
                        hasBlenderTask
                            ? "Task.json loaded partially: one or more model files are missing."
                            : "Reload level scene failed."
                    );
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Save Task && Apply Live")) {
                if (!sceneObjects.SaveLevelSceneToJson("Resources/Task.json")) {
                    Detail::AddEditorLog(
                        Detail::EditorLogType::Error,
                        "Live apply cancelled: failed to save Resources/Task.json."
                    );
                } else if (!sceneObjects.LoadLevelSceneFromJson("Resources/Task.json")) {
                    Detail::AddEditorLog(
                        Detail::EditorLogType::Warning,
                        "Task.json was applied partially: one or more assets are missing."
                    );
                } else {
                    Detail::AddEditorLog(
                        Detail::EditorLogType::Info,
                        "Saved and applied Resources/Task.json without restarting the EXE."
                    );
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Edit")) {
            ImGui::MenuItem("Undo", "Ctrl+Z", false, false);
            ImGui::MenuItem("Redo", "Ctrl+Y", false, false);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("GameObject")) {
            if (ImGui::MenuItem("Terrain")) {
                selected_ = EditorSelection::Terrain;
            }

            if (ImGui::MenuItem("Axis +")) {
                selected_ = EditorSelection::AxisPositive;
            }

            if (ImGui::MenuItem("Axis -")) {
                selected_ = EditorSelection::AxisNegative;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("MyMenu")) {
            const bool hasSelectedObject =
                selected_ == EditorSelection::DynamicObject &&
                selectedObjectIndex_ < sceneObjects.GetObjectCount();

            if (ImGui::MenuItem("Stretch Vertex 0 (+X)", nullptr, false, hasSelectedObject)) {
                if (sceneObjects.StretchObjectVertexX(selectedObjectIndex_, 0, 1.0f)) {
                    Detail::AddEditorLog(Detail::EditorLogType::Info, "Stretched vertex 0 by +1.0 on X.");
                } else {
                    Detail::AddEditorLog(Detail::EditorLogType::Error, "Vertex stretch failed.");
                }
            }

            if (ImGui::MenuItem("Create Ico Sphere")) {
                if (sceneObjects.CreateIcoSphere()) {
                    selectedObjectIndex_ = sceneObjects.GetObjectCount() - 1;
                    selected_ = EditorSelection::DynamicObject;
                    Detail::GetModelApplyTargetIndex() = selectedObjectIndex_;
                    Detail::AddEditorLog(Detail::EditorLogType::Info, "Created IcoSphere.");
                } else {
                    Detail::AddEditorLog(Detail::EditorLogType::Error, "IcoSphere creation failed.");
                }
            }

            if (ImGui::MenuItem("Add Model File Name", nullptr, false, hasSelectedObject)) {
                if (sceneObjects.GetObjectExportFileName(selectedObjectIndex_).empty()) {
                    const std::string& path = sceneObjects.GetObjectModelPath(selectedObjectIndex_);
                    const size_t slash = path.find_last_of("/\\");
                    sceneObjects.SetObjectExportFileName(
                        selectedObjectIndex_,
                        slash == std::string::npos ? path : path.substr(slash + 1)
                    );
                }
            }

            if (ImGui::MenuItem("Add Box Collider", nullptr, false, hasSelectedObject)) {
                sceneObjects.AddBoxCollider(selectedObjectIndex_);
            }

            if (workspace_ == Workspace::Effects || workspace_ == Workspace::SceneEditing) {
                ImGui::Separator();
                ImGui::MenuItem("Show Colliders", nullptr, &showColliders_);
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Workspace")) {
            if (ImGui::MenuItem("Stage Editing", nullptr, workspace_ == Workspace::StageEditing)) { ApplyWorkspace(Workspace::StageEditing); drawMode = GameSceneDrawMode::NormalObj; }
            if (ImGui::MenuItem("Game Testing", nullptr, workspace_ == Workspace::GameTesting)) { ApplyWorkspace(Workspace::GameTesting); drawMode = GameSceneDrawMode::NormalObj; }
            if (ImGui::MenuItem("Scene Editing", nullptr, workspace_ == Workspace::SceneEditing)) { ApplyWorkspace(Workspace::SceneEditing); drawMode = GameSceneDrawMode::Effect; }
            if (ImGui::MenuItem("Effects", nullptr, workspace_ == Workspace::Effects)) { ApplyWorkspace(Workspace::Effects); drawMode = GameSceneDrawMode::Effect; }
            if (ImGui::MenuItem("Animation", nullptr, workspace_ == Workspace::Animation)) { ApplyWorkspace(Workspace::Animation); drawMode = GameSceneDrawMode::Animation; }
            ImGui::Separator();
            if (ImGui::MenuItem("Save Current Dock Layout")) {
                ImGui::SaveIniSettingsToDisk("imgui.ini");
            }
            if (ImGui::MenuItem("Reload Saved Dock Layout")) {
                ImGui::LoadIniSettingsFromDisk("imgui.ini");
            }
            if (ImGui::MenuItem("Reset Visibility Layout")) ApplyWorkspace(Workspace::StageEditing);
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Window")) {
            bool changed = false;
            changed |= ImGui::MenuItem("Hierarchy", nullptr, &showHierarchyWindow_);
            changed |= ImGui::MenuItem("Inspector", nullptr, &showInspectorWindow_);
            changed |= ImGui::MenuItem("Game View", "F11", &showGameViewWindow_);
            changed |= ImGui::MenuItem("3D Stage Editor", nullptr, &showStageEditorWindow_);
            changed |= ImGui::MenuItem("Project", "Ctrl+P", &showProjectWindow_);
            changed |= ImGui::MenuItem("Console", "Ctrl+`", &showConsoleWindow_);
            changed |= ImGui::MenuItem("Debug Tools", nullptr, &showDebugToolsWindow_);
            ImGui::Separator();
            if (ImGui::MenuItem("Maximize Game View", "F11", gameViewMaximized_)) {
                gameViewMaximized_ = !gameViewMaximized_;
                showGameViewWindow_ = true;
                changed = true;
            }
            if (changed) workspace_ = Workspace::Custom;
            ImGui::EndMenu();
        }

        ImGui::Separator();

        if (ImGui::Button(stageEditor.IsGamePlayMode() ? "Stop (F5)" : "Play (F5)")) {
            stageEditor.ToggleMode();
            drawMode = GameSceneDrawMode::NormalObj;
            Detail::AddEditorLog(
                Detail::EditorLogType::Info,
                stageEditor.IsGamePlayMode() ? "GamePlay started." : "Returned to editor."
            );
        }

        ImGui::SameLine();

        if (drawMode == GameSceneDrawMode::Effect && ImGui::Button("Emit")) {
            hitEffect.Emit({ 0.0f, 3.0f, 0.0f });
            Detail::AddEditorLog(Detail::EditorLogType::Info, "Effect emitted from menu bar.");
        }

        ImGui::SameLine();
        ImGui::TextDisabled("Workspace: %s", GetWorkspaceName());

        ImGui::SameLine();
        ImGui::TextDisabled(
            "Joints:%d  Anim:%.2f",
            animationDebug.GetJointCount(),
            animationDebug.GetAnimationTime()
        );

        ImGui::EndMainMenuBar();
    }

#endif
}

void SceneDebugPanel::DrawHierarchyWindow(
    SceneObjectController& sceneObjects
) {
#ifdef USE_IMGUI
    ImGui::Begin("Hierarchy");

    ImGui::TextDisabled("Sample Scene");
    ImGui::Separator();

    ImGuiTreeNodeFlags rootFlags =
        ImGuiTreeNodeFlags_DefaultOpen |
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (ImGui::TreeNodeEx("Scene", rootFlags)) {
        if (ImGui::Selectable(
            "Terrain",
            selected_ == EditorSelection::Terrain
        )) {
            selected_ = EditorSelection::Terrain;
            selectedObjectIndex_ = 0;
            Detail::GetModelApplyTargetIndex() = 0;
        }

        if (ImGui::Selectable(
            "Axis +X",
            selected_ == EditorSelection::AxisPositive
        )) {
            selected_ = EditorSelection::AxisPositive;
            selectedObjectIndex_ = 1;
            Detail::GetModelApplyTargetIndex() = 1;
        }

        if (ImGui::Selectable(
            "Axis -X",
            selected_ == EditorSelection::AxisNegative
        )) {
            selected_ = EditorSelection::AxisNegative;
            selectedObjectIndex_ = 2;
            Detail::GetModelApplyTargetIndex() = 2;
        }

        ImGui::Separator();

        if (sceneObjects.GetObjectCount() > 3) {
            ImGui::TextDisabled("Created Objects");

            std::function<void(size_t)> drawObjectNode;
            drawObjectNode = [&](size_t index) {
                bool hasChildren = false;
                for (size_t child = 3; child < sceneObjects.GetObjectCount(); ++child) {
                    if (sceneObjects.GetObjectParent(child) == index) {
                        hasChildren = true;
                        break;
                    }
                }

                ImGuiTreeNodeFlags flags =
                    ImGuiTreeNodeFlags_OpenOnArrow |
                    ImGuiTreeNodeFlags_SpanAvailWidth;
                if (!hasChildren) {
                    flags |= ImGuiTreeNodeFlags_Leaf |
                        ImGuiTreeNodeFlags_NoTreePushOnOpen;
                }
                if (selected_ == EditorSelection::DynamicObject &&
                    selectedObjectIndex_ == index) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                ImGui::PushID(static_cast<int>(index));
                const bool open = ImGui::TreeNodeEx(
                    sceneObjects.GetObjectName(index).c_str(),
                    flags
                );
                if (ImGui::IsItemClicked()) {
                    selected_ = EditorSelection::DynamicObject;
                    selectedObjectIndex_ = index;
                    Detail::GetModelApplyTargetIndex() = index;
                }
                if (open && hasChildren) {
                    for (size_t child = 3; child < sceneObjects.GetObjectCount(); ++child) {
                        if (sceneObjects.GetObjectParent(child) == index) {
                            drawObjectNode(child);
                        }
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            };

            for (size_t index = 3; index < sceneObjects.GetObjectCount(); ++index) {
                const size_t parent = sceneObjects.GetObjectParent(index);
                if (parent == SceneObjectController::kNoParent || parent < 3) {
                    drawObjectNode(index);
                }
            }

            ImGui::Separator();
        }

        if (ImGui::Selectable(
            "Main Camera",
            selected_ == EditorSelection::Camera
        )) {
            selected_ = EditorSelection::Camera;
        }

        if (ImGui::Selectable(
            "Environment",
            selected_ == EditorSelection::Environment
        )) {
            selected_ = EditorSelection::Environment;
        }

        if (ImGui::Selectable(
            "Effects",
            selected_ == EditorSelection::Effects
        )) {
            selected_ = EditorSelection::Effects;
        }

        ImGui::TreePop();
    }

    ImGui::Spacing();
    ImGui::SeparatorText("Visibility");

    ImGui::Checkbox(
        "Show Terrain",
        &sceneObjects.ShowTerrain()
    );

    ImGui::Checkbox(
        "Show Axis",
        &sceneObjects.ShowAxis()
    );

    for (size_t index = 3; index < sceneObjects.GetObjectCount(); ++index) {
        bool visible =
            sceneObjects.IsObjectVisible(index);

        const std::string label =
            "Show " + sceneObjects.GetObjectName(index);

        if (ImGui::Checkbox(label.c_str(), &visible)) {
            sceneObjects.SetObjectVisible(index, visible);
        }
    }

    ImGui::End();
#endif
}

void SceneDebugPanel::DrawInspectorWindow(
    EngineContext* context,
    EnvironmentController& environment,
    SceneObjectController& sceneObjects,
    AnimationDebugController& animationDebug
) {
#ifdef USE_IMGUI
    ImGui::Begin("Inspector");

    if (selected_ == EditorSelection::None) {
        ImGui::TextDisabled("No object selected.");
        ImGui::End();
        return;
    }

    auto DrawHeader = [](const char* icon, const char* name) {
        ImGui::Text("%s %s", icon, name);
        ImGui::Separator();
        };

    switch (selected_) {
    case EditorSelection::Terrain:

    {
        DrawHeader("Object", "Terrain");

        ImGui::Checkbox(
            "Active",
            &sceneObjects.ShowTerrain()
        );

        Object3d* object =
            sceneObjects.GetEditorObject(
                SceneObjectController::EditorObjectType::Terrain
            );

        Detail::DrawTransformInspector(object);
        Detail::DrawMaterialInspector(object);

        if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("Resources/terrain/terrain.obj");
        }

        break;
    }

    case EditorSelection::AxisPositive:

    {
        DrawHeader("Axis", "Axis +X");

        ImGui::Checkbox(
            "Active",
            &sceneObjects.ShowAxis()
        );

        Object3d* object =
            sceneObjects.GetEditorObject(
                SceneObjectController::EditorObjectType::AxisPositive
            );

        Detail::DrawTransformInspector(object);
        Detail::DrawMaterialInspector(object);

        if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("axis.obj");
        }

        break;
    }

    case EditorSelection::AxisNegative:

    {
        DrawHeader("Axis", "Axis -X");

        ImGui::Checkbox(
            "Active",
            &sceneObjects.ShowAxis()
        );

        Object3d* object =
            sceneObjects.GetEditorObject(
                SceneObjectController::EditorObjectType::AxisNegative
            );

        Detail::DrawTransformInspector(object);
        Detail::DrawMaterialInspector(object);

        if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("axis.obj");
        }

        break;
    }

    case EditorSelection::Camera:
        DrawHeader("▣", "Main Camera");

        if (context && context->GetCamera()) {
            ImGui::TextDisabled("Camera debug controller is active.");
            ImGui::TextDisabled("Right drag : Orbit");
            ImGui::TextDisabled("Wheel      : Zoom");
        }
        break;

    case EditorSelection::Environment:
        DrawHeader("☀", "Environment");

        DrawEnvironmentTab(
            context ? context->GetObject3dCommon() : nullptr,
            environment,
            sceneObjects,
            animationDebug
        );
        break;

    case EditorSelection::Effects:
        DrawHeader("✦", "Effects");
        ImGui::TextDisabled("Open Debug Tools > Effect for detailed controls.");
        break;

    case EditorSelection::PostEffect:
        DrawHeader("▣", "Post Effect");
        DrawPostEffectTab(context);
        break;

    case EditorSelection::Sound:
        DrawHeader("♪", "Sound");
        ImGui::TextDisabled("Open Debug Tools > Sound for detailed controls.");
        break;

    case EditorSelection::DynamicObject:
    {
        const std::string& objectName =
            sceneObjects.GetObjectName(selectedObjectIndex_);

        DrawHeader("Object", objectName.empty() ? "Created Object" : objectName.c_str());

        bool visible =
            sceneObjects.IsObjectVisible(selectedObjectIndex_);

        if (ImGui::Checkbox("Active", &visible)) {
            sceneObjects.SetObjectVisible(selectedObjectIndex_, visible);
        }

        Object3d* object =
            sceneObjects.GetObject(selectedObjectIndex_);

        Detail::DrawTransformInspector(object);
        Detail::DrawMaterialInspector(object);

        if (ImGui::CollapsingHeader("Model", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("Source: %s", sceneObjects.GetObjectModelPath(selectedObjectIndex_).c_str());

            char fileName[260]{};
            strncpy_s(
                fileName,
                sizeof(fileName),
                sceneObjects.GetObjectExportFileName(selectedObjectIndex_).c_str(),
                _TRUNCATE
            );
            if (ImGui::InputText("Export File Name", fileName, sizeof(fileName))) {
                sceneObjects.SetObjectExportFileName(selectedObjectIndex_, fileName);
            }
        }

        if (ImGui::CollapsingHeader("Collider", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (SceneObjectController::BoxCollider* collider =
                sceneObjects.GetBoxCollider(selectedObjectIndex_)) {
                ImGui::TextDisabled("Type: BOX");
                if (sceneObjects.IsObjectColliding(selectedObjectIndex_)) {
                    ImGui::TextColored(
                        ImVec4(1.0f, 0.25f, 0.2f, 1.0f),
                        "Collision: HIT"
                    );
                } else {
                    ImGui::TextDisabled("Collision: None");
                }
                ImGui::DragFloat3("Center", &collider->center.x, 0.05f);
                if (ImGui::DragFloat3("Size", &collider->size.x, 0.05f, 0.01f, 1000.0f)) {
                    collider->size.x = (std::max)(collider->size.x, 0.01f);
                    collider->size.y = (std::max)(collider->size.y, 0.01f);
                    collider->size.z = (std::max)(collider->size.z, 0.01f);
                }
                if (ImGui::Button("Remove Box Collider")) {
                    sceneObjects.RemoveBoxCollider(selectedObjectIndex_);
                }
            } else if (ImGui::Button("Add Box Collider")) {
                sceneObjects.AddBoxCollider(selectedObjectIndex_);
            }
        }

        break;
    }

    default:
        ImGui::TextDisabled("No inspector available.");
        break;

    case EditorSelection::Asset:
    {
        size_t createdIndex = static_cast<size_t>(-1);

        if (Detail::DrawAssetInspector(
            context,
            sceneObjects,
            createdIndex
        )) {
            selected_ = EditorSelection::DynamicObject;
            selectedObjectIndex_ = createdIndex;
        }

        break;
    }
    }

    ImGui::End();
#endif
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void SceneDebugPanel::DrawProjectWindow() {
#ifdef USE_IMGUI
    ImGui::Begin("Project");

    auto DrawProjectAsset = [this](const char* label, const char* path) {
        if (Detail::DrawAssetItem(label, path)) {
            selected_ = EditorSelection::Asset;
        }
        };

    ImGui::TextDisabled("Assets");
    ImGui::Separator();

    if (ImGui::BeginTable(
        "ProjectBrowserTable",
        2,
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_BordersInnerV |
        ImGuiTableFlags_RowBg
    )) {
        ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("Assets", ImGuiTableColumnFlags_WidthStretch);

        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);

        ImGui::TextDisabled("Favorites");
        ImGui::Separator();

        ImGui::Selectable("Resources", true);
        ImGui::Selectable("Models");
        ImGui::Selectable("Textures");
        ImGui::Selectable("Shaders");
        ImGui::Selectable("Settings");

        ImGui::TableSetColumnIndex(1);

        if (ImGui::TreeNodeEx(
            "Models",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth
        )) {
            DrawProjectAsset("axis.obj", "Resources/axis.obj");
            DrawProjectAsset("plane.obj", "Resources/plane.obj");
            DrawProjectAsset("terrain.obj", "Resources/terrain/terrain.obj");
            DrawProjectAsset("bunny.obj", "Resources/bunny/bunny.obj");
            DrawProjectAsset("fence.obj", "Resources/fence/fence.obj");
            DrawProjectAsset("AnimatedCube.gltf", "Resources/AnimatedCube/AnimatedCube.gltf");
            DrawProjectAsset("human/walk.gltf", "Resources/human/walk.gltf");
            DrawProjectAsset("human/walk.fbx", "Resources/human/walk.fbx");

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx(
            "Textures",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth
        )) {
            DrawProjectAsset("monsterBall.png", "Resources/monsterBall.png");
            DrawProjectAsset("noise0.png", "Resources/noise0.png");
            DrawProjectAsset("uvChecker.png", "Resources/uvChecker.png");
            DrawProjectAsset("white.png", "Resources/white.png");
            DrawProjectAsset("fence.png", "Resources/fence/fence.png");

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx(
            "Shaders",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth
        )) {
            DrawProjectAsset("Object3d.VS.hlsl", "Resources/shaders/hlsl/Object3d.VS.hlsl");
            DrawProjectAsset("Object3d.PS.hlsl", "Resources/shaders/hlsl/Object3d.PS.hlsl");
            DrawProjectAsset("Particle.VS.hlsl", "Resources/shaders/hlsl/Particle.VS.hlsl");
            DrawProjectAsset("Particle.PS.hlsl", "Resources/shaders/hlsl/Particle.PS.hlsl");
            DrawProjectAsset("CopyImage.PS.hlsl", "Resources/shaders/hlsl/CopyImage.PS.hlsl");
            DrawProjectAsset("GaussianFilter.PS.hlsl", "Resources/shaders/hlsl/GaussianFilter.PS.hlsl");

            ImGui::TreePop();
        }

        if (ImGui::TreeNodeEx(
            "Settings",
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth
        )) {
            DrawProjectAsset("Fire.txt", "Resources/Settings/HitEffects/Fire.txt");
            DrawProjectAsset("cherryBlossoms.txt", "Resources/Settings/HitEffects/cherryBlossoms.txt");

            ImGui::TreePop();
        }

        ImGui::EndTable();
    }

    ImGui::End();
#endif
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void SceneDebugPanel::DrawConsoleWindow() {
#ifdef USE_IMGUI
    Detail::InitializeEditorConsoleOnce();

    static bool showInfo = true;
    static bool showWarning = true;
    static bool showError = true;
    static bool autoScroll = true;

    ImGui::Begin("Console");

    if (ImGui::Button("Clear")) {
        Detail::GetEditorLogs().clear();
    }

    ImGui::SameLine();
    ImGui::Checkbox("Info", &showInfo);

    ImGui::SameLine();
    ImGui::Checkbox("Warning", &showWarning);

    ImGui::SameLine();
    ImGui::Checkbox("Error", &showError);

    ImGui::SameLine();
    ImGui::Checkbox("Auto Scroll", &autoScroll);

    ImGui::Separator();

    ImGui::BeginChild(
        "ConsoleLogArea",
        ImVec2(0.0f, 0.0f),
        false,
        ImGuiWindowFlags_HorizontalScrollbar
    );

    for (const Detail::EditorConsoleLog& log : Detail::GetEditorLogs()) {
        if (log.type == Detail::EditorLogType::Info && !showInfo) {
            continue;
        }

        if (log.type == Detail::EditorLogType::Warning && !showWarning) {
            continue;
        }

        if (log.type == Detail::EditorLogType::Error && !showError) {
            continue;
        }

        ImGui::TextColored(
            Detail::GetEditorLogColor(log.type),
            "%s %s",
            Detail::GetEditorLogPrefix(log.type),
            log.message.c_str()
        );
    }

    if (autoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();

    ImGui::End();
#endif
}

void SceneDebugPanel::DrawDebugToolsWindow(
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
) {
#ifdef USE_IMGUI
    ImGui::Begin(
        "Debug Tools",
        nullptr,
        ImGuiWindowFlags_AlwaysVerticalScrollbar
    );

    DrawSceneControl(
        drawMode,
        hitEffect,
        animationDebug
    );

    if (ImGui::BeginTabBar("DebugToolTabs")) {
        

        if (ImGui::BeginTabItem("Animation")) {
            animationDebugPanel_.Draw(
                animationDebug
            );

            ImGui::Separator();

            // 既存のJoint・再生速度などの編集画面
            animationDebug.DrawImGui();

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Effect")) {
            selected_ = EditorSelection::Effects;

            DrawEffectTab(
                context,
                hitEffect,
                ring,
                cylinder,
                primitive
            );

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("PostEffect")) {
            selected_ = EditorSelection::PostEffect;

            DrawPostEffectTab(
                context
            );

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Environment")) {
            selected_ = EditorSelection::Environment;

            DrawEnvironmentTab(
                context->GetObject3dCommon(),
                environment,
                sceneObjects,
                animationDebug
            );

            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Sound")) {
            selected_ = EditorSelection::Sound;

            soundController.DrawImGui();

            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
#endif
}

