// ============================================================================
// ファイルの役割: シーン配置オブジェクトの生成、更新、JSON保存・読込を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
namespace {
    struct SceneControllerObb {
        Vector3 center{};
        Vector3 axis[3]{};
        float halfSize[3]{};
    };

    float SceneControllerDot(const Vector3& a, const Vector3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    float SceneControllerLength(const Vector3& value) {
        return std::sqrt(SceneControllerDot(value, value));
    }

    Vector3 SceneControllerTransformPoint(
        const Vector3& point,
        const Matrix4x4& matrix
    ) {
        return {
            point.x * matrix.m[0][0] + point.y * matrix.m[1][0] +
                point.z * matrix.m[2][0] + matrix.m[3][0],
            point.x * matrix.m[0][1] + point.y * matrix.m[1][1] +
                point.z * matrix.m[2][1] + matrix.m[3][1],
            point.x * matrix.m[0][2] + point.y * matrix.m[1][2] +
                point.z * matrix.m[2][2] + matrix.m[3][2]
        };
    }

    SceneControllerObb SceneControllerMakeObb(
        const Object3d& object,
        const SceneObjectController::BoxCollider& collider
    ) {
        const Matrix4x4& world = object.GetWorldMatrix();
        SceneControllerObb result{};
        result.center = SceneControllerTransformPoint(collider.center, world);

        const Vector3 rawAxes[3] = {
            { world.m[0][0], world.m[0][1], world.m[0][2] },
            { world.m[1][0], world.m[1][1], world.m[1][2] },
            { world.m[2][0], world.m[2][1], world.m[2][2] }
        };
        const float localHalfSize[3] = {
            std::abs(collider.size.x),
            std::abs(collider.size.y),
            std::abs(collider.size.z)
        };

        for (int axisIndex = 0; axisIndex < 3; ++axisIndex) {
            const float length = SceneControllerLength(rawAxes[axisIndex]);
            if (length > 0.000001f) {
                result.axis[axisIndex] = {
                    rawAxes[axisIndex].x / length,
                    rawAxes[axisIndex].y / length,
                    rawAxes[axisIndex].z / length
                };
                result.halfSize[axisIndex] = localHalfSize[axisIndex] * length;
            } else {
                result.axis[axisIndex] = {
                    axisIndex == 0 ? 1.0f : 0.0f,
                    axisIndex == 1 ? 1.0f : 0.0f,
                    axisIndex == 2 ? 1.0f : 0.0f
                };
                result.halfSize[axisIndex] = 0.0f;
            }
        }

        return result;
    }

    bool SceneControllerIntersects(
        const SceneControllerObb& a,
        const SceneControllerObb& b
    ) {
        constexpr float kEpsilon = 0.00001f;
        float rotation[3][3]{};
        float absoluteRotation[3][3]{};

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                rotation[i][j] = SceneControllerDot(a.axis[i], b.axis[j]);
                absoluteRotation[i][j] = std::abs(rotation[i][j]) + kEpsilon;
            }
        }

        const Vector3 centerDelta = {
            b.center.x - a.center.x,
            b.center.y - a.center.y,
            b.center.z - a.center.z
        };
        float translation[3] = {
            SceneControllerDot(centerDelta, a.axis[0]),
            SceneControllerDot(centerDelta, a.axis[1]),
            SceneControllerDot(centerDelta, a.axis[2])
        };

        for (int i = 0; i < 3; ++i) {
            const float radiusA = a.halfSize[i];
            const float radiusB =
                b.halfSize[0] * absoluteRotation[i][0] +
                b.halfSize[1] * absoluteRotation[i][1] +
                b.halfSize[2] * absoluteRotation[i][2];
            if (std::abs(translation[i]) > radiusA + radiusB) {
                return false;
            }
        }

