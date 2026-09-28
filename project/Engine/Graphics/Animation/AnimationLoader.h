// ============================================================================
// ファイルの役割: モデルファイルからアニメーションデータを読み込み、エンジン形式へ変換する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

#include <string>

#include "Animation.h"

// Animation ファイル読み込み専用クラス。
// Assimp への依存を Animation 本体から分離する。
class AnimationLoader {
public:
    static Animation Load(
        const std::string& directoryPath,
        const std::string& filename
    );
};