// ============================================================================
// ファイルの役割: デバッグ・演出用プリミティブの生成、更新、描画を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void Primitive::Emit(const Vector3& position)
{

    int count = settings_.count;

    if (count < 1) {
        count = 1;
    }

    if (count > static_cast<int>(kMaxParticles)) {
        count = static_cast<int>(kMaxParticles);
    }

    Emit(
        position,
        static_cast<uint32_t>(count)
    );

}

void Primitive::Initialize(
    DirectXCommon* dxCommon,
    TextureManager* textureManager
) {
    assert(dxCommon);
    assert(textureManager);

    dxCommon_ = dxCommon;
    textureManager_ = textureManager;

    textureHandle_ =
        textureManager_->LoadTexture(
            "Resources/white.png"
        );

    CreateRootSignature();
    CreatePipelineState();
    CreateMesh();

    const UINT bufferSize =
        sizeof(InstanceData) * kMaxParticles;

    instancingBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            dxCommon_->GetDevice(),
            bufferSize
        );

    instancingDataMapped_ =
        D3DResourceHelper::Map<InstanceData>(
            instancingBuffer_.Get()
        );

    instancingBufferView_.BufferLocation =
        instancingBuffer_
        ->GetGPUVirtualAddress();

    instancingBufferView_.SizeInBytes =
        bufferSize;

    instancingBufferView_.StrideInBytes =
        sizeof(InstanceData);
}


