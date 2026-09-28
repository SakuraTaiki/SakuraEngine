// ============================================================================
// ファイルの役割: DirectXCommonの責務を機能単位に分割し、初期化・描画・リソース操作を実装する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void DirectXCommon::InitializeRenderTexture()
{

    const Vector4 clearColor{ 0.1f, 0.25f, 0.5f, 1.0f };

    D3D12_RESOURCE_DESC resourceDesc{};
    resourceDesc.Width = width_;
    resourceDesc.Height = height_;
    resourceDesc.MipLevels = 1;
    resourceDesc.DepthOrArraySize = 1;
    resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    resourceDesc.SampleDesc.Count = 1;
    resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    D3D12_HEAP_PROPERTIES heapProperties{};
    heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_CLEAR_VALUE clearValue{};
    clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    clearValue.Color[0] = clearColor.x;
    clearValue.Color[1] = clearColor.y;
    clearValue.Color[2] = clearColor.z;
    clearValue.Color[3] = clearColor.w;

    HRESULT hr = device_->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &clearValue,
        IID_PPV_ARGS(&renderTextureResource_));
    assert(SUCCEEDED(hr));

    UINT rtvSize = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    renderTextureRtvHandle_ = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    renderTextureRtvHandle_.ptr += rtvSize * 2;

    device_->CreateRenderTargetView(renderTextureResource_.Get(), nullptr, renderTextureRtvHandle_);

    hr = device_->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        &clearValue,
        IID_PPV_ARGS(&postEffectTextureResource_));
    assert(SUCCEEDED(hr));

    postEffectTextureRtvHandle_ = rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    postEffectTextureRtvHandle_.ptr += rtvSize * 3;
    device_->CreateRenderTargetView(
        postEffectTextureResource_.Get(), nullptr, postEffectTextureRtvHandle_);

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    // The root descriptor table spans three SRVs. Reserve a second complete
    // range beginning at slot 3 for copying the processed texture.
    srvHeapDesc.NumDescriptors = 6;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

    hr = device_->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&renderTextureSrvHeap_));
    assert(SUCCEEDED(hr));

    renderTextureSrvHandleCPU_ = renderTextureSrvHeap_->GetCPUDescriptorHandleForHeapStart();
    renderTextureSrvHandleGPU_ = renderTextureSrvHeap_->GetGPUDescriptorHandleForHeapStart();

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = 1;

    device_->CreateShaderResourceView(renderTextureResource_.Get(), &srvDesc, renderTextureSrvHandleCPU_);

    renderTextureState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;


    UINT srvSize =
        device_->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
        );

    depthTextureSrvHandleCPU_ =
        renderTextureSrvHandleCPU_;

    depthTextureSrvHandleCPU_.ptr += srvSize;

    depthTextureSrvHandleGPU_ =
        renderTextureSrvHandleGPU_;

    depthTextureSrvHandleGPU_.ptr += srvSize;

    D3D12_SHADER_RESOURCE_VIEW_DESC depthSrvDesc{};
    depthSrvDesc.Format =
        DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    depthSrvDesc.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    depthSrvDesc.ViewDimension =
        D3D12_SRV_DIMENSION_TEXTURE2D;
    depthSrvDesc.Texture2D.MipLevels = 1;
    depthSrvDesc.Texture2D.MostDetailedMip = 0;
    depthSrvDesc.Texture2D.ResourceMinLODClamp = 0.0f;

    device_->CreateShaderResourceView(
        depthStencilResource_.Get(),
        &depthSrvDesc,
        depthTextureSrvHandleCPU_
    );

    //============================
    //Dissolve用
    //============================

    dissolveMaskSrvHandleCPU_ =
        renderTextureSrvHandleCPU_;

    dissolveMaskSrvHandleCPU_.ptr +=
        srvSize * 2;

    dissolveMaskSrvHandleGPU_ =
        renderTextureSrvHandleGPU_;

    dissolveMaskSrvHandleGPU_.ptr +=
        srvSize * 2;

    postEffectTextureSrvHandleCPU_ = renderTextureSrvHandleCPU_;
    postEffectTextureSrvHandleCPU_.ptr += srvSize * 3;
    postEffectTextureSrvHandleGPU_ = renderTextureSrvHandleGPU_;
    postEffectTextureSrvHandleGPU_.ptr += srvSize * 3;

    device_->CreateShaderResourceView(
        postEffectTextureResource_.Get(),
        &srvDesc,
        postEffectTextureSrvHandleCPU_);

    postEffectTextureState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;

}

