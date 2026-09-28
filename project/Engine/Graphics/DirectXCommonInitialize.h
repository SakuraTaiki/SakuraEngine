// ============================================================================
// ファイルの役割: DirectXCommonの責務を機能単位に分割し、初期化・描画・リソース操作を実装する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void DirectXCommon::Initialize(WinApp* winApp) {
    assert(winApp);

    winApp_ =
        winApp;

    width_ =
        static_cast<uint32_t>(
            (std::max)(
                winApp_->GetWidth(),
                1
                )
            );

    height_ =
        static_cast<uint32_t>(
            (std::max)(
                winApp_->GetHeight(),
                1
                )
            );

    fpsLimiter_.Initialize();

    InitializeDevice();
    InitializeCommand();
    InitializeSwapChain();
    InitializeRenderTargetView();
    InitializeDepthStencilView();
    InitializeFence();

    shaderCompiler_.Initialize();

    InitializeRenderTexture();
    InitializeCopyImagePipeline();

    // 蛻晄悄繧ｦ繧｣繝ｳ繝峨え逕滓・譎ゅ・WM_SIZE繧呈ｶ郁ｲｻ
    uint32_t ignoredWidth = 0;
    uint32_t ignoredHeight = 0;

    winApp_->ConsumeResize(
        ignoredWidth,
        ignoredHeight
    );

}
