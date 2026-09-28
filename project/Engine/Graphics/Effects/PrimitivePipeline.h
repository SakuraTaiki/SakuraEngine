// ============================================================================
// ファイルの役割: デバッグ・演出用プリミティブの生成、更新、描画を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void Primitive::CreateRootSignature()
{
    D3D12_DESCRIPTOR_RANGE descriptorRange{};
    descriptorRange.RangeType =
        D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

    descriptorRange.NumDescriptors = 1;
    descriptorRange.BaseShaderRegister = 0;

    D3D12_ROOT_PARAMETER rootParameter{};
    rootParameter.ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

    rootParameter
        .DescriptorTable
        .NumDescriptorRanges = 1;

    rootParameter
        .DescriptorTable
        .pDescriptorRanges =
        &descriptorRange;

    rootParameter.ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampler{};
    sampler.Filter =
        D3D12_FILTER_MIN_MAG_MIP_LINEAR;

    sampler.AddressU =
        D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    sampler.AddressV =
        D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    sampler.AddressW =
        D3D12_TEXTURE_ADDRESS_MODE_WRAP;

    sampler.ShaderRegister = 0;

    sampler.ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC description{};
    description.NumParameters = 1;

    description.pParameters =
        &rootParameter;

    description.NumStaticSamplers = 1;

    description.pStaticSamplers =
        &sampler;

    description.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> signatureBlob;
    ComPtr<ID3DBlob> errorBlob;

    HRESULT result =
        D3D12SerializeRootSignature(
            &description,
            D3D_ROOT_SIGNATURE_VERSION_1,
            &signatureBlob,
            &errorBlob
        );

    if (FAILED(result)) {
        if (errorBlob) {
            OutputDebugStringA(
                static_cast<const char*>(
                    errorBlob->GetBufferPointer()
                    )
            );
        }

        assert(false);
    }

    result =
        dxCommon_->GetDevice()
        ->CreateRootSignature(
            0,
            signatureBlob->GetBufferPointer(),
            signatureBlob->GetBufferSize(),
            IID_PPV_ARGS(&rootSignature_)
        );

    assert(SUCCEEDED(result));
}

void Primitive::CreatePipelineState()
{
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        {
            "POSITION",
            0,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            "TEXCOORD",
            0,
            DXGI_FORMAT_R32G32_FLOAT,
            0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            "NORMAL",
            0,
            DXGI_FORMAT_R32G32B32_FLOAT,
            0,
            D3D12_APPEND_ALIGNED_ELEMENT,
            D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
            0
        },
        {
            "INSTANCE_WVP",
            0,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            0,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
            1
        },
        {
            "INSTANCE_WVP",
            1,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            16,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
            1
        },
        {
            "INSTANCE_WVP",
            2,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            32,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
            1
        },
        {
            "INSTANCE_WVP",
            3,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            48,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
            1
        },
        {
            "INSTANCE_COLOR",
            0,
            DXGI_FORMAT_R32G32B32A32_FLOAT,
            1,
            64,
            D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
            1
        }
    };

    auto vertexShader =
        dxCommon_->CompileShader(
            L"Resources/shaders/hlsl/Particle.VS.hlsl",
            L"vs_6_0"
        );

    auto pixelShader =
        dxCommon_->CompileShader(
            L"Resources/shaders/hlsl/Particle.PS.hlsl",
            L"ps_6_0"
        );

    assert(
        vertexShader != nullptr &&
        "Primitive VS compile failed"
    );

    assert(
        pixelShader != nullptr &&
        "Primitive PS compile failed"
    );

    D3D12_GRAPHICS_PIPELINE_STATE_DESC description{};

    description.pRootSignature =
        rootSignature_.Get();

    description.InputLayout = {
        inputLayout,
        _countof(inputLayout)
    };

    description.VS = {
        vertexShader->GetBufferPointer(),
        vertexShader->GetBufferSize()
    };

    description.PS = {
        pixelShader->GetBufferPointer(),
        pixelShader->GetBufferSize()
    };

    D3DResourceHelper::SetParticlePipelineDefaults(description);

    HRESULT result =
        dxCommon_->GetDevice()
        ->CreateGraphicsPipelineState(
            &description,
            IID_PPV_ARGS(&pipelineState_)
        );

    if (FAILED(result)) {
        OutputDebugStringA(
            "Failed to create Primitive PipelineState\n"
        );

        assert(false);
    }
}

void Primitive::CreateMesh()
{
    VertexData vertices[] = {
        {
            {-0.5f, 0.5f, 0.0f, 1.0f},
            {0.0f, 0.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            {0.5f, 0.5f, 0.0f, 1.0f},
            {1.0f, 0.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            {-0.5f, -0.5f, 0.0f, 1.0f},
            {0.0f, 1.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            {-0.5f, -0.5f, 0.0f, 1.0f},
            {0.0f, 1.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            {0.5f, 0.5f, 0.0f, 1.0f},
            {1.0f, 0.0f},
            {0.0f, 0.0f, -1.0f}
        },
        {
            {0.5f, -0.5f, 0.0f, 1.0f},
            {1.0f, 1.0f},
            {0.0f, 0.0f, -1.0f}
        }
    };

    const UINT bufferSize =
        sizeof(vertices);

    vertexBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            dxCommon_->GetDevice(),
            bufferSize
        );

    VertexData* mappedData =
        D3DResourceHelper::Map<VertexData>(
            vertexBuffer_.Get()
        );

    std::memcpy(
        mappedData,
        vertices,
        bufferSize
    );

    vertexBuffer_->Unmap(
        0,
        nullptr
    );

    vertexBufferView_.BufferLocation =
        vertexBuffer_->GetGPUVirtualAddress();

    vertexBufferView_.SizeInBytes =
        bufferSize;

    vertexBufferView_.StrideInBytes =
        sizeof(VertexData);
}
