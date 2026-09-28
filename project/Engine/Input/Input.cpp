// ============================================================================
// ファイルの役割: キーボード、マウス、XInput、DirectInputゲームパッドの状態を統合して提供する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "Input.h"
#include <cassert>
#include <algorithm>
#include <cmath>

namespace {
struct DirectInputGamepadEnumeration {
    GUID instanceGuid{};
    bool found = false;
};

BOOL CALLBACK FindFirstGamepad(
    const DIDEVICEINSTANCE* instance,
    VOID* context
) {
    auto* enumeration =
        static_cast<DirectInputGamepadEnumeration*>(context);
    enumeration->instanceGuid = instance->guidInstance;
    enumeration->found = true;
    return DIENUM_STOP;
}

BOOL CALLBACK ConfigureGamepadAxis(
    const DIDEVICEOBJECTINSTANCE* object,
    VOID* context
) {
    auto* device = static_cast<IDirectInputDevice8*>(context);
    DIPROPRANGE range{};
    range.diph.dwSize = sizeof(DIPROPRANGE);
    range.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    range.diph.dwHow = DIPH_BYID;
    range.diph.dwObj = object->dwType;
    range.lMin = -32768;
    range.lMax = 32767;
    device->SetProperty(DIPROP_RANGE, &range.diph);
    return DIENUM_CONTINUE;
}
}


void Input::Initialize(WinApp* winApp)
{
    assert(winApp);
    assert(winApp->GetHwnd());
    assert(IsWindow(winApp->GetHwnd()));

    winApp_ =
        winApp;

    HRESULT result =
        DirectInput8Create(
            winApp_->GetHInstance(),
            DIRECTINPUT_VERSION,
            IID_IDirectInput8,
            reinterpret_cast<void**>(
                directInput_.GetAddressOf()
                ),
            nullptr
        );

    assert(SUCCEEDED(result));

    result =
        directInput_->CreateDevice(
            GUID_SysKeyboard,
            keyboard_.GetAddressOf(),
            nullptr
        );

    assert(SUCCEEDED(result));

    result =
        keyboard_->SetDataFormat(
            &c_dfDIKeyboard
        );

    assert(SUCCEEDED(result));

    // 標準的で安全な協調レベル
    result =
        keyboard_->SetCooperativeLevel(
            winApp_->GetHwnd(),
            DISCL_FOREGROUND |
            DISCL_NONEXCLUSIVE
        );

    assert(SUCCEEDED(result));

    result =
        keyboard_->Acquire();

    // 起動時にまだフォーカスがない場合があるため、
    // Acquireの失敗はここではassertしない
    if (FAILED(result)) {
        ZeroMemory(
            key_,
            sizeof(key_)
        );
    }

    result = directInput_->CreateDevice(
        GUID_SysMouse,
        mouse_.GetAddressOf(),
        nullptr
    );
    assert(SUCCEEDED(result));

    result = mouse_->SetDataFormat(&c_dfDIMouse2);
    assert(SUCCEEDED(result));

    result = mouse_->SetCooperativeLevel(
        winApp_->GetHwnd(),
        DISCL_FOREGROUND | DISCL_NONEXCLUSIVE
    );
    assert(SUCCEEDED(result));

    mouse_->Acquire();

    InitializeDirectInputGamepad();
}

void Input::InitializeDirectInputGamepad()
{
    directInputGamepad_.Reset();
    directInputGamepadConnected_ = false;
    ZeroMemory(
        &directInputGamepadState_,
        sizeof(directInputGamepadState_)
    );
    ZeroMemory(
        &directInputGamepadStatePre_,
        sizeof(directInputGamepadStatePre_)
    );

    if (!directInput_ || !winApp_) {
        return;
    }

    DirectInputGamepadEnumeration enumeration{};
    directInput_->EnumDevices(
        DI8DEVCLASS_GAMECTRL,
        FindFirstGamepad,
        &enumeration,
        DIEDFL_ATTACHEDONLY
    );
    if (!enumeration.found) {
        return;
    }

    HRESULT result = directInput_->CreateDevice(
        enumeration.instanceGuid,
        directInputGamepad_.GetAddressOf(),
        nullptr
    );
    if (FAILED(result)) {
        directInputGamepad_.Reset();
        return;
    }

    result = directInputGamepad_->SetDataFormat(&c_dfDIJoystick2);
    if (FAILED(result)) {
        directInputGamepad_.Reset();
        return;
    }

    result = directInputGamepad_->SetCooperativeLevel(
        winApp_->GetHwnd(),
        DISCL_FOREGROUND | DISCL_NONEXCLUSIVE
    );
    if (FAILED(result)) {
        directInputGamepad_.Reset();
        return;
    }

    directInputGamepad_->EnumObjects(
        ConfigureGamepadAxis,
        directInputGamepad_.Get(),
        DIDFT_AXIS
    );
    directInputGamepad_->Acquire();
}


