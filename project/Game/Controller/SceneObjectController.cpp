// ============================================================================
// ファイルの役割: シーン配置オブジェクトの生成、更新、JSON保存・読込を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "SceneObjectController.h"

#include "ModelManager.h"
#include "Object3dCommon.h"
#include "TextureManager.h"
#include "Json.h"

#include <fstream>
#include <functional>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <regex>
#include <sstream>

#include "SceneObjectControllerRuntime.h"
#include "SceneObjectControllerJsonHelpers.h"
#include "SceneObjectControllerSave.h"
#include "SceneObjectControllerLoad.h"
