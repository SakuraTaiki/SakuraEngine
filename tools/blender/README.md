# Codex - Blender bridge

This directory contains reproducible Blender Python scripts used to build and
export CG2 assets. Scripts are executed with Blender 4.4 in background mode.

## Connection check

```powershell
powershell -ExecutionPolicy Bypass -File tools/blender/run_blender.ps1 `
  -Script tools/blender/verify_connection.py
```

The check writes `output/blender_bridge/connection.json`.

## Asset workflow

1. Add or edit a Blender Python script under `tools/blender`.
2. Generate geometry, materials, UVs, and preview renders in Blender.
3. Save the editable `.blend` source under `project/Resources` or an agreed
   source directory.
4. Export runtime assets (`.obj`, `.mtl`, `.png`) to the matching resource
   directory.
5. Run the game and iterate from screenshots or captured video.

Keep generated asset paths inside this workspace. Do not execute Blender
scripts copied from an untrusted source without reviewing them first.
