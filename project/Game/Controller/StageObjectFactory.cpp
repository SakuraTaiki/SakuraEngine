// ============================================================================
// ファイルの役割: モデルを使用するステージ配置物の具体的な生成処理を実装する。
// 構成上の位置付け: StageObjectFactoryのConcrete Creator。
// 実装時の注意: Object3dの初期化順序と初回Updateを維持する。
// ============================================================================
#include "StageObjectFactory.h"

#include "ModelManager.h"
#include "Object3d.h"

ModelStageObjectFactory::ModelStageObjectFactory(
    Object3dCommon* object3dCommon,
    uint32_t environmentTexture,
    float environmentCoefficient)
    : object3dCommon_(object3dCommon),
      environmentTexture_(environmentTexture),
      environmentCoefficient_(environmentCoefficient) {}

std::unique_ptr<Object3d> ModelStageObjectFactory::Create(
    const StageObjectCreateInfo& createInfo) const {
    if (!object3dCommon_ || !createInfo.modelDirectory || !createInfo.modelFile) {
        return nullptr;
    }

    Model* model = ModelManager::Load(
        createInfo.modelDirectory,
        createInfo.modelFile);
    if (!model) {
        return nullptr;
    }

    auto object = std::make_unique<Object3d>();
    object->Initialize(object3dCommon_);
    object->SetModel(model);
    object->SetPosition(createInfo.position);
    object->SetRotation(createInfo.rotation);
    object->SetScale(createInfo.scale);
    object->SetColor(createInfo.color);
    object->SetEnvironmentTexture(environmentTexture_);
    object->SetEnvironmentCoefficient(environmentCoefficient_);

    // StageEditorの入力処理は3D描画より先に実行される。
    // 初回描画から正しいWVP行列を使用できるよう、生成時にGPUへ反映する。
    object->Update();
    return object;
}
