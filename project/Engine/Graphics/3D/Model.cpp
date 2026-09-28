// ============================================================================
// ファイルの役割: 3Dモデルの頂点・マテリアル・ノード・アニメーション関連データを保持する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "Model.h"
#include <fstream>
#include <sstream>
#include <cassert>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <filesystem>

using namespace Microsoft::WRL;

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
Model* Model::CreateFromOBJ(DirectXCommon* dxCommon, const std::string& directoryPath, const std::string& filename, TextureManager* textureManager) {
    Model* model = new Model();
    model->Initialize(dxCommon, directoryPath, filename, textureManager);
    return model;
}

// 処理概要: 利用する依存オブジェクトとGPU・ゲーム状態を初期化する。
// 注意事項: 他の更新・描画処理より先に一度だけ呼び出す。
void Model::Initialize(DirectXCommon* dxCommon, const std::string& directoryPath, const std::string& filename, TextureManager* textureManager) {
    // 1. OBJ読み込み
    LoadObjFile(directoryPath, filename);

    // ★安全策：頂点が一つもない場合はバッファを作らない
    if (vertices_.empty()) {
        OutputDebugStringA("Error: Model vertices are empty!\n");
        return;
    }

    // 2. バッファ生成
    CreateBuffers(dxCommon);

    if (textureManager) {
        if (!textureFilePath_.empty()) {
            textureHandle_ = textureManager->LoadTexture(textureFilePath_);
        } else {
            textureHandle_ = textureManager->LoadTexture("Resources/white.png");
        }
    }
}

// 処理概要: Modelが担当する「ReadNode」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
Node Model::ReadNode(aiNode* node) {
    Node result{};

    aiVector3D scale, translate;
    aiQuaternion rotate;

    node->mTransformation.Decompose(scale, rotate, translate);

    result.transform.scale = { scale.x, scale.y, scale.z };
    result.transform.rotate = { rotate.x, -rotate.y, -rotate.z, rotate.w };
    result.transform.translate = { -translate.x, translate.y, translate.z };

    result.localMatrix =
        Math::MakeAffineMatrix(
            result.transform.scale,
            result.transform.rotate,
            result.transform.translate);

    result.name = node->mName.C_Str();

    result.children.resize(node->mNumChildren);
    for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex) {
        result.children[childIndex] = ReadNode(node->mChildren[childIndex]);
    }

    return result;
}

void Model::LoadObjFile(const std::string& directoryPath,
    const std::string& filename)
{
    vertices_.clear();

    indices_.clear();

    skinClusterData_.clear();

    Assimp::Importer importer;

    std::string filePath = directoryPath + "/" + filename;

    const aiScene* scene = importer.ReadFile(
        filePath.c_str(),
        aiProcess_FlipWindingOrder |
        aiProcess_FlipUVs |
        aiProcess_Triangulate |
        aiProcess_GenSmoothNormals
    );

    assert(scene->HasMeshes());

    rootNode_ = ReadNode(scene->mRootNode);

    //------------------------------------
    // Mesh解析
    //------------------------------------

    for (uint32_t meshIndex = 0;
        meshIndex < scene->mNumMeshes;
        ++meshIndex)
    {
        aiMesh* mesh = scene->mMeshes[meshIndex];

        assert(mesh->HasNormals());

        uint32_t vertexOffset = static_cast<uint32_t>(vertices_.size());

        //--------------------------------
        // Vertex解析
        //--------------------------------
        vertices_.resize(vertices_.size() + mesh->mNumVertices);

        for (uint32_t vertexIndex = 0;
            vertexIndex < mesh->mNumVertices;
            ++vertexIndex)
        {
            aiVector3D position = mesh->mVertices[vertexIndex];
            aiVector3D normal = mesh->mNormals[vertexIndex];

            ModelVertexData vertex{};

            vertex.position = {
                position.x,
                position.y,
                position.z,
                1.0f
            };

            vertex.normal = {
                normal.x,
                normal.y,
                normal.z
            };

            if (mesh->HasTextureCoords(0)) {
                aiVector3D texcoord = mesh->mTextureCoords[0][vertexIndex];
                vertex.texcoord = {
                    texcoord.x,
                    texcoord.y
                };
            } else {
                vertex.texcoord = {
                    position.x,
                    position.z
                };
            }

            // 左手系変換
            vertex.position.x *= -1.0f;
            vertex.normal.x *= -1.0f;

            vertices_[vertexOffset + vertexIndex] = vertex;
        }

        //--------------------------------
        // Index解析
        //--------------------------------
        for (uint32_t faceIndex = 0;
            faceIndex < mesh->mNumFaces;
            ++faceIndex)
        {
            aiFace& face = mesh->mFaces[faceIndex];

            assert(face.mNumIndices == 3);

            for (uint32_t element = 0;
                element < face.mNumIndices;
                ++element)
            {
                uint32_t vertexIndex = face.mIndices[element];

                indices_.push_back(vertexOffset + vertexIndex);
            }
        }

        //--------------------------------
        // SkinCluster用 Bone解析
        //--------------------------------
        for (uint32_t boneIndex = 0;
            boneIndex < mesh->mNumBones;
            ++boneIndex)
        {
            aiBone* bone = mesh->mBones[boneIndex];

            std::string jointName = bone->mName.C_Str();

            JointWeightData& jointWeightData =
                skinClusterData_[jointName];

            //--------------------------------
            // InverseBindPoseMatrixの抽出
            //--------------------------------
            aiMatrix4x4 bindPoseMatrixAssimp =
                bone->mOffsetMatrix.Inverse();

            aiVector3D scale;
            aiVector3D translate;
            aiQuaternion rotate;

            bindPoseMatrixAssimp.Decompose(
                scale,
                rotate,
                translate
            );

            Matrix4x4 bindPoseMatrix =
                Math::MakeAffineMatrix(
                    { scale.x, scale.y, scale.z },
                    { rotate.x, -rotate.y, -rotate.z, rotate.w },
                    { -translate.x, translate.y, translate.z }
                );

            jointWeightData.inverseBindPoseMatrix =
                Math::Inverse(bindPoseMatrix);

            //--------------------------------
            // Weight情報の抽出
            //--------------------------------
            for (uint32_t weightIndex = 0;
                weightIndex < bone->mNumWeights;
                ++weightIndex)
            {
                uint32_t localVertexIndex =
                    bone->mWeights[weightIndex].mVertexId;

                float weight =
                    bone->mWeights[weightIndex].mWeight;

                jointWeightData.vertexWeights.push_back(
                    {
                        weight,
                        vertexOffset + localVertexIndex
                    }
                );
            }
        }


    }

    OutputDebugStringA(
        ("Loaded : " + filename +
            " VertexCount = " +
            std::to_string(vertices_.size()) + "\n").c_str());

    // ------------------------------------
    // Material解析
    // ------------------------------------
    textureFilePath_.clear();

    for (uint32_t materialIndex = 0;
        materialIndex < scene->mNumMaterials;
        ++materialIndex)
    {
        aiMaterial* material = scene->mMaterials[materialIndex];

        if (material->GetTextureCount(aiTextureType_DIFFUSE) == 0) {
            continue;
        }

        aiString textureFilePath;
        material->GetTexture(
            aiTextureType_DIFFUSE,
            0,
            &textureFilePath
        );

        std::string path = directoryPath + "/" + textureFilePath.C_Str();

        // パス区切りをWindowsでも安全にする
        std::replace(path.begin(), path.end(), '\\', '/');

        // 実在するテクスチャだけ採用する
        if (std::filesystem::exists(path)) {
            textureFilePath_ = path;
            break;
        }
    }
}

