// ============================================================================
// ファイルの役割: Win32ウィンドウとOSメッセージループをカプセル化する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "WinApp.h"

#include <mmsystem.h>
#include <string>
#include <vector>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/imgui_impl_win32.h"

extern IMGUI_IMPL_API LRESULT
ImGui_ImplWin32_WndProcHandler(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
);
#endif

#include <cassert>

#pragma comment(lib, "winmm.lib")

void WinApp::Initialize()
{
#ifdef USE_IMGUI
    ImGui_ImplWin32_EnableDpiAwareness();
#endif

    CoInitializeEx(
        nullptr,
        COINIT_MULTITHREADED
    );

    wc_.lpfnWndProc =
        WindowProc;

    wc_.lpszClassName =
        L"CG2WindowClass";

    wc_.hInstance =
        GetModuleHandle(nullptr);

    wc_.hCursor =
        LoadCursor(nullptr, IDC_ARROW);

    wc_.style =
        CS_HREDRAW |
        CS_VREDRAW;

    RegisterClass(&wc_);

    windowedStyle_ =
        WS_OVERLAPPEDWINDOW;

    RECT windowRectangle = {
        0,
        0,
        kClientWidth,
        kClientHeight
    };

    AdjustWindowRect(
        &windowRectangle,
        windowedStyle_,
        FALSE
    );

    hwnd_ = CreateWindow(
        wc_.lpszClassName,
        L"CG2",
        windowedStyle_,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        windowRectangle.right -
        windowRectangle.left,
        windowRectangle.bottom -
        windowRectangle.top,
        nullptr,
        nullptr,
        wc_.hInstance,
        this
    );

    assert(hwnd_ != nullptr);

    ShowWindow(
        hwnd_,
        SW_SHOW
    );

    UpdateWindow(hwnd_);

    RECT clientRectangle{};

    GetClientRect(
        hwnd_,
        &clientRectangle
    );

    clientWidth_ =
        static_cast<uint32_t>(
            clientRectangle.right -
            clientRectangle.left
            );

    clientHeight_ =
        static_cast<uint32_t>(
            clientRectangle.bottom -
            clientRectangle.top
            );

    resizePending_ = false;

    timeBeginPeriod(1);
}

void WinApp::Finalize()
{
    timeEndPeriod(1);

    if (hwnd_) {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }

    UnregisterClass(
        wc_.lpszClassName,
        wc_.hInstance
    );

    CoUninitialize();
}

bool WinApp::ProcessMessage()
{
    MSG message{};

    while (
        PeekMessage(
            &message,
            nullptr,
            0,
            0,
            PM_REMOVE
        )
        ) {
        if (message.message == WM_QUIT) {
            return true;
        }

        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    return false;
}

bool WinApp::RestartExecutable()
{
    std::vector<wchar_t> executablePath(32768, L'\0');
    const DWORD pathLength = GetModuleFileNameW(
        nullptr,
        executablePath.data(),
        static_cast<DWORD>(executablePath.size())
    );
    if (pathLength == 0 || pathLength >= executablePath.size()) {
        return false;
    }

    std::wstring commandLine = L"\"";
    commandLine.append(executablePath.data(), pathLength);
    commandLine += L"\"";
    std::vector<wchar_t> mutableCommandLine(
        commandLine.begin(),
        commandLine.end()
    );
    mutableCommandLine.push_back(L'\0');

    STARTUPINFOW startupInfo{};
    startupInfo.cb = sizeof(startupInfo);
    PROCESS_INFORMATION processInfo{};

    const BOOL created = CreateProcessW(
        executablePath.data(),
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        nullptr,
        &startupInfo,
        &processInfo
    );
    if (!created) {
        return false;
    }

    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);

    if (hwnd_) {
        PostMessage(hwnd_, WM_CLOSE, 0, 0);
    } else {
        PostQuitMessage(0);
    }
    return true;
}

bool WinApp::ConsumeResize(
    uint32_t& width,
    uint32_t& height
) {
    if (!resizePending_) {
        return false;
    }

    if (
        clientWidth_ == 0 ||
        clientHeight_ == 0
        ) {
        return false;
    }

    width =
        clientWidth_;

    height =
        clientHeight_;

    resizePending_ = false;

    return true;
}

