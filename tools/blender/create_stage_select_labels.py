"""Create large 3D STAGE 1 / STAGE 2 labels for the release selector."""

from pathlib import Path

import bpy


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "UI" / "StageSelect"
PNG = OUT / "stage_label.png"
BLEND = OUT / "stage_labels.blend"


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def make_texture():
    size = 64
    image = bpy.data.images.new("StageLabelTexture", width=size, height=size, alpha=True)
    pixels = []
    for y in range(size):
        for x in range(size):
            grain = (((x * 11 + y * 5) % 7) - 3) * 0.002
            pixels.extend((0.79 + grain, 0.91 + grain, 0.96 + grain, 1.0))
    image.pixels = pixels
    image.filepath_raw = str(PNG)
    image.file_format = "PNG"
    image.save()
    return image


def make_material(image):
    material = bpy.data.materials.new("StageLabelBlueWhite")
    material.diffuse_color = (0.79, 0.91, 0.96, 1.0)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    principled = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    links.new(texture.outputs["Color"], principled.inputs["Base Color"])
    principled.inputs["Roughness"].default_value = 0.68
    principled.inputs["Metallic"].default_value = 0.10
    return material


def make_text(body, name, material):
    curve = bpy.data.curves.new(f"{name}Curve", type="FONT")
    curve.body = body
    curve.align_x = "CENTER"
    curve.align_y = "CENTER"
    curve.size = 2.0
    curve.extrude = 0.12
    curve.bevel_depth = 0.022
    curve.bevel_resolution = 1
    font_path = Path("C:/Windows/Fonts/arialbd.ttf")
    if font_path.exists():
        curve.font = bpy.data.fonts.load(str(font_path))

    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    obj = bpy.context.active_object
    scale = 3.5 / max(obj.dimensions.x, 0.001)
    obj.scale = (scale, scale, scale)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=1.15192, island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    obj.data.materials.append(material)
    return obj


def export_object(obj, filename):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.wm.obj_export(
        filepath=str(OUT / filename),
        export_selected_objects=True,
        export_materials=True,
        export_uv=True,
        export_normals=True,
        export_triangulated_mesh=True,
    )


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    clear_scene()
    material = make_material(make_texture())
    stage1 = make_text("STAGE 1", "Stage1Label", material)
    stage2 = make_text("STAGE 2", "Stage2Label", material)
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
    export_object(stage1, "stage_1.obj")
    export_object(stage2, "stage_2.obj")
    print(f"STAGE_LABELS_EXPORTED:{OUT}")


if __name__ == "__main__":
    main()
