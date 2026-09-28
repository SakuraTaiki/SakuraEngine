// ============================================================================
// ファイルの役割: Compute Shaderを利用したGPUパーティクルの生成・更新・描画を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void GPUParticleManager::CreateBuffers()
{
    ID3D12Device* device = dxCommon_->GetDevice();
    const UINT particleBufferSize =
        sizeof(ParticleData) * kMaxParticles;

    D3D12_HEAP_PROPERTIES defaultHeap{};
    defaultHeap.Type = D3D12_HEAP_TYPE_DEFAULT;

    D3D12_RESOURCE_DESC particleDesc =
        D3DResourceHelper::MakeBufferResourceDesc(particleBufferSize);
    particleDesc.Flags =
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    HRESULT hr = device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &particleDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&particleBuffer_)
    );
    assert(SUCCEEDED(hr));

    
   
    D3D12_RESOURCE_DESC freeListIndexDesc =
        D3DResourceHelper::MakeBufferResourceDesc(sizeof(int32_t));
    freeListIndexDesc.Flags =
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    HRESULT freeListIndexHr = device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &freeListIndexDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&freeListIndexBuffer_)
    );
    assert(SUCCEEDED(freeListIndexHr));


    
    D3D12_RESOURCE_DESC freeListDesc =
        D3DResourceHelper::MakeBufferResourceDesc(
            sizeof(uint32_t) * kMaxParticles
        );
    freeListDesc.Flags =
        D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

    HRESULT freeListHr = device->CreateCommittedResource(
        &defaultHeap,
        D3D12_HEAP_FLAG_NONE,
        &freeListDesc,
        D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
        nullptr,
        IID_PPV_ARGS(&freeListBuffer_)
    );
    assert(SUCCEEDED(freeListHr));


    emitterBuffer_ = D3DResourceHelper::CreateUploadBuffer(
        device,
        D3DResourceHelper::AlignConstantBufferSize(sizeof(EmitterSphere)));
    emitterData_ = D3DResourceHelper::Map<EmitterSphere>(emitterBuffer_.Get());
    *emitterData_ = {};
    emitterData_->count = 10;
    emitterData_->frequency = 0.5f;
    emitterData_->radius = 1.0f;
    emitterData_->sizeMultiplier = 1.0f;

    emitterData_->mainColor =
        settings_.fireMainColor;

    emitterData_->subColor =
        settings_.fireSubColor;

    perFrameBuffer_ = D3DResourceHelper::CreateUploadBuffer(
        device,
        D3DResourceHelper::AlignConstantBufferSize(sizeof(PerFrame)));
    perFrameData_ = D3DResourceHelper::Map<PerFrame>(perFrameBuffer_.Get());
    *perFrameData_ = {};

    viewProjectionBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            device,
            D3DResourceHelper::AlignConstantBufferSize(
                sizeof(ViewProjectionData)
            )
        );
    viewProjectionData_ =
        D3DResourceHelper::Map<ViewProjectionData>(
            viewProjectionBuffer_.Get()
        );

    updateBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            device,
            D3DResourceHelper::AlignConstantBufferSize(
                sizeof(UpdateData)
            )
        );
    updateData_ =
        D3DResourceHelper::Map<UpdateData>(
            updateBuffer_.Get()
        );
}

void GPUParticleManager::CreateDescriptors()
{
    particleSrvIndex_ = srvManager_->Allocate();
    srvManager_->CreateSRVForStructuredBuffer(
        particleSrvIndex_,
        particleBuffer_.Get(),
        kMaxParticles,
        sizeof(ParticleData)
    );

    particleUavIndex_ = srvManager_->Allocate();
    srvManager_->CreateUAVForStructuredBuffer(
        particleUavIndex_,
        particleBuffer_.Get(),
        kMaxParticles,
        sizeof(ParticleData)
    );

    freeListIndexUavIndex_ = srvManager_->Allocate();
    srvManager_->CreateUAVForStructuredBuffer(
        freeListIndexUavIndex_,
        freeListIndexBuffer_.Get(),
        1,
        sizeof(int32_t)
    );

    
    freeListUavIndex_ = srvManager_->Allocate();
    srvManager_->CreateUAVForStructuredBuffer(
        freeListUavIndex_,
        freeListBuffer_.Get(),
        kMaxParticles,
        sizeof(uint32_t)
    );
}