        for (int j = 0; j < 3; ++j) {
            const float radiusA =
                a.halfSize[0] * absoluteRotation[0][j] +
                a.halfSize[1] * absoluteRotation[1][j] +
                a.halfSize[2] * absoluteRotation[2][j];
            const float radiusB = b.halfSize[j];
            const float distance = std::abs(
                translation[0] * rotation[0][j] +
                translation[1] * rotation[1][j] +
                translation[2] * rotation[2][j]
            );
            if (distance > radiusA + radiusB) {
                return false;
            }
        }

        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                const int i1 = (i + 1) % 3;
                const int i2 = (i + 2) % 3;
                const int j1 = (j + 1) % 3;
                const int j2 = (j + 2) % 3;
                const float radiusA =
                    a.halfSize[i1] * absoluteRotation[i2][j] +
                    a.halfSize[i2] * absoluteRotation[i1][j];
                const float radiusB =
                    b.halfSize[j1] * absoluteRotation[i][j2] +
                    b.halfSize[j2] * absoluteRotation[i][j1];
                const float distance = std::abs(
                    translation[i2] * rotation[i1][j] -
                    translation[i1] * rotation[i2][j]
                );
                if (distance > radiusA + radiusB) {
                    return false;
                }
            }
        }

        return true;
    }
}

