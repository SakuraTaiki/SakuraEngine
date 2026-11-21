// ============================================================================
// ファイルの役割: ステージ配置物の生成手順をFactory Methodとして抽象化する。
// 構成上の位置付け: StageEditorからObject3dの具体的な生成処理を分離する。
// 実装時の注意: 生成直後から描画できるよう、初回UpdateまでFactory内で完了させる。
// ============================================================================
#pragma once

#include <cstdint>
#include <memory>

#include "MyMath.h"

class Object3d;
class Object3dCommon;

struct StageObjectCreateInfo {
    const char* modelDirectory = "";
    const char* modelFile = "";
    Vector3 position{};
    Vector3 rotation{};
    Vector3 scale{1.0f, 1.0f, 1.0f};
    Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
};

// 生成側が具象クラスに依存せず配置物を要求するためのCreator。
class StageObjectFactory {
public:
    virtual ~StageObjectFactory() = default;

    // 派生Factoryが具体的なObject3dの生成方法を決定するFactory Method。
    virtual std::unique_ptr<Object3d> Create(
        const StageObjectCreateInfo& createInfo) const = 0;
};

// モデルを使用する通常のステージ配置物を生成するConcrete Creator。
class ModelStageObjectFactory final : public StageObjectFactory {
public:
    ModelStageObjectFactory(
        Object3dCommon* object3dCommon,
        uint32_t environmentTexture,
        float environmentCoefficient);

    std::unique_ptr<Object3d> Create(
        const StageObjectCreateInfo& createInfo) const override;

private:
    Object3dCommon* object3dCommon_ = nullptr;
    uint32_t environmentTexture_ = 0;
    float environmentCoefficient_ = 0.0f;
};
