// ============================================================================
// ファイルの役割: DirectXCommonの責務を機能単位に分割し、初期化・描画・リソース操作を実装する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
void DirectXCommon::PreDrawForRenderTexture()
{

    ResizeIfNeeded();

    TransitionResource(
        renderTextureResource_.Get(),
        renderTextureState_,
        D3D12_RESOURCE_STATE_RENDER_TARGET);

    renderTextureState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;

    D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle =
        dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();

    commandList_->OMSetRenderTargets(1, &renderTextureRtvHandle_, false, &dsvHandle);

    float clearColor[] = { 0.1f, 0.25f, 0.5f, 1.0f };
    commandList_->ClearRenderTargetView(renderTextureRtvHandle_, clearColor, 0, nullptr);
    commandList_->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    D3D12_VIEWPORT viewport{};
    viewport.Width = static_cast<float>(width_);
    viewport.Height = static_cast<float>(height_);
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    commandList_->RSSetViewports(1, &viewport);

    D3D12_RECT scissorRect{};
    scissorRect.right = width_;
    scissorRect.bottom = height_;
    commandList_->RSSetScissorRects(1, &scissorRect);


}


void DirectXCommon::DrawRenderTextureToSwapChain()
{


    TransitionResource(
        renderTextureResource_.Get(),
        renderTextureState_,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    renderTextureState_ =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    TransitionResource(
        postEffectTextureResource_.Get(),
        postEffectTextureState_,
        D3D12_RESOURCE_STATE_RENDER_TARGET);
    postEffectTextureState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;

    commandList_->OMSetRenderTargets(
        1, &postEffectTextureRtvHandle_, false, nullptr);

    // Outline邵ｺ譴ｧ諤剰怏・ｹ邵ｺ・ｪ陜｣・ｴ陷ｷ蛹ｻ繝ｻDepth郢ｧ・訴xelShader邵ｺ荵晢ｽ蛾坡・ｭ邵ｺ・ｿ髴趣ｽｼ郢ｧﾂ
    if (outlineSettings_.enabled) {
        TransitionResource(
            depthStencilResource_.Get(),
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        );
    }



    ID3D12DescriptorHeap* heaps[] = {
        renderTextureSrvHeap_.Get()
    };

    commandList_->SetDescriptorHeaps(
        1,
        heaps
    );



    commandList_->SetGraphicsRootSignature(
        copyImageRootSignature_.Get()
    );

    if (outlineSettings_.enabled) {
        commandList_->SetPipelineState(
            outlinePipelineState_.Get()
        );
    } else if (randomSettings_.enabled) {
        commandList_->SetPipelineState(
            randomPipelineState_.Get()
        );
    } else if (dissolveSettings_.enabled) {
        commandList_->SetPipelineState(
            dissolvePipelineState_.Get()
        );
    } else if (radialBlurSettings_.enabled) {
        commandList_->SetPipelineState(
            radialBlurPipelineState_.Get()
        );
    } else if (gaussianSettings_.enabled) {
        commandList_->SetPipelineState(
            gaussianPipelineState_.Get()
        );
    } else if (smoothingSettings_.enabled) {
        commandList_->SetPipelineState(
            smoothingPipelineState_.Get()
        );
    } else {
        commandList_->SetPipelineState(
            postEffectPipelineState_.Get()
        );
    }

    commandList_->IASetPrimitiveTopology(
        D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST
    );


    commandList_->SetGraphicsRootDescriptorTable(
        0,
        renderTextureSrvHandleGPU_
    );

    //==================================================
    // Outline
    //==================================================

    if (outlineSettings_.enabled) {
        struct OutlineConstants {
            float outlineColor[4];

            float outlineThreshold;
            float outlineStrength;
            float nearClip;
            float farClip;

            uint32_t outlineThickness;
            uint32_t enableOutline;
            float padding[2];
        };

        OutlineConstants constants{};

        constants.outlineColor[0] =
            outlineSettings_.color[0];

        constants.outlineColor[1] =
            outlineSettings_.color[1];

        constants.outlineColor[2] =
            outlineSettings_.color[2];

        constants.outlineColor[3] =
            outlineSettings_.color[3];

        constants.outlineThreshold =
            (std::max)(
                outlineSettings_.threshold,
                0.00001f
                );

        constants.outlineStrength =
            (std::max)(
                outlineSettings_.strength,
                0.0f
                );

        constants.nearClip =
            (std::max)(
                outlineSettings_.nearClip,
                0.00001f
                );

        constants.farClip =
            (std::max)(
                outlineSettings_.farClip,
                constants.nearClip + 0.00001f
                );

        constants.outlineThickness =
            static_cast<uint32_t>(
                std::clamp(
                    outlineSettings_.thickness,
                    1,
                    4
                )
                );

        constants.enableOutline = 1;

        commandList_->SetGraphicsRoot32BitConstants(
            1,
            12,
            &constants,
            0
        );
    }



    else if (randomSettings_.enabled) {
        if (randomSettings_.animate) {
            constexpr float deltaTime =
                1.0f / 60.0f;

            randomTime_ +=
                deltaTime;
        }

        struct RandomConstants {
            float time;
            float scale;
            float strength;
            float speed;

            uint32_t showNoiseOnly;
            float padding[3];
        };

        RandomConstants constants{};

        constants.time =
            randomTime_;

        constants.scale =
            std::clamp(
                randomSettings_.scale,
                1.0f,
                2000.0f
            );

        constants.strength =
            std::clamp(
                randomSettings_.strength,
                0.0f,
                1.0f
            );

        constants.speed =
            std::clamp(
                randomSettings_.speed,
                0.0f,
                10.0f
            );

        constants.showNoiseOnly =
            randomSettings_.showNoiseOnly
            ? 1u
            : 0u;

        commandList_->SetGraphicsRoot32BitConstants(
            1,
            8,
            &constants,
            0
        );
    }



    else if (dissolveSettings_.enabled) {
        struct DissolveConstants {
            float threshold;
            float edgeWidth;
            float edgeIntensity;
            float padding;

            float edgeColor[4];
        };

        DissolveConstants constants{};

        constants.threshold =
            std::clamp(
                dissolveSettings_.threshold,
                0.0f,
                1.0f
            );

        constants.edgeWidth =
            std::clamp(
                dissolveSettings_.edgeWidth,
                0.001f,
                0.25f
            );

        constants.edgeIntensity =
            std::clamp(
                dissolveSettings_.edgeIntensity,
                0.0f,
                1.0f
            );

        constants.edgeColor[0] =
            dissolveSettings_.edgeColor[0];

        constants.edgeColor[1] =
            dissolveSettings_.edgeColor[1];

        constants.edgeColor[2] =
            dissolveSettings_.edgeColor[2];

        constants.edgeColor[3] =
            dissolveSettings_.edgeColor[3];

        commandList_->SetGraphicsRoot32BitConstants(
            1,
            8,
            &constants,
            0
        );
    }

    //==================================================
    //RadialBlur
    //==================================================


    else if (radialBlurSettings_.enabled) {
        struct RadialBlurConstants {
            float center[2];
            float blurWidth;
            float strength;

            uint32_t sampleCount;
            float padding[3];
        };

        RadialBlurConstants constants{};

        constants.center[0] =
            std::clamp(
                radialBlurSettings_.center[0],
                0.0f,
                1.0f
            );

        constants.center[1] =
            std::clamp(
                radialBlurSettings_.center[1],
                0.0f,
                1.0f
            );

        constants.blurWidth =
            std::clamp(
                radialBlurSettings_.blurWidth,
                0.0f,
                0.1f
            );

        constants.strength =
            std::clamp(
                radialBlurSettings_.strength,
                0.0f,
                1.0f
            );

        constants.sampleCount =
            static_cast<uint32_t>(
                std::clamp(
                    radialBlurSettings_.sampleCount,
                    2,
                    32
                )
                );

        commandList_->SetGraphicsRoot32BitConstants(
            1,
            8,
            &constants,
            0
        );
    }


    //==================================================
    // GaussianFilter
    //==================================================

    else if (gaussianSettings_.enabled) {
        struct GaussianConstants {
            uint32_t blurRadius;
            float sigma;
            float blurStrength;
            float padding;
        };

        GaussianConstants constants{};

        constants.blurRadius =
            static_cast<uint32_t>(
                std::clamp(
                    gaussianSettings_.radius,
                    1,
                    4
                )
                );

        constants.sigma =
            std::clamp(
                gaussianSettings_.sigma,
                0.1f,
                10.0f
            );

        constants.blurStrength =
            std::clamp(
                gaussianSettings_.strength,
                0.0f,
                1.0f
            );

        commandList_->SetGraphicsRoot32BitConstants(
            1,
            4,
            &constants,
            0
        );
    }

    //==================================================
    // BoxFilter
    //==================================================

    else if (smoothingSettings_.enabled) {
        struct SmoothingConstants {
            uint32_t blurRadius;
            float blurStrength;
            float padding[2];
        };

        SmoothingConstants constants{};

        constants.blurRadius =
            static_cast<uint32_t>(
                std::clamp(
                    smoothingSettings_.radius,
                    1,
                    4
                )
                );

        constants.blurStrength =
            std::clamp(
                smoothingSettings_.strength,
                0.0f,
                1.0f
            );

        commandList_->SetGraphicsRoot32BitConstants(
            1,
            4,
            &constants,
            0
        );
    }



    else {
        struct PostEffectConstants {
            float vignetteIntensity;
            float vignetteRadius;
            float vignetteSoftness;
            float aspectRatio;

            uint32_t enableVignette;
            uint32_t enableGrayScale;
            uint32_t padding[2];
        };

        PostEffectConstants constants{};

        constants.vignetteIntensity =
            vignetteSettings_.intensity;

        constants.vignetteRadius =
            vignetteSettings_.radius;

        constants.vignetteSoftness =
            vignetteSettings_.softness;

        constants.aspectRatio =
            static_cast<float>(width_) /
            static_cast<float>(height_);

        constants.enableVignette =
            vignetteSettings_.enabled
            ? 1u
            : 0u;

        constants.enableGrayScale =
            enableGrayScale_
            ? 1u
            : 0u;

        commandList_->SetGraphicsRoot32BitConstants(
            1,
            8,
            &constants,
            0
        );
    }

    //==================================================
    // Fullscreen Triangle隰蜀怜愛
    //==================================================

    commandList_->DrawInstanced(
        3,
        1,
        0,
        0
    );



    if (outlineSettings_.enabled) {
        TransitionResource(
            depthStencilResource_.Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
            D3D12_RESOURCE_STATE_DEPTH_WRITE
        );
    }

    // The effect pass above is rendered to a separate texture so that the
    // ImGui Game View can display the processed image instead of the raw scene.
    TransitionResource(
        postEffectTextureResource_.Get(),
        postEffectTextureState_,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    postEffectTextureState_ = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

    UINT backBufferIndex = swapChain_->GetCurrentBackBufferIndex();
    D3D12_CPU_DESCRIPTOR_HANDLE backBufferRtv =
        rtvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart();
    const UINT rtvDescriptorSize = device_->GetDescriptorHandleIncrementSize(
        D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    backBufferRtv.ptr +=
        static_cast<SIZE_T>(backBufferIndex) * rtvDescriptorSize;
    commandList_->OMSetRenderTargets(1, &backBufferRtv, false, nullptr);

    commandList_->SetGraphicsRootSignature(copyImageRootSignature_.Get());
    commandList_->SetPipelineState(normalCopyPipelineState_.Get());
    commandList_->SetGraphicsRootDescriptorTable(
        0, postEffectTextureSrvHandleGPU_);
    commandList_->DrawInstanced(3, 1, 0, 0);
}

void DirectXCommon::CopyRenderTextureSrvTo(D3D12_CPU_DESCRIPTOR_HANDLE destinationHandle)
{
    device_->CopyDescriptorsSimple(
        1,
        destinationHandle,
        postEffectTextureSrvHandleCPU_,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );
}


void DirectXCommon::SetDissolveMaskSrv(D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle)
{

    dissolveMaskSourceHandle_ = sourceHandle;
    hasDissolveMaskSource_ = true;

    device_->CopyDescriptorsSimple(
        1,
        dissolveMaskSrvHandleCPU_,
        sourceHandle,
        D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
    );

}

void DirectXCommon::PrepareRenderTextureForImgui()
{

    TransitionResource(
        renderTextureResource_.Get(),
        renderTextureState_,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    renderTextureState_ =
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

}


