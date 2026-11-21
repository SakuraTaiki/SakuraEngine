#pragma once

#include <memory>

#include "MyMath.h"

enum class FallingFloorPhase {
    Waiting,
    Warning,
    Falling,
    Hidden
};

struct FallingFloorUpdateContext {
    float deltaTime = 0.0f;
    float runtimeTime = 0.0f;
    bool playerStanding = false;
    Vector3 spawnPosition{};
    Vector3 currentPosition{};
};

struct FallingFloorUpdateResult {
    Vector3 position{};
    FallingFloorPhase nextPhase = FallingFloorPhase::Waiting;
    bool transitionRequested = false;
    bool warningStarted = false;
    bool collisionEnabled = true;
    const char* visualFile = "falling_ruin_idle.obj";
};

// 各状態が共通の更新関数を実装し、自分自身で次の状態を決定する。
class FallingFloorState {
public:
    virtual ~FallingFloorState() = default;
    virtual FallingFloorUpdateResult Update(
        const FallingFloorUpdateContext& context) = 0;
    virtual FallingFloorPhase GetPhase() const = 0;
};

// 状態オブジェクトの所有と切り替えだけを担当するコンテキスト。
class FallingFloorBehavior final {
public:
    FallingFloorBehavior();

    FallingFloorUpdateResult Update(const FallingFloorUpdateContext& context);
    bool IsCollisionEnabled() const;
    FallingFloorPhase GetPhase() const;

private:
    void ChangeState(FallingFloorPhase phase);

    std::unique_ptr<FallingFloorState> state_;
};
