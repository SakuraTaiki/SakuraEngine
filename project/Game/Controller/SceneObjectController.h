// ============================================================================
// ファイルの役割: シーン配置オブジェクトの生成、更新、JSON保存・読込を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <string>
#include <utility>

#include "Object3d.h"

class Object3dCommon;
class Model;

class SceneObjectController {
public:
    static constexpr size_t kNoParent = static_cast<size_t>(-1);

    struct BoxCollider {
        bool enabled = false;
        Vector3 center = { 0.0f, 0.0f, 0.0f };
        Vector3 size = { 2.0f, 2.0f, 2.0f };
    };

    enum class EditorObjectType {
        Terrain,
        AxisPositive,
        AxisNegative
    };


    void Initialize(
        Object3dCommon* object3dCommon,
        uint32_t environmentTextureHandle,
        float environmentCoefficient
    );

    void Finalize();

    void Update();
    void Draw();

    void SetEnvironmentCoefficient(float environmentCoefficient);

    Object3d* GetEditorObject(EditorObjectType type);
    const Object3d* GetEditorObject(EditorObjectType type) const;

    size_t AddEditorObject(Model* model, const std::string& name);
    size_t AddEditorEmpty(const std::string& name);
    size_t AddEditorObject(
        Model* model,
        const std::string& name,
        const std::string& modelPath
    );

    void ClearEditorObjects();

    size_t GetObjectCount() const;
    Object3d* GetObject(size_t index);
    const Object3d* GetObject(size_t index) const;
    const std::string& GetObjectName(size_t index) const;
    void SetObjectName(size_t index, const std::string& name);
    bool IsObjectVisible(size_t index) const;
    void SetObjectVisible(size_t index, bool visible);

    bool SetObjectParent(size_t childIndex, size_t parentIndex);
    size_t GetObjectParent(size_t index) const;
    bool IsObjectDescendantOf(size_t index, size_t potentialAncestor) const;

    void SetObjectModelPath(size_t index, const std::string& path);
    const std::string& GetObjectModelPath(size_t index) const;

    void SetObjectExportFileName(size_t index, const std::string& fileName);
    const std::string& GetObjectExportFileName(size_t index) const;

    void SetObjectTexturePath(size_t index, const std::string& path);
    const std::string& GetObjectTexturePath(size_t index) const;

    bool CreateIcoSphere();
    bool StretchObjectVertexX(size_t index, size_t vertexIndex, float amount);
    void AddBoxCollider(size_t index);
    void RemoveBoxCollider(size_t index);
    bool HasBoxCollider(size_t index) const;
    BoxCollider* GetBoxCollider(size_t index);
    const BoxCollider* GetBoxCollider(size_t index) const;
    bool IsObjectColliding(size_t index) const;
    const std::vector<std::pair<size_t, size_t>>& GetCollisionPairs() const {
        return collisionPairs_;
    }

    bool SaveLevelSceneToJson(const std::string& filePath) const;
    bool LoadLevelSceneFromJson(const std::string& filePath);

    static size_t GetEditorObjectIndex(EditorObjectType type);

    bool& ShowTerrain() { return showTerrain_; }
    bool& ShowAxis() { return showAxis_; }

private:
    void UpdateBoxCollisions();

    Object3dCommon* object3dCommon_ = nullptr;
    uint32_t environmentTextureHandle_ = 0;
    float environmentCoefficient_ = 0.05f;

    std::vector<std::unique_ptr<Object3d>> objects_;
    std::vector<std::string> objectNames_;
    std::vector<uint8_t> objectVisible_;
    std::vector<std::string> objectModelPaths_;
    std::vector<std::string> objectExportFileNames_;
    std::vector<std::string> objectTexturePaths_;
    std::vector<BoxCollider> objectColliders_;
    std::vector<size_t> objectParentIndices_;
    std::vector<uint8_t> objectColliding_;
    std::vector<std::pair<size_t, size_t>> collisionPairs_;

    bool showTerrain_ = true;
    bool showAxis_ = true;
};
