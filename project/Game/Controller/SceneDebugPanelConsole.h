// ============================================================================
// ファイルの役割: ゲームビューと各種デバッグ編集ウィンドウをImGui上へ構築する。
// 構成上の位置付け: 大きなクラスの実装を責務別に分割した内部ヘッダー。所有クラスの状態を前提に使用する。
// 実装時の注意: 単独利用を想定せず、呼び出し順序と所有リソースの寿命を変更する場合は本体側も確認する。
// ============================================================================
#pragma once

#include <string>
#include <vector>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#endif

namespace SceneDebugPanelDetail {
    enum class EditorLogType {
        Info,
        Warning,
        Error
    };

    struct EditorConsoleLog {
        EditorLogType type;
        std::string message;
    };

    inline std::vector<EditorConsoleLog>& GetEditorLogs() {
        static std::vector<EditorConsoleLog> logs;
        return logs;
    }

    inline void AddEditorLog(EditorLogType type, const std::string& message) {
        GetEditorLogs().push_back({ type, message });
    }

    inline void InitializeEditorConsoleOnce() {
        static bool initialized = false;

        if (initialized) {
            return;
        }

        initialized = true;

        AddEditorLog(EditorLogType::Info, "Editor console initialized.");
        AddEditorLog(EditorLogType::Info, "Hierarchy / Inspector / Project panels are active.");
    }

    inline const char* GetEditorLogPrefix(EditorLogType type) {
        switch (type) {
        case EditorLogType::Info:
            return "[Info]";

        case EditorLogType::Warning:
            return "[Warning]";

        case EditorLogType::Error:
            return "[Error]";

        default:
            return "[Log]";
        }
    }

#ifdef USE_IMGUI
    inline ImVec4 GetEditorLogColor(EditorLogType type) {
        switch (type) {
        case EditorLogType::Info:
            return ImVec4(0.55f, 0.85f, 0.55f, 1.0f);

        case EditorLogType::Warning:
            return ImVec4(1.0f, 0.75f, 0.25f, 1.0f);

        case EditorLogType::Error:
            return ImVec4(1.0f, 0.35f, 0.35f, 1.0f);

        default:
            return ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
    }
#endif
} // namespace SceneDebugPanelDetail

