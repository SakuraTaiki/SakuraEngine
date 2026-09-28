// ============================================================================
// ファイルの役割: パーティクルの生成、更新、描画、GPUリソースを管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void ParticleManager::CreateRootSignature() {
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = 1;
    range.BaseShaderRegister = 0;

    D3D12_ROOT_PARAMETER rootParam = {};
    rootParam.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParam.DescriptorTable.NumDescriptorRanges = 1;
    rootParam.DescriptorTable.pDescriptorRanges = &range;
    rootParam.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_STATIC_SAMPLER_DESC sampler = {};
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ShaderRegister = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = 1;
    desc.pParameters = &rootParam;
    desc.NumStaticSamplers = 1;
    desc.pStaticSamplers = &sampler;
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> blob, errorBlob;
    HRESULT hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errorBlob);
    assert(SUCCEEDED(hr));
    hr = dxCommon_->GetDevice()->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(), IID_PPV_ARGS(&rootSignature_));
    assert(SUCCEEDED(hr));
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void ParticleManager::CreatePipelineState() {
    
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        
        { "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },

        
        { "INSTANCE_WVP", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0,  D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_WVP", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_WVP", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "INSTANCE_WVP", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },

        // Color
        { "INSTANCE_COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "EFFECT_TYPE"   , 0, DXGI_FORMAT_R32_FLOAT,1,80,D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,1},
    };

    
    auto vsBlob = dxCommon_->CompileShader(L"Resources/shaders/hlsl/HitParticle.VS.hlsl", L"vs_6_0");
    auto psBlob = dxCommon_->CompileShader(L"Resources/shaders/hlsl/HitParticle.PS.hlsl", L"ps_6_0");
    assert(vsBlob != nullptr && "VS Compile Failed");
    assert(psBlob != nullptr && "PS Compile Failed");

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.pRootSignature = rootSignature_.Get();
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.VS = { vsBlob->GetBufferPointer(), vsBlob->GetBufferSize() };
    psoDesc.PS = { psBlob->GetBufferPointer(), psBlob->GetBufferSize() };

    
    D3DResourceHelper::SetParticlePipelineDefaults(psoDesc);

    
    HRESULT hr = dxCommon_->GetDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState_));
    if (FAILED(hr)) {
        OutputDebugStringA("Failed to create GraphicsPipelineState for Particles!\n");
        assert(false);
    }
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void ParticleManager::CreateMesh() {
    VertexData vertices[] = {
        {{-0.5f,  0.5f, 0, 1}, {0.0f, 0.0f}, {0, 0, -1}},
        {{ 0.5f,  0.5f, 0, 1}, {1.0f, 0.0f}, {0, 0, -1}},
        {{-0.5f, -0.5f, 0, 1}, {0.0f, 1.0f}, {0, 0, -1}},
        {{-0.5f, -0.5f, 0, 1}, {0.0f, 1.0f}, {0, 0, -1}},
        {{ 0.5f,  0.5f, 0, 1}, {1.0f, 0.0f}, {0, 0, -1}},
        {{ 0.5f, -0.5f, 0, 1}, {1.0f, 1.0f}, {0, 0, -1}},
    };

    UINT size = sizeof(vertices);

    vertexBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            dxCommon_->GetDevice(),
            size
        );

    VertexData* data =
        D3DResourceHelper::Map<VertexData>(
            vertexBuffer_.Get()
        );
    memcpy(data, vertices, size);
    vertexBuffer_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = size;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
}
