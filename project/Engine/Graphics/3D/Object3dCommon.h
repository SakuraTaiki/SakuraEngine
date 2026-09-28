// ============================================================================
// ファイルの役割: 3Dオブジェクトで共有するパイプラインとカメラ参照を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once
#include "DirectXCommon.h"
#include <d3d12.h>
#include <wrl.h>
#include "MyMath.h"
#include"Camera.h"
#include "SrvManager.h"

// 前方宣言
class TextureManager;

// 共通のライト構造体
struct DirectionalLight {
    Vector4 color;
    Vector3 direction;
    float intensity;
};

struct SpotLight
{
    Vector4 color;          // ライト色
    Vector3 position;       // ライト位置
    float intensity;        // 輝度

    Vector3 direction;      // ライトが照らす方向
    float distance;         // 最大到達距離

    float decay;            // 距離減衰率
    float cosAngle;         // 外側コーン角度のcos
    float cosFalloffStart;  // 減衰開始角度のcos
    float padding;
};

struct PointLight {
    Vector4 color;

    Vector3 position;
    float intensity;

    float radius;
    float decay;
    float padding[2];
};

class Object3dCommon {
public:
    void Initialize(DirectXCommon* dxCommon);
    void PreDraw(); // 描画前設定

    DirectXCommon* GetDxCommon() const { return dxCommon_; }
    ID3D12RootSignature* GetRootSignature() const { return rootSignature_.Get(); }
    ID3D12PipelineState* GetPipelineState() const { return pipelineState_.Get(); }
    ID3D12RootSignature* GetSkinningComputeRootSignature() const {
        return skinningComputeRootSignature_.Get();
    }
    ID3D12PipelineState* GetSkinningComputePipelineState() const {
        return skinningComputePipelineState_.Get();
    }

    // ライト制御
    void SetDefaultLight();
    void SetLightDirection(const Vector3& direction) { if (lightData_) lightData_->direction = Math::Normalize(direction); }
    void SetLightColor(const Vector4& color) { if (lightData_) lightData_->color = color; }
    void SetLightIntensity(float intensity) { if (lightData_) lightData_->intensity = intensity; }

    //====================================
    //SpotLightSet
    //====================================

    void SetSpotLightColor(const Vector4& color) {
        if (spotLightData_) {
            spotLightData_->color = color;
        }
    }

    void SetSpotLightPosition(const Vector3& position) {
        if (spotLightData_) {
            spotLightData_->position = position;
        }
    }

    void SetSpotLightIntensity(float intensity) {
        if (spotLightData_) {
            spotLightData_->intensity = intensity;
        }
    }

    void SetSpotLightDirection(const Vector3& direction) {
        if (spotLightData_) {
            spotLightData_->direction = Math::Normalize(direction);
        }
    }

    void SetSpotLightDistance(float distance) {
        if (spotLightData_) {
            spotLightData_->distance = distance;
        }
    }

    void SetSpotLightDecay(float decay) {
        if (spotLightData_) {
            spotLightData_->decay = decay;
        }
    }

    void SetSpotLightCosAngle(float cosAngle) {
        if (spotLightData_) {
            spotLightData_->cosAngle = cosAngle;
        }
    }

    void SetSpotLightCosFalloffStart(float cosFalloffStart) {
        if (spotLightData_) {
            spotLightData_->cosFalloffStart = cosFalloffStart;
        }
    }


    //=============================================
    //PointLight
    //=============================================


    void SetPointLightColor(const Vector4& color) {
        if (pointLightData_) {
            pointLightData_->color = color;
        }
    }

    void SetPointLightPosition(const Vector3& position) {
        if (pointLightData_) {
            pointLightData_->position = position;
        }
    }

    void SetPointLightIntensity(float intensity) {
        if (pointLightData_) {
            pointLightData_->intensity = intensity;
        }
    }

    void SetPointLightRadius(float radius) {
        if (pointLightData_) {
            pointLightData_->radius = radius;
        }
    }

    void SetPointLightDecay(float decay) {
        if (pointLightData_) {
            pointLightData_->decay = decay;
        }
    }


    //=============================================
    //Imgui用LightGet関数群
    //=============================================

    //DirectionalLight
    DirectionalLight& GetDirectionalLight() {
        return *lightData_;
    }

    const DirectionalLight& GetDirectionalLight() const {
        return *lightData_;
    }

    //SpotLight
    SpotLight& GetSpotLight() {
        return *spotLightData_;
    }

    const SpotLight& GetSpotLight() const {
        return *spotLightData_;
    }

    //PointLight
    PointLight& GetPointLight() {
        return *pointLightData_;
    }

    const PointLight& GetPointLight() const {
        return *pointLightData_;
    }

    D3D12_GPU_VIRTUAL_ADDRESS GetSpotLightGPUVirtualAddress() const {
        return spotLightResource_->GetGPUVirtualAddress();
    }

    D3D12_GPU_VIRTUAL_ADDRESS GetLightGPUVirtualAddress() const { return lightResource_->GetGPUVirtualAddress(); }

    D3D12_GPU_VIRTUAL_ADDRESS
        GetPointLightGPUVirtualAddress() const {
        return pointLightResource_->GetGPUVirtualAddress();
    }

    // ★重要: TextureManagerのセット
    void SetTextureManager(TextureManager* textureManager) { textureManager_ = textureManager; }
    TextureManager* GetTextureManager() const { return textureManager_; }

    //デフォルトカメラのセット
    void SetDefaultCamera(Camera* camera) { defaultCamera_ = camera; }
    Camera* GetDefaultCamera() const { return defaultCamera_; }

    void SetSrvManager(SrvManager* srvManager) { srvManager_ = srvManager; }
    SrvManager* GetSrvManager() const { return srvManager_; }

private:
    void CreateRootSignature();
    void CreateGraphicsPipeline();
    void CreateSkinningComputePipeline();
    void CreateLightBuffer();
    void CreateSpotLightBuffer();
    void CreatePointLightBuffer();

private:
    DirectXCommon* dxCommon_ = nullptr;
    TextureManager* textureManager_ = nullptr; // テクスチャ管理クラスへのポインタ

    Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
    Microsoft::WRL::ComPtr<ID3D12RootSignature>
        skinningComputeRootSignature_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState>
        skinningComputePipelineState_;

    // 平行光源用
    Microsoft::WRL::ComPtr<ID3D12Resource> lightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> spotLightResource_;
    Microsoft::WRL::ComPtr<ID3D12Resource> pointLightResource_;


    DirectionalLight* lightData_ = nullptr;
    SpotLight* spotLightData_ = nullptr;
    PointLight* pointLightData_ = nullptr;

    //デフォルトカメラ用メンバ変数
    Camera* defaultCamera_ = nullptr;

    SrvManager* srvManager_ = nullptr;


};
