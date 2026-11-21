#pragma once

#include <cstddef>
#include <functional>
#include <memory>

#include "MyMath.h"

struct EnemyUpdateContext {
    float deltaTime = 0.0f;
    float runtimeTime = 0.0f;
    float tileWorldSize = 1.0f;
    size_t placementIndex = 0;
    Vector3 spawnPosition{};
    Vector3 currentPosition{};
    Vector3 halfExtents{};
    Vector3 playerPosition{};
    std::function<bool(const Vector3&, float, float)> isBlocked;
    std::function<bool(int, int)> isSolidTile;
};

struct EnemyUpdateResult {
    Vector3 position{};
    bool alertStarted = false;
    bool hasColorOverride = false;
    Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
};

// 敵種別ごとの振る舞いを同じインターフェースから利用するための基底クラス。
class EnemyBehavior {
public:
    virtual ~EnemyBehavior() = default;
    virtual EnemyUpdateResult Update(const EnemyUpdateContext& context) = 0;
    virtual bool UsesFacingRotation() const { return false; }
    virtual float GetFacingAngle() const { return 0.0f; }
};

// itemIdに対応する具象クラスを生成する。敵以外はnullptrを返す。
std::unique_ptr<EnemyBehavior> CreateEnemyBehavior(int itemId, float initialDirection);
