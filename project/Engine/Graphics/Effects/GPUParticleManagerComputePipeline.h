// ============================================================================
// ファイルの役割: Compute Shaderを利用したGPUパーティクルの生成・更新・描画を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void GPUParticleManager::CreateComputeRootSignature()
{
    D3D12_DESCRIPTOR_RANGE ranges[3]{};

    for (UINT i = 0; i < 3; ++i)
    {
        ranges[i].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
        ranges[i].NumDescriptors = 1;
        ranges[i].BaseShaderRegister = i;
        ranges[i].OffsetInDescriptorsFromTableStart =
            D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    }

   // 0:u0 Particle
   // 1:u1 FreeListIndex
   // 2:u2 FreeList
   // 3:b0 Emitter
   // 4:b1 PerFrame
   // 5:b2 UpdateData
    D3D12_ROOT_PARAMETER rootParameters[6]{};

    for (UINT i = 0; i < 3; ++i)
    {
        rootParameters[i].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[i].DescriptorTable.NumDescriptorRanges = 1;
        rootParameters[i].DescriptorTable.pDescriptorRanges = &ranges[i];
        rootParameters[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    }

    for (UINT i = 0; i < 3; ++i)
    {
        rootParameters[i + 3].ParameterType =
            D3D12_ROOT_PARAMETER_TYPE_CBV;
        rootParameters[i + 3].Descriptor.ShaderRegister = i;
        rootParameters[i + 3].ShaderVisibility =
            D3D12_SHADER_VISIBILITY_ALL;
    }

    
    D3D12_ROOT_SIGNATURE_DESC desc{};
    desc.NumParameters = _countof(rootParameters);
    desc.pParameters = rootParameters;

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
        IID_PPV_ARGS(&computeRootSignature_)
    );
    assert(SUCCEEDED(hr));
}


void GPUParticleManager::CreateComputePipelineState()
{
    auto updateCSBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/GPUParticle.CS.hlsl",
        L"cs_6_0"
    );

    auto initializeCSBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/InitializeGPUParticle.CS.hlsl",
        L"cs_6_0"
    );

    auto emitCSBlob = dxCommon_->CompileShader(
        L"Resources/shaders/hlsl/EmitParticle.CS.hlsl",
        L"cs_6_0"
    );

    D3D12_COMPUTE_PIPELINE_STATE_DESC desc{};
    desc.pRootSignature = computeRootSignature_.Get();

    // 更新用
    desc.CS = {
        updateCSBlob->GetBufferPointer(),
        updateCSBlob->GetBufferSize()
    };

    HRESULT hr =
        dxCommon_->GetDevice()->CreateComputePipelineState(
            &desc,
            IID_PPV_ARGS(&computePipelineState_)
        );
    assert(SUCCEEDED(hr));

    // 初期化用
    desc.CS = {
        initializeCSBlob->GetBufferPointer(),
        initializeCSBlob->GetBufferSize()
    };

    hr = dxCommon_->GetDevice()->CreateComputePipelineState(
        &desc,
        IID_PPV_ARGS(&initializePipelineState_)
    );
    assert(SUCCEEDED(hr));

    // 発生用
    desc.CS = {
        emitCSBlob->GetBufferPointer(),
        emitCSBlob->GetBufferSize()
    };

    hr = dxCommon_->GetDevice()->CreateComputePipelineState(
        &desc,
        IID_PPV_ARGS(&emitPipelineState_)
    );
    assert(SUCCEEDED(hr));
}


