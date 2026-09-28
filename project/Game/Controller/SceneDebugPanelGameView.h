// ============================================================================
// ファイルの役割: ゲームビューと各種デバッグ編集ウィンドウをImGui上へ構築する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void SceneDebugPanel::DrawGameViewWindow(
    EngineContext* context,
    SceneObjectController& sceneObjects,
    StageEditor& stageEditor
)
{
#ifdef USE_IMGUI
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_None;
    const char* windowName = "Game View";
    if (gameViewMaximized_) {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos, ImGuiCond_Always);
        ImGui::SetNextWindowSize(viewport->WorkSize, ImGuiCond_Always);
        windowFlags = ImGuiWindowFlags_NoDecoration |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoBringToFrontOnFocus;
        windowName = "Game View##Maximized";
    }
    ImGui::Begin(windowName, nullptr, windowFlags);

    if (!context || !context->GetImGuiManager()) {
        ImGui::TextDisabled("Game View unavailable.");
        ImGui::End();
        return;
    }

    if (!gameViewMaximized_) {
        ImGui::Checkbox("Gizmo", &showTransformGizmo_);

        if (workspace_ == Workspace::Effects || workspace_ == Workspace::SceneEditing) {
            ImGui::SameLine();
            ImGui::Checkbox("Colliders", &showColliders_);
        }

        ImGui::SameLine();
        ImGui::RadioButton("Move", &gizmoOperation_, 0);

        ImGui::SameLine();
        ImGui::RadioButton("Rotate", &gizmoOperation_, 1);

        ImGui::SameLine();
        ImGui::RadioButton("Scale", &gizmoOperation_, 2);

        ImGui::SameLine();
        ImGui::RadioButton("Local", &gizmoMode_, 0);

        ImGui::SameLine();
        ImGui::RadioButton("World", &gizmoMode_, 1);
    }

    ImGuiManager* imguiManager =
        context->GetImGuiManager();


    ImVec2 availableSize =
        ImGui::GetContentRegionAvail();

    if (availableSize.x <= 1.0f || availableSize.y <= 1.0f) {
        ImGui::TextDisabled("No space for Game View.");
        ImGui::End();
        return;
    }

    const uint32_t srvIndex =
        imguiManager->GetGameViewSrvIndex();

    D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle =
        context->GetSrvManager()->GetGPUDescriptorHandle(srvIndex);

    ImTextureID textureId =
        static_cast<ImTextureID>(gpuHandle.ptr);

    ImGui::Image(
        textureId,
        availableSize,
        ImVec2(0.0f, 0.0f),
        ImVec2(1.0f, 1.0f)
    );

    const ImVec2 imageMin = ImGui::GetItemRectMin();
    const ImVec2 imageMax = ImGui::GetItemRectMax();

    stageEditor.DrawGameView(
        imageMin.x,
        imageMin.y,
        imageMax.x - imageMin.x,
        imageMax.y - imageMin.y
    );

    // SceneObjectController belongs to the runtime scene.  Its colliders
    // (including the legacy terrain/axis helpers) would otherwise be drawn
    // over the stage placement view.
    if (!stageEditor.IsGamePlayMode() &&
        (workspace_ == Workspace::Effects || workspace_ == Workspace::SceneEditing)) {
        DrawColliderOverlay(
            context,
            sceneObjects,
            imageMin.x,
            imageMin.y,
            imageMax.x - imageMin.x,
            imageMax.y - imageMin.y
        );
    }

    if (!stageEditor.IsEditingGameView()) {
        DrawTransformGizmo(
            context,
            sceneObjects,
            imageMin.x,
            imageMin.y,
            imageMax.x - imageMin.x,
            imageMax.y - imageMin.y
        );
    }

    ImGui::End();
#endif
}

