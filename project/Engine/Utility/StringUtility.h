// ============================================================================
// ファイルの役割: 文字コード変換など文字列処理の共通機能を提供する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
#pragma once
#include <string>

namespace StringUtility {
	//stringをwstringに変換する
	std::wstring ConvertString(const std::string& str);

	//wstringをstringに変換する
	std::string ConvertString(const std::wstring& str);
};