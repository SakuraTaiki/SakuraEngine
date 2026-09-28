// ============================================================================
// ファイルの役割: 外部JSONライブラリをVisual Studio向けの警告設定と共に読み込む。
// 構成上の位置付け: ゲーム側コードとnlohmann/jsonの間に置く共通ラッパー。
// 実装時の注意: 抑制対象は外部ライブラリ内だけに限定し、自作コードの解析警告は残す。
// ============================================================================
#pragma once

// nlohmann/jsonは複数のコンパイラに対応する外部ライブラリであり、
// Visual Studioのコード分析では、意図されたswitchのフォールスルーと
// union相当の内部データ管理に対してC26819・C26495が報告される。
// ライブラリ本体を直接変更せず、読み込み中だけ該当警告を無効化する。
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 26495 26819)
#endif

#include "externals/json/json.hpp"

#if defined(_MSC_VER)
#pragma warning(pop)
#endif
