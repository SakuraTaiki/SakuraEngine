"""Create Japanese 3D labels for the stage gimmick guide cards."""

from pathlib import Path
import math
import bpy


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "UI" / "GimmickGuide"
OUT.mkdir(parents=True, exist_ok=True)
FONT = Path("C:/Windows/Fonts/BIZ-UDGothicB.ttc")
# BIZ UD Gothic's separate left radical in 「往」 can lose its fill when
# Blender converts the outlined curve to a mesh.  Yu Gothic Bold has a stable
# contour for this glyph and is used only for strings that contain it.
COMPLEX_GLYPH_FONT = Path("C:/Windows/Fonts/YuGothB.ttc")

LABELS = {
    # 図鑑の1行内でも文字を大きく保てるよう、意味を変えず短い説明にする。
    "stage1_falling_floor": ("崩れる床", "乗ると床が崩れます"),
    "stage1_spring_pad": ("ジャンプ台", "乗ると大ジャンプします"),
    "stage1_save_flag": ("セーブ旗", "触れると復活地点を保存"),
    "stage1_goal": ("ゴール", "触れるとクリアです"),
    "stage2_key": ("プリズムキー", "拾ってドアを開けます"),
    "stage2_locked_door": ("鍵付きドア", "鍵を持つと開きます"),
    "stage2_crash_cube": ("歩く敵", "地面を往復します"),
    "stage2_glider": ("飛ぶ敵", "空中を移動します"),
}

CATEGORIES = {
    "category_platform": "足場",
    "category_movement": "移動",
    "category_item": "アイテム",
    "category_mechanism": "仕掛け",
    "category_enemy": "敵",
    "category_progress": "進行",
}


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def make_texture():
    # 全UVを中央へ固定して使うため、ミップマップでも暗い縁を拾わない単色にする。
    # 文字色はゲーム側のSetColorで名称・説明ごとに調整する。
    image = bpy.data.images.new("GimmickGuideText", width=128, height=128, alpha=True)
    pixels = []
    for _y in range(128):
        for _x in range(128):
            pixels.extend((0.96, 0.99, 1.0, 1.0))
    image.pixels = pixels
    image.filepath_raw = str(OUT / "gimmick_guide_text.png")
    image.file_format = "PNG"
    image.save()
    return image


def make_material(image):
    material = bpy.data.materials.new("GimmickGuideTextMaterial")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    shader.inputs["Roughness"].default_value = 0.52
    links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return material


def create_label(name, body, material, target_width):
    bpy.ops.object.text_add(location=(0, 0, 0))
    text = bpy.context.object
    text.name = name
    text.data.body = body
    selected_font = COMPLEX_GLYPH_FONT if "往" in body else FONT
    if selected_font.exists():
        text.data.font = bpy.data.fonts.load(str(selected_font))
    text.data.align_x = "CENTER"
    text.data.align_y = "CENTER"
    text.data.size = 1.0
    # 複数の独立輪郭を持つ漢字では、微小なoffsetでも輪郭が交差して
    # 部首単位で面が欠けるため、太さはフォント本来の字形だけで確保する。
    text.data.offset = 0.0
    # 側面が黒く目立たない範囲で奥行きと面取りを少し増やす。
    text.data.extrude = 0.010
    text.data.bevel_depth = 0.001
    text.data.bevel_resolution = 1
    text.data.fill_mode = "BOTH"
    text.data.materials.append(material)
    bpy.context.view_layer.objects.active = text
    bpy.ops.object.convert(target="MESH")

    # All cards share a stable width while preserving the Japanese glyph ratio.
    uniform = min(target_width / max(text.dimensions.x, 0.001), 0.72 / max(text.dimensions.y, 0.001))
    text.scale = (uniform, uniform, uniform)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    # Keep the mesh handedness unchanged.  Mirroring an applied text mesh flips
    # the front-face winding, which made the game cull the glyph faces and show
    # only the bevel/side walls (the text looked chipped or hollow).
    text.rotation_euler.x = -math.pi * 0.5
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)

    uv = text.data.uv_layers.active or text.data.uv_layers.new(name="UVMap")
    for loop in uv.data:
        loop.uv = (0.5, 0.5)
    return text


def export_one(model, name):
    bpy.ops.object.select_all(action="DESELECT")
    model.select_set(True)
    bpy.context.view_layer.objects.active = model
    bpy.ops.wm.obj_export(
        filepath=str(OUT / f"{name}.obj"),
        export_selected_objects=True,
        export_materials=True,
        export_triangulated_mesh=True,
        forward_axis="NEGATIVE_Y",
        up_axis="Z",
    )
    bpy.data.objects.remove(model, do_unlink=True)


clear_scene()
image = make_texture()
material = make_material(image)
for label_name, (label_title, label_detail) in LABELS.items():
    export_one(create_label(f"{label_name}_name", label_title, material, 2.75), f"{label_name}_name")
    export_one(create_label(f"{label_name}_detail", label_detail, material, 4.30), f"{label_name}_detail")
for category_name, category_text in CATEGORIES.items():
    export_one(create_label(category_name, category_text, material, 2.20), category_name)
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "gimmick_guide_labels.blend"))
print(f"GIMMICK_GUIDE_LABELS_EXPORTED:{OUT}")
