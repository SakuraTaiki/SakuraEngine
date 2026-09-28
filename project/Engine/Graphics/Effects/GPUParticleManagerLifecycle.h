// ============================================================================
// ファイルの役割: Compute Shaderを利用したGPUパーティクルの生成・更新・描画を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void GPUParticleManager::Initialize(
    DirectXCommon* dxCommon,
    SrvManager* srvManager,
    TextureManager* textureManager
) {
    assert(dxCommon);
    assert(srvManager);
    assert(textureManager);

    dxCommon_ = dxCommon;
    srvManager_ = srvManager;
    textureManager_ = textureManager;

    textureHandle_ =
        textureManager_->LoadTexture("Resources/white.png");

    CreateBuffers();
    CreateDescriptors();
    CreateGraphicsRootSignature();
    CreateGraphicsPipelineState();
    CreateComputeRootSignature();
    CreateComputePipelineState();
    CreateMesh();
    InitializeParticlesOnGPU();
}

