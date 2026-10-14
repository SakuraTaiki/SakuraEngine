"""Blenderでタイトル画面のPRESS ENTER用OBJを再生成するスクリプト。"""

import bpy
from pathlib import Path


OUTPUT_DIRECTORY = Path(__file__).resolve().parents[1] / "Resources" / "Title"
TEXTURE_PATH = OUTPUT_DIRECTORY / "press_enter.png"
OBJ_PATH = OUTPUT_DIRECTORY / "press_enter.obj"
BLEND_PATH = OUTPUT_DIRECTORY / "press_enter.blend"


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def create_off_white_texture():
    """ゲーム名と同じ、発光しすぎない白色テクスチャを作る。"""
    width = 64
    height = 64
    image = bpy.data.images.new("PressEnterTexture", width=width, height=height)
    pixels = []
    for y in range(height):
        for x in range(width):
            variation = (((x * 17 + y * 29) % 11) - 5) * 0.0015
            pixels.extend((0.86 + variation, 0.88 + variation, 0.90 + variation, 1.0))
    image.pixels = pixels
    image.filepath_raw = str(TEXTURE_PATH)
    image.file_format = "PNG"
    image.save()
    return image


def create_text(image):
    curve = bpy.data.curves.new("PressEnterCurve", type="FONT")
    curve.body = "PRESS ENTER"
    curve.align_x = "CENTER"
    curve.align_y = "CENTER"
    curve.size = 2.0
    curve.extrude = 0.12
    curve.bevel_depth = 0.018
    curve.bevel_resolution = 1
    curve.resolution_u = 8

    font_path = Path("C:/Windows/Fonts/arialbd.ttf")
    if font_path.exists():
        curve.font = bpy.data.fonts.load(str(font_path))

    text_object = bpy.data.objects.new("PressEnter", curve)
    bpy.context.collection.objects.link(text_object)
    bpy.context.view_layer.objects.active = text_object
    text_object.select_set(True)
    bpy.ops.object.convert(target="MESH")
    text_object = bpy.context.active_object

    # ゲーム名より小さくし、下に置いても主役を奪わない横幅に整える。
    width = max(text_object.dimensions.x, 0.001)
    scale = 6.2 / width
    text_object.scale = (scale, scale, scale)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=1.15192, island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")

    material = bpy.data.materials.new("PressEnterWhite")
    material.diffuse_color = (0.86, 0.88, 0.90, 1.0)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    principled = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    links.new(texture.outputs["Color"], principled.inputs["Base Color"])
    principled.inputs["Roughness"].default_value = 0.82
    text_object.data.materials.append(material)
    return text_object


def export_assets(text_object):
    OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_PATH))
    bpy.ops.object.select_all(action="DESELECT")
    text_object.select_set(True)
    bpy.context.view_layer.objects.active = text_object
    bpy.ops.wm.obj_export(
        filepath=str(OBJ_PATH),
        export_selected_objects=True,
        export_materials=True,
        export_uv=True,
        export_normals=True,
        export_triangulated_mesh=True,
    )


def main():
    OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)
    clear_scene()
    image = create_off_white_texture()
    text_object = create_text(image)
    export_assets(text_object)


if __name__ == "__main__":
    main()
