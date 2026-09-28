// ============================================================================
// ファイルの役割: ヒットエフェクトのプリセット、保存・読込、実行時再生を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
// 処理概要: HitEffectControllerが担当する「RefreshPresetList」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
void HitEffectController::RefreshPresetList() {
    namespace fs = std::filesystem;

    const fs::path directory =
        "Resources/Settings/HitEffects";

    fs::create_directories(directory);

    presetNames_.clear();

    for (
        const fs::directory_entry& entry :
        fs::directory_iterator(directory)
        ) {
        if (!entry.is_regular_file()) {
            continue;
        }

        if (entry.path().extension() != ".txt") {
            continue;
        }

        presetNames_.push_back(
            entry.path().stem().string()
        );
    }

    std::sort(
        presetNames_.begin(),
        presetNames_.end()
    );

    if (presetNames_.empty()) {
        selectedPreset_ = 0;
    } else {
        selectedPreset_ =
            std::clamp(
                selectedPreset_,
                0,
                static_cast<int>(presetNames_.size()) - 1
            );
    }
}
