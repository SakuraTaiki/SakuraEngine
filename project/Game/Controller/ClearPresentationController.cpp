#include "ClearPresentationController.h"

#include "Camera.h"
#include "ModelManager.h"
#include "Object3dCommon.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iterator>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

namespace {
constexpr float kPi = 3.14159265358979323846f;
}

void ClearPresentationController::Initialize(
    Object3dCommon* common, uint32_t environmentTexture) {
    common_ = common;
    environmentTexture_ = environmentTexture;

    promptObject_ = CreateObject("Resources/Clear", "press_enter_stage_select.obj");
    letterObjects_.reserve(9);
    for (int i = 0; i < 9; ++i) {
        char fileName[64]{};
        std::snprintf(fileName, sizeof(fileName), "goal_clear_letter_%02d.obj", i);
        letterObjects_.push_back(CreateObject("Resources/Clear", fileName));
    }

    Model* blockModel = ModelManager::Load("Resources/Title", "title_block_particle.obj");
    constexpr Vector3 offsets[] = {
        {-7.5f, 3.7f, 25.0f}, {-6.2f, -3.0f, 27.0f}, {-4.6f, 2.3f, 28.5f},
        {-3.0f, -4.0f, 25.8f}, {-1.5f, 3.8f, 27.8f}, {-0.3f, -3.4f, 29.0f},
        {1.4f, 3.2f, 26.2f}, {2.9f, -3.8f, 28.2f}, {4.5f, 2.6f, 27.3f},
        {5.8f, -2.7f, 25.5f}, {7.1f, 3.5f, 28.6f}, {8.0f, -0.7f, 26.6f}
    };
    constexpr Vector4 colors[] = {
        {0.12f, 0.88f, 1.00f, 0.58f},
        {1.00f, 0.82f, 0.24f, 0.62f},
        {0.92f, 0.98f, 1.00f, 0.52f}
    };
    ambientBlocks_.reserve(std::size(offsets));
    for (size_t i = 0; i < std::size(offsets); ++i) {
        AmbientBlock block{};
        block.object = std::make_unique<Object3d>();
        block.object->Initialize(common_);
        block.object->SetModel(blockModel);
        block.object->SetEnableLighting(false);
        block.object->SetEnvironmentTexture(environmentTexture_);
        block.object->SetEnvironmentCoefficient(0.0f);
        block.baseOffset = offsets[i];
        block.color = colors[i % std::size(colors)];
        block.phase = static_cast<float>(i) * 0.81f;
        block.speed = 0.42f + static_cast<float>(i % 4) * 0.08f;
        block.scale = 0.20f + static_cast<float>(i % 3) * 0.055f;
        ambientBlocks_.push_back(std::move(block));
    }
    Reset();
}

void ClearPresentationController::Finalize() {
    Reset();
    ambientBlocks_.clear();
    letterObjects_.clear();
    promptObject_.reset();
    common_ = nullptr;
    environmentTexture_ = 0;
}

void ClearPresentationController::Start() {
    active_ = true;
    exitActive_ = false;
    presentationTimer_ = 0.0f;
    ambientTimer_ = 0.0f;
    exitTimer_ = 0.0f;
}

void ClearPresentationController::Reset() {
    active_ = false;
    exitActive_ = false;
    presentationTimer_ = 0.0f;
    ambientTimer_ = 0.0f;
    exitTimer_ = 0.0f;
}

void ClearPresentationController::Update(
    float deltaTime, const Camera* camera, bool enterTriggered) {
    if (!active_ || !camera) return;

    presentationTimer_ += deltaTime;
    ambientTimer_ += deltaTime;
    if (!exitActive_ && presentationTimer_ >= kInputDelay && enterTriggered) {
        exitActive_ = true;
        exitTimer_ = 0.0f;
    }
    if (exitActive_) exitTimer_ += deltaTime;

    const Vector3& cameraPosition = camera->GetTranslate();
    const float exitProgress = exitActive_
        ? std::clamp(exitTimer_ / kExitDuration, 0.0f, 1.0f) : 0.0f;

    for (auto& block : ambientBlocks_) {
        if (!block.object) continue;
        const float time = ambientTimer_ * block.speed + block.phase;
        block.object->SetPosition({
            cameraPosition.x + block.baseOffset.x + std::sin(time * 0.8f) * 0.30f,
            cameraPosition.y + block.baseOffset.y + std::sin(time) * 0.48f,
            cameraPosition.z + block.baseOffset.z - exitProgress * 2.0f});
        block.object->SetRotation({time * 0.34f, time * 0.52f, time * 0.27f});
        const float scale = block.scale * (0.88f + 0.12f * std::sin(time * 1.35f)) *
            (1.0f - exitProgress * 0.55f);
        block.object->SetScale({scale, scale, scale});
        Vector4 color = block.color;
        color.w *= 1.0f - exitProgress;
        block.object->SetColor(color);
        block.object->Update();
    }

    constexpr float letterStart = 0.56f;
    constexpr float letterDelay = 0.085f;
    constexpr float letterDuration = 0.62f;
    const float exitScale = 1.0f - exitProgress;
    for (size_t i = 0; i < letterObjects_.size(); ++i) {
        auto& letter = letterObjects_[i];
        if (!letter) continue;
        const size_t mirrored = letterObjects_.size() - 1 - i;
        const float order = static_cast<float>((std::min)(
            (std::min)(i, mirrored), static_cast<size_t>(3)));
        const float localTime = presentationTimer_ - letterStart - order * letterDelay;
        const float progress = std::clamp(localTime / letterDuration, 0.0f, 1.0f);
        const float eased = 1.0f - std::pow(1.0f - progress, 3.0f);
        const float side = i < letterObjects_.size() / 2 ? -1.0f : 1.0f;
        float finalX = (static_cast<float>(i) - 4.0f) * 0.90f;
        finalX += i < 4 ? -0.22f : (i > 3 ? 0.22f : 0.0f);
        letter->SetPosition({
            cameraPosition.x + finalX + side * (1.0f - eased) * 5.2f,
            cameraPosition.y + 0.55f + std::sin(progress * kPi) * 0.18f,
            cameraPosition.z + 21.0f + (1.0f - eased) * 4.0f});
        letter->SetRotation({kPi * 0.5f, side * (1.0f - eased) * 1.25f, 0.0f});
        const float scale = (0.78f + eased * 0.37f) * exitScale;
        letter->SetScale({scale, scale, scale});
        const float flash = progress > 0.82f ? (1.0f - progress) / 0.18f : 0.0f;
        letter->SetColor({0.72f + flash * 0.28f, 0.96f, 1.0f, eased * exitScale});
        letter->Update();
    }

    if (promptObject_ && presentationTimer_ >= kInputDelay) {
        const float localTime = presentationTimer_ - kInputDelay;
        const float appear = std::clamp(localTime / 0.35f, 0.0f, 1.0f);
        const float blink = 0.68f + 0.32f * (0.5f + 0.5f * std::sin(localTime * 4.2f));
        const float scale = 0.74f * exitScale;
        promptObject_->SetPosition({cameraPosition.x, cameraPosition.y - 1.15f,
            cameraPosition.z + 21.0f});
        promptObject_->SetRotation({kPi * 0.5f, 0.0f, 0.0f});
        promptObject_->SetScale({scale, scale, scale});
        promptObject_->SetColor({0.30f, 0.90f, 1.0f, appear * blink});
        promptObject_->Update();
    }
}

