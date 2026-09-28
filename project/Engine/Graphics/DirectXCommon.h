// ============================================================================
// ファイルの役割: DirectX 12のデバイス、コマンド、スワップチェーン、描画フレームを統括する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <dxcapi.h>
#include <cstdint>
#include <string>

#include "DirectXShaderCompiler.h"
#include "FpsLimiter.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxcompiler.lib")

class WinApp;

class DirectXCommon {
public: // 繧ｵ繝悶け繝ｩ繧ｹ螳夂ｾｩ
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public: // 繝｡繝ｳ繝宣未謨ｰ
    DirectXCommon() = default;
    ~DirectXCommon() = default;

    // 蛻晄悄蛹・
    void Initialize(WinApp* winApp);

    // 謠冗判蜑榊・逅・
    void PreDraw();
    // 謠冗判蠕悟・逅・
    void PostDraw();

    // 繧ｷ繧ｧ繝ｼ繝繝ｼ繧ｳ繝ｳ繝代う繝ｫ
    ComPtr<IDxcBlob> CompileShader(
        const std::wstring& filePath,
        const wchar_t* profile);

    // 繧ｲ繝・ち繝ｼ
    ID3D12Device* GetDevice() const { return device_.Get(); }

    ID3D12CommandQueue* GetCommandQueue() const { return commandQueue_.Get(); }

    DXGI_FORMAT GetRTVFormat() const { return DXGI_FORMAT_R8G8B8A8_UNORM; }

    DXGI_FORMAT GetDSVFormat() const { return DXGI_FORMAT_D24_UNORM_S8_UINT; }

    size_t GetSwapChainResourcesNum() const { return 2; }

    ID3D12GraphicsCommandList* GetCommandList() const { return commandList_.Get(); }
    size_t GetBackBufferCount() const { return 2; }

    void PreDrawForRenderTexture();
    void DrawRenderTextureToSwapChain();

    D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTextureSrvHandleCPU() const {
        return renderTextureSrvHandleCPU_;
    }

    D3D12_GPU_DESCRIPTOR_HANDLE GetRenderTextureSrvHandleGPU() const {
        return renderTextureSrvHandleGPU_;
    }

    void CopyRenderTextureSrvTo(D3D12_CPU_DESCRIPTOR_HANDLE destinationHandle);


    void SetGrayScale(bool enable) {
        enableGrayScale_ = enable;
    }

    bool GetGrayScale() const {
        return enableGrayScale_;
    }


    struct VignetteSettings {
        bool enabled = false;
        float intensity = 0.75f;
        float radius = 0.75f;
        float softness = 0.35f;
    };

    VignetteSettings& GetVignetteSettings() {
        return vignetteSettings_;
    }


    struct SmoothingSettings {
        bool enabled = false;
        int radius = 1;
        float strength = 1.0f;
    };

    SmoothingSettings& GetSmoothingSettings() {
        return smoothingSettings_;
    }

    //==================
    //GaussianFilter
    //==================

    struct GaussianSettings {
        bool enabled = false;
        int radius = 2;
        float sigma = 2.0f;
        float strength = 1.0f;
    };

    GaussianSettings& GetGaussianSettings() {
        return gaussianSettings_;
    }

    const GaussianSettings& GetGaussianSettings() const {
        return gaussianSettings_;
    }


    struct OutlineSettings {
        bool enabled = true;

        float color[4] = {
            0.0f,
            0.0f,
            0.0f,
            1.0f
        };

        // 蟆上＆縺・⊇縺ｩ蠑ｱ縺・ｷｱ蠎ｦ蟾ｮ繧らｷ壹↓縺ｪ繧・
        float threshold = 0.05f;

        float strength = 1.0f;

        // 1・・
        int thickness = 1;

        // Camera縺ｨ蜷後§蛟､縺ｫ縺吶ｋ
        float nearClip = 0.1f;
        float farClip = 100.0f;
    };

    OutlineSettings& GetOutlineSettings() {
        return outlineSettings_;
    }

    const OutlineSettings& GetOutlineSettings() const {
        return outlineSettings_;
    }


    struct RadialBlurSettings {
        bool enabled = false;

        // UV蠎ｧ讓吶ら判髱｢荳ｭ螟ｮ縺ｯ(0.5, 0.5)
        float center[2] = {
            0.5f,
            0.5f
        };

        float blurWidth = 0.01f;
        float strength = 1.0f;
        int sampleCount = 10;
    };

    RadialBlurSettings& GetRadialBlurSettings() {
        return radialBlurSettings_;
    }

    const RadialBlurSettings&
        GetRadialBlurSettings() const {
        return radialBlurSettings_;
    }


    struct DissolveSettings {
        bool enabled = false;

        float threshold = 0.0f;
        float edgeWidth = 0.03f;
        float edgeIntensity = 1.0f;

        float edgeColor[4] = {
            1.0f,
            0.4f,
            0.0f,
            1.0f
        };
    };

    DissolveSettings& GetDissolveSettings() {
        return dissolveSettings_;
    }

    const DissolveSettings&
        GetDissolveSettings() const {
        return dissolveSettings_;
    }

    void SetDissolveMaskSrv(
        D3D12_CPU_DESCRIPTOR_HANDLE sourceHandle
    );