void Input::Update()
{
    memcpy(
        keyPre_,
        key_,
        sizeof(key_)
    );

    HRESULT result =
        keyboard_->GetDeviceState(
            sizeof(key_),
            key_
        );

    if (
        result == DIERR_INPUTLOST ||
        result == DIERR_NOTACQUIRED
        ) {
        result =
            keyboard_->Acquire();

        while (result == DIERR_INPUTLOST) {
            result =
                keyboard_->Acquire();
        }

        if (SUCCEEDED(result)) {
            result =
                keyboard_->GetDeviceState(
                    sizeof(key_),
                    key_
                );
        }
    }

    if (FAILED(result)) {
        ZeroMemory(
            key_,
            sizeof(key_)
        );
    }

    result = mouse_->GetDeviceState(
        sizeof(mouseState_),
        &mouseState_
    );

    if (result == DIERR_INPUTLOST || result == DIERR_NOTACQUIRED) {
        mouse_->Acquire();
        result = mouse_->GetDeviceState(
            sizeof(mouseState_),
            &mouseState_
        );
    }

    if (FAILED(result)) {
        ZeroMemory(&mouseState_, sizeof(mouseState_));
    }

    gamepadStatePre_ = gamepadState_;
    ZeroMemory(&gamepadState_, sizeof(gamepadState_));
    gamepadConnected_ =
        XInputGetState(0, &gamepadState_) == ERROR_SUCCESS;

    directInputGamepadStatePre_ = directInputGamepadState_;
    directInputGamepadConnected_ = false;
    if (directInputGamepad_) {
        HRESULT gamepadResult = directInputGamepad_->Poll();
        if (FAILED(gamepadResult)) {
            gamepadResult = directInputGamepad_->Acquire();
            while (gamepadResult == DIERR_INPUTLOST) {
                gamepadResult = directInputGamepad_->Acquire();
            }
        }

        if (SUCCEEDED(gamepadResult)) {
            gamepadResult = directInputGamepad_->GetDeviceState(
                sizeof(directInputGamepadState_),
                &directInputGamepadState_
            );
        }
        directInputGamepadConnected_ = SUCCEEDED(gamepadResult);
    }

    if (!gamepadConnected_ && !directInputGamepadConnected_) {
        ++gamepadReconnectCounter_;
        if (gamepadReconnectCounter_ >= 120) {
            gamepadReconnectCounter_ = 0;
            InitializeDirectInputGamepad();
        }
    } else {
        gamepadReconnectCounter_ = 0;
    }
}

bool Input::PushKey(BYTE keyNumber)
{
    return key_[keyNumber] != 0;
}

bool Input::TriggerKey(BYTE keyNumber)
{
    return
        key_[keyNumber] != 0 &&
        keyPre_[keyNumber] == 0;
}

namespace {
float NormalizeStickAxis(SHORT value, SHORT deadZone)
{
    const int raw = static_cast<int>(value);
    const int magnitude = std::abs(raw);
    if (magnitude <= deadZone) {
        return 0.0f;
    }

    const float normalized =
        static_cast<float>(magnitude - deadZone) /
        static_cast<float>(32767 - deadZone);
    return (raw < 0 ? -1.0f : 1.0f) *
        std::clamp(normalized, 0.0f, 1.0f);
}
}

float Input::GetLeftStickX() const
{
    if (gamepadConnected_) {
        return NormalizeStickAxis(
            gamepadState_.Gamepad.sThumbLX,
            XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
        );
    }
    if (directInputGamepadConnected_) {
        return NormalizeStickAxis(
            static_cast<SHORT>(directInputGamepadState_.lX),
            6553
        );
    }
    return 0.0f;
}

float Input::GetLeftStickY() const
{
    if (gamepadConnected_) {
        return NormalizeStickAxis(
            gamepadState_.Gamepad.sThumbLY,
            XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
        );
    }
    if (directInputGamepadConnected_) {
        return -NormalizeStickAxis(
            static_cast<SHORT>(directInputGamepadState_.lY),
            6553
        );
    }
    return 0.0f;
}

bool Input::PushGamepadButton(WORD button) const
{
    if (gamepadConnected_) {
        return (gamepadState_.Gamepad.wButtons & button) != 0;
    }

    if (directInputGamepadConnected_ &&
        button == XINPUT_GAMEPAD_A) {
        // The physical south button is commonly index 0 (Switch/standard
        // DirectInput) or index 1 (PlayStation DirectInput).
        return (directInputGamepadState_.rgbButtons[0] & 0x80) != 0 ||
            (directInputGamepadState_.rgbButtons[1] & 0x80) != 0;
    }
    return false;
}

bool Input::TriggerGamepadButton(WORD button) const
{
    if (gamepadConnected_) {
        return PushGamepadButton(button) &&
            (gamepadStatePre_.Gamepad.wButtons & button) == 0;
    }

    if (directInputGamepadConnected_ &&
        button == XINPUT_GAMEPAD_A) {
        const bool current =
            (directInputGamepadState_.rgbButtons[0] & 0x80) != 0 ||
            (directInputGamepadState_.rgbButtons[1] & 0x80) != 0;
        const bool previous =
            (directInputGamepadStatePre_.rgbButtons[0] & 0x80) != 0 ||
            (directInputGamepadStatePre_.rgbButtons[1] & 0x80) != 0;
        return current && !previous;
    }
    return false;
}
