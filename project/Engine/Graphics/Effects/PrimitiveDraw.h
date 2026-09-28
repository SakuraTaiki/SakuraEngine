// ============================================================================
// ファイルの役割: デバッグ・演出用プリミティブの生成、更新、描画を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void Primitive::Draw()
{
    if (!isActive_) {
        return;
    }

    if (particles_.empty()) {
        return;
    }

    auto commandList =
        dxCommon_->GetCommandList();

    assert(
        pipelineState_ != nullptr &&
        "Primitive PipelineState not created!"
    );

    commandList->SetPipelineState(
        pipelineState_.Get()
    );

    commandList->SetGraphicsRootSignature(
        rootSignature_.Get()
    );

    commandList->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );

    commandList->IASetVertexBuffers(
        0,
        1,
        &vertexBufferView_
    );

    commandList->IASetVertexBuffers(
        1,
        1,
        &instancingBufferView_
    );

    auto textureHandle =
        textureManager_->GetSrvHandleGPU(
            textureHandle_
        );

    commandList->SetGraphicsRootDescriptorTable(
        0,
        textureHandle
    );

    uint32_t particleCount =
        static_cast<uint32_t>(
            particles_.size()
            );

    if (particleCount > kMaxParticles) {
        particleCount = kMaxParticles;
    }

    commandList->DrawInstanced(
        6,
        particleCount,
        0,
        0
    );
}


