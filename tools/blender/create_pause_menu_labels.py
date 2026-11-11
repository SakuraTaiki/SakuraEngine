"""Create the two 3D labels used by the in-game Escape menu."""

from pathlib import Path

import bpy


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "UI" / "PauseMenu"
PNG = OUT / "pause_menu_text.png"
BLEND = OUT / "pause_menu_labels.blend"


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def make_texture():
    size = 64
    image = bpy.data.images.new("PauseMenuText", width=size, height=size, alpha=True)
    pixels = []
    for y in range(size):
        for x in range(size):
            grain = (((x * 7 + y * 13) % 9) - 4) * 0.002
            pixels.extend((0.82 + grain, 0.91 + grain, 0.96 + grain, 1.0))
    image.pixels = pixels
    image.filepath_raw = str(PNG)
    image.file_format = "PNG"
    image.save()
    return image


def make_material(image):
    material = bpy.data.materials.new("PauseMenuBlueWhite")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    principled = next(node for node in nodes if node.type == "BSDF_PRINCIPLED")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    links.new(texture.outputs["Color"], principled.inputs["Base Color"])
    principled.inputs["Roughness"].default_value = 0.70
    principled.inputs["Metallic"].default_value = 0.08
    return material


def make_label(body, name, width, material):
    curve = bpy.data.curves.new(f"{name}Curve", type="FONT")
    curve.body = body
    curve.align_x = "CENTER"
    curve.align_y = "CENTER"
    curve.size = 2.0
    curve.extrude = 0.12
    curve.bevel_depth = 0.022
    curve.bevel_resolution = 1
    font = Path("C:/Windows/Fonts/arialbd.ttf")
    if font.exists():
        curve.font = bpy.data.fonts.load(str(font))
    obj = bpy.data.objects.new(name, curve)
    bpy.context.collection.objects.link(obj)
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.convert(target="MESH")
    obj = bpy.context.active_object
    scale = width / max(obj.dimensions.x, 0.001)
    obj.scale = (scale, scale, scale)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    bpy.ops.object.mode_set(mode="EDIT")
    bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project(angle_limit=1.15192, island_margin=0.02)
    bpy.ops.object.mode_set(mode="OBJECT")
    obj.data.materials.append(material)
    return obj


def export(obj, name):
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.wm.obj_export(
        filepath=str(OUT / name),
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
    restart = make_label("RESTART", "PauseRestart", 4.6, material)
    back = make_label("BACK TO SELECT", "PauseBackToSelect", 6.7, material)
    bpy.ops.wm.save_as_mainfile(filepath=str(BLEND))
    export(restart, "pause_restart.obj")
    export(back, "pause_back_to_select.obj")
    print(f"PAUSE_MENU_EXPORTED:{OUT}")


if __name__ == "__main__":
    main()
