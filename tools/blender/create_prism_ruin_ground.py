"""Build the Prism Ruin ground block and export CG2 OBJ/MTL/PNG assets."""

from pathlib import Path
import math
import bpy


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "Block" / "PrismRuin"
OUT.mkdir(parents=True, exist_ok=True)
PNG = OUT / "prism_ruin_ground.png"

COLORS = [
    (0.21, 0.28, 0.36, 1.0),    # readable blue-grey side stone
    (0.45, 0.55, 0.65, 1.0),    # bright walkable top
    (0.09, 0.14, 0.21, 1.0),    # navy frame shadow
    (0.28, 0.36, 0.46, 1.0),    # worn blue steel
    (0.26, 0.85, 0.91, 1.0),    # cyan walkable edge
    (0.50, 0.46, 0.78, 1.0),    # restrained prism violet
    (0.60, 0.68, 0.76, 1.0),    # bolt highlight
]


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def make_texture():
    width, height = 448, 64
    image = bpy.data.images.new("PrismRuinPalette", width=width, height=height, alpha=True)
    pixels = [0.0] * (width * height * 4)
    band = width // len(COLORS)
    for y in range(height):
        for x in range(width):
            color = list(COLORS[min(x // band, len(COLORS) - 1)])
            local_x = x % band
            # Restrained scratches break perfect digital uniformity without noisy AI texture.
            if (y in (18, 19) and 13 < local_x < 43) or (y == 43 and 31 < local_x < 53):
                color[0] *= 0.72
                color[1] *= 0.72
                color[2] *= 0.72
            if min(local_x, band - 1 - local_x, y, height - 1 - y) < 2:
                color[0] *= 0.78
                color[1] *= 0.78
                color[2] *= 0.78
            p = (y * width + x) * 4
            pixels[p:p + 4] = color
    image.pixels = pixels
    image.filepath_raw = str(PNG)
    image.file_format = "PNG"
    image.save()
    return image


def make_material(image):
    material = bpy.data.materials.new("PrismRuinMaterial")
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
    shader.inputs["Roughness"].default_value = 0.74
    shader.inputs["Metallic"].default_value = 0.12
    links.new(texture.outputs["Color"], shader.inputs["Base Color"])
    links.new(shader.outputs["BSDF"], output.inputs["Surface"])
    return material


def set_uv(obj, palette):
    layer = obj.data.uv_layers.active or obj.data.uv_layers.new(name="UVMap")
    u = (palette + 0.5) / len(COLORS)
    for loop in layer.data:
        loop.uv = (u, 0.5)


def box(name, location, size, palette, material, bevel=0.025, rotation=(0, 0, 0)):
    bpy.ops.mesh.primitive_cube_add(location=location, rotation=rotation)
    obj = bpy.context.object
    obj.name = name
    obj.scale = tuple(value * 0.5 for value in size)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    modifier = obj.modifiers.new("Stone edge", "BEVEL")
    modifier.width = bevel
    modifier.segments = 2
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    obj.data.materials.append(material)
    set_uv(obj, palette)
    return obj


def build(material):
    parts = []
    # Authored at the final 2.0 x 0.5 footprint. StageEditor compensates model scale
    # so existing JSON placement and collision values remain unchanged.
    parts.append(box("ArtificialStone", (0, 0, 0), (2.00, 0.50, 0.84), 0, material, 0.04))
    parts.append(box("WalkableTop", (0, 0.225, 0), (1.96, 0.05, 0.88), 1, material, 0.018))

    # Recessed outer frame does not exceed the exact two-tile footprint.
    parts.append(box("FrameTop", (0, 0.195, -0.455), (1.96, 0.06, 0.055), 3, material, 0.014))
    parts.append(box("FrameBottom", (0, -0.205, -0.455), (1.96, 0.06, 0.055), 2, material, 0.014))
    parts.append(box("FrameLeft", (-0.965, 0, -0.455), (0.07, 0.39, 0.055), 3, material, 0.014))
    parts.append(box("FrameRight", (0.965, 0, -0.455), (0.07, 0.39, 0.055), 2, material, 0.014))

    # Thin cyan seam communicates the safe walkable surface without overpowering characters.
    parts.append(box("PrismSeam", (0.12, 0.145, -0.493), (1.25, 0.035, 0.022), 4, material, 0.008))

    # Central diamond relief and one asymmetric repair plate create a hand-built ruin identity.
    parts.append(box("PrismRelief", (0.0, -0.035, -0.492), (0.23, 0.23, 0.025), 5, material, 0.018,
                     rotation=(0, 0, math.radians(45))))
    parts.append(box("RepairPlate", (-0.62, -0.07, -0.493), (0.34, 0.19, 0.026), 3, material, 0.018,
                     rotation=(0, 0, math.radians(-3))))
    for x, y in ((-0.75, -0.13), (-0.50, -0.01)):
        parts.append(box("RepairBolt", (x, y, -0.514), (0.045, 0.045, 0.018), 6, material, 0.01))

    # Duplicate the readable relief on the far side for editor camera freedom.
    parts.append(box("FarPrismRelief", (0.0, -0.035, 0.492), (0.23, 0.23, 0.025), 5, material, 0.018,
                     rotation=(0, 0, math.radians(45))))
    return parts


def export(parts):
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    model = bpy.context.object
    model.name = "PrismRuinGround"
    bpy.ops.object.shade_smooth_by_angle()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "prism_ruin_ground.blend"))
    bpy.ops.wm.obj_export(
        filepath=str(OUT / "prism_ruin_ground.obj"),
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
print(f"PRISM_RUIN_GROUND_EXPORTED:{OUT}")
