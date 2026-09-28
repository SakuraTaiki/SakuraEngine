// ============================================================================
// ファイルの役割: 2DスプライトのTransform、UV、Material、頂点情報を更新・描画する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "Sprite.h"
#include "MyMath.h" // 必ずインクルード
#include "D3DResourceHelper.h"
#include <cassert>

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void Sprite::Initialize(SpriteCommon* spriteCommon, uint32_t textureHandle) {
    assert(spriteCommon);
    spriteCommon_ = spriteCommon;
    textureHandle_ = textureHandle;

    // テクスチャ情報から初期サイズを設定
    auto& desc = spriteCommon_->GetTextureManager()->GetResourceDesc(textureHandle_);
    size_ = { (float)desc.Width, (float)desc.Height };
    textureSize_ = size_; // 初期状態は全範囲
    textureLeftTop_ = { 0.0f, 0.0f };

    CreateVertexBuffer();
    CreateMaterialBuffer();
    CreateTransformationMatrixBuffer();

    // 初期データを転送
    UpdateVertexData();
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void Sprite::Update() {
    // 頂点情報に変更があれば更新
    if (transferNeeded_) {
        UpdateVertexData();
        transferNeeded_ = false;
    }

    // 行列計算 (MyMathの関数を使用)
    // 名前空間 Math:: をつけ、関数名を合わせる
    Matrix4x4 scaleMat = Math::Matrix4x4MakeScaleMatrix({ 1.0f, 1.0f, 1.0f });
    Matrix4x4 rotateMat = Math::MakeRotateZMatrix(rotation_);
    Matrix4x4 translateMat = Math::MakeTranslateMatrix({ position_.x, position_.y, 0.0f });

    // 行列の掛け算
    Matrix4x4 worldMatrix = Math::Multiply(scaleMat, Math::Multiply(rotateMat, translateMat));

    // ビュープロジェクション行列（正射影）
    // 画面サイズ 1280x720 を想定
    Matrix4x4 viewMatrix = Math::MakeIdentity4x4();
    Matrix4x4 projectionMatrix = Math::MakeOrthographicMatrix(0.0f, 0.0f, 1280.0f, 720.0f, 0.0f, 100.0f);

    Matrix4x4 wvpMatrix = Math::Multiply(worldMatrix, Math::Multiply(viewMatrix, projectionMatrix));

    transformationMatrixData_->WVP = wvpMatrix;
}

// 処理概要: 更新済みの状態を使用して、担当する表示またはデバッグUIを描画する。
// 注意事項: GPUリソースと描画パイプラインが初期化済みであることを前提とする。
void Sprite::Draw() {
    // コマンドリスト取得
    // DirectXCommonに GetCommandList() を追加している前提
    auto commandList = spriteCommon_->GetDxCommon()->GetCommandList();

    // 1. VBVセット
    commandList->IASetVertexBuffers(0, 1, &vertexBufferView_);

    // 2. マテリアルCBV
    commandList->SetGraphicsRootConstantBufferView(0, materialResource_->GetGPUVirtualAddress());

    // 3. トランスフォームCBV
    commandList->SetGraphicsRootConstantBufferView(1, transformationMatrixResource_->GetGPUVirtualAddress());

    // 4. テクスチャSRV
    auto gpuHandle = spriteCommon_->GetTextureManager()->GetSrvHandleGPU(textureHandle_);
    commandList->SetGraphicsRootDescriptorTable(2, gpuHandle);

    // 5. 描画 (6頂点)
    commandList->DrawInstanced(6, 1, 0, 0);
}

// 処理概要: 外部から渡された値を、担当オブジェクトの状態へ反映する。
// 注意事項: 必要に応じて範囲制限や依存データの再計算も行う。
void Sprite::SetTextureRect(const Vector2& position, const Vector2& size) {
    textureLeftTop_ = position;
    textureSize_ = size;
    // size_ = size; // 切り抜きサイズに合わせて表示サイズも変えたい場合はコメントアウトを外す
    transferNeeded_ = true;
}

// 処理概要: 外部から渡された値を、担当オブジェクトの状態へ反映する。
// 注意事項: 必要に応じて範囲制限や依存データの再計算も行う。
void Sprite::SetTexture(uint32_t textureHandle) {
    textureHandle_ = textureHandle;
    auto& desc = spriteCommon_->GetTextureManager()->GetResourceDesc(textureHandle_);
    textureSize_ = { (float)desc.Width, (float)desc.Height };
    transferNeeded_ = true;
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void Sprite::CreateVertexBuffer() {
    auto device = spriteCommon_->GetDxCommon()->GetDevice();

    // 頂点リソース作成
    const size_t bufferSize =
        sizeof(VertexData) * 6;

    vertexBuffer_ =
        D3DResourceHelper::CreateUploadBuffer(
            device,
            bufferSize
        );

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = static_cast<UINT>(bufferSize);
    vertexBufferView_.StrideInBytes = sizeof(VertexData);
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void Sprite::CreateMaterialBuffer() {
    auto device = spriteCommon_->GetDxCommon()->GetDevice();
    size_t sizeIB =
        D3DResourceHelper::AlignConstantBufferSize(
            sizeof(Material)
        );

    materialResource_ =
        D3DResourceHelper::CreateUploadBuffer(
            device,
            sizeIB
        );

    materialData_ =
        D3DResourceHelper::Map<Material>(
            materialResource_.Get()
        );

    materialData_->color = { 1.0f, 1.0f, 1.0f, 1.0f };
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void Sprite::CreateTransformationMatrixBuffer() {
    auto device = spriteCommon_->GetDxCommon()->GetDevice();
    size_t sizeIB =
        D3DResourceHelper::AlignConstantBufferSize(
            sizeof(TransformationMatrix)
        );

    transformationMatrixResource_ =
        D3DResourceHelper::CreateUploadBuffer(
            device,
            sizeIB
        );

    transformationMatrixData_ =
        D3DResourceHelper::Map<TransformationMatrix>(
            transformationMatrixResource_.Get()
        );

    transformationMatrixData_->WVP = Math::MakeIdentity4x4();
}

// 処理概要: フレーム入力と経過時間を反映し、担当する状態を更新する。
// 注意事項: 描画前に呼び出し、前フレームの状態との順序を保つ。
void Sprite::UpdateVertexData() {
    VertexData* vertMap = nullptr;
    vertexBuffer_->Map(0, nullptr, (void**)&vertMap);

    // テクスチャ全体のサイズ取得
    auto& texDesc = spriteCommon_->GetTextureManager()->GetResourceDesc(textureHandle_);
    float texWidth = (float)texDesc.Width;
    float texHeight = (float)texDesc.Height;

    // UV計算
    float left = textureLeftTop_.x / texWidth;
    float right = (textureLeftTop_.x + textureSize_.x) / texWidth;
    float top = textureLeftTop_.y / texHeight;
    float bottom = (textureLeftTop_.y + textureSize_.y) / texHeight;

    // 頂点座標計算（アンカーポイント考慮）
    float leftPos = 0.0f - (anchorPoint_.x * size_.x);
    float rightPos = size_.x - (anchorPoint_.x * size_.x);
    float topPos = 0.0f - (anchorPoint_.y * size_.y);
    float bottomPos = size_.y - (anchorPoint_.y * size_.y);

    // 6頂点
    // Triangle 1
    vertMap[0].position = { leftPos, topPos, 0.0f, 1.0f };
    vertMap[0].texcoord = { left, top };
    vertMap[1].position = { leftPos, bottomPos, 0.0f, 1.0f };
    vertMap[1].texcoord = { left, bottom };
    vertMap[2].position = { rightPos, topPos, 0.0f, 1.0f };
    vertMap[2].texcoord = { right, top };

    // Triangle 2
    vertMap[3].position = { leftPos, bottomPos, 0.0f, 1.0f };
    vertMap[3].texcoord = { left, bottom };
    vertMap[4].position = { rightPos, bottomPos, 0.0f, 1.0f };
    vertMap[4].texcoord = { right, bottom };
    vertMap[5].position = { rightPos, topPos, 0.0f, 1.0f };
    vertMap[5].texcoord = { right, top };

    vertexBuffer_->Unmap(0, nullptr);
}
