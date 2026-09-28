// ============================================================================
// ファイルの役割: DirectXCommonの責務を機能単位に分割し、初期化・描画・リソース操作を実装する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
void DirectXCommon::PreDraw()
{

    UINT backBufferIndex =
        swapChain_->GetCurrentBackBufferIndex();



    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type =
        D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags =
        D3D12_RESOURCE_BARRIER_FLAG_NONE;

    barrier.Transition.pResource =
        swapChainResources_[backBufferIndex].Get();

    barrier.Transition.StateBefore =
        D3D12_RESOURCE_STATE_PRESENT;

    barrier.Transition.StateAfter =
        D3D12_RESOURCE_STATE_RENDER_TARGET;

    barrier.Transition.Subresource =
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

    commandList_->ResourceBarrier(
        1,
        &barrier
    );



    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle =
        rtvDescriptorHeap_
        ->GetCPUDescriptorHandleForHeapStart();

    UINT rtvDescriptorSize =
        device_->GetDescriptorHandleIncrementSize(
            D3D12_DESCRIPTOR_HEAP_TYPE_RTV
        );

    rtvHandle.ptr +=
        static_cast<SIZE_T>(
            backBufferIndex
            ) *
        static_cast<SIZE_T>(
            rtvDescriptorSize
            );


    commandList_->OMSetRenderTargets(
        1,
        &rtvHandle,
        false,
        nullptr
    );



    const float clearColor[] = {
        0.1f,
        0.25f,
        0.5f,
        1.0f
    };

    commandList_->ClearRenderTargetView(
        rtvHandle,
        clearColor,
        0,
        nullptr
    );



    D3D12_VIEWPORT viewport{};
    viewport.TopLeftX = 0.0f;
    viewport.TopLeftY = 0.0f;

    viewport.Width =
        static_cast<float>(width_);

    viewport.Height =
        static_cast<float>(height_);

    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;

    commandList_->RSSetViewports(
        1,
        &viewport
    );



    D3D12_RECT scissorRect{};
    scissorRect.left = 0;
    scissorRect.top = 0;

    scissorRect.right =
        static_cast<LONG>(width_);

    scissorRect.bottom =
        static_cast<LONG>(height_);

    commandList_->RSSetScissorRects(
        1,
        &scissorRect
    );
}


// 処理概要: DirectXCommonが担当する「PostDraw」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void DirectXCommon::PostDraw() {
    UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();


    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource = swapChainResources_[backBufferIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    commandList_->ResourceBarrier(1, &barrier);


    HRESULT hr = commandList_->Close();
    assert(SUCCEEDED(hr));

    ID3D12CommandList* commandLists[] = { commandList_.Get() };
    commandQueue_->ExecuteCommandLists(1, commandLists);


    swapChain_->Present(1, 0);


    fenceValue_++;
    commandQueue_->Signal(fence_.Get(), fenceValue_);

    if (fence_->GetCompletedValue() < fenceValue_) {
        fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
        WaitForSingleObject(fenceEvent_, INFINITE);
    }


    hr = commandAllocator_->Reset();
    assert(SUCCEEDED(hr));
    hr = commandList_->Reset(commandAllocator_.Get(), nullptr);
    assert(SUCCEEDED(hr));

    fpsLimiter_.Update();
}


