#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "MyMath.h"
#include "Object3d.h"

class Camera;
class Object3dCommon;
struct ImDrawList;

// クリア画面に必要な状態と操作をまとめ、StageEditorから内部実装を隠す。
class ClearPresentationController final {
public:
    void Initialize(Object3dCommon* common, uint32_t environmentTexture);
    void Finalize();

    void Start();
    void Reset();
    void Update(float deltaTime, const Camera* camera, bool enterTriggered);
    void Draw3D() const;
    void DrawOverlay(ImDrawList* draw, float x, float y, float width, float height) const;

    bool IsActive() const { return active_; }
    bool IsExitComplete() const;

private:
    struct AmbientBlock {
        std::unique_ptr<Object3d> object;
        Vector3 baseOffset{};
        Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
        float phase = 0.0f;
        float speed = 1.0f;
        float scale = 1.0f;
    };

    std::unique_ptr<Object3d> CreateObject(const char* directory, const char* fileName) const;

    Object3dCommon* common_ = nullptr;
    uint32_t environmentTexture_ = 0;
    std::unique_ptr<Object3d> promptObject_;
    std::vector<std::unique_ptr<Object3d>> letterObjects_;
    std::vector<AmbientBlock> ambientBlocks_;
    bool active_ = false;
    bool exitActive_ = false;
    float presentationTimer_ = 0.0f;
    float ambientTimer_ = 0.0f;
    float exitTimer_ = 0.0f;

    static constexpr float kInputDelay = 1.65f;
    static constexpr float kExitDuration = 0.85f;
};
