// ============================================================================
// ファイルの役割: シーン配置オブジェクトの生成、更新、JSON保存・読込を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
bool SceneObjectController::SaveLevelSceneToJson(
    const std::string& filePath
) const
{
    std::function<nlohmann::json(size_t, int)> serializeObject;
    serializeObject = [&](size_t index, int level) -> nlohmann::json {
        const Object3d* object = GetObject(index);
        if (!object) {
            return nlohmann::json();
        }

        const Transform& transform = object->GetTransform();
        const bool hasModel = !GetObjectModelPath(index).empty();
        nlohmann::json objectJson;
        objectJson["name"] = GetObjectName(index);
        objectJson["type"] = hasModel ? "MESH" : "EMPTY";
        objectJson["level"] = level;
        objectJson["model"] = GetObjectModelPath(index);
        objectJson["file_name"] = GetObjectExportFileName(index);
        objectJson["texture"] = GetObjectTexturePath(index);
        objectJson["visible"] = IsObjectVisible(index);
        objectJson["transform"] = {
            { "translation", {
                transform.translate.x,
                transform.translate.y,
                transform.translate.z
            } },
            { "rotation", {
                transform.rotate.x * 57.29577951308232f,
                transform.rotate.y * 57.29577951308232f,
                transform.rotate.z * 57.29577951308232f
            } },
            { "scaling", {
                transform.scale.x,
                transform.scale.y,
                transform.scale.z
            } }
        };

        if (const BoxCollider* collider = GetBoxCollider(index)) {
            objectJson["collider"] = {
                { "type", "BOX" },
                { "center", {
                    collider->center.x,
                    collider->center.y,
                    collider->center.z
                } },
                { "size", {
                    collider->size.x,
                    collider->size.y,
                    collider->size.z
                } }
            };
        }

        for (size_t child = 3; child < objects_.size(); ++child) {
            if (GetObjectParent(child) == index) {
                if (!objectJson.contains("children")) {
                    objectJson["children"] = nlohmann::json::array();
                }
                objectJson["children"].push_back(
                    serializeObject(child, level + 1)
                );
            }
        }
        return objectJson;
    };

    nlohmann::json sceneJson;
    sceneJson["version"] = 4;
    sceneJson["name"] = "RuntimeEditedScene";
    sceneJson["coordinate_system"] = "ENGINE_Y_UP_LH";
    sceneJson["objects"] = nlohmann::json::array();

    for (size_t index = 3; index < objects_.size(); ++index) {
        const size_t parent = GetObjectParent(index);
        if (parent == kNoParent || parent < 3) {
            sceneJson["objects"].push_back(serializeObject(index, 0));
        }
    }

    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    file << sceneJson.dump(4) << '\n';
    return true;
}