// 処理概要: 担当機能で使用するオブジェクトまたはGPUリソースを生成する。
// 注意事項: 生成条件、所有者、破棄タイミングを明確にする。
void Model::CreateBuffers(DirectXCommon* dxCommon) {
    auto device = dxCommon->GetDevice();

    UINT vertexBufferSize =
        static_cast<UINT>(sizeof(ModelVertexData) * vertices_.size());

    UINT indexBufferSize =
        static_cast<UINT>(sizeof(uint32_t) * indices_.size());

    D3D12_HEAP_PROPERTIES heapProps{};
    heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;

    D3D12_RESOURCE_DESC resDesc{};
    resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    resDesc.Height = 1;
    resDesc.DepthOrArraySize = 1;
    resDesc.MipLevels = 1;
    resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    resDesc.SampleDesc.Count = 1;

    HRESULT hr;

    //--------------------------------
    // VertexBuffer
    //--------------------------------
    resDesc.Width = vertexBufferSize;

    hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&vertexBuffer_)
    );

    assert(SUCCEEDED(hr));

    ModelVertexData* vertexMap = nullptr;
    vertexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&vertexMap));
    std::copy(vertices_.begin(), vertices_.end(), vertexMap);
    vertexBuffer_->Unmap(0, nullptr);

    vertexBufferView_.BufferLocation = vertexBuffer_->GetGPUVirtualAddress();
    vertexBufferView_.SizeInBytes = vertexBufferSize;
    vertexBufferView_.StrideInBytes = sizeof(ModelVertexData);

    //--------------------------------
    // IndexBuffer
    //--------------------------------
    resDesc.Width = indexBufferSize;

    hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&indexBuffer_)
    );

    assert(SUCCEEDED(hr));

    uint32_t* indexMap = nullptr;
    indexBuffer_->Map(0, nullptr, reinterpret_cast<void**>(&indexMap));
    std::copy(indices_.begin(), indices_.end(), indexMap);
    indexBuffer_->Unmap(0, nullptr);

    indexBufferView_.BufferLocation = indexBuffer_->GetGPUVirtualAddress();
    indexBufferView_.SizeInBytes = indexBufferSize;
    indexBufferView_.Format = DXGI_FORMAT_R32_UINT;
}

void Model::Draw(
    ID3D12GraphicsCommandList* commandList,
    const D3D12_VERTEX_BUFFER_VIEW* overrideVertexBufferView
    ) {
    if (!vertexBuffer_ || !indexBuffer_) {
        return;
    }

    const D3D12_VERTEX_BUFFER_VIEW* vertexBufferView =
        overrideVertexBufferView
        ? overrideVertexBufferView
        : &vertexBufferView_;
    commandList->IASetVertexBuffers(0, 1, vertexBufferView);

    commandList->IASetIndexBuffer(&indexBufferView_);

    commandList->DrawIndexedInstanced(
        static_cast<UINT>(indices_.size()),
        1,
        0,
        0,
        0
    );
}

// 処理概要: Modelが担当する「StretchVertexX」処理を実行する。
// 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
bool Model::StretchVertexX(size_t vertexIndex, float amount) {
    if (vertexIndex >= vertices_.size() || !vertexBuffer_) {
        return false;
    }

    vertices_[vertexIndex].position.x += amount;

    ModelVertexData* vertexMap = nullptr;
    const HRESULT hr = vertexBuffer_->Map(
        0,
        nullptr,
        reinterpret_cast<void**>(&vertexMap)
    );

    if (FAILED(hr) || !vertexMap) {
        return false;
    }

    std::copy(vertices_.begin(), vertices_.end(), vertexMap);
    vertexBuffer_->Unmap(0, nullptr);
    return true;
}