void WinApp::ToggleFullscreen()
{
    if (!hwnd_) {
        return;
    }

    if (!isFullscreen_) {
        windowedStyle_ =
            static_cast<DWORD>(
                GetWindowLongPtr(
                    hwnd_,
                    GWL_STYLE
                )
                );

        windowedPlacement_.length =
            sizeof(WINDOWPLACEMENT);

        GetWindowPlacement(
            hwnd_,
            &windowedPlacement_
        );

        const HMONITOR monitor =
            MonitorFromWindow(
                hwnd_,
                MONITOR_DEFAULTTONEAREST
            );

        MONITORINFO monitorInfo{};
        monitorInfo.cbSize =
            sizeof(MONITORINFO);

        if (
            GetMonitorInfo(
                monitor,
                &monitorInfo
            )
            ) {
            SetWindowLongPtr(
                hwnd_,
                GWL_STYLE,
                WS_POPUP |
                WS_VISIBLE
            );

            SetWindowPos(
                hwnd_,
                HWND_TOP,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right -
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom -
                monitorInfo.rcMonitor.top,
                SWP_FRAMECHANGED |
                SWP_NOOWNERZORDER
            );

            isFullscreen_ = true;
        }
    } else {
        SetWindowLongPtr(
            hwnd_,
            GWL_STYLE,
            windowedStyle_
        );

        SetWindowPlacement(
            hwnd_,
            &windowedPlacement_
        );

        SetWindowPos(
            hwnd_,
            nullptr,
            0,
            0,
            0,
            0,
            SWP_NOMOVE |
            SWP_NOSIZE |
            SWP_NOZORDER |
            SWP_NOOWNERZORDER |
            SWP_FRAMECHANGED
        );

        isFullscreen_ = false;
    }
}

LRESULT WinApp::HandleMessage(
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    switch (message) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED) {
            const uint32_t width =
                static_cast<uint32_t>(
                    LOWORD(lParam)
                    );

            const uint32_t height =
                static_cast<uint32_t>(
                    HIWORD(lParam)
                    );

            if (
                width > 0 &&
                height > 0
                ) {
                clientWidth_ = width;
                clientHeight_ = height;
                resizePending_ = true;
            }
        }

        return 0;

    case WM_KEYDOWN:
        if (
            wParam == VK_F11 &&
            !(lParam & (1LL << 30))
            ) {
            ToggleFullscreen();
            return 0;
        }
        break;

    case WM_SYSKEYDOWN:
        if (
            wParam == VK_RETURN &&
            (lParam & (1LL << 29)) &&
            !(lParam & (1LL << 30))
            ) {
            ToggleFullscreen();
            return 0;
        }
        break;

    case WM_CLOSE:
        DestroyWindow(hwnd_);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(
        hwnd_,
        message,
        wParam,
        lParam
    );
}


LRESULT CALLBACK WinApp::WindowProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    WinApp* winApp =
        reinterpret_cast<WinApp*>(
            GetWindowLongPtr(
                hwnd,
                GWLP_USERDATA
            )
            );

    if (message == WM_NCCREATE) {
        CREATESTRUCT* createStruct =
            reinterpret_cast<CREATESTRUCT*>(
                lParam
                );

        winApp =
            static_cast<WinApp*>(
                createStruct->lpCreateParams
                );

        if (!winApp) {
            return FALSE;
        }

        // CreateWindowが戻る前にHWNDを保存する
        winApp->hwnd_ =
            hwnd;

        SetWindowLongPtr(
            hwnd,
            GWLP_USERDATA,
            reinterpret_cast<LONG_PTR>(
                winApp
                )
        );
    }

    // ImGui初期化前は呼ばない
#ifdef USE_IMGUI
    if (
        ImGui::GetCurrentContext() != nullptr &&
        ImGui_ImplWin32_WndProcHandler(
            hwnd,
            message,
            wParam,
            lParam
        )
        ) {
        return true;
    }
#endif

    if (winApp) {
        if (message == WM_NCDESTROY) {
            SetWindowLongPtr(
                hwnd,
                GWLP_USERDATA,
                0
            );

            winApp->hwnd_ =
                nullptr;

            return DefWindowProc(
                hwnd,
                message,
                wParam,
                lParam
            );
        }

        return winApp->HandleMessage(
            message,
            wParam,
            lParam
        );
    }

    return DefWindowProc(
        hwnd,
        message,
        wParam,
        lParam
    );
}
