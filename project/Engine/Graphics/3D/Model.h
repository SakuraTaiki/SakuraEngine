// ============================================================================
// ファイルの役割: 3Dモデルの頂点・マテリアル・ノード・アニメーション関連データを保持する。
// 構成上の位置付け: 公開インターフェース、関連データ型、保持する状態を宣言する。
// 実装時の注意: 所有権と初期化順序が分かるよう、実装変更時は対応する.cppとの整合性を保つ。
// ============================================================================
#pragma once
#include "DirectXCommon.h"
#include "TextureManager.h"
#include "MyMath.h"
#include <string>
#include <vector>
#include<map>
#include<array>

// 頂点データ構造体 (ShaderのInputLayoutに合わせる)
struct ModelVertexData {
    Vector4 position;
    Vector2 texcoord;
    Vector3 normal;
};

struct VertexWeightData
{
    float weight;
    uint32_t vertexIndex;
};

struct JointWeightData
{
    Matrix4x4 inverseBindPoseMatrix;
    std::vector<VertexWeightData> vertexWeights;
};

struct QuaternionTransform {
    Vector3 scale;
    Quaternion rotate;
    Vector3 translate;
};

struct Node {
    QuaternionTransform transform;
    Matrix4x4 localMatrix;
    std::string name;
    std::vector<Node> children;
};

struct aiNode;

class Model {
public:
    // OBJファイルからモデル生成 (ディレクトリパスとファイル名を分けて渡す)
    static Model* CreateFromOBJ(DirectXCommon* dxCommon, const std::string& directoryPath, const std::string& filename, TextureManager* textureManager);

    void Initialize(DirectXCommon* dxCommon, const std::string& directoryPath, const std::string& filename, TextureManager* textureManager);
    void Draw(
        ID3D12GraphicsCommandList* commandList,
        const D3D12_VERTEX_BUFFER_VIEW* overrideVertexBufferView = nullptr
        );

    // ゲッター
    uint32_t GetTextureHandle() const { return textureHandle_; }

    const Node& GetRootNode() const { return rootNode_; }

    const std::map<std::string, JointWeightData>& GetSkinClusterData() const {
        return skinClusterData_;
    }

    size_t GetVertexCount() const { return vertices_.size(); }
    ID3D12Resource* GetVertexBufferResource() const {
        return vertexBuffer_.Get();
    }
    bool StretchVertexX(size_t vertexIndex, float amount);

private:
    void LoadObjFile(const std::string& directoryPath, const std::string& filename);
    void CreateBuffers(DirectXCommon* dxCommon);
    std::string textureFilePath_;
    std::map<std::string, JointWeightData> skinClusterData_;
private:
    std::vector<ModelVertexData> vertices_;
    std::vector<uint32_t> indices_;
    uint32_t textureHandle_ = 0;

    Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer_;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView_{};

    Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer_;
    D3D12_INDEX_BUFFER_VIEW indexBufferView_{};

    Node ReadNode(aiNode* node);
    Node rootNode_;
};
