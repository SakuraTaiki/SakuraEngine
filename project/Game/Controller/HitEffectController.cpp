// ============================================================================
// ファイルの役割: ヒットエフェクトのプリセット、保存・読込、実行時再生を管理する。
// 構成上の位置付け: ヘッダーで宣言した機能を実装し、外部公開する責務と内部処理を分離する。
// 実装時の注意: GPU・ファイル・入力など外部状態を扱う処理では、初期化済みかと失敗時の戻り値を確認する。
// ============================================================================
#include "HitEffectController.h"
#include "GPUParticleManager.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>


#include "HitEffectControllerRuntime.h"
#include "HitEffectControllerSave.h"
#include "HitEffectControllerLoad.h"
#include "HitEffectControllerPresetList.h"