void SceneDebugPanel::DrawColliderOverlay(
    EngineContext* context,
    SceneObjectController& sceneObjects,
    float rectX,
    float rectY,
    float rectWidth,
    float rectHeight
)
{
#ifdef USE_IMGUI
    if (!showColliders_ || !context || !context->GetCamera()) {
        return;
    }

    const Matrix4x4& viewProjection = context->GetCamera()->GetViewProjectionMatrix();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    constexpr int edges[12][2] = {
        {0,1}, {1,3}, {3,2}, {2,0},
        {4,5}, {5,7}, {7,6}, {6,4},
        {0,4}, {1,5}, {2,6}, {3,7}
    };

    auto transformPoint = [](const Vector3& p, const Matrix4x4& m) {
        return Vector4{
            p.x * m.m[0][0] + p.y * m.m[1][0] + p.z * m.m[2][0] + m.m[3][0],
            p.x * m.m[0][1] + p.y * m.m[1][1] + p.z * m.m[2][1] + m.m[3][1],
            p.x * m.m[0][2] + p.y * m.m[1][2] + p.z * m.m[2][2] + m.m[3][2],
            p.x * m.m[0][3] + p.y * m.m[1][3] + p.z * m.m[2][3] + m.m[3][3]
        };
    };

    for (size_t index = 0; index < sceneObjects.GetObjectCount(); ++index) {
        const SceneObjectController::BoxCollider* collider = sceneObjects.GetBoxCollider(index);
        const Object3d* object = sceneObjects.GetObject(index);
        if (!collider || !object || !sceneObjects.IsObjectVisible(index)) {
            continue;
        }

        const Matrix4x4& world = object->GetWorldMatrix();
        const Matrix4x4 worldViewProjection = Math::Multiply(world, viewProjection);
        ImVec2 screen[8]{};
        bool visible[8]{};

        for (int corner = 0; corner < 8; ++corner) {
            const Vector3 local = {
                collider->center.x + ((corner & 1) ? collider->size.x : -collider->size.x),
                collider->center.y + ((corner & 2) ? collider->size.y : -collider->size.y),
                collider->center.z + ((corner & 4) ? collider->size.z : -collider->size.z)
            };
            const Vector4 clip = transformPoint(local, worldViewProjection);
            if (clip.w > 0.001f) {
                const float ndcX = clip.x / clip.w;
                const float ndcY = clip.y / clip.w;
                screen[corner] = {
                    rectX + (ndcX * 0.5f + 0.5f) * rectWidth,
                    rectY + (-ndcY * 0.5f + 0.5f) * rectHeight
                };
                visible[corner] = true;
            }
        }

        for (const auto& edge : edges) {
            if (visible[edge[0]] && visible[edge[1]]) {
                const ImU32 colliderColor = sceneObjects.IsObjectColliding(index)
                    ? IM_COL32(255, 51, 38, 240)
                    : IM_COL32(26, 255, 51, 230);
                drawList->AddLine(
                    screen[edge[0]],
                    screen[edge[1]],
                    colliderColor,
                    2.0f
                );
            }
        }
    }
#endif
}

void SceneDebugPanel::DrawTransformGizmo(
    EngineContext* context,
    SceneObjectController& sceneObjects,
    float rectX,
    float rectY,
    float rectWidth,
    float rectHeight
)
{
#ifdef USE_IMGUI
    if (!showTransformGizmo_) {
        return;
    }

    if (!context || !context->GetCamera()) {
        return;
    }

    Object3d* object = nullptr;

    switch (selected_) {
    case EditorSelection::Terrain:
        object = sceneObjects.GetEditorObject(
            SceneObjectController::EditorObjectType::Terrain
        );
        break;

    case EditorSelection::AxisPositive:
        object = sceneObjects.GetEditorObject(
            SceneObjectController::EditorObjectType::AxisPositive
        );
        break;

    case EditorSelection::AxisNegative:
        object = sceneObjects.GetEditorObject(
            SceneObjectController::EditorObjectType::AxisNegative
        );
        break;

    case EditorSelection::DynamicObject:
        object = sceneObjects.GetObject(selectedObjectIndex_);
        break;

    default:
        break;
    }

    if (!object) {
        return;
    }

    Camera* camera =
        context->GetCamera();

    Transform& transform =
        object->GetTransform();

    Matrix4x4 worldMatrix = object->GetWorldMatrix();

    float world[16]{};
    float view[16]{};
    float projection[16]{};

    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            const int index = row * 4 + column;

            world[index] =
                worldMatrix.m[row][column];

            view[index] =
                camera->GetViewMatrix().m[row][column];

            projection[index] =
                camera->GetProjectionMatrix().m[row][column];
        }
    }

    ImGuizmo::SetOrthographic(false);
    ImGuizmo::SetDrawlist();

    ImGuizmo::SetRect(
        rectX,
        rectY,
        rectWidth,
        rectHeight
    );

    ImGuizmo::OPERATION operation =
        ImGuizmo::TRANSLATE;

    if (gizmoOperation_ == 1) {
        operation = ImGuizmo::ROTATE;
    } else if (gizmoOperation_ == 2) {
        operation = ImGuizmo::SCALE;
    }

    ImGuizmo::MODE mode =
        gizmoMode_ == 0
        ? ImGuizmo::LOCAL
        : ImGuizmo::WORLD;

    ImGuizmo::Manipulate(
        view,
        projection,
        operation,
        mode,
        world
    );

    if (ImGuizmo::IsUsing()) {
        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                worldMatrix.m[row][column] = world[row * 4 + column];
            }
        }

        if (const Object3d* parent = object->GetParent()) {
            worldMatrix = Math::Multiply(
                worldMatrix,
                Math::Inverse(parent->GetWorldMatrix())
            );
        }

        for (int row = 0; row < 4; ++row) {
            for (int column = 0; column < 4; ++column) {
                world[row * 4 + column] = worldMatrix.m[row][column];
            }
        }

        float translate[3]{};
        float rotateDegrees[3]{};
        float scale[3]{};

        ImGuizmo::DecomposeMatrixToComponents(
            world,
            translate,
            rotateDegrees,
            scale
        );

        transform.translate.x = translate[0];
        transform.translate.y = translate[1];
        transform.translate.z = translate[2];

        transform.rotate.x = rotateDegrees[0] * 0.017453292519943295f;
        transform.rotate.y = rotateDegrees[1] * 0.017453292519943295f;
        transform.rotate.z = rotateDegrees[2] * 0.017453292519943295f;

        transform.scale.x = scale[0];
        transform.scale.y = scale[1];
        transform.scale.z = scale[2];
    }
#endif
}


