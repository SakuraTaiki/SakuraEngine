// ============================================================================
// ファイルの役割: テクスチャの読み込み、GPU転送、SRV割り当て、再利用を管理する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <wrl.h>
#include <d3d12.h>
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "MyMath.h" // Vector2などが定義されている前提

class TextureManager {
public:
    // シングルトンなどの管理ではなく、SpriteCommonが所有する形にします
    void Initialize(DirectXCommon* dxCommon,SrvManager*srvmanager);

    // テクスチャ読み込み（読み込み済みなら既存のハンドルを返す）
    // return: テクスチャハンドル（uint32_t）
    uint32_t LoadTexture(const std::string& filePath);

    // 指定ハンドルのGPUハンドルを取得（コマンドリストセット用）
    D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureHandle);

    // テクスチャのサイズを取得（切り抜き計算用）
    const D3D12_RESOURCE_DESC& GetResourceDesc(uint32_t textureHandle);

    D3D12_CPU_DESCRIPTOR_HANDLE GetSrvHandleCPU(
        uint32_t textureHandle
    );

private:
    // 内部用：テクスチャデータ構造体
    struct TextureData {
        Microsoft::WRL::ComPtr<ID3D12Resource> resource;
        Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource;
        uint32_t srvIndex = 0;
        D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
        D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
        D3D12_RESOURCE_DESC resourceDesc;
    };

    DirectXCommon* dxCommon_ = nullptr;

    SrvManager* srvManager_ = nullptr;
   
    // テクスチャデータ一覧
    std::vector<TextureData> textures_;
    // ファイルパスとインデックスの対応マップ
    std::unordered_map<std::string, uint32_t> fileMap_;

    // 最大テクスチャ数
    static const size_t kMaxTextures = 128;
};