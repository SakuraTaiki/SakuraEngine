// ============================================================================
// ファイルの役割: 円柱形状または円柱パーティクルの頂点生成と描画を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "Cylinder.h"
#include <cassert>
#include <cmath>
#include <numbers>
#include <vector>
#include <cstring>

#include "D3DResourceHelper.h"

using namespace Microsoft::WRL;

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void Cylinder::Initialize(DirectXCommon* dxCommon, TextureManager* textureManager) {
    assert(dxCommon);
    assert(textureManager);

    dxCommon_ = dxCommon;
    textureManager_ = textureManager;

    textureHandle_ = textureManager_->LoadTexture("Resources/white.png");

    CreateRootSignature();
    CreatePipelineState();
    CreateMesh();

    UINT size = sizeof(InstanceData) * kMaxCylinders;

    instancingBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            dxCommon_->GetDevice(),
            size
        );

    instancingDataMapped_ =
        D3DResourceHelper::Map<InstanceData>(
            instancingBuffer_.Get()
        );

    instancingBufferView_.BufferLocation =
        instancingBuffer_->GetGPUVirtualAddress();

    instancingBufferView_.SizeInBytes = size;
    instancingBufferView_.StrideInBytes = sizeof(InstanceData);
}

// 処理概要: Cylinderが担当する「Emit」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void Cylinder::Emit(const Vector3& position) {
    particleSystem_.Emit(position);
}


