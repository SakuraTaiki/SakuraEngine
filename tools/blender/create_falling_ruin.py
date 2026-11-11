"""Create three hand-authored fracture stages for the falling ruin block."""

from pathlib import Path
import math
import bpy


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "Block" / "FallingRuin"
OUT.mkdir(parents=True, exist_ok=True)
PNG = OUT / "falling_ruin.png"

COLORS = [
    (0.32, 0.095, 0.018, 1.0),  # burnt-orange ruin stone
    (0.58, 0.24, 0.035, 1.0),   # worn amber top
    (0.003, 0.004, 0.006, 1.0), # near-black deep crack
    (0.12, 0.055, 0.025, 1.0),  # dark aged brace
    (1.00, 0.72, 0.07, 1.0),    # exposed yellow-orange core
    (0.72, 0.30, 0.045, 1.0),   # warning/chipped stone
    (0.95, 0.43, 0.055, 1.0),   # faded warm prism mark
]


def clear_objects():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def make_texture():
    width, height = 448, 64
    image = bpy.data.images.new("FallingRuinPalette", width=width, height=height, alpha=True)
    pixels = [0.0] * (width * height * 4)
    band = width // len(COLORS)
    for y in range(height):
        for x in range(width):
            local_x = x % band
            color = list(COLORS[min(x // band, len(COLORS) - 1)])
            # A few deliberate tool marks; avoid uniform procedural grain.
            if (y == 21 and 9 < local_x < 37) or (y == 45 and 28 < local_x < 55):
                color[0] *= 0.70
                color[1] *= 0.70
                color[2] *= 0.70
            if min(local_x, band - 1 - local_x, y, height - 1 - y) < 2:
                color[0] *= 0.8
                color[1] *= 0.8
                color[2] *= 0.8
            p = (y * width + x) * 4
            pixels[p:p + 4] = color
    image.pixels = pixels
    image.filepath_raw = str(PNG)
    image.file_format = "PNG"
    image.save()
    return image


def make_material(image):
    material = bpy.data.materials.new("FallingRuinMaterial")
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
    shader.inputs["Roughness"].default_value = 0.82
    shader.inputs["Metallic"].default_value = 0.08
    links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return material


def set_uv(obj, palette):
    layer = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")
    u = (palette + 0.5) / len(COLORS)
    for loop in layer.data:
        loop.uv = (u, 0.5)


def box(name, location, size, palette, material, bevel=0.018, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.scale = tuple(value * 0.5 for value in size)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    modifier = obj.modifiers.new("Chipped edge", "BEVEL")
    modifier.width = bevel
    modifier.segments = 1
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    obj.data.materials.append(material)
    set_uv(obj, palette)
    return obj


def crack_segment(name, start, end, thickness, palette, material, z=-0.482):
    dx = end[0] - start[0]
    dy = end[1] - start[1]
    length = math.sqrt(dx * dx + dy * dy)
    angle = math.atan2(dy, dx)
    return box(
        name,
        ((start[0] + end[0]) * 0.5, (start[1] + end[1]) * 0.5, z),
        (length, thickness, 0.022),
        palette,
        material,
        bevel=thickness * 0.35,
        rotation=(0, 0, angle),
    )


BASE_CRACKS = [
    ((-0.08, 0.15), (0.02, 0.04)),
    ((0.02, 0.04), (-0.07, -0.04)),
    ((-0.07, -0.04), (0.01, -0.14)),
    ((0.02, 0.04), (0.15, 0.00)),
]

WARNING_CRACKS = [
    ((-0.08, 0.15), (-0.25, 0.10)),
    ((-0.25, 0.10), (-0.36, 0.16)),
    ((-0.07, -0.04), (-0.24, -0.13)),
    ((0.15, 0.00), (0.31, 0.10)),
    ((0.15, 0.00), (0.34, -0.09)),
]

BROKEN_CRACKS = [
    ((-0.36, 0.16), (-0.48, 0.07)),
    ((-0.24, -0.13), (-0.43, -0.19)),
    ((0.31, 0.10), (0.48, 0.17)),
    ((0.34, -0.09), (0.49, -0.17)),
]


def build(stage, material):
    parts = []
    if stage == "broken":
        # Three rigid fragments keep the collider stable while visibly opening the main split.
        parts.append(box("LeftFragment", (-0.34, 0.005, 0), (0.31, 0.29, 0.88), 0, material, 0.025,
                         rotation=(0, 0, math.radians(-2))))
        parts.append(box("MiddleFragment", (0.0, -0.015, 0), (0.32, 0.28, 0.88), 0, material, 0.025,
                         rotation=(0, 0, math.radians(2))))
        parts.append(box("RightFragment", (0.35, 0.01, 0), (0.29, 0.29, 0.88), 0, material, 0.025,
                         rotation=(0, 0, math.radians(-1))))
    else:
        stone_palette = 5 if stage == "warning" else 0
        parts.append(box("OldStoneSlab", (0, 0, 0), (1.0, 0.30, 0.88), stone_palette, material, 0.03))

    parts.append(box("WornTop", (0, 0.13, 0), (0.94, 0.04, 0.91), 1, material, 0.014))
    parts.append(box("LeftBrace", (-0.445, 0, -0.463), (0.07, 0.25, 0.035), 3, material, 0.01))
    parts.append(box("RightBrace", (0.445, 0, -0.463), (0.07, 0.25, 0.035), 3, material, 0.01))
    parts.append(box("DeadPrismMark", (-0.31, 0.065, -0.484), (0.20, 0.025, 0.018), 6, material, 0.006))
    if stage == "warning":
        # Broad warning rims survive the game's distant camera where fine cracks become sub-pixel.
        for side in (-1.0, 1.0):
            parts.append(box("WarningTopRim", (0, 0.125, side * 0.496), (0.88, 0.032, 0.018), 4, material, 0.006))
            parts.append(box("WarningBottomRim", (0, -0.125, side * 0.496), (0.88, 0.032, 0.018), 4, material, 0.006))

    cracks = list(BASE_CRACKS)
    if stage in ("warning", "broken"):
        cracks += WARNING_CRACKS
    if stage == "broken":
        cracks += BROKEN_CRACKS
    for index, (start, end) in enumerate(cracks):
        thickness = 0.048 if stage == "idle" else (0.072 if stage == "warning" else 0.090)
        # Both faces carry the authored crack pattern so it remains readable from
        # the game camera and from the editor's freely movable camera.
        for side in (-1.0, 1.0):
            surface_z = side * 0.482
            parts.append(crack_segment(
                f"Crack_{index:02d}_{'Near' if side < 0 else 'Far'}",
                start, end, thickness, 2, material, z=surface_z))
            if stage in ("warning", "broken") and index < len(BASE_CRACKS) + 2:
                parts.append(crack_segment(
                    f"CoreGlow_{index:02d}_{'Near' if side < 0 else 'Far'}",
                    start, end, thickness * 0.42, 4, material, z=side * 0.497))
    return parts


def save_variant(stage, parts):
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    model = bpy.context.object
    model.name = f"FallingRuin_{stage.title()}"
    bpy.ops.object.shade_smooth_by_angle()
    blend_path = OUT / f"falling_ruin_{stage}.blend"
    obj_path = OUT / f"falling_ruin_{stage}.obj"
    bpy.ops.wm.save_as_mainfile(filepath=str(blend_path))
    bpy.ops.wm.obj_export(
        filepath=str(obj_path),
        export_selected_objects=True,
        export_materials=True,
        export_triangulated_mesh=True,
        forward_axis="NEGATIVE_Y",
        up_axis="Z",
    )


clear_objects()
texture = make_texture()
material = make_material(texture)
for variant in ("idle", "warning", "broken"):
    clear_objects()
    save_variant(variant, build(variant, material))
print(f"FALLING_RUIN_EXPORTED:{OUT}")
