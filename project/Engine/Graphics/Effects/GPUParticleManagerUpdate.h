// ============================================================================
// ファイルの役割: Compute Shaderを利用したGPUパーティクルの生成・更新・描画を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void GPUParticleManager::Update(
    const Matrix4x4& viewMatrix,
    const Matrix4x4& projectionMatrix
) {
    constexpr float deltaTime = 1.0f / 60.0f;
    totalTime_ += deltaTime;

    viewProjectionData_->billboard =
        Math::MakeBillboardMatrix(viewMatrix);
    viewProjectionData_->viewProjection =
        Math::Multiply(viewMatrix, projectionMatrix);

    updateData_->deltaTime = deltaTime;
    updateData_->totalTime = totalTime_;
    updateData_->particleCount = kMaxParticles;

    perFrameData_->time = totalTime_;
    perFrameData_->deltaTime = deltaTime;

    TransitionParticleResource(D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();
    srvManager_->PreDraw();

   

    if (emitRequested_)
    {
        DispatchEmit();

        
        D3D12_RESOURCE_BARRIER barriers[3]{};

        barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[0].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[0].UAV.pResource = particleBuffer_.Get();

        barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[1].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[1].UAV.pResource = freeListIndexBuffer_.Get();

        barriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
        barriers[2].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barriers[2].UAV.pResource = freeListBuffer_.Get();

        commandList->ResourceBarrier(_countof(barriers), barriers);

        emitRequested_ = false;
    }

    commandList->SetComputeRootSignature(
        computeRootSignature_.Get()
    );

    commandList->SetPipelineState(
        computePipelineState_.Get()
    );

    
    // u0 : gParticles
    commandList->SetComputeRootDescriptorTable(
        0,
        srvManager_->GetGPUDescriptorHandle(particleUavIndex_)
    );

   
    
    commandList->SetComputeRootDescriptorTable(
        1,
        srvManager_->GetGPUDescriptorHandle(freeListIndexUavIndex_)
    );

    
    commandList->SetComputeRootDescriptorTable(
        2,
        srvManager_->GetGPUDescriptorHandle(freeListUavIndex_)
    );

   
    commandList->SetComputeRootConstantBufferView(
        5,
        updateBuffer_->GetGPUVirtualAddress()
    );


    commandList->Dispatch(
        (kMaxParticles + kThreadCount - 1) /
        kThreadCount,
        1,
        1
    );

    // ===== 螟画峩・啅pdate縺ｯParticle縺ｨFreeList縺ｮ荳｡譁ｹ縺ｸ譖ｸ縺崎ｾｼ繧 =====
    D3D12_RESOURCE_BARRIER updateBarriers[3]{};

    updateBarriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    updateBarriers[0].UAV.pResource = particleBuffer_.Get();

    updateBarriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    updateBarriers[1].UAV.pResource = freeListIndexBuffer_.Get();

    updateBarriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    updateBarriers[2].UAV.pResource = freeListBuffer_.Get();

    commandList->ResourceBarrier(
        _countof(updateBarriers),
        updateBarriers
    );
}

