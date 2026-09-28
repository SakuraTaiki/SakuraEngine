#include "ImGuiManager.h"

#ifdef USE_IMGUI

#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"
#include "externals/imgui/imgui_impl_dx12.h"

#include "externals/imgui/ImGuizmo.h"

#endif

void ImGuiManager::Initialize(
    [[maybe_unused]] DirectXCommon* dxCommon,
    [[maybe_unused]] SrvManager* srvManager,
    [[maybe_unused]] WinApp* winApp)
{
#ifdef USE_IMGUI

    dxCommon_ = dxCommon;
    srvManager_ = srvManager;

    gameViewSrvIndex_ = srvManager_->Allocate();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplWin32_Init(winApp->GetHwnd());

    ImGui_ImplDX12_InitInfo initInfo{};
    initInfo.Device = dxCommon_->GetDevice();
    initInfo.CommandQueue = dxCommon_->GetCommandQueue();
    initInfo.NumFramesInFlight = static_cast<int>(dxCommon_->GetSwapChainResourcesNum());
    initInfo.RTVFormat = dxCommon_->GetRTVFormat();
    initInfo.DSVFormat = dxCommon_->GetDSVFormat();
    initInfo.SrvDescriptorHeap = srvManager_->GetDescriptorHeap();
    initInfo.UserData = srvManager_;

    initInfo.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo* info,
        D3D12_CPU_DESCRIPTOR_HANDLE* outCpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE* outGpuHandle)
        {
            SrvManager* srvManager = static_cast<SrvManager*>(info->UserData);

            uint32_t srvIndex = srvManager->Allocate();

            *outCpuHandle = srvManager->GetCPUDescriptorHandle(srvIndex);
            *outGpuHandle = srvManager->GetGPUDescriptorHandle(srvIndex);
        };

    initInfo.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo* info,
        D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle,
        D3D12_GPU_DESCRIPTOR_HANDLE)
        {
            SrvManager* srvManager = static_cast<SrvManager*>(info->UserData);

            uint32_t srvIndex = srvManager->GetDescriptorIndex(cpuHandle);
            srvManager->Free(srvIndex);
        };

    ImGui_ImplDX12_Init(&initInfo);

#endif
}

void ImGuiManager::Finalize()
{
#ifdef USE_IMGUI

    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

#endif
}

void ImGuiManager::Begin()
{
#ifdef USE_IMGUI

    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    ImGuizmo::BeginFrame();

    ImGui::DockSpaceOverViewport(
        0,
        ImGui::GetMainViewport(),
        ImGuiDockNodeFlags_PassthruCentralNode
    );

#endif
}

void ImGuiManager::End()
{
#ifdef USE_IMGUI

    ImGui::Render();

#endif
}

void ImGuiManager::Draw()
{
#ifdef USE_IMGUI

    ID3D12GraphicsCommandList* commandList = dxCommon_->GetCommandList();

    ID3D12DescriptorHeap* ppHeaps[] = {
        srvManager_->GetDescriptorHeap()
    };

    commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

    ImGui_ImplDX12_RenderDrawData(
        ImGui::GetDrawData(),
        commandList);

#endif
}

void ImGuiManager::UpdateGameViewTexture()
{

#ifdef USE_IMGUI
    if (!dxCommon_ || !srvManager_) {
        return;
    }

    dxCommon_->CopyRenderTextureSrvTo(
        srvManager_->GetCPUDescriptorHandle(gameViewSrvIndex_)
    );
#endif

}
// ============================================================================
// ファイルの役割: Dear ImGuiの初期化、フレーム開始、描画、終了処理を管理する。
// 構成上の位置付け: ImGuiManagerが宣言するデバッグUI基盤をDirectX 12上に実装する。
// 実装時の注意: 初期化と終了は必ず対にし、GPUが使用中のリソースを先に破棄しない。
// ============================================================================
