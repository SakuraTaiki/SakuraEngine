"""Create simple inactive/active Prism Save Flag models for reliable swapping."""
from pathlib import Path
import bpy

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "System" / "PrismSaveFlag"
OUT.mkdir(parents=True, exist_ok=True)
PNG = OUT / "prism_save_flag.png"

# Blender Y is displayed upside-down after this project's OBJ conversion.
COLORS = [
    (0.055, 0.42, 0.72, 1.0), (0.025, 0.08, 0.15, 1.0),
    (1.00, 0.67, 0.08, 1.0), (1.00, 0.91, 0.38, 1.0),
]

def clear():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)

def make_texture():
    width, height = 256, 64
    image = bpy.data.images.new("PrismSaveFlagPalette", width=width, height=height, alpha=True)
    band = width // len(COLORS)
    pixels = [0.0] * (width * height * 4)
    for y in range(height):
        for x in range(width):
            color = list(COLORS[min(x // band, len(COLORS) - 1)])
            local = x % band
            if min(local, band - 1 - local, y, height - 1 - y) < 2:
                color[0] *= 0.72; color[1] *= 0.72; color[2] *= 0.72
            if y == 21 and 14 < local < 49:
                color[0] *= 0.82; color[1] *= 0.82; color[2] *= 0.82
            p = (y * width + x) * 4
            pixels[p:p + 4] = color
    image.pixels = pixels
    image.filepath_raw = str(PNG)
    image.file_format = "PNG"
    image.save()
    return image

def make_material(image):
    mat = bpy.data.materials.new("PrismSaveFlagMaterial")
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    for node in list(nodes): nodes.remove(node)
    output = nodes.new("ShaderNodeOutputMaterial")
    shader = nodes.new("ShaderNodeBsdfPrincipled")
    texture = nodes.new("ShaderNodeTexImage")
    texture.image = image
    texture.interpolation = "Closest"
    shader.inputs["Roughness"].default_value = 0.64
    shader.inputs["Metallic"].default_value = 0.20
    links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return mat

def set_uv(obj, palette):
    layer = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")
    u = (palette + 0.5) / len(COLORS)
    for loop in layer.data: loop.uv = (u, 0.5)

def box(name, location, size, palette, material, bevel=0.018):
    bpy.ops.mesh.primitive_cube_add(location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = tuple(value * 0.5 for value in size)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    mod = obj.modifiers.new("Single cut bevel", "BEVEL")
    mod.width, mod.segments = bevel, 1
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=mod.name)
    obj.data.materials.append(material)
    set_uv(obj, palette)
    return obj

def flag_mesh(material):
    # Negative Blender Y is the pole's game-visible top. The flag is authored
    # there directly, so it can never appear near or below the floor.
    outline = [(-0.24, -0.49), (0.58, -0.49), (0.43, -0.36),
               (0.58, -0.23), (-0.24, -0.23)]
    depth = 0.07
    vertices = [(x, y, z) for z in (-depth * 0.5, depth * 0.5) for x, y in outline]
    faces = [(0,1,2,3,4), (9,8,7,6,5), (0,5,6,1), (1,6,7,2),
             (2,7,8,3), (3,8,9,4), (4,9,5,0)]
    mesh = bpy.data.meshes.new("SaveFlagMesh")
    mesh.from_pydata(vertices, [], faces); mesh.update()
    obj = bpy.data.objects.new("SaveFlag", mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(material)
    set_uv(obj, 2)
    mod = obj.modifiers.new("Flag edge bevel", "BEVEL")
    mod.width, mod.segments = 0.015, 1
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=mod.name)
    return obj

def build(active, material):
    # Inactive is literally a blue pole: no cap, base, core, or flag.
    parts = [box("SavePole", (-0.28, 0.02, 0), (0.09, 1.28, 0.09),
                 2 if active else 0, material)]
    if active:
        parts.append(flag_mesh(material))
        parts.append(box("FlagHighlight", (0.08, -0.505, -0.043),
                         (0.46, 0.025, 0.025), 3, material, 0.005))
    return parts

def export_model(name, parts):
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts: part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    bpy.context.object.name = "PrismSaveFlag_" + name.title()
    bpy.ops.object.shade_smooth_by_angle()
    bpy.ops.wm.obj_export(filepath=str(OUT / f"prism_save_flag_{name}.obj"),
        export_selected_objects=True, export_materials=True,
        export_triangulated_mesh=True, forward_axis="NEGATIVE_Y", up_axis="Z")

image = make_texture()
material = make_material(image)
for name, active in (("inactive", False), ("active", True)):
    clear(); export_model(name, build(active, material))
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "prism_save_flag.blend"))
print(f"PRISM_SAVE_FLAG_EXPORTED:{OUT}")