    //================================
    //Random
    //================================

    struct RandomSettings {
        bool enabled = false;
        bool animate = true;
        bool showNoiseOnly = false;

        float speed = 1.0f;
        float scale = 720.0f;
        float strength = 1.0f;
    };

    RandomSettings& GetRandomSettings() {
        return randomSettings_;
    }

    const RandomSettings&
        GetRandomSettings() const {
        return randomSettings_;
    }

    void ResetRandomTime() {
        randomTime_ = 0.0f;
    }

    void PrepareRenderTextureForImgui();

private: // 繝｡繝ｳ繝宣未謨ｰ(蜀・Κ蜃ｦ逅・
    void InitializeDevice();
    void InitializeCommand();
    void InitializeSwapChain();
    void InitializeRenderTargetView();
    void InitializeDepthStencilView();
    void InitializeFence();


    void InitializeRenderTexture();
    void InitializeCopyImagePipeline();
    void TransitionResource(ID3D12Resource* resource, D3D12_RESOURCE_STATES beforeState, D3D12_RESOURCE_STATES afterState);

    void ResizeIfNeeded();
    void WaitForGPU();

private: // 繝｡繝ｳ繝仙､画焚
    WinApp* winApp_ = nullptr;

    // DirectX荳ｻ隕√が繝悶ず繧ｧ繧ｯ繝・
    ComPtr<IDXGIFactory7> dxgiFactory_;
    ComPtr<ID3D12Device> device_;
    ComPtr<ID3D12CommandQueue> commandQueue_;
    ComPtr<ID3D12CommandAllocator> commandAllocator_;
    ComPtr<ID3D12GraphicsCommandList> commandList_;
    ComPtr<IDXGISwapChain4> swapChain_;

    // RTV (繝ｬ繝ｳ繝繝ｼ繧ｿ繝ｼ繧ｲ繝・ヨ繝薙Η繝ｼ)
    ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap_;
    ComPtr<ID3D12Resource> swapChainResources_[2];

    // DSV (豺ｱ蠎ｦ繧ｹ繝・Φ繧ｷ繝ｫ繝薙Η繝ｼ)
    ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap_;
    ComPtr<ID3D12Resource> depthStencilResource_;

    // 繝輔ぉ繝ｳ繧ｹ (蜷梧悄逕ｨ)
    ComPtr<ID3D12Fence> fence_;
    uint64_t fenceValue_ = 0;
    HANDLE fenceEvent_ = nullptr;

    // HLSL 繧ｳ繝ｳ繝代う繝ｫ諡・ｽ薙・
    DirectXShaderCompiler shaderCompiler_;

    // 60FPS 蝗ｺ螳壽球蠖薙・
    FpsLimiter fpsLimiter_;

    ComPtr<ID3D12Resource> renderTextureResource_;
    ComPtr<ID3D12Resource> postEffectTextureResource_;

    ComPtr<ID3D12DescriptorHeap> renderTextureSrvHeap_;
    D3D12_CPU_DESCRIPTOR_HANDLE renderTextureSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE renderTextureSrvHandleGPU_{};

    D3D12_CPU_DESCRIPTOR_HANDLE renderTextureRtvHandle_{};
    D3D12_CPU_DESCRIPTOR_HANDLE postEffectTextureRtvHandle_{};
    D3D12_CPU_DESCRIPTOR_HANDLE postEffectTextureSrvHandleCPU_{};
    D3D12_GPU_DESCRIPTOR_HANDLE postEffectTextureSrvHandleGPU_{};

    D3D12_CPU_DESCRIPTOR_HANDLE depthTextureSrvHandleCPU_{};

    D3D12_GPU_DESCRIPTOR_HANDLE depthTextureSrvHandleGPU_{};

    D3D12_CPU_DESCRIPTOR_HANDLE dissolveMaskSrvHandleCPU_{};

    D3D12_GPU_DESCRIPTOR_HANDLE dissolveMaskSrvHandleGPU_{};

    D3D12_CPU_DESCRIPTOR_HANDLE dissolveMaskSourceHandle_{};
    bool hasDissolveMaskSource_ = false;


    VignetteSettings vignetteSettings_{};
    SmoothingSettings smoothingSettings_{};
    GaussianSettings gaussianSettings_{};
    OutlineSettings outlineSettings_{};
    RadialBlurSettings radialBlurSettings_{};
    DissolveSettings dissolveSettings_{};
    RandomSettings randomSettings_{};


    D3D12_RESOURCE_STATES renderTextureState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;
    D3D12_RESOURCE_STATES postEffectTextureState_ = D3D12_RESOURCE_STATE_RENDER_TARGET;

    ComPtr<ID3D12RootSignature> copyImageRootSignature_;
    ComPtr<ID3D12PipelineState> copyImagePipelineState_;

    bool enableGrayScale_ = true;

    Microsoft::WRL::ComPtr<ID3D12PipelineState> normalCopyPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> grayScalePipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> postEffectPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> smoothingPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> gaussianPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> outlinePipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> radialBlurPipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> dissolvePipelineState_;
    Microsoft::WRL::ComPtr<ID3D12PipelineState> randomPipelineState_;


    uint32_t width_ = 1280;

    uint32_t height_ = 720;

    float randomTime_ = 0.0f;
};