void Cylinder::Update(
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projectionMatrix
) {
    particleSystem_.Update();

    const Matrix4x4 viewProjectionMatrix =
        Math::Multiply(
            viewMatrix,
            projectionMatrix
        );

    uint32_t index = 0;

    for (
        const auto& cylinder :
        particleSystem_.GetParticles()
        ) {
        if (index >= kMaxCylinders) {
            break;
        }

        const Matrix4x4 scaleMatrix =
            Math::Matrix4x4MakeScaleMatrix(
                cylinder.transform.scale
            );

        const Matrix4x4 translateMatrix =
            Math::MakeTranslateMatrix(
                cylinder.transform.translate
            );

        const Matrix4x4 worldMatrix =
            Math::Multiply(
                scaleMatrix,
                translateMatrix
            );

        instancingDataMapped_[index].WVP =
            Math::Multiply(
                worldMatrix,
                viewProjectionMatrix
            );

        instancingDataMapped_[index].color =
            cylinder.color;

        instancingDataMapped_[index]
            .shapeParameters = {
                cylinder.settings.bottomRadiusScale,
                cylinder.settings.topRadiusScale,
                cylinder.settings.twistAmount,
                cylinder.twistPhase
        };

        instancingDataMapped_[index]
            .effectParameters = {
                cylinder.progress,
                cylinder.settings.topFade,
                cylinder.settings.bottomFade,
                0.0f
        };

        instancingDataMapped_[index]
            .noiseParameters = {
                cylinder.settings.noiseStrength,
                cylinder.settings.noiseFrequency,
                cylinder.noisePhase,
                0.0f
        };

        ++index;
    }
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void Cylinder::Draw() {
    if (!isActive_) {
        return;
    }

    if (particleSystem_.IsEmpty()) {
        return;
    }

    auto commandList = dxCommon_->GetCommandList();

    commandList->SetPipelineState(pipelineState_.Get());
    commandList->SetGraphicsRootSignature(rootSignature_.Get());

    commandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->IASetVertexBuffers(1, 1, &instancingBufferView_);

    auto srvHandle = textureManager_->GetSrvHandleGPU(textureHandle_);
    commandList->SetGraphicsRootDescriptorTable(0, srvHandle);

    uint32_t count =
        static_cast<uint32_t>(particleSystem_.GetParticles().size());

    commandList->DrawInstanced(vertexCount_, count, 0, 0);
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void Cylinder::CreateRootSignature() {
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0;

    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParam.DescriptorTable.NumDescriptorRanges = 1;
    rootParam.DescriptorTable.pDescriptorRanges = &range;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 1;
    desc.pParameters = &rootParam;
    desc.NumStaticSamplers = 1;
    desc.pStaticSamplers = &sampler;
    desc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> blob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT hr = D3D12SerializeRootSignature(
        &desc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &blob,
        &errorBlob
    );
    assert(SUCCEEDED(hr));

    hr = dxCommon_->GetDevice()->CreateRootSignature(
        0,
        blob->GetBufferPointer(),
        blob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_)
    );
    assert(SUCCEEDED(hr));
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void Cylinder::CreatePipelineState() {
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,
        D3D12_APPEND_ALIGNED_ELEMENT,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0,
        D3D12_APPEND_ALIGNED_ELEMENT,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,
        D3D12_APPEND_ALIGNED_ELEMENT,
        D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        { "INSTANCE_WVP", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },

        { "INSTANCE_WVP", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },

        { "INSTANCE_WVP", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },

        { "INSTANCE_WVP", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },

        { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },

        {"INSTANCE_SHAPE",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,80,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,1},

        {"INSTANCE_EFFECT",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,96,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,1},

        {"INSTANCE_NOISE",0,DXGI_FORMAT_R32G32B32A32_FLOAT,1,112,
        D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,1}
    };

    auto vsBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/CylinderEffect.VS.hlsl",
        L"vs_6_0"
    );

    auto psBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/CylinderEffect.PS.hlsl",
        L"ps_6_0"
    );

    assert(vsBlob);
    assert(psBlob);

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.VS = {
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize()
    };
    psoDesc.PS = {
        psBlob->GetBufferPointer(),
        psBlob->GetBufferSize()
    };

    D3DResourceHelper::SetParticlePipelineDefaults(psoDesc);

    HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&pipelineState_)
    );
    assert(SUCCEEDED(hr));
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void Cylinder::CreateMesh() {
    const uint32_t kCylinderDivide = 32;
    const float kTopRadius = 1.0f;
    const float kBottomRadius = 1.0f;
    const float kHeight = 1.0f;

    const float radianPerDivide =
        2.0f * std::numbers::pi_v<float> /
        float(kCylinderDivide);

    std::vector<VertexData> vertices;
    vertices.reserve(kCylinderDivide * 6);

    for (uint32_t index = 0; index < kCylinderDivide; ++index) {
        float sin = std::sin(index * radianPerDivide);
        float cos = std::cos(index * radianPerDivide);

        float sinNext =
            std::sin((index + 1) * radianPerDivide);

        float cosNext =
            std::cos((index + 1) * radianPerDivide);

        float u = float(index) / float(kCylinderDivide);
        float uNext = float(index + 1) / float(kCylinderDivide);

        VertexData v0 = {
            { -sin * kTopRadius, kHeight, cos * kTopRadius, 1.0f },
            { u, 0.0f },
            { -sin, 0.0f, cos }
        };

        VertexData v1 = {
            { -sinNext * kTopRadius, kHeight, cosNext * kTopRadius, 1.0f },
            { uNext, 0.0f },
            { -sinNext, 0.0f, cosNext }
        };

        VertexData v2 = {
            { -sin * kBottomRadius, 0.0f, cos * kBottomRadius, 1.0f },
            { u, 1.0f },
            { -sin, 0.0f, cos }
        };

        VertexData v3 = {
            { -sinNext * kBottomRadius, 0.0f, cosNext * kBottomRadius, 1.0f },
            { uNext, 1.0f },
            { -sinNext, 0.0f, cosNext }
        };

        vertices.push_back(v0);
        vertices.push_back(v1);
        vertices.push_back(v2);

        vertices.push_back(v2);
        vertices.push_back(v1);
        vertices.push_back(v3);
    }

    vertexCount_ = static_cast<uint32_t>(vertices.size());

    UINT size = UINT(sizeof(VertexData) * vertices.size());

    vertexBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            dxCommon_->GetDevice(),
            size
        );

    VertexData* data =
        D3DResourceHelper::Map<VertexData>(
            vertexBuffer_.Get()
        );

    memcpy(data, vertices.data(), size);
    vertexBuffer_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation =
        vertexBuffer_->GetGPUVirtualAddress();

    vertexBufferView_.SizeInBytes = size;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
}
