"""BlenderでPrism Passageのタイトル用OBJを再生成するスクリプト。"""

import bpy
from pathlib import Path


OUTPUT_DIRECTORY = Path(__file__).resolve().parents[1] / "Resources" / "Title"
TEXTURE_PATH = OUTPUT_DIRECTORY / "prism_passage_title.png"
OBJ_PATH = OUTPUT_DIRECTORY / "prism_passage_title.obj"
BLEND_PATH = OUTPUT_DIRECTORY / "prism_passage_title.blend"


def clear_scene():
    """初期オブジェクトを削除し、毎回同じ結果を生成できるようにする。"""
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def create_off_white_texture():
    """目立ちすぎない白を基調に、手作業感のある微かな色むらを加える。"""
    width = 64
    height = 64
    image = bpy.data.images.new("PrismPassageTitleTexture", width=width, height=height)
    pixels = []
    for y in range(height):
        for x in range(width):
            # 規則的すぎない範囲で明度を少しだけ変え、真っ白な発光感を避ける。
            variation = (((x * 17 + y * 29) % 11) - 5) * 0.0015
            pixels.extend((0.86 + variation, 0.88 + variation, 0.90 + variation, 1.0))
    image.pixels = pixels
    image.filepath_raw = str(TEXTURE_PATH)
    image.file_format = "PNG"
    image.save()
    return image


def create_title(image):
    """太字フォントを押し出し、読みやすいブロック文字へ変換する。"""
    curve = bpy.data.curves.new("PrismPassageTitleCurve", type="FONT")
    curve.body = "PRISM PASSAGE"
    curve.align_x = "CENTER"
    curve.align_y = "CENTER"
    curve.size = 2.0
    curve.extrude = 0.18
    curve.bevel_depth = 0.025
    curve.bevel_resolution = 1
    curve.resolution_u = 8

    font_path = Path("C:/Windows/Fonts/arialbd.ttf")
    if font_path.exists():
        curve.font = bpy.data.fonts.load(str(font_path))

    title = bpy.data.objects.new("PrismPassageTitle", curve)
    bpy.context.collection.objects.link(title)
    bpy.context.view_layer.objects.active = title
    title.select_set(True)

    bpy.ops.object.convert(target="MESH")
    title = bpy.context.active_object

    # 横幅を14ワールド単位へ正規化し、ゲーム画面中央で読みやすい大きさにする。
    width = max(title.dimensions.x, 0.001)
    scale = 14.0 / width
    title.scale = (scale, scale, scale)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=1.15192, island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")

    material = bpy.data.materials.new("PrismPassageTitleWhite")
    material.diffuse_color = (0.86, 0.88, 0.90, 1.0)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    # Blenderの表示言語によりノード名が変わるため、名前ではなく種類で取得する。
    principled = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    links.new(texture.outputs["Color"], principled.inputs["Base Color"])
    principled.inputs["Roughness"].default_value = 0.82
    title.data.materials.append(material)

    # ゲーム側ではXY平面を正面として扱い、厚みだけをZ方向へ持たせる。
    title.location = (0.0, 0.0, 0.0)
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)
    return title


def export_assets(title):
    """編集用blendと、ゲームで読み込むOBJ/MTLを保存する。"""
    OUTPUT_DIRECTORY.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND_PATH))
    bpy.ops.object.select_all(action="DESELECT")
    title.select_set(True)
    bpy.context.view_layer.objects.active = title
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
    title = create_title(image)
    export_assets(title)


if __name__ == "__main__":
    main()