void SceneObjectController::Initialize(
    Object3dCommon* object3dCommon,
    uint32_t environmentTextureHandle,
    float environmentCoefficient
) {
    object3dCommon_ = object3dCommon;
    environmentTextureHandle_ = environmentTextureHandle;
    environmentCoefficient_ = environmentCoefficient;

    Model* modelPlane =
        ModelManager::Load("Resources/terrain", "terrain.obj");

    Model* modelAxis =
        ModelManager::Load("axis.obj");

    {
        std::unique_ptr<Object3d> object =
            std::make_unique<Object3d>();

        object->Initialize(object3dCommon);
        object->SetModel(modelPlane);
        object->SetPosition({ 0.0f, 1.0f, 0.0f });
        object->SetRotation({ 0.0f, 0.0f, 0.0f });
        object->SetScale({ 0.5f, 0.5f, 0.5f });

        object->SetEnvironmentTexture(environmentTextureHandle);
        object->SetEnvironmentCoefficient(environmentCoefficient);

        objects_.push_back(std::move(object));
        objectNames_.push_back("Terrain");
        objectVisible_.push_back(1);
        objectModelPaths_.push_back("Resources/terrain/terrain.obj");
        objectExportFileNames_.push_back("terrain.obj");
        objectTexturePaths_.push_back("");
        objectColliders_.push_back({});
        objectParentIndices_.push_back(kNoParent);
        objectColliding_.push_back(0);
    }

    {
        std::unique_ptr<Object3d> object =
            std::make_unique<Object3d>();

        object->Initialize(object3dCommon);
        object->SetModel(modelAxis);
        object->SetPosition({ 2.0f, 0.0f, 0.0f });
        object->SetRotation({ 1.57f, 0.0f, 0.0f });

        object->SetEnvironmentTexture(environmentTextureHandle);
        object->SetEnvironmentCoefficient(environmentCoefficient);

        objects_.push_back(std::move(object));
        objectNames_.push_back("Axis +X");
        objectVisible_.push_back(1);
        objectModelPaths_.push_back("axis.obj");
        objectExportFileNames_.push_back("axis.obj");
        objectTexturePaths_.push_back("");
        objectColliders_.push_back({});
        objectParentIndices_.push_back(kNoParent);
        objectColliding_.push_back(0);
    }

    {
        std::unique_ptr<Object3d> object =
            std::make_unique<Object3d>();

        object->Initialize(object3dCommon);
        object->SetModel(modelAxis);
        object->SetPosition({ -2.0f, 0.0f, 0.0f });
        object->SetRotation({ 1.57f, 0.0f, 0.0f });

        object->SetEnvironmentTexture(environmentTextureHandle);
        object->SetEnvironmentCoefficient(environmentCoefficient);

        objects_.push_back(std::move(object));
        objectNames_.push_back("Axis -X");
        objectVisible_.push_back(1);
        objectModelPaths_.push_back("axis.obj");
        objectExportFileNames_.push_back("axis.obj");
        objectTexturePaths_.push_back("");
        objectColliders_.push_back({});
        objectParentIndices_.push_back(kNoParent);
        objectColliding_.push_back(0);
    }
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void SceneObjectController::Finalize() {
    objects_.clear();
    objectNames_.clear();
    objectVisible_.clear();
    objectModelPaths_.clear();
    objectExportFileNames_.clear();
    objectTexturePaths_.clear();
    objectColliders_.clear();
    objectParentIndices_.clear();
    objectColliding_.clear();
    collisionPairs_.clear();
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void SceneObjectController::Update() {
    std::vector<uint8_t> updateState(objects_.size(), 0);
    std::function<void(size_t)> updateObject;
    updateObject = [&](size_t index) {
        if (index >= objects_.size() || updateState[index] == 2) {
            return;
        }
        if (updateState[index] == 1) {
            return;
        }

        updateState[index] = 1;
        const size_t parent = GetObjectParent(index);
        if (parent != kNoParent) {
            updateObject(parent);
        }
        objects_[index]->Update();
        updateState[index] = 2;
    };

    for (size_t index = 0; index < objects_.size(); ++index) {
        updateObject(index);
    }
    UpdateBoxCollisions();
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void SceneObjectController::Draw() {
    for (size_t index = 0; index < objects_.size(); ++index) {
        if (index == 0 && !showTerrain_) {
            continue;
        }

        if ((index == 1 || index == 2) && !showAxis_) {
            continue;
        }

        if (index >= 3 && index < objectVisible_.size() && !objectVisible_[index]) {
            continue;
        }

        objects_[index]->Draw();
    }
}

void SceneObjectController::SetEnvironmentCoefficient(
    float environmentCoefficient
) {
    for (auto& object : objects_) {
        object->SetEnvironmentCoefficient(environmentCoefficient);
    }
}

Object3d* SceneObjectController::GetEditorObject(EditorObjectType type)
{
    return GetObject(GetEditorObjectIndex(type));
}

const Object3d* SceneObjectController::GetEditorObject(EditorObjectType type) const
{
    return GetObject(GetEditorObjectIndex(type));
}

size_t SceneObjectController::AddEditorObject(Model* model, const std::string& name)
{
    return AddEditorObject(model, name, "");
}

size_t SceneObjectController::AddEditorObject(
    Model* model,
    const std::string& name,
    const std::string& modelPath
)
{
    if (!object3dCommon_ || !model) {
        return static_cast<size_t>(-1);
    }

    std::unique_ptr<Object3d> object =
        std::make_unique<Object3d>();

    object->Initialize(object3dCommon_);
    object->SetModel(model);
    object->SetPosition({ 0.0f, 0.0f, 0.0f });
    object->SetRotation({ 0.0f, 0.0f, 0.0f });
    object->SetScale({ 1.0f, 1.0f, 1.0f });
    object->SetEnvironmentTexture(environmentTextureHandle_);
    object->SetEnvironmentCoefficient(environmentCoefficient_);

    const size_t index = objects_.size();

    objects_.push_back(std::move(object));
    objectNames_.push_back(name);
    objectVisible_.push_back(1);
    objectModelPaths_.push_back(modelPath);
    const size_t slashPos = modelPath.find_last_of("/\\");
    objectExportFileNames_.push_back(
        slashPos == std::string::npos ? modelPath : modelPath.substr(slashPos + 1)
    );
    objectTexturePaths_.push_back("");
    objectColliders_.push_back({});
    objectParentIndices_.push_back(kNoParent);
    objectColliding_.push_back(0);

    return index;
}

size_t SceneObjectController::GetObjectCount() const
{
    return objects_.size();
}

Object3d* SceneObjectController::GetObject(size_t index)
{
    if (index >= objects_.size()) {
        return nullptr;
    }

    return objects_[index].get();
}

const Object3d* SceneObjectController::GetObject(size_t index) const
{
    if (index >= objects_.size()) {
        return nullptr;
    }

    return objects_[index].get();
}

const std::string& SceneObjectController::GetObjectName(size_t index) const
{
    static const std::string kEmptyName = "";

    if (index >= objectNames_.size()) {
        return kEmptyName;
    }

    return objectNames_[index];
}

void SceneObjectController::ClearEditorObjects()
{
    if (objects_.size() <= 3) {
        return;
    }

    objects_.resize(3);
    objectNames_.resize(3);
    objectVisible_.resize(3);
    objectModelPaths_.resize(3);
    objectExportFileNames_.resize(3);
    objectTexturePaths_.resize(3);
    objectColliders_.resize(3);
    objectParentIndices_.resize(3, kNoParent);
    objectColliding_.resize(3, 0);
    collisionPairs_.clear();
}

size_t SceneObjectController::AddEditorEmpty(const std::string& name)
{
    if (!object3dCommon_) {
        return kNoParent;
    }

    std::unique_ptr<Object3d> object = std::make_unique<Object3d>();
    object->Initialize(object3dCommon_);
    object->SetPosition({ 0.0f, 0.0f, 0.0f });
    object->SetRotation({ 0.0f, 0.0f, 0.0f });
    object->SetScale({ 1.0f, 1.0f, 1.0f });
    object->SetEnvironmentTexture(environmentTextureHandle_);
    object->SetEnvironmentCoefficient(environmentCoefficient_);

    const size_t index = objects_.size();
    objects_.push_back(std::move(object));
    objectNames_.push_back(name);
    objectVisible_.push_back(1);
    objectModelPaths_.push_back("");
    objectExportFileNames_.push_back("");
    objectTexturePaths_.push_back("");
    objectColliders_.push_back({});
    objectParentIndices_.push_back(kNoParent);
    objectColliding_.push_back(0);
    return index;
}

bool SceneObjectController::SetObjectParent(size_t childIndex, size_t parentIndex)
{
    if (childIndex >= objects_.size() || childIndex < 3) {
        return false;
    }

    if (parentIndex != kNoParent) {
        if (parentIndex >= objects_.size() || childIndex == parentIndex ||
            IsObjectDescendantOf(parentIndex, childIndex)) {
            return false;
        }
    }

    objectParentIndices_[childIndex] = parentIndex;
    objects_[childIndex]->SetParent(
        parentIndex == kNoParent ? nullptr : objects_[parentIndex].get()
    );
    return true;
}

size_t SceneObjectController::GetObjectParent(size_t index) const
{
    return index < objectParentIndices_.size()
        ? objectParentIndices_[index]
        : kNoParent;
}

bool SceneObjectController::IsObjectDescendantOf(
    size_t index,
    size_t potentialAncestor
) const
{
    size_t current = GetObjectParent(index);
    size_t guard = 0;
    while (current != kNoParent && guard++ < objects_.size()) {
        if (current == potentialAncestor) {
            return true;
        }
        current = GetObjectParent(current);
    }
    return false;
}

void SceneObjectController::SetObjectName(size_t index, const std::string& name)
{
    if (index < objectNames_.size()) {
        objectNames_[index] = name;
    }
}

void SceneObjectController::SetObjectModelPath(
    size_t index,
    const std::string& path
)
{
    if (index >= objectModelPaths_.size()) {
        return;
    }

    objectModelPaths_[index] = path;
}

const std::string& SceneObjectController::GetObjectModelPath(size_t index) const
{
    static const std::string kEmpty = "";

    if (index >= objectModelPaths_.size()) {
        return kEmpty;
    }

    return objectModelPaths_[index];
}

void SceneObjectController::SetObjectExportFileName(
    size_t index,
    const std::string& fileName
)
{
    if (index < objectExportFileNames_.size()) {
        objectExportFileNames_[index] = fileName;
    }
}

const std::string& SceneObjectController::GetObjectExportFileName(size_t index) const
{
    static const std::string kEmpty = "";
    return index < objectExportFileNames_.size()
        ? objectExportFileNames_[index]
        : kEmpty;
}

bool SceneObjectController::CreateIcoSphere()
{
    Model* model = ModelManager::Load("Resources/Editor", "ico_sphere.obj");
    if (!model || model->GetVertexCount() == 0) {
        return false;
    }

    size_t suffix = 1;
    std::string name = "IcoSphere";
    while (std::find(objectNames_.begin(), objectNames_.end(), name) != objectNames_.end()) {
        name = "IcoSphere." + std::to_string(suffix++);
    }

    return AddEditorObject(
        model,
        name,
        "Resources/Editor/ico_sphere.obj"
    ) != static_cast<size_t>(-1);
}

bool SceneObjectController::StretchObjectVertexX(
    size_t index,
    size_t vertexIndex,
    float amount
)
{
    Object3d* object = GetObject(index);
    return object && object->GetModel()
        ? object->GetModel()->StretchVertexX(vertexIndex, amount)
        : false;
}

void SceneObjectController::AddBoxCollider(size_t index)
{
    if (index < objectColliders_.size()) {
        objectColliders_[index].enabled = true;
    }
}

void SceneObjectController::RemoveBoxCollider(size_t index)
{
    if (index < objectColliders_.size()) {
        objectColliders_[index] = {};
    }
}

bool SceneObjectController::HasBoxCollider(size_t index) const
{
    return index < objectColliders_.size() && objectColliders_[index].enabled;
}

SceneObjectController::BoxCollider* SceneObjectController::GetBoxCollider(size_t index)
{
    return HasBoxCollider(index) ? &objectColliders_[index] : nullptr;
}

const SceneObjectController::BoxCollider* SceneObjectController::GetBoxCollider(size_t index) const
{
    return HasBoxCollider(index) ? &objectColliders_[index] : nullptr;
}

bool SceneObjectController::IsObjectColliding(size_t index) const
{
    return index < objectColliding_.size() && objectColliding_[index] != 0;
}

void SceneObjectController::UpdateBoxCollisions()
{
    objectColliding_.assign(objects_.size(), 0);
    collisionPairs_.clear();

    for (size_t first = 3; first < objects_.size(); ++first) {
        const BoxCollider* firstCollider = GetBoxCollider(first);
        if (!firstCollider || !objects_[first]) {
            continue;
        }

        const SceneControllerObb firstObb =
            SceneControllerMakeObb(*objects_[first], *firstCollider);

        for (size_t second = first + 1; second < objects_.size(); ++second) {
            const BoxCollider* secondCollider = GetBoxCollider(second);
            if (!secondCollider || !objects_[second]) {
                continue;
            }

            const SceneControllerObb secondObb =
                SceneControllerMakeObb(*objects_[second], *secondCollider);

            if (SceneControllerIntersects(firstObb, secondObb)) {
                objectColliding_[first] = 1;
                objectColliding_[second] = 1;
                collisionPairs_.push_back({ first, second });
            }
        }
    }
}

void SceneObjectController::SetObjectTexturePath(
    size_t index,
    const std::string& path
)
{
    if (index >= objectTexturePaths_.size()) {
        return;
    }

    objectTexturePaths_[index] = path;
}

const std::string& SceneObjectController::GetObjectTexturePath(size_t index) const
{
    static const std::string kEmpty = "";

    if (index >= objectTexturePaths_.size()) {
        return kEmpty;
    }

    return objectTexturePaths_[index];
}

bool SceneObjectController::IsObjectVisible(size_t index) const
{
    if (index == 0) {
        return showTerrain_;
    }

    if (index == 1 || index == 2) {
        return showAxis_;
    }

    if (index >= objectVisible_.size()) {
        return false;
    }

    return objectVisible_[index] != 0;
}

void SceneObjectController::SetObjectVisible(size_t index, bool visible)
{
    if (index == 0) {
        showTerrain_ = visible;
        return;
    }

    if (index == 1 || index == 2) {
        showAxis_ = visible;
        return;
    }

    if (index >= objectVisible_.size()) {
        return;
    }

    objectVisible_[index] = visible ? 1 : 0;
}

size_t SceneObjectController::GetEditorObjectIndex(EditorObjectType type)
{
    switch (type) {
    case EditorObjectType::Terrain:
        return 0;

    case EditorObjectType::AxisPositive:
        return 1;

    case EditorObjectType::AxisNegative:
        return 2;

    default:
        return 0;
    }
}

