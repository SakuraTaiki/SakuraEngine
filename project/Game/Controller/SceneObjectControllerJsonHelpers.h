// ============================================================================
// ファイルの役割: シーン配置オブジェクトの生成、更新、JSON保存・読込を管理する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
namespace {

    Vector3 ConvertBlenderEulerDegreesToEngine(
        float xDegrees,
        float yDegrees,
        float zDegrees
    )
    {
        constexpr float kDegreesToRadians = 0.017453292519943295f;
        const float x = xDegrees * kDegreesToRadians;
        const float y = yDegrees * kDegreesToRadians;
        const float z = zDegrees * kDegreesToRadians;

        // Blender's XYZ Euler rotation expressed for this engine's row-vector
        // matrix convention.
        const Matrix4x4 blenderRotation = Math::Multiply(
            Math::MakeRotateXMatrix(x),
            Math::Multiply(
                Math::MakeRotateYMatrix(y),
                Math::MakeRotateZMatrix(z)
            )
        );

        // Model::LoadObjFile mirrors every imported OBJ vertex on local X.
        // Account for that existing model-space conversion here.
        Matrix4x4 modelImportConversion = Math::MakeIdentity4x4();
        modelImportConversion.m[0][0] = -1.0f;

        // Blender Z-up RH -> engine Y-up LH: (x, y, z) -> (x, z, y).
        Matrix4x4 coordinateConversion{};
        coordinateConversion.m[0][0] = 1.0f;
        coordinateConversion.m[1][2] = 1.0f;
        coordinateConversion.m[2][1] = 1.0f;
        coordinateConversion.m[3][3] = 1.0f;

        // vObj * XMirror * engineRotation ==
        // vObj * blenderRotation * coordinateConversion
        const Matrix4x4 engineRotation = Math::Multiply(
            Math::Multiply(modelImportConversion, blenderRotation),
            coordinateConversion
        );

        // Decompose the engine's Rx * Ry * Rz convention back to Euler.
        const float sinY = std::clamp(-engineRotation.m[0][2], -1.0f, 1.0f);
        const float engineY = std::asin(sinY);
        const float cosY = std::cos(engineY);

        float engineX = 0.0f;
        float engineZ = 0.0f;
        if (std::abs(cosY) > 0.000001f) {
            engineX = std::atan2(
                engineRotation.m[1][2],
                engineRotation.m[2][2]
            );
            engineZ = std::atan2(
                engineRotation.m[0][1],
                engineRotation.m[0][0]
            );
        } else {
            engineX = std::atan2(
                -engineRotation.m[2][1],
                engineRotation.m[1][1]
            );
        }

        return { engineX, engineY, engineZ };
    }

    bool ObjFileDeclaresObjectName(
        const std::filesystem::path& path,
        const std::string& objectName
    )
    {
        std::ifstream input(path);
        if (!input.is_open()) {
            return false;
        }

        std::string line;
        size_t inspectedLineCount = 0;
        constexpr size_t kMaxHeaderLines = 512;

        while (inspectedLineCount < kMaxHeaderLines &&
               // 処理概要: stdが担当する「getline」処理を実行する。
               // 注意事項: 呼び出し順序と所有データの整合性を保ちながら状態を更新する。
               std::getline(input, line)) {
            ++inspectedLineCount;

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (line.rfind("o ", 0) != 0 &&
                line.rfind("g ", 0) != 0) {
                continue;
            }

            const std::string declaredName = line.substr(2);
            if (declaredName == objectName) {
                return true;
            }
        }

        return false;
    }

    std::string FindObjPathByObjectName(
        const std::filesystem::path& rootDirectory,
        const std::string& objectName
    )
    {
        if (objectName.empty()) {
            return "";
        }

        std::error_code error;
        for (std::filesystem::recursive_directory_iterator iterator(
            rootDirectory,
            std::filesystem::directory_options::skip_permission_denied,
            error
        ), end; iterator != end && !error; iterator.increment(error)) {
            if (!iterator->is_regular_file(error)) {
                continue;
            }

            std::string extension = iterator->path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            if (extension != ".obj") {
                continue;
            }

            if (ObjFileDeclaresObjectName(iterator->path(), objectName)) {
                return iterator->path().generic_string();
            }
        }

        return "";
    }

    std::string EscapeJsonString(const std::string& value)
    {
        std::string result;

        for (char c : value) {
            if (c == '\\') {
                result += "\\\\";
            } else if (c == '"') {
                result += "\\\"";
            } else {
                result += c;
            }
        }

        return result;
    }

    bool SplitModelPath(
        const std::string& path,
        std::string& directoryPath,
        std::string& modelName
    )
    {
        const size_t slashPos =
            path.find_last_of("/\\");

        if (slashPos == std::string::npos) {
            directoryPath = "Resources";
            modelName = path;
            return true;
        }

        directoryPath =
            path.substr(0, slashPos);

        modelName =
            path.substr(slashPos + 1);

        return true;
    }

    std::string ExtractJsonString(
        const std::string& text,
        const std::string& key
    )
    {
        const std::regex pattern(
            "\"" + key + "\"\\s*:\\s*\"([^\"]*)\""
        );

        std::smatch match;

        if (std::regex_search(text, match, pattern)) {
            return match[1].str();
        }

        return "";
    }

    bool ExtractJsonBool(
        const std::string& text,
        const std::string& key,
        bool defaultValue
    )
    {
        const std::regex pattern(
            "\"" + key + "\"\\s*:\\s*(true|false)"
        );

        std::smatch match;

        if (std::regex_search(text, match, pattern)) {
            return match[1].str() == "true";
        }

        return defaultValue;
    }

    Vector3 ExtractJsonVector3(
        const std::string& text,
        const std::string& key,
        const Vector3& defaultValue
    )
    {
        const std::regex pattern(
            "\"" + key + "\"\\s*:\\s*\\[\\s*"
            "([-+0-9.eE]+)\\s*,\\s*"
            "([-+0-9.eE]+)\\s*,\\s*"
            "([-+0-9.eE]+)\\s*\\]"
        );

        std::smatch match;

        if (!std::regex_search(text, match, pattern)) {
            return defaultValue;
        }

        return {
            std::stof(match[1].str()),
            std::stof(match[2].str()),
            std::stof(match[3].str())
        };
    }

} // namespace

