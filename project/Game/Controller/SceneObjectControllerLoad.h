// ============================================================================
// ファイルの役割: シーン配置オブジェクトの生成、更新、JSON保存・読込を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
bool SceneObjectController::LoadLevelSceneFromJson(
    const std::string& filePath
)
{
    std::ifstream file(filePath);

    if (!file.is_open()) {
        return false;
    }

    nlohmann::json sceneJson;

    try {
        file >> sceneJson;
    }
    catch (...) {
        return false;
    }

    if (!sceneJson.contains("objects") ||
        !sceneJson["objects"].is_array()) {
        return false;
    }

    const std::string coordinateSystem = sceneJson.value(
        "coordinate_system",
        "BLENDER_Z_UP_RH"
    );
    const bool convertFromBlender =
        coordinateSystem != "ENGINE_Y_UP_LH";

    const std::filesystem::path sceneDirectory =
        std::filesystem::path(filePath).parent_path();

    bool hadLoadError = false;
    // Keep runtime object instances alive across reloads. Reusing objects by
    // their Blender name avoids a visible blank frame and preserves references
    // held by future gameplay components.
    std::vector<uint8_t> touchedObjects(objects_.size(), 0);
    for (size_t index = 0; index < 3 && index < touchedObjects.size(); ++index) {
        touchedObjects[index] = 1;
    }

    auto findReusableObject = [&](const std::string& objectName) {
        if (objectName.empty()) {
            return kNoParent;
        }
        for (size_t index = 3; index < objectNames_.size(); ++index) {
            if (!touchedObjects[index] && objectNames_[index] == objectName) {
                return index;
            }
        }
        return kNoParent;
    };

    std::function<void(const nlohmann::json&, size_t)> loadObject;
    loadObject = [&](const nlohmann::json& objectJson, size_t parentIndex) {
        const std::string name =
            objectJson.value("name", "");

        std::string modelPath = objectJson.value("model", "");
        const std::string exportFileName = objectJson.value("file_name", "");

        if (modelPath.empty() && !exportFileName.empty()) {
            const std::filesystem::path exportedPath(exportFileName);
            const std::filesystem::path sceneRelativePath =
                exportedPath.is_absolute() || sceneDirectory.empty()
                ? exportedPath
                : sceneDirectory / exportedPath;
            const std::string defaultPath =
                exportFileName.find('/') == std::string::npos &&
                exportFileName.find('\\') == std::string::npos
                ? "Resources/" + exportFileName
                : exportFileName;

            if (std::filesystem::exists(sceneRelativePath)) {
                modelPath = sceneRelativePath.generic_string();
            } else if (std::filesystem::exists(defaultPath)) {
                modelPath = defaultPath;
            } else {
                // Blender形式はfile_nameしか持たないため、Resourcesの
                // サブディレクトリも検索する。
                const std::filesystem::path targetName =
                    std::filesystem::path(exportFileName).filename();
                std::error_code error;

                for (std::filesystem::recursive_directory_iterator iterator(
                    "Resources",
                    std::filesystem::directory_options::skip_permission_denied,
                    error
                ), end; iterator != end && !error; iterator.increment(error)) {
                    if (iterator->is_regular_file(error) &&
                        iterator->path().filename() == targetName) {
                        modelPath = iterator->path().generic_string();
                        break;
                    }
                }
            }
        }

        // The class Blender add-on falls back to "<Blender object name>.obj"
        // when file_name was not explicitly assigned. Imported OBJ objects can
        // have a different Blender name than their source filename
        // (for example Cube_Cube.001 -> axis.obj). In that case, identify the
        // source OBJ by its internal `o` or `g` declaration.
        if (modelPath.empty() && !name.empty()) {
            modelPath = FindObjPathByObjectName("Resources", name);
        }

        const std::string texturePath =
            objectJson.value("texture", "");

        const std::string objectType = objectJson.value(
            "type",
            modelPath.empty() ? "EMPTY" : "MESH"
        );
        size_t index = findReusableObject(name);
        const bool reusedObject = index != kNoParent;
        std::string modelName;
        Model* loadedModel = nullptr;

        if (modelPath.empty()) {
            if (objectType == "MESH") {
                hadLoadError = true;
            }
            if (index == kNoParent) {
                index = AddEditorEmpty(name.empty() ? "Empty" : name);
            }
        } else {
            if (!std::filesystem::exists(modelPath)) {
                hadLoadError = true;
                if (index == kNoParent) {
                    index = AddEditorEmpty(name.empty() ? exportFileName : name);
                }
            } else {
                std::string directoryPath;
                if (!SplitModelPath(modelPath, directoryPath, modelName)) {
                    hadLoadError = true;
                    if (index == kNoParent) {
                        index = AddEditorEmpty(name.empty() ? exportFileName : name);
                    }
                } else {
                    loadedModel = ModelManager::Load(directoryPath, modelName);
                    if (!loadedModel) {
                        hadLoadError = true;
                        if (index == kNoParent) {
                            index = AddEditorEmpty(name.empty() ? modelName : name);
                        }
                    } else {
                        if (index == kNoParent) {
                            index = AddEditorObject(
                                loadedModel,
                                name.empty() ? modelName : name,
                                modelPath
                            );
                        } else if (GetObjectModelPath(index) != modelPath) {
                            // The model instance is already valid when only
                            // placement/collider data changed. Avoid resetting
                            // it during a live edit so rendering stays continuous.
                            objects_[index]->SetModel(loadedModel);
                            SetObjectModelPath(index, modelPath);
                        }
                    }
                }
            }
        }

        if (reusedObject && !loadedModel) {
            objects_[index]->SetModel(nullptr);
            SetObjectModelPath(index, modelPath);
        }

        if (index >= touchedObjects.size()) {
            touchedObjects.resize(objects_.size(), 0);
        }
        if (index != kNoParent && index < touchedObjects.size()) {
            touchedObjects[index] = 1;
            SetObjectName(index, name.empty() ? modelName : name);
        }

        Object3d* object =
            GetObject(index);

        if (!object) {
            hadLoadError = true;
            return;
        }

        if (!SetObjectParent(index, parentIndex)) {
            hadLoadError = true;
            return;
        }

        SetObjectExportFileName(
            index,
            exportFileName.empty() ? modelName : exportFileName
        );

        Transform& transform =
            object->GetTransform();

        if (objectJson.contains("position") &&
            objectJson["position"].is_array() &&
            objectJson["position"].size() >= 3) {
            const float x = objectJson["position"][0].get<float>();
            const float y = objectJson["position"][1].get<float>();
            const float z = objectJson["position"][2].get<float>();
            transform.translate = convertFromBlender
                ? Vector3{ x, z, y }
                : Vector3{ x, y, z };
        }

        if (objectJson.contains("rotation") &&
            objectJson["rotation"].is_array() &&
            objectJson["rotation"].size() >= 3) {
            const float x = objectJson["rotation"][0].get<float>();
            const float y = objectJson["rotation"][1].get<float>();
            const float z = objectJson["rotation"][2].get<float>();
            transform.rotate = convertFromBlender
                ? Vector3{ -x, -z, -y }
                : Vector3{ x, y, z };
        }

        if (objectJson.contains("scale") &&
            objectJson["scale"].is_array() &&
            objectJson["scale"].size() >= 3) {
            const float x = objectJson["scale"][0].get<float>();
            const float y = objectJson["scale"][1].get<float>();
            const float z = objectJson["scale"][2].get<float>();
            // Scale is expressed on the model's local axes. OBJ vertices keep
            // those axes in this engine (apart from the existing X mirror),
            // so Y/Z must not be swapped here.
            transform.scale = Vector3{ x, y, z };
        }

        if (objectJson.contains("transform") && objectJson["transform"].is_object()) {
            const auto& transformJson = objectJson["transform"];

            if (transformJson.contains("translation") &&
                transformJson["translation"].is_array() &&
                transformJson["translation"].size() >= 3) {
                const float x = transformJson["translation"][0].get<float>();
                const float y = transformJson["translation"][1].get<float>();
                const float z = transformJson["translation"][2].get<float>();
                transform.translate = convertFromBlender
                    ? Vector3{ x, z, y }
                    : Vector3{ x, y, z };
            }

            if (transformJson.contains("rotation") &&
                transformJson["rotation"].is_array() &&
                transformJson["rotation"].size() >= 3) {
                const float x = transformJson["rotation"][0].get<float>();
                const float y = transformJson["rotation"][1].get<float>();
                const float z = transformJson["rotation"][2].get<float>();
                transform.rotate = convertFromBlender
                    ? ConvertBlenderEulerDegreesToEngine(x, y, z)
                    : Vector3{
                        x * 0.017453292519943295f,
                        y * 0.017453292519943295f,
                        z * 0.017453292519943295f
                    };
            }

            if (transformJson.contains("scaling") &&
                transformJson["scaling"].is_array() &&
                transformJson["scaling"].size() >= 3) {
                const float x = transformJson["scaling"][0].get<float>();
                const float y = transformJson["scaling"][1].get<float>();
                const float z = transformJson["scaling"][2].get<float>();
                transform.scale = Vector3{ x, y, z };
            }
        }

        SetObjectVisible(
            index,
            objectJson.value("visible", true)
        );

        if (!texturePath.empty() &&
            object3dCommon_ &&
            object3dCommon_->GetTextureManager()) {

            const uint32_t textureHandle =
                object3dCommon_->GetTextureManager()->LoadTexture(texturePath);

            object->SetOverrideTexture(textureHandle);
            SetObjectTexturePath(index, texturePath);
        }

        RemoveBoxCollider(index);
        if (objectJson.contains("collider") && objectJson["collider"].is_object()) {
            const auto& colliderJson = objectJson["collider"];
            if (colliderJson.value("type", "") == "BOX") {
                AddBoxCollider(index);
                BoxCollider* collider = GetBoxCollider(index);
                if (colliderJson.contains("center") && colliderJson["center"].is_array() &&
                    colliderJson["center"].size() >= 3) {
                    const float x = colliderJson["center"][0].get<float>();
                    const float y = colliderJson["center"][1].get<float>();
                    const float z = colliderJson["center"][2].get<float>();
                    collider->center = convertFromBlender
                        ? Vector3{ -x, y, z }
                        : Vector3{ x, y, z };
                }
                if (colliderJson.contains("size") && colliderJson["size"].is_array() &&
                    colliderJson["size"].size() >= 3) {
                    const float x = colliderJson["size"][0].get<float>();
                    const float y = colliderJson["size"][1].get<float>();
                    const float z = colliderJson["size"][2].get<float>();
                    collider->size = Vector3{ x, y, z };
                }
            }
        }

        if (objectJson.contains("children") && objectJson["children"].is_array()) {
            for (const auto& child : objectJson["children"]) {
                loadObject(child, index);
            }
        }
    };

    try {
        for (const auto& objectJson : sceneJson["objects"]) {
            loadObject(objectJson, kNoParent);
        }
    }
    catch (...) {
        return false;
    }

    // Objects removed from the source file are retired without invalidating
    // their addresses. This is safer for components that may keep references.
    for (size_t index = 3; index < objects_.size(); ++index) {
        if (index >= touchedObjects.size() || !touchedObjects[index]) {
            SetObjectVisible(index, false);
            RemoveBoxCollider(index);
            SetObjectParent(index, kNoParent);
        }
    }

    return !hadLoadError;
}
