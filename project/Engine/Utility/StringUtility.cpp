// ============================================================================
// ファイルの役割: 文字コード変換など文字列処理の共通機能を提供する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "StringUtility.h"
#include <cstdlib>
#include <cwchar>

namespace StringUtility {
	//stringをwstringに変換する
	std::wstring ConvertString(const std::string& str) {
		size_t size = str.size();
		std::wstring wstr(size, L' ');
		mbstowcs_s(nullptr, &wstr[0], size + 1, str.c_str(), size);
		return wstr;
	}
	//wstringをstringに変換する
	std::string ConvertString(const std::wstring& str) {
		size_t size = str.size();
		std::string sstr(size, ' ');
		wcstombs_s(nullptr, &sstr[0], size + 1, str.c_str(), size);
		return sstr;
	}
};
