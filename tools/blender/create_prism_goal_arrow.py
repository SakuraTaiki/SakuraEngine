"""Create the 3D downward Prism Beacon preview arrow."""

from pathlib import Path
import bpy


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "Goal" / "PrismArrow"
OUT.mkdir(parents=True, exist_ok=True)
PNG = OUT / "prism_goal_arrow.png"

COLORS = [
    (0.20, 0.12, 0.035, 1.0),
    (0.75, 0.46, 0.08, 1.0),
    (1.00, 0.86, 0.34, 1.0),
    (1.00, 0.96, 0.66, 1.0),
    (0.20, 0.86, 0.92, 1.0),
]


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def texture():
    width, height = 320, 64
    image = bpy.data.images.new("PrismGoalArrowPalette", width=width, height=height, alpha=True)
    band = width // len(COLORS)
    pixels = [0.0] * (width * height * 4)
    for y in range(height):
        for x in range(width):
            color = list(COLORS[min(x // band, len(COLORS) - 1)])
            if min(x % band, band - 1 - x % band, y, height - 1 - y) < 2:
                color[0] *= 0.82
                color[1] *= 0.82
                color[2] *= 0.82
            p = (y * width + x) * 4
            pixels[p:p + 4] = color
    image.pixels = pixels
    image.filepath_raw = str(PNG)
    image.file_format = "PNG"
    image.save()
    return image


def material(image):
    mat = bpy.data.materials.new("PrismGoalArrowMaterial")
    mat.use_nodes = True
    nodes = mat.node_tree.nodes
    links = mat.node_tree.links
    for node in list(nodes):
        nodes.remove(node)
    output = nodes.new("ShaderNodeOutputMaterial")
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    tex = nodes.new("ShaderNodeTexImage")
    tex.image = image
    tex.interpolation = "Closest"
    shader.inputs["Roughness"].default_value = 0.38
    shader.inputs["Metallic"].default_value = 0.30
    links.new(tex.outputs["Color"], shader.inputs["Base Color"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return mat


def set_uv(obj, index):
    layer = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")
    u = (index + 0.5) / len(COLORS)
    for loop in layer.data:
        loop.uv = (u, 0.5)


def arrow_head(mat):
    # The OBJ axis conversion used by CG2 reverses Blender's authored Y for this
    # asset. Author the tip at +Y so the runtime silhouette points downward.
    outline = [(-0.36, -0.04), (0.36, -0.04), (0.0, 0.44)]
    depth = 0.11
    vertices = [(x, y, -depth) for x, y in outline] + [(x, y, depth) for x, y in outline]
    faces = [(2, 1, 0), (3, 4, 5), (0, 1, 4, 3), (1, 2, 5, 4), (2, 0, 3, 5)]
    mesh = bpy.data.meshes.new("DownTriangleGeometry")
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new("DownwardTriangle", mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(mat)
    set_uv(obj, 2)
    bevel = obj.modifiers.new("Rounded triangle edge", "BEVEL")
    bevel.width = 0.025
    bevel.segments = 2
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    return obj


def arrow_shaft(mat):
    bpy.ops.mesh.primitive_cube_add(location=(0.0, -0.25, 0.0))
    obj = bpy.context.object
    obj.name = "ArrowShaft"
    obj.scale = (0.13, 0.23, 0.10)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(mat)
    set_uv(obj, 2)
    bevel = obj.modifiers.new("Rounded shaft edge", "BEVEL")
    bevel.width = 0.025
    bevel.segments = 2
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    return obj


def inset(name, z, mat):
    bpy.ops.mesh.primitive_cube_add(location=(0, -0.095, z))
    obj = bpy.context.object
    obj.name = name
    obj.scale = (0.045, 0.245, 0.014)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(mat)
    set_uv(obj, 3)
    return obj


def export(parts):
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    bpy.context.object.name = "PrismGoalArrow"
    bpy.ops.object.shade_smooth_by_angle()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "prism_goal_arrow.blend"))
    bpy.ops.wm.obj_export(
        filepath=str(OUT / "prism_goal_arrow.obj"),
        export_selected_objects=True,
        export_materials=True,
        export_triangulated_mesh=True,
        forward_axis="NEGATIVE_Y",
        up_axis="Z",
    )


clear_scene()
image = texture()
mat = material(image)
export([
    arrow_shaft(mat),
    arrow_head(mat),
    inset("FrontHighlight", -0.116, mat),
    inset("BackHighlight", 0.116, mat),
])
print(f"PRISM_GOAL_ARROW_EXPORTED:{OUT}")
