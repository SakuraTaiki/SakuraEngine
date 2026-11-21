"""Create a legible 3D Japanese 'ゴールを目指せ' objective label."""

from pathlib import Path
import math
import bpy


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "UI" / "Objective"
OUT.mkdir(parents=True, exist_ok=True)
PNG = OUT / "objective_reach_goal.png"


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def make_texture():
    size = 64
    image = bpy.data.images.new("ObjectiveReachGoalCyan", width=size, height=size, alpha=True)
    pixels = []
    for y in range(size):
        for x in range(size):
            edge = min(x, size - 1 - x, y, size - 1 - y)
            color = (0.80, 0.97, 1.0, 1.0) if edge > 2 else (0.10, 0.55, 0.68, 1.0)
            pixels.extend(color)
    image.pixels = pixels
    image.filepath_raw = str(PNG)
    image.file_format = "PNG"
    image.save()
    return image


def make_material(image):
    material = bpy.data.materials.new("ObjectiveReachGoalMaterial")
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    nodes.clear()
    output = nodes.new("ShaderNodeOutputMaterial")
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    shader.inputs["Roughness"].default_value = 0.48
    shader.inputs["Metallic"].default_value = 0.08
    links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return material


def build(material):
    bpy.ops.object.text_add(location=(0, 0, 0))
    text = bpy.context.object
    text.name = "ObjectiveReachGoal"
    text.data.body = "ゴールを目指せ"
    font_path = Path("C:/Windows/Fonts/BIZ-UDGothicB.ttc")
    if font_path.exists():
        text.data.font = bpy.data.fonts.load(str(font_path))
    text.data.align_x = "CENTER"
    text.data.align_y = "CENTER"
    text.data.size = 1.0
    text.data.space_character = 1.08
    text.data.extrude = 0.035
    text.data.bevel_depth = 0.004
    text.data.bevel_resolution = 1
    text.data.fill_mode = "BOTH"
    text.data.materials.append(material)
    bpy.context.view_layer.objects.active = text
    bpy.ops.object.convert(target="MESH")

    # Only normalize by height so complex kanji strokes retain their spacing.
    uniform_scale = 0.68 / max(text.dimensions.y, 0.001)
    text.scale = (uniform_scale, uniform_scale, uniform_scale)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    # Runtime rotates objective labels +90 degrees around X. Store the glyph
    # face in the same X/Z plane as the existing pickup-key label so that
    # rotation presents the front face rather than the thin extrusion edge.
    # Use the opposite X-facing direction and pre-mirror X. The runtime's
    # objective renderer applies a negative X scale for its legacy labels;
    # baking this compensation keeps the Japanese text upright and readable.
    text.rotation_euler.x = -math.pi * 0.5
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    text.scale.x = -1.0
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)

    uv = text.data.uv_layers.active or text.data.uv_layers.new(name="UVMap")
    for loop in uv.data:
        loop.uv = (0.5, 0.5)
    return text


def export(model):
    bpy.ops.object.select_all(action="DESELECT")
    model.select_set(True)
    bpy.context.view_layer.objects.active = model
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "objective_reach_goal.blend"))
    bpy.ops.wm.obj_export(
        filepath=str(OUT / "objective_reach_goal.obj"),
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
print(f"OBJECTIVE_REACH_GOAL_EXPORTED:{OUT}")
