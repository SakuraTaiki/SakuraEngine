"""Create the SELECT STAGE 3D heading used by the release stage-select screen."""

from pathlib import Path

import bpy


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "UI" / "StageSelect"
PNG = OUT / "select_stage.png"
OBJ = OUT / "select_stage.obj"
BLEND = OUT / "select_stage.blend"


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def create_texture():
    size = 64
    image = bpy.data.images.new("SelectStageTexture", width=size, height=size, alpha=True)
    pixels = [0.0] * (size * size * 4)
    for y in range(size):
        for x in range(size):
            edge = min(x, size - 1 - x, y, size - 1 - y)
            grain = (((x * 13 + y * 7) % 9) - 4) * 0.002
            if edge <= 2:
                color = (0.30 + grain, 0.70 + grain, 0.82 + grain, 1.0)
            else:
                color = (0.82 + grain, 0.91 + grain, 0.96 + grain, 1.0)
            index = (y * size + x) * 4
            pixels[index:index + 4] = color
    image.pixels = pixels
    image.filepath_raw = str(PNG)
    image.file_format = "PNG"
    image.save()
    return image


def create_material(image):
    material = bpy.data.materials.new("SelectStageBlueWhite")
    material.diffuse_color = (0.82, 0.91, 0.96, 1.0)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    principled = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    links.new(texture.outputs["Color"], principled.inputs["Base Color"])
    principled.inputs["Roughness"].default_value = 0.72
    principled.inputs["Metallic"].default_value = 0.08
    return material


def create_text(material):
    curve = bpy.data.curves.new("SelectStageCurve", type="FONT")
    curve.body = "SELECT STAGE"
    curve.align_x = "CENTER"
    curve.align_y = "CENTER"
    curve.size = 2.0
    curve.extrude = 0.11
    curve.bevel_depth = 0.018
    curve.bevel_resolution = 1
    curve.resolution_u = 8

    font_path = Path("C:/Windows/Fonts/arialbd.ttf")
    if font_path.exists():
        curve.font = bpy.data.fonts.load(str(font_path))

    text = bpy.data.objects.new("SelectStage", curve)
    bpy.context.collection.objects.link(text)
    bpy.context.view_layer.objects.active = text
    text.select_set(True)
    bpy.ops.object.convert(target="MESH")
    text = bpy.context.active_object

    width = max(text.dimensions.x, 0.001)
    scale = 7.2 / width
    text.scale = (scale, scale, scale)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=1.15192, island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    text.data.materials.append(material)
    return text


def export(text):
    OUT.mkdir(parents=True, exist_ok=True)
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
    bpy.ops.object.select_all(action="DESELECT")
    text.select_set(True)
    bpy.context.view_layer.objects.active = text
    bpy.ops.wm.obj_export(
        filepath=str(OBJ),
        export_selected_objects=True,
        export_materials=True,
        export_uv=True,
        export_normals=True,
        export_triangulated_mesh=True,
    )


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    clear_scene()
    export(create_text(create_material(create_texture())))
    print(f"SELECT_STAGE_EXPORTED:{OUT}")


if __name__ == "__main__":
    main()
