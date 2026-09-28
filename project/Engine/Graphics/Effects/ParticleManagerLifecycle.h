// ============================================================================
// ファイルの役割: パーティクルの生成、更新、描画、GPUリソースを管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void ParticleManager::Initialize(DirectXCommon* dxCommon, TextureManager* textureManager) {
    assert(dxCommon);
    dxCommon_ = dxCommon;
    textureManager_ = textureManager;

    
    textureHandle_ = textureManager_->LoadTexture("Resources/white.png");

    // 2. 繝代う繝励Λ繧､繝ｳ逕滓・
    CreateRootSignature();
    CreatePipelineState();

    
    CreateMesh();

    
    {
        auto device = dxCommon_->GetDevice();
        UINT size = sizeof(InstanceData) * kMaxParticles;

        instancingBuffer_ =
            D3DResourceHelper::CreateUploadBuffer(
                device,
                size
            );

        instancingDataMapped_ =
            D3DResourceHelper::Map<InstanceData>(
                instancingBuffer_.Get()
            );

        instancingBufferView_.BufferLocation = instancingBuffer_->GetGPUVirtualAddress();
        instancingBufferView_.SizeInBytes = size;
        instancingBufferView_.StrideInBytes = sizeof(InstanceData);
    }
}


