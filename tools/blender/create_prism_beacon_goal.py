"""Create the Prism Beacon goal and export OBJ/MTL/PNG runtime assets."""

from pathlib import Path
import math
import bpy


ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / "project" / "Resources" / "Goal" / "PrismBeacon"
OUT.mkdir(parents=True, exist_ok=True)
PNG = OUT / "prism_beacon.png"

COLORS = [
    (0.035, 0.045, 0.075, 1.0), # dark foundation
    (0.34, 0.22, 0.07, 1.0),    # aged gold shadow
    (0.82, 0.57, 0.13, 1.0),    # restrained gold
    (1.00, 0.82, 0.30, 1.0),    # gold highlight
    (0.055, 0.72, 0.82, 1.0),   # cyan prism
    (0.58, 0.94, 1.00, 1.0),    # bright cyan core
    (0.76, 0.79, 0.84, 1.0),    # pale stone metal
    (0.24, 0.10, 0.32, 1.0),    # violet inset
]


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def make_texture():
    width, height = 512, 64
    image = bpy.data.images.new("PrismBeaconPalette", width=width, height=height, alpha=True)
    pixels = [0.0] * (width * height * 4)
    band = width // len(COLORS)
    for y in range(height):
        for x in range(width):
            local_x = x % band
            color = list(COLORS[min(x // band, len(COLORS) - 1)])
            # Sparse intentional wear on the metal rather than procedural noise.
            if (y == 17 and 10 < local_x < 32) or (y == 46 and 38 < local_x < 56):
                color[0] *= 0.78
                color[1] *= 0.78
                color[2] *= 0.78
            if min(local_x, band - 1 - local_x, y, height - 1 - y) < 2:
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


def make_material(image):
    material = bpy.data.materials.new("PrismBeaconMaterial")
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
    shader.inputs["Roughness"].default_value = 0.48
    shader.inputs["Metallic"].default_value = 0.28
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
    modifier = obj.modifiers.new("Crafted bevel", "BEVEL")
    modifier.width = bevel
    modifier.segments = 2
    bpy.context.view_layer.objects.active = obj
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    obj.data.materials.append(material)
    set_uv(obj, palette)
    return obj


def crystal(name, location, radius, scale, palette, material):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=1, radius=radius, location=location)
    obj = bpy.context.object
    obj.name = name
    obj.scale = scale
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    obj.data.materials.append(material)
    set_uv(obj, palette)
    return obj


def build(material):
    parts = []
    # Broad visual base, while gameplay collision remains the original 0.3 x 2.0 trigger.
    parts.append(box("Foundation", (0, -0.91, 0), (0.78, 0.18, 0.62), 0, material, 0.055))
    parts.append(box("GoldBase", (0, -0.78, 0), (0.62, 0.16, 0.52), 2, material, 0.045))

    # Two unequal pillars and an offset repair band keep it from looking perfectly generated.
    parts.append(box("LeftPillar", (-0.27, 0.0, 0), (0.13, 1.62, 0.46), 1, material, 0.035))
    parts.append(box("RightPillar", (0.27, 0.02, 0), (0.13, 1.58, 0.46), 2, material, 0.035))
    parts.append(box("RepairBand", (-0.27, -0.18, -0.25), (0.18, 0.19, 0.04), 6, material, 0.018,
                     rotation=(0, 0, math.radians(-4))))
    parts.append(box("Crown", (0, 0.83, 0), (0.70, 0.18, 0.52), 2, material, 0.05))

    # Central core is tall enough to remain identifiable during the intro camera sweep.
    parts.append(crystal("PrismCore", (0, 0.02, 0), 0.29, (0.72, 2.15, 0.70), 4, material))
    parts.append(crystal("CoreLight", (0.0, 0.05, -0.255), 0.16, (0.46, 2.3, 0.20), 5, material))
    parts.append(crystal("CoreLightFar", (0.0, 0.05, 0.255), 0.16, (0.46, 2.3, 0.20), 5, material))

    # Upward crown marker reads as a destination, not another stage obstacle.
    parts.append(crystal("CrownPrism", (0, 1.00, 0), 0.20, (0.82, 0.78, 0.62), 3, material))
    parts.append(box("LeftFootRune", (-0.18, -0.69, -0.278), (0.16, 0.035, 0.025), 7, material, 0.008))
    parts.append(box("RightFootRune", (0.14, -0.69, -0.278), (0.24, 0.035, 0.025), 4, material, 0.008))
    return parts


def export(parts):
    bpy.ops.object.select_all(action="DESELECT")
    for part in parts:
        part.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]
    bpy.ops.object.join()
    model = bpy.context.object
    model.name = "PrismBeaconGoal"
    bpy.ops.object.shade_smooth_by_angle()
    bpy.ops.wm.save_as_mainfile(filepath=str(OUT / "prism_beacon.blend"))
    bpy.ops.wm.obj_export(
        filepath=str(OUT / "prism_beacon.obj"),
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
print(f"PRISM_BEACON_EXPORTED:{OUT}")
