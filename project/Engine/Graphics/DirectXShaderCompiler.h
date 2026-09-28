// ============================================================================
// ファイルの役割: HLSLをDXCでコンパイルし、診断情報とバイトコードを取得する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once

// DXCが使用するWindows型とSAL注釈を先に定義する
#include <Windows.h>
#include <dxcapi.h>
#include <string>
#include <wrl.h>

#pragma comment(lib, "dxcompiler.lib")

// HLSL のコンパイルだけを担当するクラス。
// DirectXCommon から DXC 関連の処理を分離する。
class DirectXShaderCompiler {
public:
    template <class T>
    using ComPtr = Microsoft::WRL::ComPtr<T>;

    void Initialize();

    ComPtr<IDxcBlob> Compile(
        const std::wstring& filePath,
        const wchar_t* profile
    );

private:
    // DXC 用オブジェクト。
    ComPtr<IDxcUtils> dxcUtils_;
    ComPtr<IDxcCompiler3> dxcCompiler_;
    ComPtr<IDxcIncludeHandler> includeHandler_;
};