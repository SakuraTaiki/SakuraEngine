"""Build the side-facing Prism Glider flying enemy for CG2."""

from pathlib import Path
import bpy

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "Enemy" / "PrismGlider"
OUT.mkdir(parents=True, exist_ok=True)
PNG = OUT / "prism_glider.png"

COLORS = [
    (0.08, 0.07, 0.15, 1.0),    # dark shell
    (0.36, 0.16, 0.58, 1.0),    # purple armour
    (0.025, 0.035, 0.07, 1.0),  # visor
    (0.20, 0.92, 1.00, 1.0),    # cyan eye
    (1.00, 0.34, 0.07, 1.0),    # orange thruster
    (0.56, 0.31, 0.76, 1.0),    # wing edge
]


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def make_texture():
    width, height = 384, 64
    image = bpy.data.images.new("PrismGliderPalette", width=width, height=height, alpha=True)
    pixels = [0.0] * (width * height * 4)
    band = width // len(COLORS)
    for y in range(height):
        for x in range(width):
            color = list(COLORS[min(x // band, len(COLORS) - 1)])
            if min(x % band, band - 1 - x % band, y, height - 1 - y) < 2:
                color[0] *= 0.76
                color[1] *= 0.76
                color[2] *= 0.76
            p = (y * width + x) * 4
            pixels[p:p + 4] = color
    image.pixels = pixels
    image.filepath_raw = str(PNG)
    image.file_format = "PNG"
    image.save()
    return image


def make_material(image):
    material = bpy.data.materials.new("PrismGliderMaterial")
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
    shader.inputs["Roughness"].default_value = 0.58
    shader.inputs["Metallic"].default_value = 0.28
    links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return material


def set_uv(obj, index):
    layer = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")
    u = (index + 0.5) / len(COLORS)
    for loop in layer.data:
        loop.uv = (u, 0.5)


def box(name, location, size, palette, material, bevel=0.04, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.scale = tuple(value * 0.5 for value in size)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    mod = obj.modifiers.new("Machined bevel", "BEVEL")
    mod.width = bevel
    mod.segments = 2
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=mod.name)
    obj.data.materials.append(material)
    set_uv(obj, palette)
    return obj


def build(material):
    parts = []
    parts.append(box("Body", (0.0, 0.0, 0), (0.88, 0.42, 0.54), 0, material, 0.085))
    parts.append(box("Nose", (0.51, 0.02, 0), (0.36, 0.30, 0.42), 1, material, 0.055,
                     rotation=(0, -0.22, 0.10)))
    parts.append(box("RearEngine", (-0.49, 0.0, 0), (0.28, 0.32, 0.44), 1, material, 0.05))

    # Swept wings remain thick enough to read throughout a 180-degree turn.
    parts.append(box("TopWing", (-0.04, 0.33, 0), (0.64, 0.20, 0.38), 5, material, 0.045,
                     rotation=(0.0, 0.0, -0.22)))
    parts.append(box("BottomWing", (-0.04, -0.31, 0), (0.58, 0.17, 0.36), 1, material, 0.04,
                     rotation=(0.0, 0.0, 0.18)))

    # Side-profile face is duplicated on both camera-facing surfaces.
    for z in (-0.286, 0.286):
        parts.append(box("FaceVisor", (0.30, 0.07, z), (0.38, 0.22, 0.03), 2, material, 0.022))
        parts.append(box("CyanEye", (0.41, 0.08, z * 1.015), (0.13, 0.09, 0.025), 3, material, 0.014))
        parts.append(box("EngineGlow", (-0.59, 0.0, z * 0.72), (0.06, 0.19, 0.18), 4, material, 0.012))

    # Offset dorsal sensor prevents a sterile perfectly mirrored silhouette.
    parts.append(box("DorsalSensor", (-0.17, 0.48, -0.08), (0.15, 0.22, 0.16), 3, material, 0.025,
                     rotation=(0.0, 0.0, -0.18)))
    return parts


def export(parts):
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    model = bpy.context.object
    model.name = "PrismGlider"
    bpy.ops.object.shade_smooth_by_angle()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "prism_glider.blend"))
    bpy.ops.wm.obj_export(
        filepath=str(OUT / "prism_glider.obj"),
        export_selected_objects=True,
        export_materials=True,
        export_triangulated_mesh=True,
        forward_axis="NEGATIVE_Y",
        up_axis="Z",
    )


clear_scene()
texture = make_texture()
material = make_material(texture)
export(build(material))
print(f"PRISM_GLIDER_EXPORTED:{OUT}")
