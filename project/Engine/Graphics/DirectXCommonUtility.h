// ============================================================================
// ファイルの役割: DirectXCommonの責務を機能単位に分割し、初期化・描画・リソース操作を実装する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
void DirectXCommon::TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState)
{

    if (beforeState == afterState) {
        return;
    }

    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = beforeState;

    barrier.Transition.StateAfter = afterState;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList_->ResourceBarrier(1, &barrier);

}

void DirectXCommon::ResizeIfNeeded()
{

    if (!winApp_) {
        return;
    }

    uint32_t newWidth = 0;
    uint32_t newHeight = 0;

    if (
        !winApp_->ConsumeResize(
            newWidth,
            newHeight
        )
        ) {
        return;
    }

    if (
        newWidth == 0 ||
        newHeight == 0
        ) {
        return;
    }

    if (
        newWidth == width_ &&
        newHeight == height_
        ) {
        return;
    }

    WaitForGPU();

    width_ =
        newWidth;

    height_ =
        newHeight;


    for (
        auto& resource :
        swapChainResources_
        ) {
        resource.Reset();
    }

    depthStencilResource_.Reset();
    renderTextureResource_.Reset();
    postEffectTextureResource_.Reset();

    rtvDescriptorHeap_.Reset();
    dsvDescriptorHeap_.Reset();

    renderTextureSrvHeap_.Reset();

    HRESULT result =
        swapChain_->ResizeBuffers(
            static_cast<UINT>(
                GetBackBufferCount()
                ),
            width_,
            height_,
            DXGI_FORMAT_R8G8B8A8_UNORM,
            0
        );

    assert(SUCCEEDED(result));

    InitializeRenderTargetView();
    InitializeDepthStencilView();
    InitializeRenderTexture();

    if (hasDissolveMaskSource_) {
        SetDissolveMaskSrv(dissolveMaskSourceHandle_);
    }

}

void DirectXCommon::WaitForGPU()
{

    if (
        !commandQueue_ ||
        !fence_
        ) {
        return;
    }

    ++fenceValue_;

    HRESULT result =
        commandQueue_->Signal(
            fence_.Get(),
            fenceValue_
        );

    assert(SUCCEEDED(result));

    if (
        fence_->GetCompletedValue() <
        fenceValue_
        ) {
        result =
            fence_->SetEventOnCompletion(
                fenceValue_,
                fenceEvent_
            );

        assert(SUCCEEDED(result));

        WaitForSingleObject(
            fenceEvent_,
            INFINITE
        );
    }

}

