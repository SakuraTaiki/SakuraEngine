// ============================================================================
// ファイルの役割: ゲーム内BGM・SEとデバッグ再生UIを管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "SoundController.h"
#include "Input.h"

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void SoundController::Initialize() {
    sound_.Initialize();

    wavSoundData_ =
        sound_.SoundLoadFile("Resources/Sound/Alarm01.wav");

    mp4SoundData_ =
        sound_.SoundLoadFile("Resources/Sound/AlarmMovie.mp4");

    mp3SoundData_ =
        sound_.SoundLoadFile("Resources/Sound/maou_bgm_neorock83.mp3");
}

// 処理概要: 所有しているリソースと実行状態を安全に終了する。
// 注意事項: 再初期化やアプリ終了時に参照を残さない。
void SoundController::Finalize() {
    sound_.Finalize();
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void SoundController::Update(Input* input) {
    if (!input) {
        return;
    }

    if (input->TriggerKey(DIK_M)) {
        sound_.SoundPlay(mp4SoundData_, mp4Volume_);
    }

    if (input->TriggerKey(DIK_N)) {
        sound_.SoundPlay(mp3SoundData_, mp3Volume_);
    }

    if (input->TriggerKey(DIK_UP)) {
        mp3Volume_ += 0.1f;

        if (mp3Volume_ > 1.0f) {
            mp3Volume_ = 1.0f;
        }
    }

    if (input->TriggerKey(DIK_DOWN)) {
        mp3Volume_ -= 0.1f;

        if (mp3Volume_ < 0.0f) {
            mp3Volume_ = 0.0f;
        }
    }
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void SoundController::DrawImGui() {
#ifdef USE_IMGUI
    ImGui::Text("Sound Volume");
    ImGui::Separator();

    ImGui::SliderFloat("Wav Volume", &wavVolume_, 0.0f, 1.0f);
    ImGui::SliderFloat("Mp4 Volume", &mp4Volume_, 0.0f, 1.0f);
    ImGui::SliderFloat("Mp3 Volume", &mp3Volume_, 0.0f, 1.0f);

    ImGui::Spacing();
    ImGui::TextDisabled("M key : Play mp4");
    ImGui::TextDisabled("N key : Play mp3");
    ImGui::TextDisabled("UP / DOWN : Change mp3 volume");
#endif
}