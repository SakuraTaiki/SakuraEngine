"""Verify that Codex can run Blender and exchange files with the CG2 workspace."""

import json
import platform
import sys
from pathlib import Path

import bpy


SCRIPT_DIR = Path(__file__).resolve().parent
WORKSPACE_ROOT = SCRIPT_DIR.parents[1]
OUTPUT_DIR = WORKSPACE_ROOT / "output" / "blender_bridge"
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

result = {
    "connected": True,
    "blender_version": bpy.app.version_string,
    "python_version": platform.python_version(),
    "workspace": str(WORKSPACE_ROOT),
    "executable": sys.executable,
}

(OUTPUT_DIR / "connection.json").write_text(
    json.dumps(result, ensure_ascii=False, indent=2),
    encoding="utf-8",
)

print("CODEX_BLENDER_BRIDGE_OK")
print(json.dumps(result, ensure_ascii=False))
