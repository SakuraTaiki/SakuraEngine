// ============================================================================
// ファイルの役割: ゲーム内BGM・SEとデバッグ再生UIを管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include "Sound.h"

class Input;

class SoundController {
public:
    void Initialize();
    void Finalize();

    void Update(Input* input);
    void DrawImGui();

private:
    Sound sound_;

    Sound::SoundData wavSoundData_{};
    Sound::SoundData mp4SoundData_{};
    Sound::SoundData mp3SoundData_{};

    float wavVolume_ = 0.5f;
    float mp4Volume_ = 0.5f;
    float mp3Volume_ = 0.5f;
};