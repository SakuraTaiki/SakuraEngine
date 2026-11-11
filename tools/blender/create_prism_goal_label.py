"""Create the extruded 3D GOAL label used by the intro camera preview."""

from pathlib import Path
import math
import bpy


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "Goal" / "PrismLabel"
OUT.mkdir(parents=True, exist_ok=True)
PNG = OUT / "prism_goal_label.png"


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def make_texture():
    size = 64
    image = bpy.data.images.new("PrismGoalLabelGold", width=size, height=size, alpha=True)
    pixels = [0.0] * (size * size * 4)
    for y in range(size):
        for x in range(size):
            edge = min(x, size - 1 - x, y, size - 1 - y)
            color = (1.0, 0.91, 0.48, 1.0) if edge > 2 else (0.64, 0.38, 0.07, 1.0)
            p = (y * size + x) * 4
            pixels[p:p + 4] = color
    image.pixels = pixels
    image.filepath_raw = str(PNG)
    image.file_format = "PNG"
    image.save()
    return image


def make_material(image):
    material = bpy.data.materials.new("PrismGoalLabelMaterial")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    for node in list(nodes):
        nodes.remove(node)
    output = nodes.new("ShaderNodeOutputMaterial")
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    shader.inputs["Roughness"].default_value = 0.40
    shader.inputs["Metallic"].default_value = 0.25
    links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return material


def build(material):
    bpy.ops.object.text_add(location=(0, 0, 0))
    text = bpy.context.object
    text.name = "PrismGoalLabel"
    text.data.body = "GOAL"
    text.data.align_x = "CENTER"
    text.data.align_y = "CENTER"
    text.data.size = 1.0
    text.data.extrude = 0.075
    text.data.bevel_depth = 0.018
    text.data.bevel_resolution = 2
    text.data.fill_mode = "BOTH"
    text.data.materials.append(material)
    bpy.context.view_layer.objects.active = text
    bpy.ops.object.convert(target="MESH")

    # Normalize for a compact label above the arrow.
    scale_x = 1.08 / max(text.dimensions.x, 0.001)
    scale_y = 0.30 / max(text.dimensions.y, 0.001)
    text.scale = (scale_x, scale_y, 1.0)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    # The CG2 OBJ conversion reverses authored Y. Rotate the source 180 degrees
    # around X so the runtime label remains upright without mirrored lettering.
    text.rotation_euler.x = math.pi
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)

    uv = text.data.uv_layers.active or text.data.uv_layers.new(name="UVMap")
    for loop in uv.data:
        loop.uv = (0.5, 0.5)
    return text


def export(model):
    bpy.ops.object.select_all(action="DESELECT")
    model.select_set(True)
    bpy.context.view_layer.objects.active = model
    bpy.ops.object.shade_smooth_by_angle()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "prism_goal_label.blend"))
    bpy.ops.wm.obj_export(
        filepath=str(OUT / "prism_goal_label.obj"),
        export_selected_objects=True,
        export_materials=True,
        export_triangulated_mesh=True,
        forward_axis="NEGATIVE_Y",
        up_axis="Z",
    )


clear_scene()
image = make_texture()
material = make_material(image)
export(build(material))
print(f"PRISM_GOAL_LABEL_EXPORTED:{OUT}")
