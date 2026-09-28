// ============================================================================
// ファイルの役割: キーボード、マウス、XInput、DirectInputゲームパッドの状態を統合して提供する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once
#include <Windows.h>
#include <wrl.h>

#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h> // DirectInputのヘッダー
#include <Xinput.h>
#include "WinApp.h"

// ライブラリのリンク指示
#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "xinput.lib")

class Input {
public:
    // 名前空間省略
    template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:
    // 初期化
    void Initialize(WinApp* winApp);
    // 更新
    void Update();

    // キーが押されているか判定
    // keyNumber: DIK_SPACE などのキーコード
    bool PushKey(BYTE keyNumber);

    // キーがトリガーされたか（押した瞬間）判定
    bool TriggerKey(BYTE keyNumber);

    bool IsGamepadConnected() const {
        return gamepadConnected_ || directInputGamepadConnected_;
    }
    float GetLeftStickX() const;
    float GetLeftStickY() const;
    bool PushGamepadButton(WORD button) const;
    bool TriggerGamepadButton(WORD button) const;

    LONG GetMouseDeltaX() const { return mouseState_.lX; }
    LONG GetMouseDeltaY() const { return mouseState_.lY; }
    LONG GetMouseWheelDelta() const { return mouseState_.lZ; }
    bool PushMouseButton(size_t button) const {
        return button < _countof(mouseState_.rgbButtons) &&
            (mouseState_.rgbButtons[button] & 0x80) != 0;
    }

private:
    void InitializeDirectInputGamepad();

    WinApp* winApp_ = nullptr;

    ComPtr<IDirectInput8> directInput_;
    ComPtr<IDirectInputDevice8> keyboard_;
    ComPtr<IDirectInputDevice8> mouse_;
    ComPtr<IDirectInputDevice8> directInputGamepad_;

    // キーボードの入力状態（全キー256個）
    DIMOUSESTATE2 mouseState_ = {};
    BYTE key_[256] = {};
    BYTE keyPre_[256] = {}; // 1フレーム前の状態（トリガー判定用）
    XINPUT_STATE gamepadState_ = {};
    XINPUT_STATE gamepadStatePre_ = {};
    bool gamepadConnected_ = false;
    DIJOYSTATE2 directInputGamepadState_ = {};
    DIJOYSTATE2 directInputGamepadStatePre_ = {};
    bool directInputGamepadConnected_ = false;
    uint32_t gamepadReconnectCounter_ = 0;
};
