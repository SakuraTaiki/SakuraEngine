// ============================================================================
// ファイルの役割: Compute Shaderを利用したGPUパーティクルの生成・更新・描画を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void GPUParticleManager::CreateMesh()
{

    VertexData vertices[] = {
     {{-0.5f,  0.5f, 0.0f, 1.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
     {{ 0.5f,  0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
     {{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},

     {{-0.5f, -0.5f, 0.0f, 1.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
     {{ 0.5f,  0.5f, 0.0f, 1.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
     {{ 0.5f, -0.5f, 0.0f, 1.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
    };

    const UINT size = sizeof(vertices);

    vertexBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            dxCommon_->GetDevice(),
            size
        );

    VertexData* data =
        D3DResourceHelper::Map<VertexData>(
            vertexBuffer_.Get()
        );

    std::memcpy(data, vertices, size);
    vertexBuffer_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation =
        vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = size;
    vertexBufferView_.StrideInBytes = sizeof(VertexData);

}


void GPUParticleManager::InitializeParticlesOnGPU()
{
    updateData_->deltaTime = 0.0f;
    updateData_->totalTime = 0.0f;
    updateData_->particleCount = kMaxParticles;

    TransitionParticleResource(
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS
    );

    ID3D12GraphicsCommandList* commandList =
        dxCommon_->GetCommandList();

    srvManager_->PreDraw();

    commandList->SetComputeRootSignature(
        computeRootSignature_.Get()
    );
    commandList->SetPipelineState(
        initializePipelineState_.Get()
    );

    
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
        (kMaxParticles + kThreadCount - 1) / kThreadCount,
        1,
        1
    );

   
    D3D12_RESOURCE_BARRIER barriers[3]{};

    barriers[0].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[0].UAV.pResource = particleBuffer_.Get();

    barriers[1].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[1].UAV.pResource = freeListIndexBuffer_.Get();

    barriers[2].Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    barriers[2].UAV.pResource = freeListBuffer_.Get();

    commandList->ResourceBarrier(_countof(barriers), barriers);
}



void GPUParticleManager::DispatchEmit()
{
    ID3D12GraphicsCommandList* commandList =
        dxCommon_->GetCommandList();

    commandList->SetComputeRootSignature(
        computeRootSignature_.Get()
    );
    commandList->SetPipelineState(
        emitPipelineState_.Get()
    );

    
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
        3,
        emitterBuffer_->GetGPUVirtualAddress()
    );

   
    commandList->SetComputeRootConstantBufferView(
        4,
        perFrameBuffer_->GetGPUVirtualAddress()
    );

    commandList->Dispatch(1, 1, 1);
}


void GPUParticleManager::TransitionParticleResource(
    D3D12_RESOURCE_STATES afterState
) {
    if (particleResourceState_ == afterState) {
        return;
    }

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = particleBuffer_.Get();
    barrier.Transition.StateBefore = particleResourceState_;
    barrier.Transition.StateAfter = afterState;
    barrier.Transition.Subresource =
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    dxCommon_->GetCommandList()->ResourceBarrier(
        1,
        &barrier
    );

    particleResourceState_ = afterState;
}
