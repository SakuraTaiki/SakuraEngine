// ============================================================================
// ファイルの役割: Compute Shaderを利用したGPUパーティクルの生成・更新・描画を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void GPUParticleManager::Draw()
{

    if (!settings_.enabled)
    {
        return;
    }

    ID3D12GraphicsCommandList* commandList =
        dxCommon_->GetCommandList();

    TransitionParticleResource(
        D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE
    );

    commandList->SetPipelineState(
        graphicsPipelineState_.Get()
    );
    commandList->SetGraphicsRootSignature(
        graphicsRootSignature_.Get()
    );
    commandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );
    commandList->IASetVertexBuffers(
        0,
        1,
        &vertexBufferView_
    );

    commandList->SetGraphicsRootDescriptorTable(
        0,
        textureManager_->GetSrvHandleGPU(textureHandle_)
    );
    commandList->SetGraphicsRootDescriptorTable(
        1,
        srvManager_->GetGPUDescriptorHandle(particleSrvIndex_)
    );
    commandList->SetGraphicsRootConstantBufferView(
        2,
        viewProjectionBuffer_->GetGPUVirtualAddress()
    );

    commandList->DrawInstanced(
        6,
        kMaxParticles,
        0,
        0
    );
}