void ClearPresentationController::Draw3D() const {
    if (!active_) return;
    for (const auto& block : ambientBlocks_) if (block.object) block.object->Draw();
    for (const auto& letter : letterObjects_) if (letter) letter->Draw();
    if (promptObject_ && presentationTimer_ >= kInputDelay) promptObject_->Draw();
}

bool ClearPresentationController::IsExitComplete() const {
    return active_ && exitActive_ && exitTimer_ >= kExitDuration;
}

std::unique_ptr<Object3d> ClearPresentationController::CreateObject(
    const char* directory, const char* fileName) const {
    auto object = std::make_unique<Object3d>();
    object->Initialize(common_);
    object->SetModel(ModelManager::Load(directory, fileName));
    object->SetEnableLighting(false);
    object->SetEnvironmentTexture(environmentTexture_);
    object->SetEnvironmentCoefficient(0.0f);
    return object;
}

void ClearPresentationController::DrawOverlay(
    ImDrawList* draw, float x, float y, float width, float height) const {
#ifdef USE_IMGUI
    if (!active_ || !draw) return;
    const auto smoothStep = [](float value) {
        const float t = std::clamp(value, 0.0f, 1.0f);
        return t * t * (3.0f - 2.0f * t);
    };
    constexpr int columns = 12;
    constexpr int rows = 7;
    const float blockWidth = width / static_cast<float>(columns);
    const float blockHeight = height / static_cast<float>(rows);
    const float entryOpen = smoothStep((presentationTimer_ - 0.18f) / 0.72f);
    const float exitBuild = exitActive_ ? smoothStep(exitTimer_ / kExitDuration) : 0.0f;
    const float wallAmount = exitActive_ ? exitBuild : 1.0f - entryOpen;
    if (wallAmount > 0.001f) {
        for (int row = 0; row < rows; ++row) {
            for (int column = 0; column < columns; ++column) {
                const float centerX = x + (static_cast<float>(column) + 0.5f) * blockWidth;
                const float centerY = y + (static_cast<float>(row) + 0.5f) * blockHeight;
                const float sideX = centerX < x + width * 0.5f ? -1.0f : 1.0f;
                const float sideY = centerY < y + height * 0.5f ? -1.0f : 1.0f;
                const bool horizontal = std::abs(centerX - (x + width * 0.5f)) / width >
                    std::abs(centerY - (y + height * 0.5f)) / height;
                const float travel = (1.0f - wallAmount) * 92.0f;
                const float cx = centerX + (horizontal ? sideX * travel : 0.0f);
                const float cy = centerY + (!horizontal ? sideY * travel : 0.0f);
                const float halfW = blockWidth * 0.50f * wallAmount;
                const float halfH = blockHeight * 0.50f * wallAmount;
                const bool gold = (row + column * 2) % 7 == 0;
                const ImU32 fill = gold ? IM_COL32(245, 185, 55, 245)
                    : IM_COL32(24, 106 + (row % 3) * 12, 145 + (column % 3) * 13, 245);
                draw->AddRectFilled({cx - halfW + 2.0f, cy - halfH + 2.0f},
                    {cx + halfW - 2.0f, cy + halfH - 2.0f}, fill, 3.0f);
                draw->AddRect({cx - halfW + 3.0f, cy - halfH + 3.0f},
                    {cx + halfW - 3.0f, cy + halfH - 3.0f},
                    IM_COL32(139, 241, 255, 210), 3.0f, ImDrawFlags_None, 1.5f);
            }
        }
    }
    const float reveal = std::clamp(presentationTimer_ / 0.72f, 0.0f, 1.0f);
    const float eased = reveal * reveal * (3.0f - 2.0f * reveal);
    const int whiteAlpha = static_cast<int>((1.0f - eased) * 255.0f);
    if (whiteAlpha > 0) {
        draw->AddRectFilled({x, y}, {x + width, y + height},
            IM_COL32(244, 252, 255, whiteAlpha));
    }
#else
    (void)draw; (void)x; (void)y; (void)width; (void)height;
#endif
}
