// ============================================================================
// ファイルの役割: HLSLをDXCでコンパイルし、診断情報とバイトコードを取得する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "DirectXShaderCompiler.h"

#include <cassert>

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void DirectXShaderCompiler::Initialize() {
    HRESULT hr = DxcCreateInstance(
        CLSID_DxcUtils,
        IID_PPV_ARGS(&dxcUtils_)
    );
    assert(SUCCEEDED(hr));

    hr = DxcCreateInstance(
        CLSID_DxcCompiler,
        IID_PPV_ARGS(&dxcCompiler_)
    );
    assert(SUCCEEDED(hr));

    hr = dxcUtils_->CreateDefaultIncludeHandler(&includeHandler_);
    assert(SUCCEEDED(hr));
}

DirectXShaderCompiler::ComPtr<IDxcBlob> DirectXShaderCompiler::Compile(
    const std::wstring& filePath,
    const wchar_t* profile
) {
    OutputDebugStringW(L"----------------------------------------\n");
    OutputDebugStringW(L"Begin CompileShader: ");
    OutputDebugStringW(filePath.c_str());
    OutputDebugStringW(L"\n");

    // HLSL ファイルを読み込む。
    ComPtr<IDxcBlobEncoding> shaderSource = nullptr;

    HRESULT hr = dxcUtils_->LoadFile(
        filePath.c_str(),
        nullptr,
        &shaderSource
    );

    if (FAILED(hr)) {
        OutputDebugStringA("ERROR: Failed to load shader file.\n");
        OutputDebugStringW(filePath.c_str());
        OutputDebugStringA("\n----------------------------------------\n");

        assert(false && "Shader File Not Found!");
        return nullptr;
    }

    DxcBuffer shaderSourceBuffer{};
    shaderSourceBuffer.Ptr = shaderSource->GetBufferPointer();
    shaderSourceBuffer.Size = shaderSource->GetBufferSize();
    shaderSourceBuffer.Encoding = DXC_CP_UTF8;

    // main 関数を指定して、指定 profile でコンパイルする。
    LPCWSTR arguments[] = {
        filePath.c_str(),
        L"-E", L"main",
        L"-T", profile,
        L"-Zi", L"-Qembed_debug",
        L"-Od",
        L"-Zpr",
    };

    ComPtr<IDxcResult> shaderResult = nullptr;

    hr = dxcCompiler_->Compile(
        &shaderSourceBuffer,
        arguments,
        _countof(arguments),
        includeHandler_.Get(),
        IID_PPV_ARGS(&shaderResult)
    );

    if (FAILED(hr)) {
        OutputDebugStringA("ERROR: DxcCompiler::Compile failed.\n");
        assert(false);
        return nullptr;
    }

    // コンパイルエラーがあれば表示して止める。
    ComPtr<IDxcBlobUtf8> shaderError = nullptr;

    shaderResult->GetOutput(
        DXC_OUT_ERRORS,
        IID_PPV_ARGS(&shaderError),
        nullptr
    );

    if (shaderError != nullptr && shaderError->GetStringLength() != 0) {
        std::string errorMsg = shaderError->GetStringPointer();

        OutputDebugStringA("----------------------------------------\n");
        OutputDebugStringA("HLSL Compile Error:\n");
        OutputDebugStringA(errorMsg.c_str());
        OutputDebugStringA("----------------------------------------\n");

        assert(false && "Shader Compile Error");
    }

    ComPtr<IDxcBlob> shaderBlob = nullptr;

    hr = shaderResult->GetOutput(
        DXC_OUT_OBJECT,
        IID_PPV_ARGS(&shaderBlob),
        nullptr
    );
    assert(SUCCEEDED(hr));

    OutputDebugStringA("CompileShader Success!\n");
    OutputDebugStringA("----------------------------------------\n");

    return shaderBlob;
}