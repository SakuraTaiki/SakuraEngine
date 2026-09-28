// ============================================================================
// ファイルの役割: フレーム時間を計測し、目標フレームレートへ実行速度を調整する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "FpsLimiter.h"

#include <thread>

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void FpsLimiter::Initialize() {
    reference_ = std::chrono::steady_clock::now();
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void FpsLimiter::Update() {
    using namespace std::chrono;

    const microseconds kMinTime(
        static_cast<int64_t>(1000000.0f / 60.0f)
    );

    const microseconds kMinCheckTime(
        static_cast<int64_t>(1000000.0f / 65.0f)
    );

    steady_clock::time_point now = steady_clock::now();
    microseconds elapsed = duration_cast<microseconds>(now - reference_);

    // かなり早く処理が終わったときだけ待機する。
    if (elapsed < kMinCheckTime) {
        while (steady_clock::now() - reference_ < kMinTime) {
            std::this_thread::sleep_for(microseconds(1));
        }
    }

    reference_ = steady_clock::now();
}