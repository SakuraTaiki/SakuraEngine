#include "EnemyBehavior.h"

#include <algorithm>
#include <cmath>

namespace {
constexpr float kPi = 3.14159265358979323846f;

class DirectionalEnemyBehavior : public EnemyBehavior {
public:
    explicit DirectionalEnemyBehavior(float initialDirection)
        : direction_(initialDirection >= 0.0f ? 1.0f : -1.0f),
          facingAngle_(direction_ > 0.0f ? 0.0f : kPi) {}

    bool UsesFacingRotation() const override { return true; }
    float GetFacingAngle() const override { return facingAngle_; }

protected:
    void SetDirection(float direction) {
        if (std::abs(direction) > 0.0001f) direction_ = direction > 0.0f ? 1.0f : -1.0f;
    }

    float GetDirection() const { return direction_; }

    void UpdateFacing(float deltaTime, float turnDuration) {
        const float targetAngle = direction_ > 0.0f ? 0.0f : kPi;
        float angleDelta = targetAngle - facingAngle_;
        while (angleDelta > kPi) angleDelta -= kPi * 2.0f;
        while (angleDelta < -kPi) angleDelta += kPi * 2.0f;
        const float blend = (std::min)(1.0f, deltaTime / turnDuration);
        facingAngle_ += angleDelta * blend;
    }

private:
    float direction_ = 1.0f;
    float facingAngle_ = 0.0f;
};

class WalkerBehavior final : public DirectionalEnemyBehavior {
public:
    using DirectionalEnemyBehavior::DirectionalEnemyBehavior;

    EnemyUpdateResult Update(const EnemyUpdateContext& context) override {
        EnemyUpdateResult result{};
        result.position = context.currentPosition;
        constexpr float speed = 1.6f;
        const float direction = GetDirection();
        Vector3 candidate = result.position;
        candidate.x += direction * speed * context.deltaTime;
        const int aheadX = static_cast<int>(std::floor(
            (candidate.x + direction * (context.halfExtents.x + 0.04f)) /
            context.tileWorldSize));
        const int groundY = static_cast<int>(std::floor(
            (candidate.y - (std::max)(context.halfExtents.y,
                context.tileWorldSize * 0.5f) - 0.06f) / context.tileWorldSize));
        const bool wallAhead = context.isBlocked(candidate,
            context.halfExtents.x, context.halfExtents.y);
        const bool ledgeAhead = !context.isSolidTile(aheadX, groundY);
        if (wallAhead || ledgeAhead) {
            SetDirection(-direction);
        } else {
            result.position = candidate;
        }
        UpdateFacing(context.deltaTime, 0.20f);
        return result;
    }
};

class FlyerBehavior final : public DirectionalEnemyBehavior {
public:
    using DirectionalEnemyBehavior::DirectionalEnemyBehavior;

    EnemyUpdateResult Update(const EnemyUpdateContext& context) override {
        EnemyUpdateResult result{};
        result.position = context.currentPosition;
        const float phase = static_cast<float>(context.placementIndex) * 0.73f;
        Vector3 candidate = context.spawnPosition;
        candidate.x += std::sin(context.runtimeTime * 1.25f + phase) * 2.0f;
        candidate.y += std::sin(context.runtimeTime * 2.1f + phase) * 0.65f;
        if (!context.isBlocked(candidate, context.halfExtents.x, context.halfExtents.y)) {
            const float horizontalVelocity =
                std::cos(context.runtimeTime * 1.25f + phase) * 2.5f;
            SetDirection(horizontalVelocity);
            result.position = candidate;
        }
        UpdateFacing(context.deltaTime, 0.24f);
        return result;
    }
};

class ChaserBehavior final : public EnemyBehavior {
public:
    EnemyUpdateResult Update(const EnemyUpdateContext& context) override {
        EnemyUpdateResult result{};
        result.position = context.currentPosition;
        constexpr float detectDistance = 7.0f;
        constexpr float loseDistance = 9.0f;
        const float playerDx = context.playerPosition.x - result.position.x;
        const float playerDy = context.playerPosition.y - result.position.y;
        const float playerDistance = std::sqrt(playerDx * playerDx + playerDy * playerDy);
        if (!alerted_ && playerDistance <= detectDistance) {
            alerted_ = true;
            result.alertStarted = true;
        } else if (alerted_ && playerDistance >= loseDistance) {
            alerted_ = false;
        }

        const Vector3 target = alerted_ ? context.playerPosition : context.spawnPosition;
        const float dx = target.x - result.position.x;
        const float dy = target.y - result.position.y;
        const float distance = std::sqrt(dx * dx + dy * dy);
        if (distance > 0.02f) {
            const float moveDistance = (alerted_ ? 2.6f : 1.4f) * context.deltaTime;
            const float step = (std::min)(moveDistance, distance) / distance;
            Vector3 candidate = result.position;
            candidate.x += dx * step;
            if (!context.isBlocked(candidate, context.halfExtents.x, context.halfExtents.y)) {
                result.position.x = candidate.x;
            }
            candidate = result.position;
            candidate.y += dy * step;
            if (!context.isBlocked(candidate, context.halfExtents.x, context.halfExtents.y)) {
                result.position.y = candidate.y;
            }
        }
        result.hasColorOverride = true;
        result.color = alerted_ ? Vector4{1.0f, 0.9f, 0.1f, 1.0f}
                                : Vector4{0.95f, 0.12f, 0.12f, 1.0f};
        return result;
    }

private:
    bool alerted_ = false;
};
}

std::unique_ptr<EnemyBehavior> CreateEnemyBehavior(int itemId, float initialDirection) {
    switch (itemId) {
    case 22:
        return std::make_unique<WalkerBehavior>(initialDirection);
    case 23:
        return std::make_unique<FlyerBehavior>(initialDirection);
    case 24:
        return std::make_unique<ChaserBehavior>();
    default:
        return nullptr;
    }
}
