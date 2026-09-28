// ============================================================================
// ファイルの役割: キューブマップを使用した背景描画を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "Skybox.h"
#include <cassert>
#include "DirectXCommon.h"
#include "D3DResourceHelper.h"
#include "MyMath.h"
#include "TextureManager.h"
#include <string>

#pragma comment(lib, "d3dcompiler.lib")

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void Skybox::Initialize(DirectXCommon* dxCommon, TextureManager* textureManager, const std::string& texturePath) {
    assert(dxCommon);
    assert(textureManager);

    dxCommon_ = dxCommon;
    textureManager_ = textureManager;

    textureHandle_ = textureManager_->LoadTexture(texturePath);

    CreateRootSignature();
    CreateGraphicsPipeline();
    CreateVertexBuffer();
    CreateConstantBuffers();

    viewMatrix_ = Math::MakeIdentity4x4();
    projectionMatrix_ = Math::MakeIdentity4x4();

    Update();
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void Skybox::CreateRootSignature() {
    D3D12_ROOT_SIGNATURE_DESC descriptionRootSignature{};
    descriptionRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    D3D12_ROOT_PARAMETER rootParameters[3] = {};

    rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[0].Descriptor.ShaderRegister = 0;
    rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    rootParameters[1].Descriptor.ShaderRegister = 0;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

    D3D12_DESCRIPTOR_RANGE descriptorRange[1] = {};
    descriptorRange[0].BaseShaderRegister = 0;
    descriptorRange[0].NumDescriptors = 1;
    descriptorRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[2].DescriptorTable.pDescriptorRanges = descriptorRange;
    rootParameters[2].DescriptorTable.NumDescriptorRanges = _countof(descriptorRange);
    rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    descriptionRootSignature.pParameters = rootParameters;
    descriptionRootSignature.NumParameters = _countof(rootParameters);
    descriptionRootSignature.pStaticSamplers = staticSamplers;
    descriptionRootSignature.NumStaticSamplers = _countof(staticSamplers);

    ID3DBlob* signatureBlob = nullptr;
    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3D12SerializeRootSignature(
        &descriptionRootSignature,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob);

    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA(static_cast<char*>(errorBlob->GetBufferPointer()));
        }
        assert(false);
    }

    hr = dxCommon_->GetDevice()->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature_));

    assert(SUCCEEDED(hr));
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void Skybox::CreateGraphicsPipeline() {
    auto vsBlob = dxCommon_->CompileShader(L"Resources/shaders/hlsl/Skybox.VS.hlsl", L"vs_6_0");
    auto psBlob = dxCommon_->CompileShader(L"Resources/shaders/hlsl/Skybox.PS.hlsl", L"ps_6_0");

    assert(vsBlob);
    assert(psBlob);

    D3D12_INPUT_ELEMENT_DESC inputElementDescs[1] = {};
    inputElementDescs[0].SemanticName = "POSITION";
    inputElementDescs[0].SemanticIndex = 0;
    inputElementDescs[0].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    inputElementDescs[0].AlignedByteOffset = 0;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;

    psoDesc.DepthStencilState.DepthEnable = true;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleMask = D3D12_DEFAULT_SAMPLE_MASK;

    HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&pipelineState_));

    assert(SUCCEEDED(hr));
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void Skybox::CreateVertexBuffer() {
    VertexData vertices[] = {
        {{ 1.0f,  1.0f,  1.0f, 1.0f}}, {{ 1.0f,  1.0f, -1.0f, 1.0f}}, {{ 1.0f, -1.0f,  1.0f, 1.0f}},
        {{ 1.0f, -1.0f,  1.0f, 1.0f}}, {{ 1.0f,  1.0f, -1.0f, 1.0f}}, {{ 1.0f, -1.0f, -1.0f, 1.0f}},

        {{-1.0f,  1.0f, -1.0f, 1.0f}}, {{-1.0f,  1.0f,  1.0f, 1.0f}}, {{-1.0f, -1.0f, -1.0f, 1.0f}},
        {{-1.0f, -1.0f, -1.0f, 1.0f}}, {{-1.0f,  1.0f,  1.0f, 1.0f}}, {{-1.0f, -1.0f,  1.0f, 1.0f}},

        {{-1.0f,  1.0f,  1.0f, 1.0f}}, {{ 1.0f,  1.0f,  1.0f, 1.0f}}, {{-1.0f, -1.0f,  1.0f, 1.0f}},
        {{-1.0f, -1.0f,  1.0f, 1.0f}}, {{ 1.0f,  1.0f,  1.0f, 1.0f}}, {{ 1.0f, -1.0f,  1.0f, 1.0f}},

        {{ 1.0f,  1.0f, -1.0f, 1.0f}}, {{-1.0f,  1.0f, -1.0f, 1.0f}}, {{ 1.0f, -1.0f, -1.0f, 1.0f}},
        {{ 1.0f, -1.0f, -1.0f, 1.0f}}, {{-1.0f,  1.0f, -1.0f, 1.0f}}, {{-1.0f, -1.0f, -1.0f, 1.0f}},

        {{-1.0f,  1.0f, -1.0f, 1.0f}}, {{ 1.0f,  1.0f, -1.0f, 1.0f}}, {{-1.0f,  1.0f,  1.0f, 1.0f}},
        {{-1.0f,  1.0f,  1.0f, 1.0f}}, {{ 1.0f,  1.0f, -1.0f, 1.0f}}, {{ 1.0f,  1.0f,  1.0f, 1.0f}},

        {{-1.0f, -1.0f,  1.0f, 1.0f}}, {{ 1.0f, -1.0f,  1.0f, 1.0f}}, {{-1.0f, -1.0f, -1.0f, 1.0f}},
        {{-1.0f, -1.0f, -1.0f, 1.0f}}, {{ 1.0f, -1.0f,  1.0f, 1.0f}}, {{ 1.0f, -1.0f, -1.0f, 1.0f}},
    };

    vertexResource_ =
        D3DResourceHelper::CreateUploadBuffer(
            dxCommon_->GetDevice(),
            sizeof(vertices)
        );

    VertexData* vertexData =
        D3DResourceHelper::Map<VertexData>(
            vertexResource_.Get()
        );
    memcpy(vertexData, vertices, sizeof(vertices));
    vertexResource_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexResource_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = sizeof(vertices);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void Skybox::CreateConstantBuffers() {
    transformationResource_ =
        D3DResourceHelper::CreateUploadBuffer(
            dxCommon_->GetDevice(),
            D3DResourceHelper::AlignConstantBufferSize(
                sizeof(TransformationMatrix)
            )
        );

    transformationData_ =
        D3DResourceHelper::Map<TransformationMatrix>(
            transformationResource_.Get()
        );

    materialResource_ =
        D3DResourceHelper::CreateUploadBuffer(
            dxCommon_->GetDevice(),
            D3DResourceHelper::AlignConstantBufferSize(
                sizeof(Material)
            )
        );

    materialData_ =
        D3DResourceHelper::Map<Material>(
            materialResource_.Get()
        );
    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

// 処理概要: 外部から渡された値を、担当オブジェクトの状態へ反映する。
// 注意事項: 必要に応じて範囲制限や依存データの再計算も行う。
void Skybox::SetCamera(const Matrix4x4& view, const Matrix4x4& projection) {
    viewMatrix_ = view;

    viewMatrix_.m[3][0] = 0.0f;
    viewMatrix_.m[3][1] = 0.0f;
    viewMatrix_.m[3][2] = 0.0f;

    projectionMatrix_ = projection;
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void Skybox::Update() {
    Matrix4x4 worldMatrix = Math::MakeAffineMatrix(
        transform_.scale,
        transform_.rotate,
        transform_.translate);

    Matrix4x4 wvpMatrix = Math::Multiply(worldMatrix, Math::Multiply(viewMatrix_, projectionMatrix_));

    transformationData_->World = worldMatrix;
    transformationData_->WVP = wvpMatrix;
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void Skybox::Draw() {
    auto commandList = dxCommon_->GetCommandList();

    commandList->SetGraphicsRootSignature(rootSignature_.Get());
    commandList->SetPipelineState(pipelineState_.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootConstantBufferView(1, transformationResource_->GetGPUVirtualAddress());
    commandList->SetGraphicsRootDescriptorTable(2, textureManager_->GetSrvHandleGPU(textureHandle_));

    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);
    commandList->DrawInstanced(36, 1, 0, 0);
}