void DirectXCommon::InitializeCopyImagePipeline()
{
    HRESULT hr = S_FALSE;

    D3D12_DESCRIPTOR_RANGE descriptorRange{};
    descriptorRange.BaseShaderRegister = 0;
    // t0 = RenderTexture
    // t1 = DepthTexture
    // t2 = DissolveMask
    descriptorRange.NumDescriptors = 3;
    descriptorRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    descriptorRange.OffsetInDescriptorsFromTableStart =
        D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    D3D12_ROOT_PARAMETER rootParameters[2]{};
    rootParameters[0].ParameterType =
        D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    rootParameters[0].DescriptorTable.pDescriptorRanges =
        &descriptorRange;
    rootParameters[0].DescriptorTable.NumDescriptorRanges = 1;
    rootParameters[0].ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;

    rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    rootParameters[1].Constants.ShaderRegister = 0;
    rootParameters[1].Constants.RegisterSpace = 0;
    rootParameters[1].Constants.Num32BitValues = 12;
    rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

    
    D3D12_STATIC_SAMPLER_DESC staticSamplers[2]{};

    // s0 : Color用Linear
    staticSamplers[0].Filter =
        D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    staticSamplers[0].AddressU =
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressV =
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].AddressW =
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
    staticSamplers[0].ComparisonFunc =
        D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].MaxLOD =
        D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].ShaderVisibility =
        D3D12_SHADER_VISIBILITY_PIXEL;

    // s1 : Depth用Point
    staticSamplers[1] = staticSamplers[0];
    staticSamplers[1].Filter =
        D3D12_FILTER_MIN_MAG_MIP_POINT;
    staticSamplers[1].ShaderRegister = 1;


    D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
    rootSignatureDesc.Flags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    rootSignatureDesc.pParameters = rootParameters;
    rootSignatureDesc.NumParameters = 2;
    rootSignatureDesc.pStaticSamplers = staticSamplers;
    rootSignatureDesc.NumStaticSamplers = 2;

    Microsoft::WRL::ComPtr<ID3DBlob> signatureBlob;
    Microsoft::WRL::ComPtr<ID3DBlob> errorBlob;

    hr = D3D12SerializeRootSignature(
        &rootSignatureDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &signatureBlob,
        &errorBlob
    );

    if (FAILED(hr)) {
        if (errorBlob) {
            OutputDebugStringA(
                reinterpret_cast<char*>(
                    errorBlob->GetBufferPointer()
                    )
            );
        }
        assert(false);
    }

    hr = device_->CreateRootSignature(
        0,
        signatureBlob->GetBufferPointer(),
        signatureBlob->GetBufferSize(),
        IID_PPV_ARGS(&copyImageRootSignature_)
    );
    assert(SUCCEEDED(hr));

    auto vsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/Fullscreen.VS.hlsl",
            L"vs_6_0"
        );

    auto normalPsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/CopyImage.PS.hlsl",
            L"ps_6_0"
        );

    auto grayPsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/Grayscale.PS.hlsl",
            L"ps_6_0"
        );

    auto postEffectPsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/Vignette.PS.hlsl",
            L"ps_6_0"
        );

    auto smoothingPsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/BoxFilter.PS.hlsl",
            L"ps_6_0"
        );

    auto gaussianPsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/GaussianFilter.PS.hlsl",
            L"ps_6_0"
        );

    auto outlinePsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/DepthBasedOutline.PS.hlsl",
            L"ps_6_0"
        );


    auto radialBlurPsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/RadialBlur.PS.hlsl",
            L"ps_6_0"
        );


    auto dissolvePsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/Dissolve.PS.hlsl",
            L"ps_6_0"
        );

    auto randomPsBlob =
        CompileShader(
            L"Resources/shaders/hlsl/RandomNoise.PS.hlsl",
            L"ps_6_0"
        );

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{};

    psoDesc.pRootSignature =
        copyImageRootSignature_.Get();

    psoDesc.InputLayout.pInputElementDescs = nullptr;
    psoDesc.InputLayout.NumElements = 0;

    psoDesc.VS = {
        vsBlob->GetBufferPointer(),
        vsBlob->GetBufferSize()
    };

    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask =
        D3D12_COLOR_WRITE_ENABLE_ALL;

    psoDesc.RasterizerState.FillMode =
        D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode =
        D3D12_CULL_MODE_NONE;

    psoDesc.DepthStencilState.DepthEnable = false;
    psoDesc.DepthStencilState.StencilEnable = false;

    psoDesc.SampleMask =
        D3D12_DEFAULT_SAMPLE_MASK;

    psoDesc.PrimitiveTopologyType =
        D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] =
        DXGI_FORMAT_R8G8B8A8_UNORM;

    psoDesc.SampleDesc.Count = 1;

    // 通常表示用 PSO
    psoDesc.PS = {
        normalPsBlob->GetBufferPointer(),
        normalPsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&normalCopyPipelineState_)
    );
    assert(SUCCEEDED(hr));

    // グレースケール用 PSO
    psoDesc.PS = {
        grayPsBlob->GetBufferPointer(),
        grayPsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&grayScalePipelineState_)
    );
    assert(SUCCEEDED(hr));

    psoDesc.PS = {
        postEffectPsBlob->GetBufferPointer(),
        postEffectPsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&postEffectPipelineState_)
    );
    assert(SUCCEEDED(hr));

    //smoothing
    psoDesc.PS = {
        smoothingPsBlob->GetBufferPointer(),
        smoothingPsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&smoothingPipelineState_)
    );
    assert(SUCCEEDED(hr));

    //GaussianFilter
    psoDesc.PS = {
    gaussianPsBlob->GetBufferPointer(),
    gaussianPsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(
            &gaussianPipelineState_
        )
    );

    assert(SUCCEEDED(hr));


    //Outline
    psoDesc.PS = {
    outlinePsBlob->GetBufferPointer(),
    outlinePsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(&outlinePipelineState_)
    );

    assert(SUCCEEDED(hr));


    psoDesc.PS = {
    radialBlurPsBlob->GetBufferPointer(),
    radialBlurPsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(
            &radialBlurPipelineState_
        )
    );

    assert(SUCCEEDED(hr));

    //=============================
    //Dissolve用
    //=============================

    psoDesc.PS = {
    dissolvePsBlob->GetBufferPointer(),
    dissolvePsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(
            &dissolvePipelineState_
        )
    );

    assert(SUCCEEDED(hr));


    //==================================
    //Random用
    //==================================

    psoDesc.PS = {
    randomPsBlob->GetBufferPointer(),
    randomPsBlob->GetBufferSize()
    };

    hr = device_->CreateGraphicsPipelineState(
        &psoDesc,
        IID_PPV_ARGS(
            &randomPipelineState_
        )
    );

    assert(SUCCEEDED(hr));

}

