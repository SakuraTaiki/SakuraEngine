#include "FallingFloorState.h"

#include <cmath>

namespace {
class WaitingState final : public FallingFloorState {
public:
    FallingFloorUpdateResult Update(
        const FallingFloorUpdateContext& context) override {
        FallingFloorUpdateResult result{};
        result.position = context.spawnPosition;
        result.nextPhase = FallingFloorPhase::Waiting;
        if (context.playerStanding) {
            result.transitionRequested = true;
            result.nextPhase = FallingFloorPhase::Warning;
            result.warningStarted = true;
        }
        return result;
    }

    FallingFloorPhase GetPhase() const override {
        return FallingFloorPhase::Waiting;
    }
};

class WarningState final : public FallingFloorState {
public:
    FallingFloorUpdateResult Update(
        const FallingFloorUpdateContext& context) override {
        timer_ -= context.deltaTime;
        FallingFloorUpdateResult result{};
        result.position = context.spawnPosition;
        result.position.x += std::sin(context.runtimeTime * 55.0f) * 0.035f;
        result.nextPhase = FallingFloorPhase::Warning;
        result.visualFile = "falling_ruin_warning.obj";
        if (timer_ <= 0.0f) {
            result.transitionRequested = true;
            result.nextPhase = FallingFloorPhase::Falling;
        }
        return result;
    }

    FallingFloorPhase GetPhase() const override {
        return FallingFloorPhase::Warning;
    }

private:
    float timer_ = 0.7f;
};

class FallingState final : public FallingFloorState {
public:
    FallingFloorUpdateResult Update(
        const FallingFloorUpdateContext& context) override {
        velocity_ -= 18.0f * context.deltaTime;
        FallingFloorUpdateResult result{};
        result.position = context.currentPosition;
        result.position.y += velocity_ * context.deltaTime;
        result.nextPhase = FallingFloorPhase::Falling;
        result.visualFile = "falling_ruin_broken.obj";
        if (result.position.y < -3.0f) {
            result.position = context.spawnPosition;
            result.collisionEnabled = false;
            result.transitionRequested = true;
            result.nextPhase = FallingFloorPhase::Hidden;
        }
        return result;
    }

    FallingFloorPhase GetPhase() const override {
        return FallingFloorPhase::Falling;
    }

private:
    float velocity_ = 0.0f;
};

class HiddenState final : public FallingFloorState {
public:
    FallingFloorUpdateResult Update(
        const FallingFloorUpdateContext& context) override {
        timer_ -= context.deltaTime;
        FallingFloorUpdateResult result{};
        result.position = context.spawnPosition;
        result.nextPhase = FallingFloorPhase::Hidden;
        result.collisionEnabled = false;
        if (timer_ <= 0.0f) {
            result.transitionRequested = true;
            result.nextPhase = FallingFloorPhase::Waiting;
            result.collisionEnabled = true;
        }
        return result;
    }

    FallingFloorPhase GetPhase() const override {
        return FallingFloorPhase::Hidden;
    }

private:
    float timer_ = 1.6f;
};

std::unique_ptr<FallingFloorState> CreateState(FallingFloorPhase phase) {
    switch (phase) {
    case FallingFloorPhase::Waiting:
        return std::make_unique<WaitingState>();
    case FallingFloorPhase::Warning:
        return std::make_unique<WarningState>();
    case FallingFloorPhase::Falling:
        return std::make_unique<FallingState>();
    case FallingFloorPhase::Hidden:
        return std::make_unique<HiddenState>();
    }
    return std::make_unique<WaitingState>();
}
}

FallingFloorBehavior::FallingFloorBehavior()
    : state_(CreateState(FallingFloorPhase::Waiting)) {}

FallingFloorUpdateResult FallingFloorBehavior::Update(
    const FallingFloorUpdateContext& context) {
    FallingFloorUpdateResult result = state_->Update(context);
    if (result.transitionRequested) ChangeState(result.nextPhase);
    return result;
}

bool FallingFloorBehavior::IsCollisionEnabled() const {
    return state_ && state_->GetPhase() != FallingFloorPhase::Hidden;
}

FallingFloorPhase FallingFloorBehavior::GetPhase() const {
    return state_ ? state_->GetPhase() : FallingFloorPhase::Waiting;
}

void FallingFloorBehavior::ChangeState(FallingFloorPhase phase) {
    state_ = CreateState(phase);
}
