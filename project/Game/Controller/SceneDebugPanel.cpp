// ============================================================================
// ファイルの役割: ゲームビューと各種デバッグ編集ウィンドウをImGui上へ構築する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "SceneDebugPanel.h"

#include "EngineContext.h"
#include "HitEffectController.h"
#include "AnimationDebugController.h"
#include "SoundController.h"
#include "CameraDebugController.h"
#include "EnvironmentController.h"
#include "SceneObjectController.h"
#include "StageEditor.h"
#include "Object3dCommon.h"

#include "Ring.h"
#include "Cylinder.h"
#include "Primitive.h"
#include "ParticleManager.h"
#include "DirectXCommon.h"
#include "ImGuiManager.h"
#include "camera.h"
#include "GPUParticleManager.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <cstring>
#include <functional>
#include <filesystem>

#ifdef USE_IMGUI
#include "externals/imgui/imgui.h"
#include "externals/imgui/ImGuizmo.h"
#endif

#include "SceneDebugPanelDetail.h"

namespace Detail = SceneDebugPanelDetail;


#include "SceneDebugPanelWindows.h"
#include "SceneDebugPanelPostEffect.h"
#include "SceneDebugPanelEnvironment.h"
#include "SceneDebugPanelGameView.h"
#include "SceneDebugPanelEffectTab.h"
