"""Create the Prism Spring Pad source animation and runtime OBJ variants."""

from pathlib import Path
import math
import bpy


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "Gimmick" / "PrismSpringPad"
OUT.mkdir(parents=True, exist_ok=True)
PNG = OUT / "prism_spring_pad.png"

COLORS = [
    (0.12, 0.19, 0.28, 1.0),  # blue-gray base
    (0.33, 0.45, 0.57, 1.0),  # worn upper metal
    (0.025, 0.055, 0.10, 1.0),# navy gaps
    (0.05, 0.88, 1.0, 1.0),  # cyan energy
    (0.55, 0.22, 1.0, 1.0),  # violet charge
    (0.86, 0.94, 1.0, 1.0),  # pale contact plate
]


def clear():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def texture():
    width, height = 384, 64
    image = bpy.data.images.new("PrismSpringPadPalette", width=width, height=height, alpha=True)
    pixels = [0.0] * (width * height * 4)
    band = width // len(COLORS)
    for y in range(height):
        for x in range(width):
            color = list(COLORS[min(x // band, len(COLORS) - 1)])
            local = x % band
            # Deliberate small machining marks keep the texture authored and readable.
            if (y in (17, 45) and 12 < local < 47) or min(local, band - 1 - local, y, height - 1 - y) < 2:
                color[0] *= 0.72
                color[1] *= 0.72
                color[2] *= 0.72
            p = (y * width + x) * 4
            pixels[p:p + 4] = color
    image.pixels = pixels
    image.filepath_raw = str(PNG)
    image.file_format = "PNG"
    image.save()
    return image


def material(image):
    mat = bpy.data.materials.new("PrismSpringPadMaterial")
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
    shader.inputs["Roughness"].default_value = 0.68
    shader.inputs["Metallic"].default_value = 0.18
    links.new(tex.outputs["Color"], shader.inputs["Base Color"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return mat


def uv(obj, palette):
    layer = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")
    u = (palette + 0.5) / len(COLORS)
    for loop in layer.data:
        loop.uv = (u, 0.5)


def box(name, location, size, palette, mat, bevel=0.025, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.scale = tuple(v * 0.5 for v in size)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    mod = obj.modifiers.new("Hand cut bevel", "BEVEL")
    mod.width = bevel
    mod.segments = 1
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=mod.name)
    obj.data.materials.append(mat)
    uv(obj, palette)
    return obj


def build(deck_y, charge, mat):
    parts = []
    parts.append(box("RuinBase", (0, -0.13, 0), (1.0, 0.20, 0.88), 0, mat, 0.035))
    parts.append(box("BaseTop", (0, -0.015, 0), (0.92, 0.055, 0.82), 1, mat, 0.018))
    parts.append(box("DarkGap", (0, 0.035, 0), (0.72, 0.055, 0.64), 2, mat, 0.014))
    for side in (-1.0, 1.0):
        x = side * 0.33
        parts.append(box("EnergyRail", (x, (deck_y + 0.02) * 0.5, 0),
                         (0.09, max(0.07, deck_y + 0.10), 0.62), 4 if charge else 3, mat, 0.016))
        parts.append(box("SideBrace", (side * 0.44, -0.01, 0), (0.08, 0.22, 0.76), 0, mat, 0.015))
    parts.append(box("MovingDeck", (0, deck_y, 0), (0.82, 0.10, 0.72), 5, mat, 0.025))
    # Three broad chevrons remain readable from the distant side camera.
    chevron_y = deck_y + 0.057
    for z in (-0.18, 0.0, 0.18):
        parts.append(box("ArrowLeft", (-0.075, chevron_y, z), (0.24, 0.025, 0.055), 3, mat, 0.008,
                         rotation=(0, math.radians(-28), 0)))
        parts.append(box("ArrowRight", (0.075, chevron_y, z), (0.24, 0.025, 0.055), 3, mat, 0.008,
                         rotation=(0, math.radians(28), 0)))
    return parts


def join_export(name, parts):
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    obj = bpy.context.object
    obj.name = f"PrismSpringPad_{name.title()}"
    bpy.ops.object.shade_smooth_by_angle()
    bpy.ops.wm.obj_export(
        filepath=str(OUT / f"prism_spring_pad_{name}.obj"),
        export_selected_objects=True,
        export_materials=True,
        export_triangulated_mesh=True,
        forward_axis="NEGATIVE_Y",
        up_axis="Z",
    )


image = texture()
mat = material(image)
for variant, deck_y, charge in (
    ("idle", 0.15, False),
    ("compressed", 0.065, True),
    ("launch", 0.29, True),
):
    clear()
    join_export(variant, build(deck_y, charge, mat))

# Keep an editable Blender source with a clearly named animated deck.
clear()
parts = build(0.15, False, mat)
deck = next(obj for obj in parts if obj.name == "MovingDeck")
deck.keyframe_insert(data_path="location", frame=1)
deck.location.y = 0.065
deck.keyframe_insert(data_path="location", frame=7)
deck.location.y = 0.29
deck.keyframe_insert(data_path="location", frame=12)
deck.location.y = 0.15
deck.keyframe_insert(data_path="location", frame=19)
bpy.context.scene.frame_start = 1
bpy.context.scene.frame_end = 19
bpy.context.scene.render.fps = 60
bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "prism_spring_pad.blend"))
print(f"PRISM_SPRING_PAD_EXPORTED:{OUT}")
